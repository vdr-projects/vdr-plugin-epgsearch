/*                                                                  -*- c++ -*-
Copyright (C) 2004-2011 Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The author can be reached at cwieninger@gmx.de

The project's page is at http://winni.vdr-developer.org/epgsearch
*/

#ifndef __EPGSEARCHCONFLCH_H
#define __EPGSEARCHCONFLCH_H

#include "epgsearchtools.h"
#include <vector>
#include <set>
#include <list>

#define DO_MULTIPLE_RECORDINGS 1
#define DO_REC_AND_PLAY_ON_PRIMARY_DEVICE 1

class cConflictCheckTime;
class TimerObjSort;

// --- cConflictCheckTimerObj --------------------------------------------------------
class cConflictCheckTimerObj : public cTimerObj
{
    const cEvent* event;
 public:
    time_t start;
    time_t stop;
    int device;
    int origIndex;
    int recDuration;
    time_t lastRecStart;
    time_t lastRecStop;
    cConflictCheckTime* conflCheckTime;
    std::set<cConflictCheckTimerObj*,TimerObjSort>* concurrentTimers;
    bool ignore;


    cConflictCheckTimerObj(cTimer* Timer, time_t Start, time_t Stop, int Device = -1, int OrigIndex=-1);
    ~cConflictCheckTimerObj();
    int Compare(const cListObject &ListObject) const;
    const cEvent* Event();
    const cEvent* SetEventFromSchedule();
    int Matches(const cEvent *Event, int *Overlap) const;
    cTimer* OrigTimer() {return Timers.GetTimer(timer); }
};

class TimerObjSort
{
public:
    bool operator() (cConflictCheckTimerObj* a, cConflictCheckTimerObj* b)
    {
        return (a->Compare(*b) < 0);
    }
};


// --- cConflictCheckTime --------------------------------------------------------
class cConflictCheckTime : public cListObject
{
 public:
    time_t evaltime;
    std::set<cConflictCheckTimerObj*,TimerObjSort> startingTimers;
    std::set<cConflictCheckTimerObj*,TimerObjSort> stoppingTimers;
    std::set<cConflictCheckTimerObj*,TimerObjSort> failedTimers;
    std::set<cConflictCheckTimerObj*,TimerObjSort> concurrentRecs;
    bool ignore;

    cConflictCheckTime(time_t EvalTime) : evaltime(EvalTime), ignore(false) {}
    int Compare(const cListObject &ListObject) const
	{
	    cConflictCheckTime *p = (cConflictCheckTime *)&ListObject;
	    return evaltime - p->evaltime;
	}
};

// --- cConflictCheckDevice --------------------------------------------------------
// This class tries to emulate the behaviour of a DVB device
// NOTE: The case device == NULL is only for debugging purposes
class cConflictCheckDevice
{
 public:
    std::set<cConflictCheckTimerObj*,TimerObjSort> recTimers;
    cDevice* device;
    int devicenr;

    cConflictCheckDevice() { device = NULL; devicenr = 0; }
    int Priority() const
	{
	    int prio = -1;
	    for(std::set<cConflictCheckTimerObj*,TimerObjSort>::iterator it = recTimers.begin(); it != recTimers.end(); ++it)
		prio = max(prio, (*it)->timer->Priority());
	    return prio;
	};
    int CardIndex(void) const { if (device) return device->CardIndex(); else return devicenr;}
    bool Receiving() const { return !recTimers.empty(); }
    bool IsTunedTo (const cChannel* Channel) const
	{
	    for(std::set<cConflictCheckTimerObj*,TimerObjSort>::iterator it = recTimers.begin(); it != recTimers.end(); ++it)
		if ((*it)->timer->Channel()->Source() == Channel->Source() &&
		    (*it)->timer->Channel()->Transponder() == Channel->Transponder())
		    return true;
	    return false;
	}
    bool HasDecoder() const { if (device) return device->HasDecoder(); else return (devicenr == 3); }
    bool HasCi() const { if (device) return device->HasCi(); else return (devicenr == 3); }
    bool IsPrimaryDevice() const { if (device) return device->IsPrimaryDevice(); else return (devicenr == 3); }
    bool ProvidesSource(int Source) const
	{
	    if (device) return device->ProvidesSource(Source);
	    else
	    {
//		int type = Source & cSource::st_Mask;
//		if (devicenr == 0) return type == cSource::stCable;
//		if (devicenr > 0) return type == cSource::stTerr;
//		return false;
		return true;
	    }
	}
    cCamSlot *CamSlot(void) const { if (device) return device->CamSlot(); else return NULL;}
    int Ca() const
	{
	    for(std::set<cConflictCheckTimerObj*,TimerObjSort>::iterator it = recTimers.begin(); it != recTimers.end(); ++it)
		return (*it)->timer->Channel()->Ca();
	    return 0;
	}
    bool HasPid(int Pid) const { return true; }
    bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const
	{
	    bool result = false;
	    bool hasPriority = Priority < 0 || Priority > this->Priority();
	    bool needsDetachReceivers = false;

	    if (ProvidesSource(Channel->Source())) {
		result = hasPriority;
		if (Priority >= 0 && Receiving()) {
		    if (IsTunedTo(Channel)) {
		      if ((Channel->Vpid() && !HasPid(Channel->Vpid())) || (Channel->Apid(0) && !HasPid(Channel->Apid(0)))) {
#ifdef DO_MULTIPLE_RECORDINGS
			    if (CamSlot() && Channel->Ca() >= CA_ENCRYPTED_MIN) {
				if (CamSlot()->CanDecrypt(Channel))
				    result = true;
				else
				    needsDetachReceivers = true;
			    }
			    else if (!IsPrimaryDevice())
				result = true;
#ifdef DO_REC_AND_PLAY_ON_PRIMARY_DEVICE
			    else
#if APIVERSNUM < 10725
				result = Priority >= Setup.PrimaryLimit;
#else
				result = Priority >= 0;
#endif
#endif
#endif
			}
			else
#if APIVERSNUM < 10725
			    result = !IsPrimaryDevice() || Priority >= Setup.PrimaryLimit;
#else
			    result = !IsPrimaryDevice() || Priority >= 0;
#endif
		    }
		    else
			needsDetachReceivers = true;
		}
	    }
	    if (NeedsDetachReceivers)
		*NeedsDetachReceivers = needsDetachReceivers;
	    return result;
	}
};

// --- cConflictCheck --------------------------------------------------------
class cConflictCheck
{
    cList<cConflictCheckTimerObj>* timerList;
    cList<cConflictCheckTime>* evaltimeList;
    cList<cConflictCheckTime>* failedList;
    std::set<cConflictCheckTimerObj*> pendingTimers;
    cConflictCheckDevice *devices;

    int numDevices;
    time_t maxCheck;
    std::vector<eModuleStatus> camSlotStatusArray;
 public:
    int relevantConflicts;
    int numConflicts;
    time_t nextRelevantConflictDate;

    cConflictCheck();
    ~cConflictCheck();
    void InitDevicesInfo();
    void Check();
    cList<cConflictCheckTimerObj>* CreateCurrentTimerList();
    cList<cConflictCheckTime>* CreateEvaluationTimeList(cList<cConflictCheckTimerObj>*);
    cList<cConflictCheckTime>* CreateConflictList(cList<cConflictCheckTime>*, cList<cConflictCheckTimerObj>* timerList);
    int GetDevice(cConflictCheckTimerObj* TimerObj, bool *NeedsDetachReceivers);
    cList<cConflictCheckTime>* GetFailed() { return failedList; }
    cList<cConflictCheckTimerObj>* GetTimers() { return timerList; }
    void AddConflict(cConflictCheckTimerObj* TimerObj, cConflictCheckTime* Checktime, std::set<cConflictCheckTimerObj*>& pendingTimers);
    int ProcessCheckTime(cConflictCheckTime* checkTime);
    bool TimerInConflict(cTimer*);
    void EvaluateConflCheckCmd();
    eModuleStatus CamSlotModuleStatus(cCamSlot *CamSlot);
};

#endif

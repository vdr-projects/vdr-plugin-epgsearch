/*                                                                  -*- c++ -*-
Copyright (C) 2004-2013 Christian Wieninger

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

#include <string>
#include <list>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif
#include "timer_thread.h"
#include "epgsearchcfg.h"
#include "epgsearchtools.h"
#include "services.h"
#include "svdrpclient.h"
#include "timerstatus.h"
#include "recstatus.h"
#include "recdone.h"
#include "conflictcheck_thread.h"
#include "status_thread.h"
#include <math.h>

#include <vdr/tools.h>
#include <vdr/plugin.h>

#define ALLOWED_BREAK_INSECS 2

extern int updateForced;

cStatusThread *cStatusThread::m_Instance = NULL;
StatusThreadStatus cStatusThread::m_Status = StatusThreadReady;
int gl_StatusProgged=0; // Flag that indicates, when programming is finished

cStatusThread::cStatusThread()
: cThread("EPGSearch: recstatus")
{
    m_Active = false;
}

cStatusThread::~cStatusThread() {
    if (m_Active)
	Stop();
    cStatusThread::m_Instance = NULL;
}

void cStatusThread::Init(const cDevice *Device, const char *Name, const char *Filename, bool On) {
    if (m_Instance == NULL) {
        m_Instance = new cStatusThread;
    }
    else {
        if (m_Instance->m_Active) {
            LogFile.eSysLog("Epgsearch recstatus_thread called too fast"); //should stack
            return;
        }
    }
    m_Instance->m_device=Device;
    m_Instance->m_name=Name;
    m_Instance->m_filename=Filename;
    m_Instance->m_on=On;
    m_Instance->Start();
}

void cStatusThread::Exit(void) {
    if (m_Instance != NULL) {
	m_Instance->Stop();
	DELETENULL(m_Instance);
    }

}

void cStatusThread::Stop(void) {
    m_Active = false;
    Cancel(3);
}

void cStatusThread::Action(void)
{
   m_Active = true;
   time_t now = time(NULL);
   // insert new timers currently recording in TimersRecording
   if (m_on && m_name)
   {
      if (EPGSearchConfig.checkTimerConflOnRecording)
         cConflictCheckThread::Init((cPluginEpgsearch*)cPluginManager::GetPlugin("epgsearch"), true);

      LOCK_TIMERS_READ;

      for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti))
         if (ti->Recording())
         {
            // check if this is a new entry
            cRecDoneTimerObj *tiRFound = NULL;
            for (cRecDoneTimerObj *tiR = gl_recStatusMonitor->TimersRecording.First(); tiR; tiR = gl_recStatusMonitor->TimersRecording.Next(tiR))
               if (tiR->timer == ti)
               {
                  tiRFound = tiR;
                  break;
               }

            if (tiRFound) // already handled, perhaps a resume
            {
               if (tiRFound->lastBreak > 0 && now - tiRFound->lastBreak <= ALLOWED_BREAK_INSECS)
               {
                  LogFile.Log(1,"accepting resume of '%s' on device %d", m_name, m_device->CardIndex());
                  tiRFound->lastBreak = 0;
               }
               continue;
            }

            cRecDoneTimerObj* timerObj = new cRecDoneTimerObj(ti, m_device->DeviceNumber());
            gl_recStatusMonitor->TimersRecording.Add(timerObj);

            cSearchExt* search = TriggeredFromSearchTimer(ti);
            if (!search || (search->avoidRepeats == 0 && search->delMode == 0)) // ignore if not avoid repeats and no auto-delete
               continue;

            bool vpsUsed = ti->HasFlags(tfVps) && ti->Event() && ti->Event()->Vps();
            LogFile.Log(1,"recording started '%s' on device %d (search timer '%s'); VPS used: %s", m_name, m_device->CardIndex(), search->search, vpsUsed ? "Yes": "No");
            const cEvent* event = ti->Event();
            if (!event)
            {
               event = GetEvent(ti);
               if (event)
                  LogFile.Log(3,"timer had no event: assigning '%s'", event->Title());
            }
            if (!event)
            {
               LogFile.Log(1,"no event for timer found! will be ignored in done list");
               continue;
            }
            time_t now = time(NULL);
            if (vpsUsed || now < ti->StartTime() + 60) // allow a delay of one minute
            {
               timerObj->recDone = new cRecDone(ti, event, search);
               return;
            }
            else
               LogFile.Log(1,"recording started too late! will be ignored");
         }
   }

   if (!m_on)
   {
      cMutexLock RecsDoneLock(&RecsDone);
      // remove timers that finished recording from TimersRecording
      // incomplete recordings are kept for a while, perhaps they will be resumed
      cRecDoneTimerObj *tiR = gl_recStatusMonitor->TimersRecording.First();
      while(tiR)
      {
         // check if timer still exists
         bool found = false;
         LOCK_TIMERS_READ;
         for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti))
            if (ti == tiR->timer)
            {
               found = true;
               break;
            }
         if (found && !tiR->timer->Recording())
         {
            if (tiR->recDone)
            {
               cSearchExt* search = SearchExts.GetSearchFromID(tiR->recDone->searchID);
               if (!search) return;

               // check if recording has ended before timer end

               bool complete = true;
	       LOCK_RECORDINGS_READ;
	       const cRecording *pRecording = Recordings->GetByName(m_filename);
	       long timerLengthSecs = tiR->timer->StopTime()-tiR->timer->StartTime();
	       int recFraction = 100;
	       if (pRecording && timerLengthSecs)
	       {
		  int recLen = gl_recStatusMonitor->RecLengthInSecs(pRecording);
		  recFraction = double(recLen) * 100 / timerLengthSecs;
	       }
	       bool vpsUsed = tiR->timer->HasFlags(tfVps) && tiR->timer->Event() && tiR->timer->Event()->Vps();
               if ((!vpsUsed && now < tiR->timer->StopTime()) || recFraction < (vpsUsed ? 90: 98)) // assure timer has reached its end or at least 98% were recorded
               {
                  complete = false;
                  LogFile.Log(1,"finished: '%s' (not complete! - recorded only %d%%); search timer: '%s'; VPS used: %s", tiR->timer->File(), recFraction, search->search, vpsUsed ? "Yes": "No");
               }
               else
	       {
                  LogFile.Log(1,"finished: '%s'; search timer: '%s'; VPS used: %s", tiR->timer->File(), search->search, vpsUsed ? "Yes": "No");
		  if (recFraction < 100)
		    LogFile.Log(2,"recorded %d%%'", recFraction);
	       }
               if (complete)
               {
                  RecsDone.Add(tiR->recDone);
                  LogFile.Log(1,"added rec done for '%s~%s';%s", tiR->recDone->title?tiR->recDone->title:"unknown title",
                              tiR->recDone->shortText?tiR->recDone->shortText:"unknown subtitle",
                              search->search);
                  RecsDone.Save();
                  tiR->recDone = NULL; // prevent deletion
                  tiR->lastBreak = 0;

                  // check for search timers to delete automatically
                  SearchExts.CheckForAutoDelete(search);

                  // trigger a search timer update (skip running events)
		  search->skipRunningEvents = true;
                  updateForced = 1;
               }
               else if (tiR->lastBreak == 0) // store first break
                  tiR->lastBreak = now;
            }
            if (tiR->lastBreak == 0 || (now - tiR->lastBreak) > ALLOWED_BREAK_INSECS)
            { // remove finished recordings or those with an unallowed break
               if (tiR->recDone) delete tiR->recDone; // clean up
               cRecDoneTimerObj *tiRNext = gl_recStatusMonitor->TimersRecording.Next(tiR);
               gl_recStatusMonitor->TimersRecording.Del(tiR);
               tiR = tiRNext;
               continue;
            }
            break;
         }
         if (!found)
         {
            if (tiR->recDone) delete tiR->recDone; // clean up
            cRecDoneTimerObj *tiRNext = gl_recStatusMonitor->TimersRecording.Next(tiR);
            gl_recStatusMonitor->TimersRecording.Del(tiR);
            tiR = tiRNext;
            continue;
         }
         tiR = gl_recStatusMonitor->TimersRecording.Next(tiR);
      }
   }
   m_Active = false;
}

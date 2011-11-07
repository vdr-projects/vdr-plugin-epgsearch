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

#include "epgsearchtools.h"
#include "recstatus.h"
#include "recdone.h"
#include "conflictcheck_thread.h"
#include "epgsearchcfg.h"
#include <math.h>

#define ALLOWED_BREAK_INSECS 2

extern int updateForced;
extern int gl_InfoConflict;


cRecStatusMonitor* gl_recStatusMonitor = NULL;

cRecStatusMonitor::cRecStatusMonitor()
{
}

void cRecStatusMonitor::Recording(const cDevice *Device, const char *Name, const char* Filename, bool On)
{
   time_t now = time(NULL);
   // insert new timers currently recording in TimersRecording
   if (On && Name)
   {
      if (EPGSearchConfig.checkTimerConflOnRecording)
         cConflictCheckThread::Init((cPluginEpgsearch*)cPluginManager::GetPlugin("epgsearch"), true);

      for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti))
         if (ti->Recording())
         {
            // check if this is a new entry
            cRecDoneTimerObj *tiRFound = NULL;
            for (cRecDoneTimerObj *tiR = TimersRecording.First(); tiR; tiR = TimersRecording.Next(tiR))
               if (tiR->timer == ti)
               {
                  tiRFound = tiR;
                  break;
               }

            if (tiRFound) // already handled, perhaps a resume
            {
               if (tiRFound->lastBreak > 0 && now - tiRFound->lastBreak <= ALLOWED_BREAK_INSECS)
               {
                  LogFile.Log(1,"accepting resume of '%s' on device %d", Name, Device->CardIndex());
                  tiRFound->lastBreak = 0;
               }
               continue;
            }

            cRecDoneTimerObj* timerObj = new cRecDoneTimerObj(ti, Device->DeviceNumber());
            TimersRecording.Add(timerObj);

            cSearchExt* search = TriggeredFromSearchTimer(ti);
            if (!search || (search->avoidRepeats == 0 && search->delMode == 0)) // ignore if not avoid repeats and no auto-delete
               continue;

            bool vpsUsed = ti->HasFlags(tfVps) && ti->Event() && ti->Event()->Vps();
            LogFile.Log(1,"recording started '%s' on device %d (search timer '%s'); VPS used: %s", Name, Device->CardIndex(), search->search, vpsUsed ? "Yes": "No");
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

   if (!On)
   {
      cMutexLock RecsDoneLock(&RecsDone);
      // remove timers that finished recording from TimersRecording
      // incomplete recordings are kept for a while, perhaps they will be resumed
      cRecDoneTimerObj *tiR = TimersRecording.First();
      while(tiR)
      {
         // check if timer still exists
         bool found = false;
         for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti))
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
	       cRecording *pRecording = Recordings.GetByName(Filename);
	       long timerLengthSecs = tiR->timer->StopTime()-tiR->timer->StartTime();
	       int recFraction = 100;
	       if (pRecording && timerLengthSecs)
	       {
		  int recLen = RecLengthInSecs(pRecording);
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
               cRecDoneTimerObj *tiRNext = TimersRecording.Next(tiR);
               TimersRecording.Del(tiR);
               tiR = tiRNext;
               continue;
            }
            break;
         }
         if (!found)
         {
            if (tiR->recDone) delete tiR->recDone; // clean up
            cRecDoneTimerObj *tiRNext = TimersRecording.Next(tiR);
            TimersRecording.Del(tiR);
            tiR = tiRNext;
            continue;
         }
         tiR = TimersRecording.Next(tiR);
      }
   }
}

int cRecStatusMonitor::TimerRecDevice(cTimer* timer)
{
   if (!timer) return 0;
   for (cRecDoneTimerObj *tiR = TimersRecording.First(); tiR; tiR = TimersRecording.Next(tiR))
      if (tiR->timer == timer && timer->Recording()) return tiR->deviceNr+1;
   return 0;
}

bool cRecStatusMonitor::IsPesRecording(cRecording *pRecording)
{
#if VDRVERSNUM < 10703
  return true;
#else
  return pRecording && pRecording->IsPesRecording();
#endif
}

#define LOC_INDEXFILESUFFIX     "/index"

#if VDRVERSNUM < 10703

int cRecStatusMonitor::RecLengthInSecs(cRecording *pRecording)
{
  struct stat buf;
  cString fullname = cString::sprintf("%s%s", pRecording->FileName(), "/index.vdr");
  if (stat(fullname, &buf) == 0)
  {
    struct tIndex { int offset; uchar type; uchar number; short reserved; };
    int delta = buf.st_size % sizeof(tIndex);
    if (delta)
    {
      delta = sizeof(tIndex) - delta;
      esyslog("ERROR: invalid file size (%ld) in '%s'", buf.st_size, *fullname);
    }
    return (buf.st_size + delta) / sizeof(tIndex) / SecondsToFrames(1);
  }
  else
    return -1;
}

#else

struct tIndexTs {
  uint64_t offset:40; // up to 1TB per file (not using off_t here - must definitely be exactly 64 bit!)
  int reserved:7;     // reserved for future use
  int independent:1;  // marks frames that can be displayed by themselves (for trick modes)
  uint16_t number:16; // up to 64K files per recording
  tIndexTs(off_t Offset, bool Independent, uint16_t Number)
  {
    offset = Offset;
    reserved = 0;
    independent = Independent;
    number = Number;
  }
  };

int cRecStatusMonitor::RecLengthInSecs(cRecording *pRecording)
{
  struct stat buf;
  cString fullname = cString::sprintf("%s%s", pRecording->FileName(), IsPesRecording(pRecording) ? LOC_INDEXFILESUFFIX ".vdr" : LOC_INDEXFILESUFFIX);
  if (pRecording->FileName() && *fullname && access(fullname, R_OK) == 0 && stat(fullname, &buf) == 0)
    {
      double frames = buf.st_size ? (buf.st_size - 1) / sizeof(tIndexTs) + 1 : 0;
      double Seconds = 0;
      modf((frames + 0.5) / pRecording->FramesPerSecond(), &Seconds);
      return Seconds;
    }
  return -1;
}
#endif


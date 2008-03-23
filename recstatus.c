/*
Copyright (C) 2004-2008 Christian Wieninger

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

#define ALLOWED_BREAK_INSECS 2

extern int updateForced;
extern int gl_InfoConflict;


cRecStatusMonitor* gl_recStatusMonitor = NULL;

cRecStatusMonitor::cRecStatusMonitor()
{
}

void cRecStatusMonitor::Recording(const cDevice *Device, const char *Name, const char*, bool On) 
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

            LogFile.Log(1,"recording started '%s' on device %d (search timer '%s')", Name, Device->CardIndex(), search->search);
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
            if (now < ti->StartTime() + 60) // allow a delay of one minute
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
               if (now < tiR->timer->StopTime())
               {
                  complete = false;
                  LogFile.Log(1,"finished: '%s' (not complete!); search timer: '%s'", tiR->timer->File(), search->search);
               }
               else
                  LogFile.Log(1,"finished: '%s'; search timer: '%s'", tiR->timer->File(), search->search);
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

                  // trigger a search timer update
                  updateForced = 1;			
               }
               else
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

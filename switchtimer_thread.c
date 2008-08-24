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

#include "switchtimer_thread.h"
#include "epgsearchcfg.h"
#include "epgsearchtools.h"
#include <vdr/tools.h>
#include <vdr/plugin.h>

#define MSG_DELAY 5
#define SWITCHTIMER_WAIT 20

cSwitchTimerThread *cSwitchTimerThread::m_Instance = NULL;

cSwitchTimerThread::cSwitchTimerThread(void)
   : cThread("EPGSearch: switchtimer")
{
   m_Active = false;
   m_lastUpdate = time(NULL);
}

cSwitchTimerThread::~cSwitchTimerThread() {
   if (m_Active) 
      Stop();
}

void cSwitchTimerThread::Init(void) {
   if (m_Instance == NULL && SwitchTimers.Count() > 0) {
      m_Instance = new cSwitchTimerThread;
      m_Instance->Start();
   }
}

void cSwitchTimerThread::Exit(void) {
   if (m_Instance != NULL) {
      m_Instance->Stop();
      DELETENULL(m_Instance);
   }
}

void cSwitchTimerThread::Stop(void) {
   m_Active = false;
   Wait.Signal();
   Cancel(6);
}

void cSwitchTimerThread::Action(void) 
{
  m_Active = true;
  
  // let VDR do its startup
  for(int wait = 0; wait < SWITCHTIMER_WAIT && m_Active; wait++)
    Wait.Wait(1000);

   time_t nextUpdate = time(NULL);
   while (m_Active) 
   {
      time_t now = time(NULL);
      if (now >= nextUpdate)
      {
         LogFile.Log(3,"locking switch timers");
         cMutexLock SwitchTimersLock(&SwitchTimers);
         LogFile.Log(3,"switch timer check started");
         cSwitchTimer* switchTimer = SwitchTimers.First();
         while (switchTimer && m_Active) 
         {
            const cEvent* event = switchTimer->event;
            if (event && event->StartTime() - now < switchTimer->switchMinsBefore*60 + MSG_DELAY + 1)
            {
               cChannel *channel = Channels.GetByChannelID(event->ChannelID(), true, true);
               bool doswitch = (switchTimer->announceOnly == 0);
               SwitchTimers.Del(switchTimer);
               if (channel && (event->EndTime() >= now))
               {
                  // announce and switch
                  if (doswitch)
                  {
                     LogFile.Log(1,"switching to channel %d", channel->Number());
                     cString cmd = cString::sprintf("CHAN %d", channel->Number());
                     SendViaSVDRP(cmd);

		     if (switchTimer->unmute && cDevice::PrimaryDevice()->IsMute())
		       cDevice::PrimaryDevice()->ToggleMute();
                  }
                  cString Message = cString::sprintf("%s: %s - %s", event->Title(), 
						     CHANNELNAME(channel), GETTIMESTRING(event));
                  SendMsg(Message);
                  if (m_Active)
		    Wait.Wait(1000 * MSG_DELAY);
               }
               SwitchTimers.Save();
               break;
            }
            switchTimer = SwitchTimers.Next(switchTimer);
         }
         LogFile.Log(3,"switch timer check finished");
         if (m_Active)
	    Wait.Wait(1000 * MSG_DELAY);
         m_lastUpdate = time(NULL);
         nextUpdate = long(m_lastUpdate/60)*60+ 60 - MSG_DELAY ; // check at each full minute minus 5sec
         if (SwitchTimers.Count() == 0)
            m_Active = false;
      }
      while (m_Active && time(NULL)%MSG_DELAY != 0) // sync heart beat to MSG_DELAY
	Wait.Wait(1000);
      Wait.Wait(1000);
   };
   m_Instance = NULL;
}


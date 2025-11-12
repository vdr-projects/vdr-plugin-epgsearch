/*                                                                  -*- c++ -*-
Copyright (C) 2004-2013 Christian Wieninger 2017 Johann Friedrichs

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
*/

#include <string>
#include <list>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif
#include "recdone_thread.h"
#include "recdone.h"
#include "epgsearchcfg.h"

#include <vdr/tools.h>
#include <vdr/plugin.h>
#define ALLOWED_BREAK_INSECS 2

extern int updateForced;

cRecdoneThread::cRecdoneThread()
    : cThread("EPGSearch: recdone")
{
}

cRecdoneThread::~cRecdoneThread()
{
}

bool cRecdoneThread::IsPesRecording(const cRecording *pRecording)
{
    return pRecording && pRecording->IsPesRecording();
}

#define LOC_INDEXFILESUFFIX     "/index"

struct tIndexTs {
    uint64_t offset: 40; // up to 1TB per file (not using off_t here - must definitely be exactly 64 bit!)
    int reserved: 7;    // reserved for future use
    int independent: 1; // marks frames that can be displayed by themselves (for trick modes)
    uint16_t number: 16; // up to 64K files per recording
    tIndexTs(off_t Offset, bool Independent, uint16_t Number) {
        offset = Offset;
        reserved = 0;
        independent = Independent;
        number = Number;
    }
};

int cRecdoneThread::RecLengthInSecs(const cRecording *pRecording)
{
    struct stat buf;
    cString fullname = cString::sprintf("%s%s", pRecording->FileName(), IsPesRecording(pRecording) ? LOC_INDEXFILESUFFIX ".vdr" : LOC_INDEXFILESUFFIX);
    if (pRecording->FileName() && *fullname && access(fullname, R_OK) == 0 && stat(fullname, &buf) == 0) {
        double frames = buf.st_size ? (buf.st_size - 1) / sizeof(tIndexTs) + 1 : 0;
        double Seconds = 0;
        modf((frames + 0.5) / pRecording->FramesPerSecond(), &Seconds);
        return Seconds;
    }
    return -1;
}

void cRecdoneThread::Action(void)
{
    LogFile.Log(1, "started recdone_thread");
    cMutexLock RecsDoneLock(&RecsDone);
    time_t now = time(NULL);
    // remove timers that finished recording from TimersRecording
    // incomplete recordings are kept for a while, perhaps they will be resumed
    LOCK_TIMERS_READ; // must be done before TimersRecordingLock
    while (m_fnames.size()) {
        std::vector<std::string>::iterator it = m_fnames.begin();
        const char *m_filename = (*it).c_str();
        LogFile.Log(1, "recdone_thread processing %s", m_filename);
        cMutexLock TimersRecordingLock(&TimersRecording);
        cRecDoneTimerObj *tiR = TimersRecording.First();
        while (tiR) {
            // check if a timer still exists
            // if we find a timer we take its values, else from recDone
            bool found = false;
            for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti))
                if (ti == tiR->timer) {
                    found = true;
                    break;
                }

            if (!tiR->recDone) {
                LogFile.Log(3, "recdone: this tiR has no recDone struct");
                tiR = TimersRecording.Next(tiR);
                continue;
            }
            if (found && tiR->timer->Recording()) {
                LogFile.Log(3, "recdone: tiR for %s is recording",tiR->recDone->fileName);
                tiR = TimersRecording.Next(tiR);
                continue;
            }

            LogFile.Log(2, "recdone: processing tiR for %s",tiR->recDone->fileName);
            if (strcmp(tiR->recDone->fileName,m_filename) != 0) {
                LogFile.Log(3, "recdone: tiR not the same filename");
                tiR = TimersRecording.Next(tiR);
                continue;
            }

            // we have the correct entry for the check
            bool complete = true;
            cSearchExt* search = SearchExts.GetSearchFromID(tiR->recDone->searchID);
            int recFraction = 100;
#if defined(APIVERSNUM) && APIVERSNUM > 20503
            int recordingErrors = 0;
#endif
            time_t stopTime = found ? tiR->timer->StopTime() : tiR->recDone->timerStop;

            if (tiR->lastBreak == -1 || !search) { // started too late or missing searchID
                LogFile.Log(2, "started too late : '%s' or missing searchID %d", found?tiR->timer->File():m_filename, tiR->recDone->searchID);
                tiR->lastBreak = 0; // triggers deletion
            }
            else {
                // check if recording length is as expected
                const cRecording *pRecording;
                {
                    LOCK_RECORDINGS_READ;
                    pRecording = Recordings->GetByName(m_filename);
                    long timerLengthSecs = found ? tiR->timer->StopTime() - tiR->timer->StartTime() : tiR->recDone->timerStop - tiR->recDone->timerStart;
                    int recLen = 0;
                    if (pRecording && timerLengthSecs) {
                        recLen = RecLengthInSecs(pRecording);
                        recFraction = double(recLen) * 100 / timerLengthSecs;
                    }
#if defined(APIVERSNUM) && APIVERSNUM > 20503
                    if (pRecording) {
                        const cRecordingInfo *Info = pRecording->Info();
                        recordingErrors = Info->Errors();
                        if (recordingErrors)
                            LogFile.Log(2, "Recording had %d errors", recordingErrors);
                    }
#endif
                }
                bool vpsUsed = found ? tiR->timer->HasFlags(tfVps) && tiR->timer->Event() && tiR->timer->Event()->Vps():tiR->recDone->vpsUsed;
                if ((!vpsUsed && now < stopTime) || recFraction < (vpsUsed ? 90 : 98)) { // assure timer has reached its end or at least 98% were recorded
                    complete = false;
#if defined(APIVERSNUM) && APIVERSNUM > 20503
                    LogFile.Log(1, "finished: '%s' (not complete! - recorded only %d%% %d errors); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recFraction, recordingErrors, search->search, vpsUsed ? "Yes" : "No");
                    dsyslog("epgsearch: finished: '%s' (not complete! - recorded only %d%% %d errors); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recFraction, recordingErrors, search->search, vpsUsed ? "Yes" : "No");
#else
                    LogFile.Log(1, "finished: '%s' (not complete! - recorded only %d%%); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recFraction, search->search, vpsUsed ? "Yes" : "No");
                    dsyslog("epgsearch: finished: '%s' (not complete! - recorded only %d%%); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recFraction, search->search, vpsUsed ? "Yes" : "No");
#endif
#if defined(APIVERSNUM) && APIVERSNUM > 20503
                } else  if (recordingErrors > EPGSearchConfig.AllowedErrors) {
                        complete = false;  // reception errors
                        LogFile.Log(1, "finished: '%s' but %d errors); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recordingErrors, search->search, vpsUsed ? "Yes" : "No");
                        dsyslog("epgsearch: finished: '%s' but %d errors); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, recordingErrors, search->search, vpsUsed ? "Yes" : "No");
#endif
                } else {
                    // recording complete
                    LogFile.Log(1, "finished: '%s' (complete); search timer: '%s'; VPS used: %s", found?tiR->timer->File():m_filename, search->search, vpsUsed ? "Yes" : "No");
                    if (recFraction < 100) {
                        LogFile.Log(2, "recorded %d%%'", recFraction);
                        dsyslog("epgsearch: finished: '%s' (complete) recorded %d%%", found?tiR->timer->File():m_filename, (recFraction<100) ? recFraction : 100);
                    }
                    else
                        dsyslog("epgsearch: finished: '%s' (complete)", found?tiR->timer->File():m_filename);
                }
                if (complete) { // add to epgsearchdone.data
                    RecsDone.Add(tiR->recDone);
                    LogFile.Log(1, "added rec done for '%s~%s';%s", tiR->recDone->title ? tiR->recDone->title : "unknown title",
                                tiR->recDone->shortText ? tiR->recDone->shortText : "unknown subtitle",
                                search->search);
                    RecsDone.Save();
                    tiR->recDone = NULL; // prevent deletion (now in RecsDone)
                    tiR->lastBreak = 0;

                    // check for search timers to delete automatically
                    SearchExts.CheckForAutoDelete(search);

                    // trigger a search timer update (skip running events)
                    search->skipRunningEvents = true;
                    updateForced = 1;
                } else if (tiR->lastBreak == 0) // not complete: assure break is set
                    tiR->lastBreak = now;
            }
            // cleanup
            if (!tiR->lastBreak || (now - tiR->lastBreak) > ALLOWED_BREAK_INSECS) {
                // remove finished recordings or those with an unallowed break
                if (tiR->recDone) { // if added to searchdone recDone is NULL
                  LogFile.Log(3, "recdone: remove tiR and recdone for %s", tiR->recDone->fileName);
                  delete tiR->recDone; // clean up
                }
                cRecDoneTimerObj *tiRNext = TimersRecording.Next(tiR);
                TimersRecording.Del(tiR);
                tiR = tiRNext;
                continue;
            }
            tiR = TimersRecording.Next(tiR);
        } // while tiR
        m_fnames.erase(it);
    } // while fnames
    LogFile.Log(1, "recdone_thread ended");
}

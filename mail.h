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

#ifndef __EPGSEARCH_MAIL_H
#define __EPGSEARCH_MAIL_H

#include <string>
#include <set>

#include <vdr/thread.h>
#include <vdr/plugin.h>
#include "conflictcheck.h"

using std::string;
using std::set;

// --- cMailNotifier --------------------------------------------------------
class cMailNotifier 
{
 protected:
    string subject;
    string body;

    bool SendMailViaSendmail();
    bool SendMailViaScript();
    bool SendMail();
    bool ExecuteMailScript(string ScriptArgs);
 public:
    string scriptReply;

    cMailNotifier() {}
    cMailNotifier(string Subject, string Body);
    bool TestMailAccount(string MailAddressTo, string MailAddress, string MailServer, string AuthUser, string AuthPass);
    static string LoadTemplate(const string& templtype);
    static string GetTemplValue(const string& templ, const string& entry);

    static string MailCmd;
};

class cMailTimerNotification
{
    tEventID eventID;
    tChannelID channelID;
    uint timerMod;

    const cEvent* GetEvent() const;

 public:
 cMailTimerNotification(tEventID EventID, tChannelID ChannelID, uint TimerMod = tmNoChange) 
   : eventID(EventID), channelID(ChannelID), timerMod(TimerMod) {}
    bool operator< (const cMailTimerNotification &N) const;
    string Format(const string& templ) const;
};

class cMailDelTimerNotification
{
    tChannelID channelID;
    time_t start;
 public:
    string formatted;

    cMailDelTimerNotification(cTimer* t, const cEvent* pEvent, const string& templ); 
    bool operator< (const cMailDelTimerNotification &N) const;
};

class cMailUpdateNotifier : public cMailNotifier
{
    set<cMailTimerNotification> newTimers;
    set<cMailTimerNotification> modTimers;
    set<cMailDelTimerNotification> delTimers;

    string mailTemplate;
 public:
    cMailUpdateNotifier();
    void AddNewTimerNotification(tEventID EventID, tChannelID ChannelID);
    void AddModTimerNotification(tEventID EventID, tChannelID ChannelID, uint timerMod = tmNoChange);
    void AddRemoveTimerNotification(cTimer* t, const cEvent* e = NULL);
    void SendUpdateNotifications();
};

class cMailConflictNotifier : public cMailNotifier
{
 public:
    void SendConflictNotifications(cConflictCheck& conflictcheck);
};

#endif

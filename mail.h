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

#ifndef __EPGSEARCH_MAIL_H
#define __EPGSEARCH_MAIL_H

#include <string>
#include <set>

#include <vdr/thread.h>
#include <vdr/plugin.h>
#include "conflictcheck.h"

// --- cMailNotifier --------------------------------------------------------
class cMailNotifier
{
protected:
    std::string subject;
    std::string body;

    bool SendMailViaSendmail();
    bool SendMailViaScript();
    bool SendMail(bool force = false);
    bool ExecuteMailScript(std::string ScriptArgs);
public:
    std::string scriptReply;

    cMailNotifier() {}
    cMailNotifier(std::string Subject, std::string Body);
    bool TestMailAccount(std::string MailAddressTo, std::string MailAddress, std::string MailServer, std::string AuthUser, std::string AuthPass);
    static std::string LoadTemplate(const std::string& templtype);
    static std::string GetTemplValue(const std::string& templ, const std::string& entry);

    static std::string MailCmd;
};

class cMailTimerNotification
{
    friend class cMailUpdateNotifier;
    tEventID eventID;
    tChannelID channelID;
    uint timerMod;

protected:
    virtual const cEvent* GetEvent() const;

public:
    cMailTimerNotification(tEventID EventID, tChannelID ChannelID, uint TimerMod = tmNoChange)
        : eventID(EventID), channelID(ChannelID), timerMod(TimerMod) {}
    virtual bool operator< (const cMailTimerNotification &N) const;
    virtual std::string Format(const std::string& templ) const;
};

class cMailDelTimerNotification
{
    friend class cMailUpdateNotifier;
    time_t start;
    tChannelID channelID;
public:
    std::string formatted;

    cMailDelTimerNotification(const cTimer* t, const cEvent* pEvent, const std::string& templ);
    cMailDelTimerNotification(const std::string& Formatted, tChannelID ChannelID, time_t Start);
    bool operator< (const cMailDelTimerNotification &N) const;
    std::string Format(const std::string& templ) const {
        return formatted;
    }
};

class cMailAnnounceEventNotification : public cMailTimerNotification
{
    friend class cMailUpdateNotifier;
    int searchextID;
public:
    cMailAnnounceEventNotification(tEventID EventID, tChannelID ChannelID, int SearchExtID)
        : cMailTimerNotification(EventID, ChannelID), searchextID(SearchExtID) {}
    std::string Format(const std::string& templ) const;
};

class cMailUpdateNotifier : public cMailNotifier
{
    std::set<cMailTimerNotification> newTimers;
    std::set<cMailTimerNotification> modTimers;
    std::set<cMailDelTimerNotification> delTimers;
    std::set<cMailAnnounceEventNotification> announceEvents;

    std::string mailTemplate;
public:
    cMailUpdateNotifier();
    void AddNewTimerNotification(tEventID EventID, tChannelID ChannelID);
    void AddModTimerNotification(tEventID EventID, tChannelID ChannelID, uint timerMod = tmNoChange);
    void AddRemoveTimerNotification(const cTimer* t, const cEvent* e = NULL);
    void AddRemoveTimerNotification(const std::string& Formatted, tChannelID ChannelID, time_t Start);
    void AddAnnounceEventNotification(tEventID EventID, tChannelID ChannelID, int SearchExtID);
    void SendUpdateNotifications();
};

class cMailConflictNotifier : public cMailNotifier
{
public:
    void SendConflictNotifications(cConflictCheck& conflictcheck);
};

#endif

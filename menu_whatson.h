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

#ifndef __EPGSEARCH_MENUWHATSON_H
#define __EPGSEARCH_MENUWHATSON_H

#include <vdr/menuitems.h>
#include "epgsearchcfg.h"
#include "templatefile.h"
#include "menu_event.h"

// --- cMenuMyScheduleItem ------------------------------------------------------
class cMenuMyScheduleItem : public cOsdItem {
public:
  const cEvent *event;
  cChannel *channel;
  showMode mode;
  int timerMatch;
  bool inSwitchList;
  cMenuTemplate* menuTemplate;

  cMenuMyScheduleItem(const cEvent *Event, cChannel *Channel = NULL, showMode ShowMode = showNow, cMenuTemplate* menuTemplate = NULL);
  bool Update(bool Force = false);
};

// --- cMenuWhatsOnSearch ----------------------------------------------------------
class cMenuWhatsOnSearch : public cOsdMenu {
private:
  int helpKeys;
  const cSchedules* schedules;
  eOSState Record(void);
  eOSState ExtendedSearch(void);
  static int currentChannel;
  cEventObjects eventObjects;
 public:
  static cChannel *scheduleChannel;
  static cList<cShowMode> showModes;
  static showMode currentShowMode;
  static int shiftTime;
  static time_t seekTime;
  time_t GetTimeT(int iTime);
  static showMode GetNextMode();
  cMenuWhatsOnSearch(const cSchedules *Schedules, int CurrentChannelNr);
  ~cMenuWhatsOnSearch();
  void LoadSchedules();
  static int CurrentChannel(void) { return currentChannel; }
  static void SetCurrentChannel(int ChannelNr) { currentChannel = ChannelNr; }
  static cChannel *ScheduleChannel(cChannel* forceChannel = NULL);
  virtual eOSState ProcessKey(eKeys Key);
  virtual eOSState Switch(void);
  virtual eOSState Shift(int);
  virtual eOSState Commands(eKeys Key);
  virtual eOSState ShowSummary();
  void SetHelpKeys(bool Force = false);
  int GetTab(int Tab);
  bool Update(void);
  void CreateShowModes();
  static cShowMode* GetShowMode(showMode mode);
  void UpdateCurrent();
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind();
  virtual void Display(void);
#endif
};

#endif

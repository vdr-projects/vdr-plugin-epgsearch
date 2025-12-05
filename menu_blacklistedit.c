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

#include <vector>
#include "menu_blacklistedit.h"
#include "changrp.h"
#include "epgsearchcats.h"
#include "epgsearchtools.h"
#include "menu_dirselect.h"
#include "menu_searchedit.h"
#include "menu_searchtemplate.h"
#include "epgsearchcfg.h"

#include <math.h>

extern cChannelGroups ChannelGroups;
extern cSearchExtCats SearchExtCats;

extern const char AllowedChars[];

// --- cMenuBlacklistEdit --------------------------------------------------------
cMenuBlacklistEdit::cMenuBlacklistEdit(cBlacklist *Blacklist, bool New)
    : cOsdMenu(tr("Edit blacklist"), 32)
{
    SetMenuCategory(mcSetupPlugins);
    SearchModes[0] = tr("phrase");
    SearchModes[1] = tr("all words");
    SearchModes[2] = tr("at least one word");
    SearchModes[3] = tr("match exactly");
    SearchModes[4] = tr("regular expression");
    SearchModes[5] = tr("fuzzy");

    DaysOfWeek[0] = strdup(WeekDayName(0));
    DaysOfWeek[1] = strdup(WeekDayName(1));
    DaysOfWeek[2] = strdup(WeekDayName(2));
    DaysOfWeek[3] = strdup(WeekDayName(3));
    DaysOfWeek[4] = strdup(WeekDayName(4));
    DaysOfWeek[5] = strdup(WeekDayName(5));
    DaysOfWeek[6] = strdup(WeekDayName(6));
    DaysOfWeek[7] = strdup(tr("user-defined"));

    UseChannelSel[0] = trVDR("no");
    UseChannelSel[1] = tr("interval");
    UseChannelSel[2] = tr("channel group");
    UseChannelSel[3] = tr("only FTA");

    ContentsMatchingMode[0] = tr("all selected descriptors");
    ContentsMatchingMode[1] = tr("any selected descriptor");
    ContentsMatchingMode[2] = tr("some selected descriptor per group");

    SpecialCharacteristicsMatchingMode[0] = tr("all selected descriptors");
    SpecialCharacteristicsMatchingMode[1] = tr("any selected descriptor");

    EPGInfoMatchingMode[0] = tr("all categories");
    EPGInfoMatchingMode[1] = tr("all except missing categories");
    EPGInfoMatchingMode[2] = tr("at least one category");

    if (New) {
        cSearchExt* SearchTempl = NULL; // copy the default settings, if we have a default template
        cMutexLock SearchTemplatesLock(&SearchTemplates);
        cSearchExt *SearchExtTempl = SearchTemplates.First();
        while (SearchExtTempl) {
            if (SearchExtTempl->ID == EPGSearchConfig.DefSearchTemplateID)
                SearchTempl = SearchExtTempl;
            SearchExtTempl = SearchTemplates.Next(SearchExtTempl);
        }
        if (SearchTempl)
            Blacklist->CopyFromTemplate(SearchTempl);
    }

    blacklist = Blacklist;
    addIfConfirmed = New;

    if (blacklist) {
        data = *blacklist;
        UserDefDayOfWeek = 0;
        if (blacklist->DayOfWeek < 0) {
            UserDefDayOfWeek = blacklist->DayOfWeek;
            data.DayOfWeek = 7;
        }

        menuitemsChGr = NULL;
        channelGroupName = NULL;

        channelMin = channelMax = cDevice::CurrentChannel();
        channelGroupNr = 0;
        if (data.useChannel == 1) {
            channelMin = data.channelMin->Number();
            channelMax = data.channelMax->Number();
        }
        if (data.useChannel == 2) {
            channelGroupNr = ChannelGroups.GetIndex(data.channelGroup);
            if (channelGroupNr == -1) {
                free(data.channelGroup);
                data.channelGroup = NULL;
                channelGroupNr = 0; // no selection
            } else {
                channelGroupName = strdup(data.channelGroup);
                channelGroupNr++;
            }
        }

        // collect content string IDs
        std::set<std::string> contentStrings;
        for (unsigned int i = 0; i < CONTENT_DESCRIPTOR_MAX; i++) {
            const std::string contentDescr = cEvent::ContentToString(i);
            if (!contentDescr.empty() && contentStrings.find(contentDescr) == contentStrings.end()) {
                contentStrings.insert(contentDescr);
                contentStringIDs.push_back(i);
            }
        }

        // set the flags for the content descriptors
        contentStringFlags = (int*) malloc((CONTENT_DESCRIPTOR_MAX + 1) * sizeof(int));
        for (unsigned int i = 0; i <= CONTENT_DESCRIPTOR_MAX; i++)
            contentStringFlags[i] = data.HasContentID(i);

        catvaluesNumeric = NULL;
        if (SearchExtCats.Count() > 0) {
            catvaluesNumeric = (int*) malloc(SearchExtCats.Count() * sizeof(int));
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                catvaluesNumeric[index] = atol(blacklist->catvalues[index]);
                SearchExtCat = SearchExtCats.Next(SearchExtCat);
                index++;
            }
        }
        Set();
    }
}

void cMenuBlacklistEdit::Set()
{
    int current = Current();
    Clear();

    Add(new cMenuEditStrItem(tr("Search term"), data.search, sizeof(data.search), tr(AllowedChars)));
    Add(new cMenuEditStraItem(tr("Search mode"),     &data.mode, 6, SearchModes));
    if (data.mode == 5) // fuzzy
        Add(new cMenuEditIntItem(IndentMenuItem(tr("Tolerance")), &data.fuzzyTolerance, 1, 9));
    Add(new cMenuEditBoolItem(tr("Match case"), &data.useCase, trVDR("no"), trVDR("yes")));
    Add(new cMenuEditBoolItem(tr("Use title"), &data.useTitle, trVDR("no"), trVDR("yes")));
    Add(new cMenuEditBoolItem(tr("Use subtitle"), &data.useSubtitle, trVDR("no"), trVDR("yes")));
    Add(new cMenuEditBoolItem(tr("Use description"), &data.useDescription, trVDR("no"), trVDR("yes")));

    Add(new cMenuEditBoolItem(tr("Use parental rating"), &data.useParentalRating, trVDR("no"), trVDR("yes")));
    if (data.useParentalRating == true) {
        Add(new cMenuEditIntItem(tr("  Min. rating"), &data.minParentalRating, 0, 18));
        Add(new cMenuEditIntItem(tr("  Max. rating"), &data.maxParentalRating, 0, 18));
    }

    Add(new cMenuEditBoolItem(tr("Use content descriptors"), &data.useContentsFilter, trVDR("no"), trVDR("yes")));
    if (data.useContentsFilter) {
        Add(new cMenuEditStraItem(tr("  Contents matching mode"), &data.contentsCategoryMatchingMode, 3, ContentsMatchingMode));
        std::vector<int>::const_iterator it;
        for (unsigned int i = 0; i < contentStringIDs.size(); i++) {
            int level = (contentStringIDs[i] % 0x10 == 0 ? 1 : 2);
            if (contentStringIDs[i] == 0xB0) {
                Add(new cMenuEditStraItem(tr("  Special characteristics"), &data.contentsCharacteristicsMatchingMode, 2, SpecialCharacteristicsMatchingMode));
                level = 2;
            }
            Add(new cMenuEditBoolItem(IndentMenuItem(tr(cEvent::ContentToString(contentStringIDs[i])), level), &contentStringFlags[contentStringIDs[i]], trVDR("no"), trVDR("yes")));
        }
    }

    // show Categories only if we have them
    if (SearchExtCats.Count() > 0) {
        Add(new cMenuEditBoolItem(tr("Use extended EPG info"), &data.useExtEPGInfo, trVDR("no"), trVDR("yes")));
        if (data.useExtEPGInfo) {
            Add(new cMenuEditStraItem(IndentMenuItem(tr("Category matching mode")), &data.extEPGInfoMatchingMode, 3, EPGInfoMatchingMode));
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                if (SearchExtCat->searchmode >= 10)
                    Add(new cMenuEditIntItem(IndentMenuItem(SearchExtCat->menuname), &catvaluesNumeric[index], 0, 999999, ""));
                else
                    Add(new cMenuEditStrItem(IndentMenuItem(SearchExtCat->menuname), data.catvalues[index], MaxFileName, tr(AllowedChars)));
                SearchExtCat = SearchExtCats.Next(SearchExtCat);
                index++;
            }
        }
    }

    Add(new cMenuEditStraItem(tr("Use channel"), &data.useChannel, 4, UseChannelSel));
    if (data.useChannel == 1) {
        Add(new cMenuEditChanItem(tr("  from channel"), &channelMin));
        Add(new cMenuEditChanItem(tr("  to channel"), &channelMax));
    }
    if (data.useChannel == 2) {
        // create the char array for the menu display
        delete [] menuitemsChGr;
        menuitemsChGr = ChannelGroups.CreateMenuitemsList();
        int oldchannelGroupNr = channelGroupNr;
        channelGroupNr = ChannelGroups.GetIndex(channelGroupName);
        if (channelGroupNr == -1) {
            if (oldchannelGroupNr > 0 && oldchannelGroupNr <= ChannelGroups.Count()) // perhaps its name was changed
                channelGroupNr = oldchannelGroupNr;
            else
                channelGroupNr = 0; // no selection
        } else
            channelGroupNr++;
        Add(new cMenuEditStraItem(IndentMenuItem(tr("Channel group")), &channelGroupNr, ChannelGroups.Count() + 1, menuitemsChGr));
    }

    Add(new cMenuEditBoolItem(tr("Use time"), &data.useTime, trVDR("no"), trVDR("yes")));
    if (data.useTime == true) {
        Add(new cMenuEditTimeItem(tr("  Start after"),        &data.startTime));
        Add(new cMenuEditTimeItem(tr("  Start before"),         &data.stopTime));
    }
    Add(new cMenuEditBoolItem(tr("Use duration"), &data.useDuration, trVDR("no"), trVDR("yes")));
    if (data.useDuration == true) {
        Add(new cMenuEditTimeItem(tr("  Min. duration"), &data.minDuration));
        Add(new cMenuEditTimeItem(tr("  Max. duration"), &data.maxDuration));
    }
    Add(new cMenuEditBoolItem(tr("Use day of week"), &data.useDayOfWeek, trVDR("no"), trVDR("yes")));
    if (data.useDayOfWeek) {
        if (data.DayOfWeek < 0) {
            UserDefDayOfWeek = data.DayOfWeek;
            data.DayOfWeek = 7;
        }
        Add(new cMenuEditStraItem(IndentMenuItem(tr("Day of week")), &data.DayOfWeek, 8, DaysOfWeek));
    }
    Add(new cMenuEditBoolItem(tr("Use global"), &data.isGlobal, trVDR("no"), trVDR("yes")));

    SetCurrent(Get(current));
}

cMenuBlacklistEdit::~cMenuBlacklistEdit()
{
    if (addIfConfirmed) delete blacklist; // apparently it wasn't confirmed
    delete [] menuitemsChGr;
    free(channelGroupName);
    free(catvaluesNumeric);
    free(contentStringFlags);
    for (size_t i = 0; i < LENGTHOF(DaysOfWeek); i++)
        free(DaysOfWeek[i]);
}

eOSState cMenuBlacklistEdit::ProcessKey(eKeys Key)
{
    bool bHadSubMenu = HasSubMenu();

    int iTemp_mode = data.mode;
    int iTemp_useTime = data.useTime;
    int iTemp_useChannel = data.useChannel;
    int iTemp_useDuration = data.useDuration;
    int iTemp_useDayOfWeek = data.useDayOfWeek;
    int iTemp_useParentalRating = data.useParentalRating;
    int iTemp_useContentsFilter = data.useContentsFilter;
    int iTemp_useExtEPGInfo = data.useExtEPGInfo;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (iTemp_mode != data.mode ||
        iTemp_useTime != data.useTime ||
        iTemp_useChannel != data.useChannel ||
        iTemp_useDuration != data.useDuration ||
        iTemp_useDayOfWeek != data.useDayOfWeek ||
        iTemp_useParentalRating != data.useParentalRating ||
        iTemp_useContentsFilter != data.useContentsFilter ||
        iTemp_useExtEPGInfo != data.useExtEPGInfo) {
        Set();
        Display();
    }
    const char* ItemText = Get(Current())->Text();

    if (!HasSubMenu()) {
        if (strlen(ItemText) > 0 && strstr(ItemText, tr("  from channel")) == ItemText && ((Key >= k0 &&  Key <= k9) || Key == kLeft || Key == kRight)) {
            channelMax = channelMin;
            Set();
            Display();
        }
    }

    int iOnUserDefDayItem = 0;
    int iOnUseChannelGroups = 0;
    int iOnChannelGroup = 0;
    int iOnTerm = 0;
    int iOnExtCatItemBrowsable = 0;
    int iCatIndex = -1;
    char* catname = NULL;

    if (!HasSubMenu() && strlen(ItemText) > 0) {
        // check, if on an item of ext. EPG info
        int iOnExtCatItem = 0;
        cSearchExtCat *SearchExtCat = SearchExtCats.First();
        int index = 0;
        while (SearchExtCat) {
            if (strstr(ItemText, IndentMenuItem(SearchExtCat->menuname)) == ItemText) {
                iOnExtCatItem = 1;
                if (SearchExtCat->nvalues > 0)
                    iOnExtCatItemBrowsable = 1;
                iCatIndex = index;
                catname = SearchExtCat->menuname;
                break;
            }
            index++;
            SearchExtCat = SearchExtCats.Next(SearchExtCat);
        }

        if (strstr(ItemText, tr("Search term")) == ItemText) {
            if (!InEditMode(ItemText, tr("Search term"), data.search)) { // show template for a new search
                SetHelp(NULL, NULL, NULL, tr("Button$Templates"));
                iOnTerm = 1;
            }
        }
        if (strstr(ItemText, IndentMenuItem(tr("Day of week"))) == ItemText) {
            if (data.DayOfWeek == 7) {
                SetHelp(trVDR("Button$Edit"));
                iOnUserDefDayItem = 1;
            } else
                SetHelp(NULL);
        } else if (strstr(ItemText, tr("Use channel")) == ItemText && data.useChannel == 2) {
            SetHelp(NULL, NULL, NULL, tr("Button$Setup"));
            iOnUseChannelGroups = 1;
        } else if (strstr(ItemText, IndentMenuItem(tr("Channel group"))) == ItemText) {
            SetHelp(NULL, NULL, NULL, tr("Button$Setup"));
            iOnChannelGroup = 1;
        } else if (iOnExtCatItem) {
            if (!InEditMode(ItemText, IndentMenuItem(catname), data.catvalues[iCatIndex]) ||
                SearchExtCats.Get(iCatIndex)->searchmode >= 10)
                SetHelp(NULL, NULL, NULL, iOnExtCatItemBrowsable ? tr("Button$Select") : NULL);
        } else if (strstr(ItemText, tr("Search term")) != ItemText)
            SetHelp(NULL, NULL, NULL, NULL);
    }
    if (state == osUnknown) {
        if (HasSubMenu())
            return osContinue;
        switch (Key) {
        case kOk:
            if (data.useChannel == 1) {
                LOCK_CHANNELS_READ;
                const cChannel *ch = Channels->GetByNumber(channelMin);
                if (ch)
                    data.channelMin = ch;
                else {
                    ERROR(tr("*** Invalid Channel ***"));
                    break;
                }
                ch = Channels->GetByNumber(channelMax);
                if (ch)
                    data.channelMax = ch;
                else {
                    ERROR(tr("*** Invalid Channel ***"));
                    break;
                }
                if (channelMin > channelMax) {
                    ERROR(tr("Please check channel criteria!"));
                    return osContinue;
                }
            }
            if (data.useChannel == 2)
                data.channelGroup = strdup(menuitemsChGr[channelGroupNr]);

            if (blacklist) {
                *blacklist = data;
                if (data.DayOfWeek == 7)
                    blacklist->DayOfWeek = UserDefDayOfWeek;

                // transfer numeric cat values back to search
                cSearchExtCat *SearchExtCat = SearchExtCats.First();
                int index = 0;
                while (SearchExtCat) {
                    if (SearchExtCat->searchmode >= 10) {
                        if (blacklist->catvalues[index]) free(blacklist->catvalues[index]);
                        msprintf(&blacklist->catvalues[index], "%d", catvaluesNumeric[index]);
                    }
                    SearchExtCat = SearchExtCats.Next(SearchExtCat);
                    index++;
                }

                blacklist->SetContentsFilter(data.useContentsFilter ? contentStringFlags : NULL);

                if (addIfConfirmed) {
                    cMutexLock BlacklistLock(&Blacklists);
                    blacklist->ID = Blacklists.GetNewID();
                    Blacklists.Add(blacklist);
                }

                Blacklists.Save();
                addIfConfirmed = false;
            }
            return osBack;
        case kRed:
            if (iOnUserDefDayItem)
                state = AddSubMenu(new cMenuEditDaysOfWeek(&UserDefDayOfWeek));
            break;

        case kBlue:
            if (iOnUseChannelGroups || iOnChannelGroup) {
                free(channelGroupName);
                channelGroupName = strdup(menuitemsChGr[channelGroupNr]);
                state = AddSubMenu(new cMenuChannelGroups(&channelGroupName));
            }
            if (iOnTerm)
                state = AddSubMenu(new cMenuEPGSearchTemplate(NULL, &data, addIfConfirmed));
            if (iOnExtCatItemBrowsable)
                state = AddSubMenu(new cMenuCatValuesSelect(data.catvalues[iCatIndex], iCatIndex, SearchExtCats.Get(iCatIndex)->searchmode));
            break;
        case kGreen:
        case kYellow:
            state = osContinue;
        default:
            break;
        }
    }
    if ((iOnUseChannelGroups || iOnChannelGroup || iOnTerm || iOnExtCatItemBrowsable) && bHadSubMenu && !HasSubMenu()) { // return form submenu
        if (iOnTerm) {
            if (data.DayOfWeek < 0) {
                UserDefDayOfWeek = data.DayOfWeek;
                data.DayOfWeek = 7;
            }
            if (data.useChannel == 2) {
                channelGroupNr = ChannelGroups.GetIndex(data.channelGroup);
                free(channelGroupName);
                channelGroupName = strdup(data.channelGroup);
            }
            if (data.useContentsFilter) {
                for (unsigned int i = 0; i <= CONTENT_DESCRIPTOR_MAX; i++)
                    contentStringFlags[i] = data.HasContentID(i);
            }
        }
        if (iOnExtCatItemBrowsable && SearchExtCats.Count() > 0) {
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                if (SearchExtCat->searchmode >= 10)
                    catvaluesNumeric[index] = atoi(data.catvalues[index]);
                SearchExtCat = SearchExtCats.Next(SearchExtCat);
                index++;
            }
        }
        Set();
        Display();
    }
    return state;
}


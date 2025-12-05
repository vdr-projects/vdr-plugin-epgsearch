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
#include <string>
#include <map>

#include "menu_searchedit.h"
#include "changrp.h"
#include "epgsearchcats.h"
#include "epgsearchtools.h"
#include "menu_dirselect.h"
#include "menu_recsdone.h"
#include "menu_searchtemplate.h"
#include "epgsearchcfg.h"
#include "blacklist.h"
#include "menu_blacklists.h"
#include "epgsearch.h"
#include "searchtimer_thread.h"
#include <math.h>
#include "templatefile.h"

cChannelGroups ChannelGroups;
cSearchExtCats SearchExtCats;

// --- cMenuEditSearchExt --------------------------------------------------------
cMenuEditSearchExt::cMenuEditSearchExt(cSearchExt *SearchExt, bool New, bool Template, bool FromEPG)
    : cOsdMenu(tr("Edit search"), 32)
{
    SetMenuCategory(mcPlugin);
    templateMode = Template;

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

    SearchTimerActions[0] = tr("Record");
    SearchTimerActions[1] = tr("Announce by OSD");
    SearchTimerActions[2] = tr("Switch only");
    SearchTimerActions[3] = tr("Announce and switch");
    SearchTimerActions[4] = tr("Announce by mail");
    SearchTimerActions[5] = tr("Inactive record");

    BlacklistModes[0] = tr("only globals");
    BlacklistModes[1] = tr("Selection");
    BlacklistModes[2] = tr("all");
    BlacklistModes[3] = trVDR("no");

    DelModes[0] = trVDR("no");
    DelModes[1] = tr("count recordings");
    DelModes[2] = tr("count days");

    SearchTimerModes[0] = trVDR("no");
    SearchTimerModes[1] = trVDR("yes");
    SearchTimerModes[2] = tr("user-defined");

    CompareSubtitleModes[0] = trVDR("no");
    CompareSubtitleModes[1] = trVDR("yes");
    CompareSubtitleModes[2] = tr("allow empty");

    CompareDateModes[0] = trVDR("no");
    CompareDateModes[1] = tr("same day");
    CompareDateModes[2] = tr("same week");
    CompareDateModes[3] = tr("same month");

    EPGInfoMatchingMode[0] = tr("all categories");
    EPGInfoMatchingMode[1] = tr("all except missing categories");
    EPGInfoMatchingMode[2] = tr("at least one category");

    if (!templateMode && New) {
        cSearchExt* SearchTempl = NULL; // copy the default settings, if we have a default template
        cMutexLock SearchTemplatesLock(&SearchTemplates);
        cSearchExt *SearchExtTempl = SearchTemplates.First();
        while (SearchExtTempl) {
            if (SearchExtTempl->ID == EPGSearchConfig.DefSearchTemplateID)
                SearchTempl = SearchExtTempl;
            SearchExtTempl = SearchTemplates.Next(SearchExtTempl);
        }
        if (SearchTempl)
            SearchExt->CopyFromTemplate(SearchTempl, FromEPG);
    }

    searchExt = SearchExt;
    addIfConfirmed = New;
    if (!templateMode)
        SetHelp(NULL, NULL,  tr("Button$Help"), tr("Button$Templates"));

    if (searchExt) {
        data = *searchExt;
        UserDefDayOfWeek = 0;
        if (searchExt->DayOfWeek < 0) {
            UserDefDayOfWeek = searchExt->DayOfWeek;
            data.DayOfWeek = 7;
        }

        menuitemsChGr = NULL;
        channelGroupName = NULL;

        channelMin = channelMax = cDevice::CurrentChannel();
        channelGroupNr = 0;
        if (data.channelMin)
            channelMin = data.channelMin->Number();
        if (data.channelMax)
            channelMax = data.channelMax->Number();

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

        catarrayAvoidRepeats = NULL;
        catvaluesNumeric = NULL;
        if (SearchExtCats.Count() > 0) {
            // fill an array, that stores yes/no for using categories in avoid repeats
            catarrayAvoidRepeats = (int*) malloc(SearchExtCats.Count() * sizeof(int));
            catvaluesNumeric = (int*) malloc(SearchExtCats.Count() * sizeof(int));
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                catarrayAvoidRepeats[index] = (SearchExt->catvaluesAvoidRepeat & (1 << index)) ? 1 : 0;
                catvaluesNumeric[index] = atol(SearchExt->catvalues[index]);
                SearchExtCat = SearchExtCats.Next(SearchExtCat);
                index++;
            }
        }
        blacklists.Clear();
        if (data.blacklistMode == blacklistsSelection) {
            cBlacklistObject* blacklistObj = searchExt->blacklists.First();
            while (blacklistObj) {
                blacklists.Add(new cBlacklistObject(blacklistObj->blacklist));
                blacklistObj = searchExt->blacklists.Next(blacklistObj);
            }
        }

        Set();
    }
}

void cMenuEditSearchExt::AddHelp(cOsdItem* item, const char* helpText)
{
    helpTexts[item] = helpText;
}

eOSState cMenuEditSearchExt::Help()
{
    eOSState state = osContinue;
    cOsdItem* item = Get(Current());
    if (item) {
        const char* helpText;
        const char* itemText = item->Text();
        if (helpText = helpTexts[item]) {
            char* title = NULL;
            if (msprintf(&title, "%s - %s", tr("Button$Help"), itemText) != -1) {
                if (strchr(title, ':')) *strchr(title, ':') = 0;
                state = AddSubMenu(new cMenuText(title, helpText));
                free(title);
            }
        }
    }
    return state;
}

void cMenuEditSearchExt::Set()
{
    cOsdItem* item;
    int current = Current();
    Clear();
    helpTexts.clear();

    if (templateMode) {
        Add(item = new cMenuEditStrItem(tr("Template name"), data.search, sizeof(data.search), tr(AllowedChars)));
        AddHelp(item, tr("Help$Specify the name of the template."));
    } else {
        Add(item = new cMenuEditStrItem(tr("Search term"), data.search, sizeof(data.search), tr(AllowedChars)));
        AddHelp(item, tr("Help$Specify here the term to search for."));
    }

    Add(item = new cMenuEditStraItem(tr("Search mode"),     &data.mode, 6, SearchModes));
    AddHelp(item, tr("Help$The following search modes exist:\n\n- phrase: searches for sub terms\n- all words: all single words must appear\n- at least one word: at least one single word must appear\n- match exactly: must match exactly\n- regular expression: match a regular expression\n- fuzzy searching: searches approximately"));

    if (data.mode == 5) { // fuzzy
        Add(new cMenuEditIntItem(IndentMenuItem(tr("Tolerance")), &data.fuzzyTolerance, 1, 9));
        AddHelp(item, tr("Help$This sets the tolerance of fuzzy searching. The value represents the allowed errors."));
    }

    Add(item = new cMenuEditBoolItem(tr("Match case"), &data.useCase, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if your search should match the case."));
    Add(item = new cMenuEditBoolItem(tr("Use title"), &data.useTitle, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if you like to search in the title of an event."));
    Add(item = new cMenuEditBoolItem(tr("Use subtitle"), &data.useSubtitle, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if you like to search in the episode of an event."));
    Add(item = new cMenuEditBoolItem(tr("Use description"), &data.useDescription, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if you like to search in the summary of an event."));

    Add(item = new cMenuEditBoolItem(tr("Use parental rating"), &data.useParentalRating, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if you like to consider the parental rating of an event."));
    if (data.useParentalRating == true) {
        Add(new cMenuEditIntItem(tr("  Min. rating"), &data.minParentalRating, 0, 18));
        Add(new cMenuEditIntItem(tr("  Max. rating"), &data.maxParentalRating, 0, 18));
    }

    Add(item = new cMenuEditBoolItem(tr("Use content descriptors"), &data.useContentsFilter, trVDR("no"), trVDR("yes")));
    AddHelp(item, tr("Help$Set this to 'yes' if you want to search the contents by a descriptor."));
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
        Add(item = new cMenuEditBoolItem(tr("Use extended EPG info"), &data.useExtEPGInfo, trVDR("no"), trVDR("yes")));
        AddHelp(item, tr("Help$The summary of an event, can contain additional information like 'Genre', 'Category', 'Year',... called 'EPG categories' within EPGSearch. External EPG providers often deliver this information. This allows refining a search and other nice things, like searching for the 'tip of the day'. To use it set this to 'yes'."));
        if (data.useExtEPGInfo) {
            Add(item = new cMenuEditStraItem(IndentMenuItem(tr("Category matching mode")), &data.extEPGInfoMatchingMode, 3, EPGInfoMatchingMode));
            AddHelp(item, tr("Help$By default, all selected categories must be present in an event's summary and match the specified values for the event to be included in search results. This option allows including events with missing or just some matching categories. But handle this with care to avoid a huge amount of results."));
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                if (SearchExtCat->searchmode >= 10)
                    Add(item = new cMenuEditIntItem(IndentMenuItem(SearchExtCat->menuname), &catvaluesNumeric[index], 0, 999999, ""));
                else
                    Add(item = new cMenuEditStrItem(IndentMenuItem(SearchExtCat->menuname), data.catvalues[index], MaxFileName, tr(AllowedChars)));
                AddHelp(item, tr("Help$The file 'epgsearchcats.conf' specifies the search mode for this entry. One can search by text or by value. You can also edit a list of predefined values in this file that can be selected here."));
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
        Add(new cMenuEditTimeItem(tr("  Start after"), &data.startTime));
        Add(new cMenuEditTimeItem(tr("  Start before"), &data.stopTime));
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
        Add(new cMenuEditStraItem(IndentMenuItem(tr("Day of week")),     &data.DayOfWeek, 8, DaysOfWeek));
    }
    Add(new cMenuEditStraItem(tr("Use blacklists"), &data.blacklistMode, 4, BlacklistModes));

    if (EPGSearchConfig.showFavoritesMenu)
        Add(new cMenuEditBoolItem(tr("Use in favorites menu"), &data.useInFavorites, trVDR("no"), trVDR("yes")));

    int countSearchTemplates = 0;
    if ((countSearchTemplates = cTemplFile::CountSearchResultsTemplates()) > 1) {
        Add(new cMenuEditStraItem(tr("Result menu layout"), &data.menuTemplate, countSearchTemplates, cTemplFile::SearchTemplates));
    }

    Add(new cMenuEditStraItem(tr("Use as search timer"), &data.useAsSearchTimer, 3, SearchTimerModes));
    if (data.useAsSearchTimer) {
        Add(new cMenuEditStraItem(IndentMenuItem(tr("Action")), &data.action, 6, SearchTimerActions));
        if (data.action == searchTimerActionSwitchOnly) {
            Add(new cMenuEditIntItem(IndentMenuItem(tr("Switch ... minutes before start")), &data.switchMinsBefore, 0, 99));
            Add(new cMenuEditBoolItem(IndentMenuItem(tr("Unmute sound")), &data.unmuteSoundOnSwitch, trVDR("no"), trVDR("yes")));
        }
        if (data.action ==  searchTimerActionAnnounceAndSwitch) {
            Add(new cMenuEditIntItem(IndentMenuItem(tr("Ask ... minutes before start")), &data.switchMinsBefore, 0, 99));
            Add(new cMenuEditBoolItem(IndentMenuItem(tr("Unmute sound")), &data.unmuteSoundOnSwitch, trVDR("no"), trVDR("yes")));
        }
        if ((data.action == searchTimerActionRecord) || (data.action == searchTimerActionInactiveRecord)) {
            Add(new cMenuEditBoolItem(tr("  Series recording"), &data.useEpisode, trVDR("no"), trVDR("yes")));
            Add(new cMenuEditStrItem(IndentMenuItem(tr("Directory")), data.directory, sizeof(data.directory), tr(AllowedChars)));
            Add(new cMenuEditIntItem(IndentMenuItem(tr("Delete recordings after ... days")), &data.delAfterDays, 0, 999));
            if (data.delAfterDays > 0)
                Add(new cMenuEditIntItem(IndentMenuItem(IndentMenuItem(tr("Keep ... recordings"))), &data.recordingsKeep, 0, 999));
            Add(new cMenuEditIntItem(IndentMenuItem(tr("Pause when ... recordings exist")), &data.pauseOnNrRecordings, 0, 999));
            Add(new cMenuEditBoolItem(IndentMenuItem(tr("Avoid repeats")), &data.avoidRepeats, trVDR("no"), trVDR("yes")));
            if (data.avoidRepeats) {
                Add(new cMenuEditIntItem(IndentMenuItem(tr("Allowed repeats"), 2), &data.allowedRepeats, 0, 99));
                if (data.allowedRepeats > 0)
                    Add(new cMenuEditIntItem(IndentMenuItem(tr("Only repeats within ... days"), 2), &data.repeatsWithinDays, 0, 999));
                Add(new cMenuEditBoolItem(IndentMenuItem(tr("Compare title"), 2), &data.compareTitle, trVDR("no"), trVDR("yes")));
                Add(new cMenuEditStraItem(IndentMenuItem(tr("Compare subtitle"), 2), &data.compareSubtitle, 3, CompareSubtitleModes));
                Add(new cMenuEditBoolItem(IndentMenuItem(tr("Compare summary"), 2), &data.compareSummary, trVDR("no"), trVDR("yes")));
                if (data.compareSummary)
                    Add(new cMenuEditIntItem(IndentMenuItem(tr("Min. match in %"), 3), &data.compareSummaryMatchInPercent, 1, 100));
                Add(new cMenuEditStraItem(IndentMenuItem(tr("Compare date"), 2), &data.compareDate, 4, CompareDateModes));
                // show 'Compare categories' only if we have them
                if (SearchExtCats.Count() > 0) {
                    cSearchExtCat *SearchExtCat = SearchExtCats.First();
                    int iUsed = 0;
                    int index = 0;
                    while (SearchExtCat) {
                        if (catarrayAvoidRepeats[index])
                            iUsed++;
                        SearchExtCat = SearchExtCats.Next(SearchExtCat);
                        index++;
                    }

                    cString itemtext = cString::sprintf("%s (%d/%d)", tr("Compare categories"), iUsed, SearchExtCats.Count());
                    Add(new cOsdItem(IndentMenuItem(IndentMenuItem(itemtext))));
                }
            }

            Add(new cMenuEditIntItem(IndentMenuItem(trVDR("Priority")), &data.Priority, 0, MAXPRIORITY));
            Add(new cMenuEditIntItem(IndentMenuItem(trVDR("Lifetime")), &data.Lifetime, 0, MAXLIFETIME));
            Add(new cMenuEditIntItem(IndentMenuItem(trVDR("Setup.Recording$Margin at start (min)")), &data.MarginStart, 0, INT_MAX));
            Add(new cMenuEditIntItem(IndentMenuItem(trVDR("Setup.Recording$Margin at stop (min)")), &data.MarginStop, 0, INT_MAX));
            Add(new cMenuEditBoolItem(IndentMenuItem(tr("VPS")), &data.useVPS, trVDR("no"), trVDR("yes")));
        }
        if ((data.action == searchTimerActionRecord) || (data.action == searchTimerActionInactiveRecord)) {
            Add(new cMenuEditStraItem(IndentMenuItem(tr("Auto delete")), &data.delMode, 3, DelModes));
            if (data.delMode == 1)
                Add(new cMenuEditIntItem(IndentMenuItem(IndentMenuItem(tr("after ... recordings"))), &data.delAfterCountRecs, 0, 999));
            else if (data.delMode == 2)
                Add(new cMenuEditIntItem(IndentMenuItem(IndentMenuItem(tr("after ... days after first rec."))), &data.delAfterDaysOfFirstRec, 0, 999));
        }
    }
    SetCurrent(Get(current));
}

cMenuEditSearchExt::~cMenuEditSearchExt()
{
    if (addIfConfirmed) delete searchExt; // apparently it wasn't confirmed
    delete [] menuitemsChGr;
    free(channelGroupName);
    free(catarrayAvoidRepeats);
    free(catvaluesNumeric);
    free(contentStringFlags);
    for (size_t i = 0; i < LENGTHOF(DaysOfWeek); i++)
        free(DaysOfWeek[i]);
}

eOSState cMenuEditSearchExt::ProcessKey(eKeys Key)
{
    bool bHadSubMenu = HasSubMenu();

    int iTemp_mode = data.mode;
    int iTemp_useTime = data.useTime;
    int iTemp_useChannel = data.useChannel;
    int iTemp_useDuration = data.useDuration;
    int iTemp_useDayOfWeek = data.useDayOfWeek;
    int iTemp_useAsSearchTimer = data.useAsSearchTimer;
    int iTemp_useParentalRating = data.useParentalRating;
    int iTemp_useContentsFilter = data.useContentsFilter;
    int iTemp_useExtEPGInfo = data.useExtEPGInfo;
    int iTemp_avoidRepeats = data.avoidRepeats;
    int iTemp_allowedRepeats = data.allowedRepeats;
    int iTemp_delAfterDays = data.delAfterDays;
    int iTemp_action = data.action;
    int iTemp_delMode = data.delMode;
    int iTemp_compareSummary = data.compareSummary;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (iTemp_mode != data.mode ||
        iTemp_useTime != data.useTime ||
        iTemp_useChannel != data.useChannel ||
        iTemp_useDuration != data.useDuration ||
        iTemp_useDayOfWeek != data.useDayOfWeek ||
        iTemp_useAsSearchTimer != data.useAsSearchTimer ||
        iTemp_useParentalRating != data.useParentalRating ||
        iTemp_useContentsFilter != data.useContentsFilter ||
        iTemp_useExtEPGInfo != data.useExtEPGInfo ||
        iTemp_avoidRepeats != data.avoidRepeats ||
        iTemp_allowedRepeats != data.allowedRepeats ||
        iTemp_delAfterDays != data.delAfterDays ||
        iTemp_action != data.action ||
        iTemp_delMode != data.delMode ||
        iTemp_compareSummary != data.compareSummary) {
        Set();
        Display();
        if ((iTemp_useAsSearchTimer != data.useAsSearchTimer || iTemp_action != data.action) && data.useAsSearchTimer) { // if search timer menu is dropped then scroll down to display all contents
            int cur = Current();
            PageDown();
            SetCurrent(Get(cur));
            Display();
        }
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
    int iOnDirectoryItem = 0;
    int iOnUseChannelGroups = 0;
    int iOnChannelGroup = 0;
    int iOnAvoidRepeats = 0;
    int iOnCompareCats = 0;
    int iOnTerm = 0;
    int iOnUseBlacklistsSelection = 0;
    int iOnExtCatItemBrowsable = 0;
    int iOnUseAsSearchTimer = 0;

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
                SetHelp(NULL, NULL,  tr("Button$Help"), tr("Button$Templates"));
                iOnTerm = 1;
            }
        } else if (strstr(ItemText, IndentMenuItem(tr("Day of week"))) == ItemText) {
            if (data.DayOfWeek == 7) {
                SetHelp(trVDR("Button$Edit"));
                iOnUserDefDayItem = 1;
            } else
                SetHelp(NULL, NULL, tr("Button$Help"));
        } else if (strstr(ItemText, tr("Use as search timer")) == ItemText) {
            if (data.useAsSearchTimer == 2) {
                SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
                iOnUseAsSearchTimer = 1;
            } else
                SetHelp(NULL, NULL, tr("Button$Help"));
        } else if (strstr(ItemText, IndentMenuItem(tr("Directory"))) == ItemText) {
            if (!InEditMode(ItemText, IndentMenuItem(tr("Directory")), data.directory))
                SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Select"));
            iOnDirectoryItem = 1;
        } else if (strstr(ItemText, tr("Use channel")) == ItemText && data.useChannel == 2) {
            SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
            iOnUseChannelGroups = 1;
        } else if (strstr(ItemText, IndentMenuItem(tr("Channel group"))) == ItemText) {
            SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
            iOnChannelGroup = 1;
        } else if (strstr(ItemText, tr("Use blacklists")) == ItemText && data.blacklistMode == blacklistsSelection) {
            SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
            iOnUseBlacklistsSelection = 1;
        } else if (strstr(ItemText, IndentMenuItem(tr("Avoid repeats"))) == ItemText) {
            SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
            iOnAvoidRepeats = 1;
        } else if (strstr(ItemText, IndentMenuItem(IndentMenuItem(tr("Compare categories")))) == ItemText) {
            SetHelp(NULL, NULL, tr("Button$Help"), tr("Button$Setup"));
            iOnCompareCats = 1;
        } else if (iOnExtCatItem) {
            if (!InEditMode(ItemText, IndentMenuItem(catname), data.catvalues[iCatIndex]) ||
                SearchExtCats.Get(iCatIndex)->searchmode >= 10)
                SetHelp(NULL, NULL, tr("Button$Help"), iOnExtCatItemBrowsable ? tr("Button$Select") : NULL);
        } else if (strstr(ItemText, tr("Search term")) != ItemText)
            SetHelp(NULL, NULL, tr("Button$Help"), NULL);
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

            if ((data.useTitle || data.useSubtitle || data.useDescription) &&
                (int(strlen(data.search)) < 3) &&
                !Interface->Confirm(tr("Edit$Search text too short - use anyway?")))
                break;

            if (searchExt) {
                *searchExt = data;
                if (data.DayOfWeek == 7)
                    searchExt->DayOfWeek = UserDefDayOfWeek;

                // transfer cat selection for 'avoid repeats' back to search
                cSearchExtCat *SearchExtCat = SearchExtCats.First();
                int index = 0;
                searchExt->catvaluesAvoidRepeat = 0;
                while (SearchExtCat) {
                    if (catarrayAvoidRepeats[index])
                        searchExt->catvaluesAvoidRepeat += (1 << index);
                    SearchExtCat = SearchExtCats.Next(SearchExtCat);
                    index++;
                }

                // transfer numeric cat values back to search
                SearchExtCat = SearchExtCats.First();
                index = 0;
                while (SearchExtCat) {
                    if (SearchExtCat->searchmode >= 10) {
                        if (searchExt->catvalues[index]) free(searchExt->catvalues[index]);
                        msprintf(&searchExt->catvalues[index], "%d", catvaluesNumeric[index]);
                    }
                    SearchExtCat = SearchExtCats.Next(SearchExtCat);
                    index++;
                }

                searchExt->SetContentsFilter(data.useContentsFilter ? contentStringFlags : NULL);

                if (data.blacklistMode == blacklistsSelection) {
                    searchExt->blacklists.Clear();
                    cBlacklistObject* blacklistObj = blacklists.First();
                    while (blacklistObj) {
                        searchExt->blacklists.Add(new cBlacklistObject(blacklistObj->blacklist));
                        blacklistObj = blacklists.Next(blacklistObj);
                    }
                } else
                    searchExt->blacklists.Clear();

                if (addIfConfirmed) {
                    cMutexLock SearchExtsLock(&SearchExts);
                    searchExt->ID = SearchExts.GetNewID();
                    SearchExts.Add(searchExt);
                }
                if (searchExt->useAsSearchTimer && !EPGSearchConfig.useSearchTimers) { // enable search timer thread if necessary
                    cSearchTimerThread::Init((cPluginEpgsearch*) cPluginManager::GetPlugin("epgsearch"), true);
                    INFO(tr("Search timers activated in setup."));
                }

                SearchExts.Save();
                addIfConfirmed = false;
            }
            return osBack;
        case kRed:
            if (iOnUserDefDayItem)
                state = AddSubMenu(new cMenuEditDaysOfWeek(&UserDefDayOfWeek));
            break;

        case kBlue:
            if (iOnDirectoryItem && !InEditMode(ItemText, IndentMenuItem(tr("Directory")), data.directory))
                state = AddSubMenu(new cMenuDirSelect(data.directory));
            if (iOnUseChannelGroups || iOnChannelGroup) {
                free(channelGroupName);
                channelGroupName = strdup(menuitemsChGr[channelGroupNr]);
                state = AddSubMenu(new cMenuChannelGroups(&channelGroupName));
            }
            if (iOnAvoidRepeats)
                state = AddSubMenu(new cMenuRecsDone(searchExt));
            if (iOnCompareCats)
                state = AddSubMenu(new cMenuSearchEditCompCats(catarrayAvoidRepeats));
            if (iOnTerm)
                state = AddSubMenu(new cMenuEPGSearchTemplate(&data, NULL, addIfConfirmed));
            if (iOnUseBlacklistsSelection)
                state = AddSubMenu(new cMenuBlacklistsSelection(&blacklists));
            if (iOnExtCatItemBrowsable)
                state = AddSubMenu(new cMenuCatValuesSelect(data.catvalues[iCatIndex], iCatIndex, SearchExtCats.Get(iCatIndex)->searchmode));
            if (iOnUseAsSearchTimer)
                state = AddSubMenu(new cMenuSearchActivationSettings(&data));
            break;
        case kGreen:
            state = osContinue;
            break;
        case kYellow:
        case kInfo:
            state = Help();
            break;
        default:
            break;
        }
    }
    if ((iOnUseChannelGroups || iOnChannelGroup || iOnCompareCats || iOnTerm || iOnExtCatItemBrowsable) && bHadSubMenu && !HasSubMenu()) { // return form submenu
        if (iOnTerm) {
            if (data.DayOfWeek < 0) {
                UserDefDayOfWeek = data.DayOfWeek;
                data.DayOfWeek = 7;
            }
            if (data.useChannel == 1) {
                channelMin = data.channelMin->Number();
                channelMax = data.channelMax->Number();
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
            if (SearchExtCats.Count() > 0) {
                cSearchExtCat *SearchExtCat = SearchExtCats.First();
                int index = 0;
                while (SearchExtCat) {
                    catarrayAvoidRepeats[index] = (data.catvaluesAvoidRepeat & (1 << index)) ? 1 : 0;
                    SearchExtCat = SearchExtCats.Next(SearchExtCat);
                    index++;
                }
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

// --- cMenuEditDaysOfWeek --------------------------------------------------------
cMenuEditDaysOfWeek::cMenuEditDaysOfWeek(int* DaysOfWeek, int Offset, bool Negate)
    : cOsdMenu(tr("Edit user-defined days of week"), 30)
{
    SetMenuCategory(mcPlugin);
    int i = 0;
    offset = Offset;
    negate = Negate;
    pDaysOfWeek = DaysOfWeek;

    if (negate)
        *pDaysOfWeek = -*pDaysOfWeek;

    for (i = 0; i < 7; i++)
        Days[(i + offset) % 7] = ((*pDaysOfWeek) & (int)pow(2, i)) ? 1 : 0;
    for (i = 0; i < 7; i++)
        Add(new cMenuEditBoolItem(WeekDayName((i + 1) % 7), &Days[(i + 1) % 7], trVDR("no"), trVDR("yes")));

    SetHelp(NULL);
}

eOSState cMenuEditDaysOfWeek::ProcessKey(eKeys Key)
{
    if (Key == kBack && negate)
        *pDaysOfWeek = -*pDaysOfWeek;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
        switch (Key) {
        case kOk:
            *pDaysOfWeek = 0;
            for (int i = 0; i < 7; i++)
                *pDaysOfWeek += Days[i] ? (int)pow(2, (i + 7 - offset) % 7) : 0;
            if (negate)
                *pDaysOfWeek = -*pDaysOfWeek;
            state = osBack;
            break;
        default:
            break;
        }
    }
    return state;
}

// --- cMenuSearchEditCompCats --------------------------------------------------------
cMenuSearchEditCompCats::cMenuSearchEditCompCats(int* catarrayAvoidRepeats)
    : cOsdMenu(tr("Compare categories"), 30)
{
    SetMenuCategory(mcPlugin);
    search_catarrayAvoidRepeats = catarrayAvoidRepeats;
    edit_catarrayAvoidRepeats = (int*) malloc(SearchExtCats.Count() * sizeof(int));
    cSearchExtCat *SearchExtCat = SearchExtCats.First();
    int index = 0;
    while (SearchExtCat) {
        edit_catarrayAvoidRepeats[index] = catarrayAvoidRepeats[index];
        cString menutext = cString::sprintf("%s %s", tr("Compare"), SearchExtCat->menuname);
        Add(new cMenuEditBoolItem(menutext, &edit_catarrayAvoidRepeats[index], trVDR("no"), trVDR("yes")));
        SearchExtCat = SearchExtCats.Next(SearchExtCat);
        index++;
    }
}

cMenuSearchEditCompCats::~cMenuSearchEditCompCats()
{
    free(edit_catarrayAvoidRepeats);
}

eOSState cMenuSearchEditCompCats::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
        case kOk: {
            cSearchExtCat *SearchExtCat = SearchExtCats.First();
            int index = 0;
            while (SearchExtCat) {
                search_catarrayAvoidRepeats[index] = edit_catarrayAvoidRepeats[index];
                SearchExtCat = SearchExtCats.Next(SearchExtCat);
                index++;
            }
        }
        state = osBack;
        break;
        default:
            break;
        }
    }
    return state;
}

// --- cMenuBlacklistsSelection --------------------------------------------------------
cMenuBlacklistsSelection::cMenuBlacklistsSelection(cList<cBlacklistObject>* pBlacklists)
    : cOsdMenu(tr("Select blacklists"), 30)
{
    SetMenuCategory(mcPlugin);
    blacklists = pBlacklists;
    blacklistsSel = new int[Blacklists.Count()];
    cMutexLock BlacklistLock(&Blacklists);
    cBlacklist* blacklist = Blacklists.First();
    int index = 0;
    while (blacklist) {
        blacklistsSel[index] = false;
        cBlacklistObject* blacklistObjSel = blacklists->First();
        while (blacklistObjSel) {
            if (blacklistObjSel->blacklist->ID == blacklist->ID) {
                blacklistsSel[index] = true;
                break;
            }
            blacklistObjSel = blacklists->Next(blacklistObjSel);
        }
        blacklist = Blacklists.Next(blacklist);
        index++;
    }

    SetHelp(tr("Button$Invert selection"), tr("Button$All yes"), tr("Button$All no"), tr("Button$Setup"));
    Set();
}

cMenuBlacklistsSelection::~cMenuBlacklistsSelection()
{
    delete [] blacklistsSel;
}

// --- cMenuBlacklistsSelectionItem ----------------------------------------------------------
class cMenuBlacklistsSelectionItem : public cMenuEditBoolItem
{
    const char* name;
public:
    cMenuBlacklistsSelectionItem(const char *Name, int *Value, const char *FalseString = NULL, const char *TrueString = NULL):
        cMenuEditBoolItem(Name, Value, FalseString, TrueString) {
        name = Name;
    }
    int Compare(const cListObject &ListObject) const {
        cMenuBlacklistsSelectionItem *p = (cMenuBlacklistsSelectionItem*)&ListObject;
        return strcasecmp(name, p->name);
    }
};

void cMenuBlacklistsSelection::Set()
{
    int current = Current();
    Clear();
    cMutexLock BlacklistLock(&Blacklists);
    cBlacklist* blacklist = Blacklists.First();
    int index = 0;
    while (blacklist) {
        Add(new cMenuBlacklistsSelectionItem(blacklist->search, &blacklistsSel[index], trVDR("no"), trVDR("yes")));
        blacklist = Blacklists.Next(blacklist);
        index++;
    }
    SetCurrent(Get(current));
    Sort();
}

eOSState cMenuBlacklistsSelection::ProcessKey(eKeys Key)
{
    bool bHadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
        case kOk: {
            cMutexLock BlacklistLock(&Blacklists);
            blacklists->Clear();
            cBlacklist* blacklist = Blacklists.First();
            int index = 0;
            while (blacklist) {
                if (blacklistsSel[index++])
                    blacklists->Add(new cBlacklistObject(blacklist));
                blacklist = Blacklists.Next(blacklist);
            }
        }
        state = osBack;
        break;
        case kRed:
        case kGreen:
        case kYellow: {
            cMutexLock BlacklistLock(&Blacklists);
            cBlacklist* blacklist = Blacklists.First();
            int index = 0;
            while (blacklist) {
                blacklistsSel[index] = (Key == kGreen ? 1 : (Key == kRed ? 1 - blacklistsSel[index] : 0));
                blacklist = Blacklists.Next(blacklist);
                index++;
            }
            Set();
            Display();
            return osContinue;
        }
        break;
        case kBlue:
            state = AddSubMenu(new cMenuBlacklists);
            break;
        default:
            break;
        }
    }
    if (bHadSubMenu && !HasSubMenu()) { // return form submenu
        Clear();
        delete [] blacklistsSel;
        blacklistsSel = new int[Blacklists.Count()];
        cMutexLock BlacklistLock(&Blacklists);
        cBlacklist* blacklist = Blacklists.First();
        int index = 0;
        while (blacklist) {
            blacklistsSel[index] = false;
            cBlacklistObject* blacklistObjSel = blacklists->First();
            while (blacklistObjSel) {
                if (blacklistObjSel->blacklist->ID == blacklist->ID) {
                    blacklistsSel[index] = true;
                    break;
                }
                blacklistObjSel = blacklists->Next(blacklistObjSel);
            }
            blacklist = Blacklists.Next(blacklist);
            index++;
        }
        Set();
        Display();
    }

    return state;
}

// --- cMenuCatValuesSelect --------------------------------------------------------
cMenuCatValuesSelect::cMenuCatValuesSelect(char* CatValues, int CatIndex, int SearchMode)
    : cOsdMenu(tr("Values for EPG category"), 1, 40)
{
    SetMenuCategory(mcPlugin);

    catValues = CatValues;
    catIndex = CatIndex;
    searchMode = SearchMode;
    cSearchExtCat* SearchExtCat = SearchExtCats.Get(catIndex);
    if (SearchExtCat) {
        sel_cats.assign(SearchExtCat->nvalues, false);
        for (int i = 0; i < SearchExtCat->nvalues; i++) {
            char *pstrSearchToken, *pptr;
            char *pstrSearch = strdup(catValues);
            pstrSearchToken = strtok_r(pstrSearch, ",;|~", &pptr);
            while (pstrSearchToken) {
                if (SearchExtCat->values[i] && strcmp(SearchExtCat->values[i], skipspace(pstrSearchToken)) == 0)
                    sel_cats[i] = true;
                pstrSearchToken = strtok_r(NULL, ",;|~", &pptr);
            }
            free(pstrSearch);
        }
    }
    Set();
    SetHelp(trVDR("Button$On/Off"), NULL, NULL, tr("Button$Apply"));
}

void cMenuCatValuesSelect::Set()
{
    int current = Current();
    int selCount = 0;
    Clear();

    std::string SearchMode =  std::string(tr("Search mode")) + ": ";
    if (searchMode == 0)
        SearchMode += tr("phrase");
    else if (searchMode == 1)
        SearchMode += tr("all words");
    else if (searchMode == 2)
        SearchMode += tr("at least one word");
    else if (searchMode == 3)
        SearchMode += tr("match exactly");
    else if (searchMode == 4)
        SearchMode += tr("regular expression");
    else if (searchMode == 10)
        SearchMode += tr("less");
    else if (searchMode == 11)
        SearchMode += tr("less or equal");
    else if (searchMode == 12)
        SearchMode += tr("greater");
    else if (searchMode == 13)
        SearchMode += tr("greater or equal");
    else if (searchMode == 14)
        SearchMode += tr("equal");
    else if (searchMode == 15)
        SearchMode += tr("not equal");
    cOsdItem* sItem = new cOsdItem(SearchMode.c_str());
    Add(sItem);
    sItem->SetSelectable(false);

    cSearchExtCat* SearchExtCat = SearchExtCats.Get(catIndex);
    if (SearchExtCat) {
        for (int i = 0; i < SearchExtCat->nvalues; i++) {
            cString entry = cString::sprintf("%c\t%s", sel_cats[i] ? '*' : ' ',  SearchExtCat->values[i]);
            if (sel_cats[i]) selCount++;
            Add(new cOsdItem(entry));
        }
    }
    SetCurrent(Get(current));
    if (SearchExtCat) {
        cString title = cString::sprintf("%s (%d/%d)", tr("Values for EPG category"), selCount, SearchExtCat->nvalues);
        if (*title) SetTitle(title);
    }
    Display();
}

eOSState cMenuCatValuesSelect::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
        case kOk:
        case kRed:
            if (Current() > 0) {
                cSearchExtCat* SearchExtCat = SearchExtCats.Get(catIndex);
                if (SearchExtCat && SearchExtCat->searchmode >= 10 && catValues) { // only one numeric value, so jump back
                    strcpy(catValues, SearchExtCat->values[Current() - 1]);
                    state = osBack;
                } else {
                    sel_cats[Current() - 1] = !sel_cats[Current() - 1];
                    Set();
                    Display();
                }
            }
            break;
        case kBlue: {
            *catValues = 0;
            cSearchExtCat* SearchExtCat = SearchExtCats.Get(catIndex);
            if (SearchExtCat) {
                for (int i = 0; i < SearchExtCat->nvalues; i++) {
                    if (sel_cats[i]) {
                        if (*catValues)
                            strcat(catValues, ", ");
                        strcat(catValues, SearchExtCat->values[i]);
                    }
                }
            }
            state = osBack;
            break;
        }
        default:
            break;
        }
    }
    return state;
}

// --- cMenuSearchActivationSettings --------------------------------------------------------
cMenuSearchActivationSettings::cMenuSearchActivationSettings(cSearchExt *SearchExt)
    : cOsdMenu(tr("Activation of search timer"), 25)
{
    SetMenuCategory(mcPlugin);

    searchExt = SearchExt;
    if (searchExt) {
        Add(new cMenuEditDateItem(tr("First day"), &searchExt->useAsSearchTimerFrom, NULL));
        Add(new cMenuEditDateItem(tr("Last day"), &searchExt->useAsSearchTimerTil, NULL));
    }
}

eOSState cMenuSearchActivationSettings::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
        case kOk:
            state = osBack;
            break;
        default:
            break;
        }
    }
    return state;
}

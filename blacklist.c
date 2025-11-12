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
#include "epgsearchtools.h"
#include "blacklist.h"
#include "epgsearchcats.h"
#include <vdr/tools.h>
#include "menu_dirselect.h"
#include "changrp.h"
#include "menu_search.h"
#include "menu_searchedit.h"
#include "menu_searchresults.h"
#include <math.h>
#include <sstream>

cBlacklists Blacklists;

// -- cBlacklist -----------------------------------------------------------------
cBlacklist::cBlacklist(void)
{
    buffer = NULL;
    ID = -1;
    *search = 0;
    useTime = false;
    startTime = 0000;
    stopTime = 2359;
    useChannel = false;
    {
        LOCK_CHANNELS_READ;
        channelMin = Channels->GetByNumber(cDevice::CurrentChannel());
        channelMax = channelMin;
    }
    channelGroup = NULL;
    useCase = false;
    mode = 0;
    useTitle = true;
    useSubtitle = true;
    useDescription = true;
    useDuration = false;
    minDuration = 0;
    maxDuration = 130;
    useDayOfWeek = false;
    DayOfWeek = 0;
    useExtEPGInfo = false;
    catvalues = (char**) malloc(SearchExtCats.Count() * sizeof(char*));
    cSearchExtCat *SearchExtCat = SearchExtCats.First();
    int index = 0;
    while (SearchExtCat) {
        catvalues[index] = (char*)malloc(MaxFileName);
        *catvalues[index] = 0;
        SearchExtCat = SearchExtCats.Next(SearchExtCat);
        index++;
    }
    extEPGInfoMatchingMode = 0;
    fuzzyTolerance = 1;
    isGlobal = 0;
    useContentsFilter = false;
    contentsCategoryMatchingMode = 0;
    contentsCharacteristicsMatchingMode = 0;
    useParentalRating = false;
    minParentalRating = 0;
    maxParentalRating = 18;
}

cBlacklist::~cBlacklist(void)
{
    free(buffer);

    if (catvalues) {
        cSearchExtCat *SearchExtCat = SearchExtCats.First();
        int index = 0;
        while (SearchExtCat) {
            free(catvalues[index]);
            SearchExtCat = SearchExtCats.Next(SearchExtCat);
            index++;
        }
        free(catvalues);
    }
}

cBlacklist& cBlacklist::operator= (const cBlacklist &Blacklist)
{
    ID = Blacklist.ID;
    strcpy(search, Blacklist.search);
    useTime = Blacklist.useTime;
    startTime = Blacklist.startTime;
    stopTime = Blacklist.stopTime;
    useChannel = Blacklist.useChannel;
    channelMin = Blacklist.channelMin;
    channelMax = Blacklist.channelMax;
    free(channelGroup);
    channelGroup = Blacklist.channelGroup ? strdup(Blacklist.channelGroup) : NULL;
    useCase = Blacklist.useCase;
    mode = Blacklist.mode;
    useTitle = Blacklist.useTitle;
    useSubtitle = Blacklist.useSubtitle;
    useDescription = Blacklist.useDescription;
    useDuration = Blacklist.useDuration;
    minDuration = Blacklist.minDuration;
    maxDuration = Blacklist.maxDuration;
    useDayOfWeek = Blacklist.useDayOfWeek;
    DayOfWeek = Blacklist.DayOfWeek;
    useExtEPGInfo = Blacklist.useExtEPGInfo;
    cSearchExtCat *SearchExtCat = SearchExtCats.First();
    int index = 0;
    while (SearchExtCat) {
        *catvalues[index] = 0;
        strcpy(catvalues[index], Blacklist.catvalues[index]);
        SearchExtCat = SearchExtCats.Next(SearchExtCat);
        index++;
    }
    extEPGInfoMatchingMode = Blacklist.extEPGInfoMatchingMode;
    fuzzyTolerance = Blacklist.fuzzyTolerance;
    isGlobal = 0;
    useContentsFilter = Blacklist.useContentsFilter;
    contentsFilter = Blacklist.contentsFilter;
    contentsCategoryMatchingMode = Blacklist.contentsCategoryMatchingMode;
    contentsCharacteristicsMatchingMode = Blacklist.contentsCharacteristicsMatchingMode;
    useParentalRating = Blacklist.useParentalRating;
    minParentalRating = Blacklist.minParentalRating;
    maxParentalRating = Blacklist.maxParentalRating;
    return *this;
}

void cBlacklist::CopyFromTemplate(const cSearchExt* templ)
{
    useTime = templ->useTime;
    startTime = templ->startTime;
    stopTime = templ->stopTime;
    useChannel = templ->useChannel;
    channelMin = templ->channelMin;
    channelMax = templ->channelMax;
    free(channelGroup);
    channelGroup = templ->channelGroup ? strdup(templ->channelGroup) : NULL;
    useCase = templ->useCase;
    mode = templ->mode;
    useTitle = templ->useTitle;
    useSubtitle = templ->useSubtitle;
    useDescription = templ->useDescription;
    useDuration = templ->useDuration;
    minDuration = templ->minDuration;
    maxDuration = templ->maxDuration;
    useDayOfWeek = templ->useDayOfWeek;
    DayOfWeek = templ->DayOfWeek;
    useExtEPGInfo = templ->useExtEPGInfo;
    cSearchExtCat *SearchExtCat = SearchExtCats.First();
    int index = 0;
    while (SearchExtCat) {
        strcpy(catvalues[index], templ->catvalues[index]);
        SearchExtCat = SearchExtCats.Next(SearchExtCat);
        index++;
    }
    extEPGInfoMatchingMode = templ->extEPGInfoMatchingMode;
    fuzzyTolerance = templ->fuzzyTolerance;
    isGlobal = 0;
    useContentsFilter = templ->useContentsFilter;
    contentsFilter = templ->contentsFilter;
    contentsCategoryMatchingMode = templ->contentsCategoryMatchingMode;
    contentsCharacteristicsMatchingMode = templ->contentsCharacteristicsMatchingMode;
    useParentalRating = templ->useParentalRating;
    minParentalRating = templ->minParentalRating;
    maxParentalRating = templ->maxParentalRating;
}

bool cBlacklist::operator< (const cListObject &ListObject)
{
    cBlacklist *BL = (cBlacklist *)&ListObject;
    return strcasecmp(search, BL->search) < 0;
}

const char *cBlacklist::ToText(void)
{
    char tmp_Start[5] = "";
    char tmp_Stop[5] = "";
    char tmp_minDuration[5] = "";
    char tmp_maxDuration[5] = "";
    char* tmp_search = encodeSpecialCharacters(search);
    char* tmp_chanSel = NULL;
    char* tmp_catvalues = NULL;

    if (useTime) {
        sprintf(tmp_Start, "%04d", startTime);
        sprintf(tmp_Stop, "%04d", stopTime);
    }
    if (useDuration) {
        sprintf(tmp_minDuration, "%04d", minDuration);
        sprintf(tmp_maxDuration, "%04d", maxDuration);
    }

    if (useChannel == 1) {
        if (channelMin->Number() < channelMax->Number())
            msprintf(&tmp_chanSel, "%s|%s", CHANNELSTRING(channelMin), CHANNELSTRING(channelMax));
        else
            msprintf(&tmp_chanSel, "%s", CHANNELSTRING(channelMin));
    }
    if (useChannel == 2) {
        int channelGroupNr = ChannelGroups.GetIndex(channelGroup);
        if (channelGroupNr == -1) {
            LogFile.eSysLog("channel group %s does not exist!", channelGroup);
            useChannel = 0;
        } else
            tmp_chanSel = strdup(channelGroup);
    }

    if (useExtEPGInfo) {
        cSearchExtCat *SearchExtCat = SearchExtCats.First();
        int index = 0;
        while (SearchExtCat) {
            char* catvalue = NULL;
            if (msprintf(&catvalue, "%s", catvalues[index]) != -1) {
                char* temp = catvalue;
                catvalue = encodeSpecialCharacters(catvalue, true);
                free(temp);
                if (index == 0)
                    msprintf(&tmp_catvalues, "%d#%s", SearchExtCat->id, catvalue);
                else {
                    temp = tmp_catvalues;
                    msprintf(&tmp_catvalues, "%s|%d#%s", tmp_catvalues, SearchExtCat->id, catvalue);
                    free(temp);
                }
                free(catvalue);
            }
            SearchExtCat = SearchExtCats.Next(SearchExtCat);
            index++;
        }
    }

    free(buffer);
    msprintf(&buffer, "%d:%s:%d:%s:%s:%d:%s:%d:%d:%d:" "%d:%d:%d:%s:%s:%d:%d:%d:%s:%d:"     //  1..20
                      "%d:%d:%s:%d:%d:%d:%d:%d",                                            // 21..28
             ID,                        //  1
             tmp_search,
             useTime,
             tmp_Start,
             tmp_Stop,
             useChannel,
             (useChannel > 0 && useChannel < 3) ? tmp_chanSel : "0",
             useCase,
             mode,
             useTitle,
             useSubtitle,               // 11
             useDescription,
             useDuration,
             tmp_minDuration,
             tmp_maxDuration,
             useDayOfWeek,
             DayOfWeek,
             useExtEPGInfo,
             useExtEPGInfo ? tmp_catvalues : "",
             fuzzyTolerance,
             extEPGInfoMatchingMode,      // 21
             isGlobal,
             contentsFilter.c_str(),
             contentsCategoryMatchingMode,
             contentsCharacteristicsMatchingMode,
             useParentalRating,
             minParentalRating,
             maxParentalRating);

    free(tmp_search);
    free(tmp_chanSel);
    free(tmp_catvalues);

    return buffer;
}

bool cBlacklist::Parse(const char *s)
{
    char *line;
    char *pos;
    char *pos_next;
    int parameter = 1;
    int valuelen;
    char value[MaxFileName];

    contentsFilter = "";

    pos = line = strdup(s);
    pos_next = pos + strlen(pos);
    if (*pos_next == '\n') *pos_next = 0;
    while (*pos) {
        while (*pos == ' ') pos++;
        if (*pos) {
            if (*pos != ':') {
                pos_next = strchr(pos, ':');
                if (!pos_next)
                    pos_next = pos + strlen(pos);
                valuelen = pos_next - pos + 1;
                if (valuelen > MaxFileName) valuelen = MaxFileName;
                strn0cpy(value, pos, valuelen);
                pos = pos_next;
                switch (parameter) {
                case 1:
                    ID = atoi(value);
                    break;
                case 2: {
                    char* s = decodeSpecialCharacters(value);
                    strn0cpy(search, s, sizeof(search));
                    free(s);
                    }
                    break;
                case 3:
                    useTime = atoi(value);
                    break;
                case 4:
                    startTime = atoi(value);
                    break;
                case 5:
                    stopTime = atoi(value);
                    break;
                case 6:
                    useChannel = atoi(value);
                    break;
                case 7:
                    if (useChannel == 0) {
                        channelMin = NULL;
                        channelMax = NULL;
                    } else if (useChannel == 1) {
                        int minNum = 0, maxNum = 0;
                        int fields = sscanf(value, "%d-%d", &minNum, &maxNum);
                        if (fields == 0) { // stored with ID
#ifdef __FreeBSD__
                            char *channelMinbuffer = MALLOC(char, 32);
                            char *channelMaxbuffer = MALLOC(char, 32);
                            int channels = sscanf(value, "%31[^|]|%31[^|]", channelMinbuffer, channelMaxbuffer);
#else
                            char *channelMinbuffer = NULL;
                            char *channelMaxbuffer = NULL;
                            int channels = sscanf(value, "%m[^|]|%m[^|]", &channelMinbuffer, &channelMaxbuffer);
#endif
                            LOCK_CHANNELS_READ;
                            channelMin = Channels->GetByChannelID(tChannelID::FromString(channelMinbuffer), true, true);
                            if (!channelMin) {
                                LogFile.eSysLog("ERROR: channel %s not defined", channelMinbuffer);
                                channelMin = channelMax = NULL;
                                useChannel = 0;
                            }
                            if (channels == 1)
                                channelMax = channelMin;
                            else {
                                channelMax = Channels->GetByChannelID(tChannelID::FromString(channelMaxbuffer), true, true);
                                if (!channelMax) {
                                    LogFile.eSysLog("ERROR: channel %s not defined", channelMaxbuffer);
                                    channelMin = channelMax = NULL;
                                    useChannel = 0;
                                }
                            }
                            free(channelMinbuffer);
                            free(channelMaxbuffer);
                        }
                    } else if (useChannel == 2)
                        channelGroup = strdup(value);
                    break;
                case 8:
                    useCase = atoi(value);
                    break;
                case 9:
                    mode = atoi(value);
                    break;
                case 10:
                    useTitle = atoi(value);
                    break;
                case 11:
                    useSubtitle = atoi(value);
                    break;
                case 12:
                    useDescription = atoi(value);
                    break;
                case 13:
                    useDuration = atoi(value);
                    break;
                case 14:
                    minDuration = atoi(value);
                    break;
                case 15:
                    maxDuration = atoi(value);
                    break;
                case 16:
                    useDayOfWeek = atoi(value);
                    break;
                case 17:
                    DayOfWeek = atoi(value);
                    break;
                case 18:
                    useExtEPGInfo = atoi(value);
                    break;
                case 19:
                    if (!ParseExtEPGValues(value)) {
                        LogFile.eSysLog("ERROR reading ext. EPG values - 1");
                        free(line);
                        return false;
                    }
                    break;
                case 20:
                    fuzzyTolerance = atoi(value);
                    break;
                case 21:
                    extEPGInfoMatchingMode = atoi(value);
                    break;
                case 22:
                    isGlobal = atoi(value);
                    break;
                case 23:
                    // no need for replacing special characters, as these would cause the check to fail
                    value[MaxFileName - 1] = '\0';  // simulate strn0cpy()
                    contentsFilter = value;
                    useContentsFilter = !contentsFilter.empty();
                    break;
                case 24:
                    contentsCategoryMatchingMode = atoi(value);
                    break;
                case 25:
                    contentsCharacteristicsMatchingMode = atoi(value);
                    break;
                case 26:
                    useParentalRating = atoi(value);
                    break;
                case 27:
                    minParentalRating = atoi(value);
                    break;
                case 28:
                    maxParentalRating = atoi(value);
                    break;
                default:
                    break;
                } //switch
            }
            parameter++;
        }
        if (*pos) pos++;
    } //while

    free(line);
    return (parameter >= 19) ? true : false;
}

bool cBlacklist::ParseExtEPGValues(const char *s)
{
    char *line;
    char *pos;
    char *pos_next;
    int valuelen;
    char value[MaxFileName];

    pos = line = strdup(s);
    pos_next = pos + strlen(pos);
    if (*pos_next == '\n') *pos_next = 0;
    while (*pos) {
        while (*pos == ' ') pos++;
        if (*pos) {
            if (*pos != '|') {
                pos_next = strchr(pos, '|');
                if (!pos_next)
                    pos_next = pos + strlen(pos);
                valuelen = pos_next - pos + 1;
                if (valuelen > MaxFileName) valuelen = MaxFileName;
                strn0cpy(value, pos, valuelen);
                pos = pos_next;
                if (!ParseExtEPGEntry(value)) {
                    LogFile.eSysLog("ERROR reading ext. EPG value: %s", value);
                    free(line);
                    return false;
                }
            }
        }
        if (*pos) pos++;
    } //while

    free(line);
    return true;
}

bool cBlacklist::ParseExtEPGEntry(const char *s)
{
    char *line;
    char *pos;
    char *pos_next;
    int parameter = 1;
    int valuelen;
    char value[MaxFileName];
    int currentid = -1;

    pos = line = strdup(s);
    pos_next = pos + strlen(pos);
    if (*pos_next == '\n') *pos_next = 0;
    while (*pos) {
        while (*pos == ' ') pos++;
        if (*pos) {
            if (*pos != '#') {
                pos_next = strchr(pos, '#');
                if (!pos_next)
                    pos_next = pos + strlen(pos);
                valuelen = pos_next - pos + 1;
                if (valuelen > MaxFileName) valuelen = MaxFileName;
                strn0cpy(value, pos, valuelen);
                pos = pos_next;
                switch (parameter) {
                case 1: {
                    currentid = atoi(value);
                    int index = SearchExtCats.GetIndexFromID(currentid);
                    if (index > -1)
                        strcpy(catvalues[index], "");
                }
                break;
                case 2:
                    if (currentid > -1) {
                        int index = SearchExtCats.GetIndexFromID(currentid);
                        if (index > -1) {
                            char* s = decodeSpecialCharacters(value, true);
                            strn0cpy(catvalues[index], s, MaxFileName);
                            free(s);
                        }
                    }
                    break;
                default:
                    break;
                } //switch
            }
            parameter++;
        }
        if (*pos) pos++;
    } //while

    free(line);
    return (parameter >= 2) ? true : false;
}

bool cBlacklist::Save(FILE *f)
{
    return fprintf(f, "%s\n", ToText()) > 0;
}

const cEvent * cBlacklist::GetEventByBlacklist(const cSchedule *schedule, const cEvent *Start, int MarginStop)
{
    const cEvent *pe = NULL;
    const cEvent *p1 = NULL;

    if (Start)
        p1 = schedule->Events()->Next(Start);
    else
        p1 = schedule->Events()->First();

    time_t tNow = time(NULL);
    char* szTest = NULL;
    char* searchText = strdup(search);

    if (!useCase)
        ToLower(searchText);

    int searchStart = 0, searchStop = 0;
    if (useTime) {
        searchStart = startTime;
        searchStop = stopTime;
        if (searchStop < searchStart)
            searchStop += 2400;
    }
    int minSearchDuration = 0;
    int maxSearchDuration = 0;
    if (useDuration) {
        minSearchDuration = minDuration / 100 * 60 + minDuration % 100;
        maxSearchDuration = maxDuration / 100 * 60 + maxDuration % 100;
    }

    for (const cEvent *p = p1; p; p = schedule->Events()->Next(p)) {
        // ignore events without title
        if (!p->Title() || strlen(p->Title()) == 0)
            continue;

        free(szTest);
        msprintf(&szTest, "%s%s%s%s%s", (useTitle ? p->Title() : ""), (useSubtitle || useDescription) ? "~" : "",
                 (useSubtitle ? p->ShortText() : ""), useDescription ? "~" : "",
                 (useDescription ? p->Description() : ""));

        if (tNow < p->EndTime() + MarginStop * 60) {
            if (!useCase)
                ToLower(szTest);

            if (useTime) {
                time_t tEvent = p->StartTime();
                struct tm tmEvent;
                localtime_r(&tEvent, &tmEvent);
                int eventStart = tmEvent.tm_hour * 100 + tmEvent.tm_min;
                int eventStart2 = tmEvent.tm_hour * 100 + tmEvent.tm_min + 2400;
                if ((eventStart < searchStart || eventStart > searchStop) &&
                    (eventStart2 < searchStart || eventStart2 > searchStop))
                    continue;
            }
            if (useDuration) {
                int duration = p->Duration() / 60;
                if (minSearchDuration > duration || maxSearchDuration < duration)
                    continue;
            }

            if (useDayOfWeek) {
                time_t tEvent = p->StartTime();
                struct tm tmEvent;
                localtime_r(&tEvent, &tmEvent);
                if (DayOfWeek >= 0 && DayOfWeek != tmEvent.tm_wday)
                    continue;
                if (DayOfWeek < 0) {
                    int iFound = 0;
                    for (int i = 0; i < 7; i++)
                        if (abs(DayOfWeek) & (int)pow(2, i) && i == tmEvent.tm_wday) {
                            iFound = 1;
                            break;
                        }
                    if (!iFound)
                        continue;
                }
            }

            if (strlen(szTest) > 0) {
                if (!MatchesSearchMode(szTest, searchText, mode, " ,;|~", fuzzyTolerance))
                    continue;
            }

            if (useParentalRating) {
                int rating = p->ParentalRating();
                if (minParentalRating > rating || maxParentalRating < rating)
                    continue;
            }

            if (useContentsFilter && !MatchesContentsFilter(p))
                continue;

            if (useExtEPGInfo && !MatchesExtEPGInfo(p))
                continue;
            pe = p;
            break;
        }
    }
    free(szTest);
    free(searchText);
    return pe;
}

// returns a pointer array to the matching search results
cSearchResults* cBlacklist::Run(cSearchResults* pSearchResults, int MarginStop)
{
    LogFile.Log(3, "start search for blacklist '%s'", search);
    LOCK_CHANNELS_READ;
    LOCK_SCHEDULES_READ;
    const cSchedule *Schedule = Schedules->First();

    while (Schedule) {
        const cChannel* channel = Channels->GetByChannelID(Schedule->ChannelID(), true, true);
        if (!channel) {
            Schedule = Schedules->Next(Schedule);
            continue;
        }

        if (useChannel == 1 && channelMin && channelMax) {
            // caution: reordering the channels via OSD can yield unexpected results
            if (channelMin->Number() > channel->Number() || channelMax->Number() < channel->Number()) {
                Schedule = Schedules->Next(Schedule);
                continue;
            }
        }
        if (useChannel == 2 && channelGroup) {
            cChannelGroup* group = ChannelGroups.GetGroupByName(channelGroup);
            if (!group || !group->ChannelInGroup(channel)) {
                Schedule = Schedules->Next(Schedule);
                continue;
            }
        }
        if (useChannel == 3) {
            if (channel->Ca() >= CA_ENCRYPTED_MIN) {
                Schedule = Schedules->Next(Schedule);
                continue;
            }
        }

        const cEvent *pPrevEvent = NULL;
        do {
            const cEvent* event = GetEventByBlacklist(Schedule, pPrevEvent, MarginStop);
            pPrevEvent = event;
            if (event && Channels->GetByChannelID(event->ChannelID(), true, true)) {
                if (!pSearchResults) pSearchResults = new cSearchResults;
                pSearchResults->Add(new cSearchResult(event, this));
            }
        } while (pPrevEvent);
        Schedule = Schedules->Next(Schedule);
    }
    LogFile.Log(3, "found %d event(s) for blacklist '%s'", pSearchResults ? pSearchResults->Count() : 0, search);

    return pSearchResults;
}

bool cBlacklist::MatchesExtEPGInfo(const cEvent* e)
{
    if (!e || !e->Description())
        return false;
    bool matching = extEPGInfoMatchingMode != 2;
    cSearchExtCat* SearchExtCat = SearchExtCats.First();
    while (SearchExtCat) {
        char* value = NULL;
        int index = SearchExtCats.GetIndexFromID(SearchExtCat->id);
        if (index > -1)
            value = catvalues[index];
        if (value && SearchExtCat->searchmode >= 10 && atol(value) == 0) // numerical value != 0 ?
            value = NULL;
        if (value && *value) {
            char* testvalue = GetExtEPGValue(e, SearchExtCat);
            if (!testvalue) {
                if (extEPGInfoMatchingMode == 0)
                    return false;
            } else {
                // compare case insensitive
                char* valueLower = strdup(value);
                ToLower(valueLower);
                ToLower(testvalue);
                bool valueMatches = MatchesSearchMode(testvalue, valueLower, SearchExtCat->searchmode, ",;|~", fuzzyTolerance);
                free(testvalue);
                free(valueLower);
                if (extEPGInfoMatchingMode != 2)
                    matching &= valueMatches;
                else
                    matching |= valueMatches;
            }
        }
        SearchExtCat = SearchExtCats.Next(SearchExtCat);
    }
    return matching;
}

bool cBlacklist::HasContentID(int contentID)
{
    for (unsigned int i = 0; i < contentsFilter.size(); i += 2) {
        std::string hexContentID = contentsFilter.substr(i, 2);
        if (hexContentID.size() != 2) return false;
        std::istringstream iss(hexContentID);
        int tmpContentID = 0;
        if (!(iss >> std::noshowbase >> std::hex >> tmpContentID)) return false;
        if (contentID == tmpContentID) return true;
    }
    return false;
}

void cBlacklist::SetContentsFilter(const int* contentDescriptorFlags)
{
    // create the hex array of content descriptor IDs
    std::ostringstream oss;
    for (unsigned int i = 0; contentDescriptorFlags && i <= CONTENT_DESCRIPTOR_MAX; i++) {
        if (contentDescriptorFlags[i]) {
            oss << std::hex << std::noshowbase << i;
        }
    }
    contentsFilter = oss.str();
}

bool cBlacklist::MatchesContentsFilter(const cEvent* e)
{
    if (!e || !e->Contents(0)) return false;
    // ensure match with VDR settings
    constexpr unsigned int contentIdentifiersPerGroup = 16;
    // check if each content filter ID is contained in the events descriptors;
    // at least one entry within a content group must match, and all non-empty
    // groups must have at least one matching entry
    unsigned int checkedGroups = 0;         // set bit indicates that group is contained in contentsFilter
    unsigned int checkedCharacteristics = 0;
    unsigned int matchingGroups = 0;        // set bit indicates that at least a matching content ID within the group
    unsigned int matchingCharacteristics = 0;
    for (unsigned int i = 0; i < contentsFilter.size(); i += 2) {
        std::string hexContentID = contentsFilter.substr(i, 2);
        if (hexContentID.size() != 2) return false;
        std::istringstream iss(hexContentID);
        int searchContentID = 0;
        if (!(iss >> std::hex >> searchContentID)) return false;
        unsigned int gid = searchContentID / contentIdentifiersPerGroup;
        unsigned int cid = searchContentID % contentIdentifiersPerGroup;
        int c = 0, eventContentID = 0;
        bool found = false;
        while ((eventContentID = e->Contents(c++)) > 0) {
            if (gid == 0xB) {
                // special characteristics use their own matching mode
                checkedCharacteristics |= 1 << cid;
                if (eventContentID == searchContentID) {
                    matchingCharacteristics |= 1 << cid;
                    found = true;   // to prevent from unintended exit
                    break;
                }
            } else {
                checkedGroups |= 1 << gid;
                if (eventContentID == searchContentID) {
                    if (contentsCategoryMatchingMode == 1) {
                        // any descriptor satisfies the matching mode
                        return true;
                    }
                    matchingGroups |= 1 << gid;
                    found = true;
                    break;
                }
            }
        }
        if (contentsCategoryMatchingMode == 0 && !found) {
            // only all descriptors satisfy the matching mode
            return false;
        }
    }
    // check special characteristics w.r.t. matching mode
    bool characteristicsIsMatching;
    if (contentsCharacteristicsMatchingMode == 0) {
        // must be exact match
        characteristicsIsMatching = checkedCharacteristics == matchingCharacteristics;
    } else {
        // one match satisfies, provided that some characteristics was selected at all
        characteristicsIsMatching = !checkedCharacteristics || matchingCharacteristics;
    }
    // some selected descriptor within each touched group
    return characteristicsIsMatching && (contentsCategoryMatchingMode == 0 || checkedGroups == matchingGroups);
}

// -- cBlacklists ----------------------------------------------------------------
int cBlacklists::GetNewID()
{
    int newID = -1;
    cMutexLock BlacklistLock(this);
    cBlacklist *l = (cBlacklist *)First();
    while (l) {
        newID = std::max(newID, l->ID);
        l = (cBlacklist *)l->Next();
    }
    return newID + 1;
}

cBlacklist* cBlacklists::GetBlacklistFromID(int ID)
{
    if (ID == -1)
        return NULL;
    cMutexLock BlacklistLock(this);
    cBlacklist *l = (cBlacklist *)First();
    while (l) {
        if (l->ID == ID)
            return l;
        l = (cBlacklist *)l->Next();
    }
    return NULL;
}

bool cBlacklists::Exists(const cBlacklist* Blacklist)
{
    cMutexLock BlacklistLock(this);
    cBlacklist *l = (cBlacklist*)First();
    while (l) {
        if (l == Blacklist)
            return true;
        l = (cBlacklist*)l->Next();
    }
    return false;
}

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

#ifndef __EPGSEARCH_TOOLS_INC__
#define __EPGSEARCH_TOOLS_INC__

#include <string>
#include <vdr/config.h> // For VDRVERSNUM only
#include <vdr/font.h>
#include "epgsearchext.h"
#include "recdone.h"
#include "i18n.h"

using std::string;

#define MAXPARSEBUFFER KILOBYTE(10)

#undef CHANNELNAME
#define CHANNELNAME(x) (x ? x->ShortName(true) : "")

#undef TIMESTRING
#define TIMESTRING(x) *(TimeString(x)) 

#undef DATESTRING
#define DATESTRING(x) *(DateString(x)) 

#undef GETDATESTRING
#define GETDATESTRING(x) *(x->GetDateString()) 

#undef GETTIMESTRING
#define GETTIMESTRING(x) *(x->GetTimeString()) 

#undef PRINTDAY
#define PRINTDAY *cTimer::PrintDay

#undef DAYDATETIME
#define DAYDATETIME(x) *DayDateTime(x) 

#undef CHANNELSTRING
#define CHANNELSTRING(x) (*x->GetChannelID().ToString()) 

#undef WEEKDAYNAME
#define WEEKDAYNAME(x) (*WeekDayName(x)) 

#undef ADDDIR
#define ADDDIR *AddDirectory 

#undef CONFIGDIR
#define CONFIGDIR (!ConfigDir?cPlugin::ConfigDirectory():ConfigDir)

#define CHNUMWIDTH  (numdigits(Channels.MaxNumber()) + 2)

#define SHORTTEXT(EVENT) \
  (EVENT && EPGSearchConfig.showShortText && !isempty((EVENT)->ShortText()))?" ~ ":"", \
  (EVENT && EPGSearchConfig.showShortText && !isempty((EVENT)->ShortText()))?(EVENT)->ShortText():""

#define ISRADIO(x) ((x)->Vpid()==0||(x)->Vpid()==1||(x)->Vpid()==0x1fff)

// Icons used in WarEagle and VDRSymbols-Font
#define ICON_REC               0xF9
#define ICON_RUNNING           0xFB
#define ICON_CLOCK             0xFD
#define ICON_CLOCK_HALF        0x74
#define ICON_BAR_OPEN          0x82
#define ICON_BAR_FULL          0xFF
#define ICON_BAR_EMPTY         0x83
#define ICON_BAR_CLOSE         0x84
#define ICON_VPS               0x56


// UTF-8 Icons
#define ICON_BAR_OPEN_UTF8     "\uE007"
#define ICON_BAR_FULL_UTF8     "\uE008"
#define ICON_BAR_EMPTY_UTF8    "\uE009"
#define ICON_BAR_CLOSE_UTF8    "\uE00A"
#define ICON_REC_UTF8          "\uE00B"
#define ICON_CLOCK_UTF8        "\uE00C"
#define ICON_CLOCK_HALF_UTF8   "\uE014"
#define ICON_RUNNING_UTF8      "\uE012"
#define ICON_VPS_UTF8          "\uE013"

#if APIVERSNUM < 10503 
#define Utf8BufSize(s) ((s) * 4)
#endif

extern const char AllowedChars[];

extern char* ConfigDir;

// Helper functions
class cSearchExt;
class cSearchExtCat;
class cEvent;

char* IndentMenuItem(const char*);
void PatchFont(eDvbFont);
bool MatchesSearchMode(const char* test, const char* values, int searchmode, const char* delim, int tolerance);
char* GetExtEPGValue(const cEvent* e, cSearchExtCat* SearchExtCat);
char* GetExtEPGValue(const char* description, const char* catname);
char* GetAuxValue(const char* aux, const char* name);
char* GetAuxValue(const cRecording *recording, const char* name);
char* GetAuxValue(const cTimer* timer, const char* name);
string UpdateAuxValue(string aux, string section, string value);
string UpdateAuxValue(string aux, string section, long num);
void ToLower(char* szText);
char *strreplacei(char *s, const char *s1, const char *s2);
char *strreplaceall(char* source, char ch, const char* replacement);
void sleepMSec(long ms);
void sleepSec(long s);
bool SendViaSVDRP(cString SVDRPcmd);
int SendMsg(cString Message, bool confirm = false, int seconds = 0);
bool InEditMode(const char* ItemText, const char* ItemName, const char* ItemValue);
cSearchExt* TriggeredFromSearchTimer(const cTimer* timer);
int TriggeredFromSearchTimerID(const cTimer* timer);
double FuzzyMatch(const char* s1, const char* s2, int maxLength);
bool DescriptionMatches(const char* eDescr, const char* rDescr);
const cEvent* GetEvent(cTimer* timer);
char* GetRawDescription(const char* descr);
void PrepareTimerFile(const cEvent* event, cTimer* timer);
int CompareEventTime(const void *p1, const void *p2);
int CompareEventChannel(const void *p1, const void *p2);
bool EventsMatch(const cEvent* event1, const cEvent* event2, bool compareTitle, bool compareSubtitle, bool compareSummary, unsigned long catvaluesAvoidRepeat);
int ChannelNrFromEvent(const cEvent* pEvent);
void DelTimer(int index);
char* FixSeparators(char* buffer, char sep);
cString DateTime(time_t t);
string NumToString(long l);
int FindIgnoreCase(const string& expr, const string& query);
bool EqualsNoCase(const string& a, const string& b);
string Strip(const string& input);
string ReplaceAll(const string& input, const string& what, const string& with);
string GetAlNum(const string& s);
string EscapeString(const string& S);
string QuoteApostroph(const string& S);
string MD5(const string& input);
time_t GetDateTime(time_t day, int start);
void SetAux(cTimer* timer, string aux);
int msprintf(char **strp, const char *fmt, ...);

// --- cTimerObj --------------------------------------------------------
class cTimerObj : public cListObject {
public:
    cTimer* timer;
    cTimerObj(cTimer* Timer) : timer(Timer) {}
    ~cTimerObj() { timer = NULL;  } // do not delete anything!
};

// --- cTimerObjList --------------------------------------------------------
class cTimerObjList : public cList<cTimerObj> {
public:
    void DelTimer(cTimer* t)
	{
	    for (cTimerObj* pTObj = First(); pTObj; pTObj = Next(pTObj))
		if (pTObj->timer == t)
		{
		    Del(pTObj);
		    return;
		}
	}
};

// --- icstring ------------------------------------------
// a case-insensitive string class

struct ignorecase_traits : public std::char_traits<char> {
    // return whether c1 and c2 are equal
    static bool eq(const char& c1, const char& c2) {
      return (c1==c2 || std::toupper(c1)==std::toupper(c2));
    }
    // return whether c1 is less than c2
    static bool lt(const char& c1, const char& c2) {
      return std::toupper(c1)<std::toupper(c2);
    }
    // compare up to n characters of s1 and s2
    static int compare(const char* s1, const char* s2,
                       std::size_t n) {
        for (std::size_t i=0; i<n; ++i) {
            if (!eq(s1[i],s2[i])) {
                return lt(s1[i],s2[i])?-1:1;
            }
        }
        return 0;
    }
    // search c in s
    static const char* find(const char* s, std::size_t n,
                            const char& c) {
        for (std::size_t i=0; i<n; ++i) {
            if (eq(s[i],c)) {
                return &(s[i]);
            }
        }
        return 0;
    }
};

// define a special type for such strings
typedef std::basic_string<char,ignorecase_traits> icstring;


// --- eTimerMod -------------------------------------------------------------
enum eTimerMod { tmNoChange=0, tmStartStop=1, tmFile=2, tmAuxEventID=4 };

#endif

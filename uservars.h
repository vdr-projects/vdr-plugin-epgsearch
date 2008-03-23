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

#ifndef __USERVARS_INC__
#define __USERVARS_INC__

#include <algorithm>
#include <string>
#include <set>
#include <map>
#include <sstream>
#include <vdr/plugin.h>
#include <vdr/videodir.h>
#include "varparser.h"
#include "epgsearchtools.h"
#include "epgsearchcats.h"

using std::string;
using std::set;
using std::map;
using std::ostringstream;

string NumToString(long num);
int FindIgnoreCase(const string& expr, const string& query);

class cUserVar : public cListObject {
    const cEvent* oldEvent; // cache 
    bool oldescapeStrings; 

    string oldResult;
    string EvaluateCondExpr(const cEvent* e, bool escapeStrings = false);
    string EvaluateCompExpr(const cEvent* e, bool escapeStrings = false);
    string EvaluateShellCmd(const cEvent* e);
public:
    cUserVar();
    cVarParser varparser;
    set<cUserVar*> usedVars;

    virtual string Evaluate(const cEvent* e, bool escapeStrings = false);

    string EvaluateInternalVars(const string& Expr, const cEvent* e, bool escapeStrings = false);
    string EvaluateInternalTimerVars(const string& Expr, const cTimer* t);
    string EvaluateInternalSearchVars(const string& Expr, const cSearchExt* s);
    string EvaluateExtEPGVars(const string& Expr, const cEvent* e, bool escapeStrings = false);
    string EvaluateUserVars(const string& Expr, const cEvent* e, bool escapeStrings = false);
    virtual string Name() { return varparser.varName; }
    virtual bool IsCondExpr() { return varparser.IsCondExpr(); }
    virtual bool IsShellCmd() { return varparser.IsShellCmd(); }
    bool DependsOnVar(const string& varName);
    bool DependsOnVar(cUserVar* var);
    bool AddDepVar(cUserVar* var);
    void ResetCache();
};

class cExtEPGVar : public cUserVar {
 public:
    const string name;
    cExtEPGVar(const string& Name) : name(Name) {}
    string Name() { return name; }
    bool IsCondExpr() { return false; }
    bool IsShellCmd() { return false; }
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";

	    cSearchExtCat* SearchExtCat = SearchExtCats.First();
	    while (SearchExtCat) 
	    {
		string varName = string("%") + SearchExtCat->name + string("%");
		int varPos = FindIgnoreCase(varName, name);
		if (varPos == 0)
		{
		    char* value = GetExtEPGValue(e, SearchExtCat);
		    string res =  value?value:"";
		    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
		}
		SearchExtCat = SearchExtCats.Next(SearchExtCat);
	    }
	    return "";
	}
};

class cInternalVar : public cUserVar {
    const string name;
 public:
    cInternalVar(const string& Name) : name(Name) {}
    string Name() { return name; }
    bool IsCondExpr() { return false; }
    bool IsShellCmd() { return false; }
};

class cTitleVar : public cInternalVar {
public:
    cTitleVar() : cInternalVar("%title%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{
	    string res = (e && !isempty(e->Title()))? e->Title() : ""; 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cSubtitleVar : public cInternalVar {
public:
    cSubtitleVar() : cInternalVar("%subtitle%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{
	    string res = (e && !isempty(e->ShortText()))? e->ShortText() : ""; 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cSummaryVar : public cInternalVar {
public:
    cSummaryVar() : cInternalVar("%summary%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    string res = (e && !isempty(e->Description()))? e->Description() : ""; 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cHTMLSummaryVar : public cInternalVar {
public:
    cHTMLSummaryVar() : cInternalVar("%htmlsummary%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (e && !isempty(e->Description()))
	    {
		string res = ReplaceAll(e->Description(), "\n", "<br />");
		if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	    }
	    else
		return "";
	}
};

class cEventIDVar : public cInternalVar {
public:
    cEventIDVar() : cInternalVar("%eventid%") {}
    string Evaluate(const cEvent* e,  bool escapeStrings = false) 
    { 
      if (e) 
	{
	  ostringstream os;
	  os << e->EventID();
	  return os.str();
	}
      else return ""; }
};

class cTimeVar : public cInternalVar {
public:
    cTimeVar() : cInternalVar("%time%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    string res = (e? *(e->GetTimeString()) : ""); 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cTimeEndVar : public cInternalVar {
public:
    cTimeEndVar() : cInternalVar("%timeend%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    string res = (e? *(e->GetEndTimeString()) : ""); 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cTime_wVar : public cInternalVar {
public:
    cTime_wVar() : cInternalVar("%time_w%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    string res = (e? WEEKDAYNAME(e->StartTime()) : ""); 
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cTime_dVar : public cInternalVar {
public:
    cTime_dVar() : cInternalVar("%time_d%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    char day[3] = "";
	    struct tm tm_r;
	    const time_t t = e->StartTime();
	    tm *tm = localtime_r(&t, &tm_r);
	    strftime(day, sizeof(day), "%d", tm);
	    if (escapeStrings) return "'" + EscapeString(day) + "'"; else return day;
	}
};

class cTime_lngVar : public cInternalVar {
public:
    cTime_lngVar() : cInternalVar("%time_lng%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    ostringstream os;
	    os << e->StartTime();
	    if (escapeStrings) return "'" + EscapeString(os.str()) + "'"; else return os.str();
	}
};

class cTimeSpanVar : public cInternalVar {
public:
    cTimeSpanVar() : cInternalVar("%timespan%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    time_t diff = e->StartTime() - time(NULL);
	    string res;
	    if (labs(diff) >= SECSINDAY)
	    {
	      cString buffer;
	      if (diff > 0)
		buffer = cString::sprintf(tr("in %02ldd"), long(diff / SECSINDAY));
	      else
		buffer = cString::sprintf("%02ldd", long(-diff / SECSINDAY));
	      res = buffer;
	    }
	    else if (labs(diff) >= (60 * 60))
	    {
	      cString buffer;
	      if (diff > 0)
		buffer = cString::sprintf(tr("in %02ldh"), long(diff / (60*60)));
	      else
		buffer = cString::sprintf("%02ldh", long(-diff / (60*60)));
	      res = buffer;
	    }
	    else
	    {
	      cString buffer;
	      if (diff > 0)
		buffer = cString::sprintf(tr("in %02ldm"), long(diff / 60));
	      else
		buffer = cString::sprintf("%02ldm", long(-diff / 60));
	      res = buffer;
	    }
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cLength_Var : public cInternalVar {
public:
    cLength_Var() : cInternalVar("%length%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    return (e? NumToString(e->Duration()) : ""); 
	}
};

class cDateVar : public cInternalVar {
public:
    cDateVar() : cInternalVar("%date%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    char date[9] = "";
	    struct tm tm_r;
	    const time_t t = e->StartTime();
	    tm *tm = localtime_r(&t, &tm_r);
	    strftime(date, sizeof(date), "%d.%m.%y", tm);
	    if (escapeStrings) return "'" + EscapeString(date) + "'"; else return date;
	}
};

class cDateShortVar : public cInternalVar {
public:
    cDateShortVar() : cInternalVar("%datesh%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    char dateshort[7] = "";
	    struct tm tm_r;
	    const time_t t = e->StartTime();
	    tm *tm = localtime_r(&t, &tm_r);
	    strftime(dateshort, sizeof(dateshort), "%d.%m.", tm);
	    if (escapeStrings) return "'" + EscapeString(dateshort) + "'"; else return dateshort;
	}
};

class cChannelNrVar : public cInternalVar {
public:
    cChannelNrVar() : cInternalVar("%chnr%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    int chnr = ChannelNrFromEvent(e);
	    if (chnr < 0) return "";
	    return NumToString(chnr);
	}
};

class cChannelShortVar : public cInternalVar {
public:
    cChannelShortVar() : cInternalVar("%chsh%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    cChannel *channel = Channels.GetByChannelID(e->ChannelID(), true);
	    string res = channel?channel->ShortName(true):"";
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cChannelLongVar : public cInternalVar {
public:
    cChannelLongVar() : cInternalVar("%chlng%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    cChannel *channel = Channels.GetByChannelID(e->ChannelID(), true);
	    string res = channel?channel->Name():"";
	    if (escapeStrings) return "'" + EscapeString(res) + "'"; else return res;
	}
};

class cChannelDataVar : public cInternalVar {
 public:
    cChannelDataVar() : cInternalVar("%chdata%") {}
    string Evaluate(const cEvent* e, bool escapeStrings = false) 
	{ 
	    if (!e) return "";
	    cChannel *channel = Channels.GetByChannelID(e->ChannelID(), true);
	    return channel?CHANNELSTRING(channel):"";
	}
};

// independet variables
class cColonVar : public cInternalVar {
 public:
    cColonVar() : cInternalVar("%colon%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    return ":";
	}
};
 
class cDateNowVar : public cInternalVar {
 public:
    cDateNowVar() : cInternalVar("%datenow%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    char date[9] = "";
	    struct tm tm_r;
	    const time_t t = time(NULL);
	    tm *tm = localtime_r(&t, &tm_r);
	    strftime(date, sizeof(date), "%d.%m.%y", tm);
	    if (escapeStrings) return "'" + EscapeString(date) + "'"; else return date;
	}
};

class cDateShortNowVar : public cInternalVar {
public:
    cDateShortNowVar() : cInternalVar("%dateshnow%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    char dateshort[7] = "";
	    struct tm tm_r;
	    const time_t t = time(NULL);
	    tm *tm = localtime_r(&t, &tm_r);
	    strftime(dateshort, sizeof(dateshort), "%d.%m.", tm);
	    if (escapeStrings) return "'" + EscapeString(dateshort) + "'"; else return dateshort;
	}
};

class cTimeNowVar : public cInternalVar {
public:
    cTimeNowVar() : cInternalVar("%timenow%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    return TIMESTRING(time(NULL));
	}
};

class cVideodirVar : public cInternalVar {
 public:
    cVideodirVar() : cInternalVar("%videodir%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    return VideoDirectory;
	}
};
 
class cPlugconfdirVar : public cInternalVar {
 public:
    static string dir;
    cPlugconfdirVar() : cInternalVar("%plugconfdir%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    return dir;
	}
};

class cEpgsearchconfdirVar : public cInternalVar {
 public:
    static string dir;
    cEpgsearchconfdirVar() : cInternalVar("%epgsearchdir%") {}
    string Evaluate(const cEvent*, bool escapeStrings = false) 
	{ 
	    return CONFIGDIR;
	}
};

// timer variables
class cTimerVar {
    const string name;
 public:    
    cTimerVar(const string& Name) : name(Name) {}
    virtual ~cTimerVar() {}
    string Name() { return name; }
    virtual string Evaluate(const cTimer* t) = 0;
};

class cTimerDateVar : public cTimerVar {
 public:
    cTimerDateVar() : cTimerVar("%timer.date%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    return DATESTRING(t->StartTime());
	}
};

class cTimerStartVar : public cTimerVar {
 public:
    cTimerStartVar() : cTimerVar("%timer.start%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    return TIMESTRING(t->StartTime());
	}
};

class cTimerStopVar : public cTimerVar {
 public:
    cTimerStopVar() : cTimerVar("%timer.stop%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    return TIMESTRING(t->StopTime());
	}
};

class cTimerFileVar : public cTimerVar {
 public:
    cTimerFileVar() : cTimerVar("%timer.file%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    return t->File();
	}
};

class cTimerChnrVar : public cTimerVar {
 public:
    cTimerChnrVar() : cTimerVar("%timer.chnr%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t || !t->Channel()) return "";
	    return NumToString(t->Channel()->Number());
	}
};

class cTimerChannelShortVar : public cTimerVar {
public:
    cTimerChannelShortVar() : cTimerVar("%timer.chsh%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t || !t->Channel()) return "";
	    return t->Channel()->ShortName(true);
	}
};

class cTimerChannelLongVar : public cTimerVar {
public:
    cTimerChannelLongVar() : cTimerVar("%timer.chlng%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t || !t->Channel()) return "";
	    return t->Channel()->Name();
	}
};

class cTimerSearchVar : public cTimerVar {
 public:
    cTimerSearchVar() : cTimerVar("%timer.search%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    cSearchExt* s = TriggeredFromSearchTimer(t);
	    if (!s) return "";
	    return s->search;
	}
};

class cTimerSearchIDVar : public cTimerVar {
 public:
    cTimerSearchIDVar() : cTimerVar("%timer.searchid%") {}
    string Evaluate(const cTimer* t) 
	{ 
	    if (!t) return "";
	    int ID = TriggeredFromSearchTimerID(t);
	    if (ID < 0) return "";
	    return NumToString(ID);
	}
};

// search variables
class cSearchVar {
    const string name;
 public:    
    cSearchVar(const string& Name) : name(Name) {}
    virtual ~cSearchVar() {}
    string Name() { return name; }
    virtual string Evaluate(const cSearchExt* s) = 0;
};

class cSearchQueryVar : public cSearchVar {
public:
    cSearchQueryVar() : cSearchVar("%search.query%") {}
    string Evaluate(const cSearchExt* s) 
	{ 
	    if (!s) return "";
	    return s->search;
	}
};

class cUserVars : public cList<cUserVar> {
 public:
    cTitleVar titleVar;
    cSubtitleVar subtitleVar;
    cSummaryVar summaryVar;
    cHTMLSummaryVar htmlsummaryVar;
    cEventIDVar eventIDVar;
    cTimeVar timeVar;
    cTimeEndVar timeEndVar;
    cTime_wVar time_wVar;
    cTime_dVar time_dVar;
    cTime_lngVar time_lngVar;
    cTimeSpanVar time_spanVar;
    cLength_Var length_Var;
    cDateVar dateVar;
    cDateShortVar dateShortVar;
    cChannelNrVar chnrVar;
    cChannelShortVar chShortVar;
    cChannelLongVar chLongVar;
    cChannelDataVar chDataVar;
    cSearchQueryVar searchQueryVar;

    cColonVar colonVar;
    cDateNowVar dateNowVar;
    cDateShortNowVar dateShortNowVar;
    cTimeNowVar timeNowVar;
    cVideodirVar videodirVar;
    cPlugconfdirVar plugconfdirVar;
    cEpgsearchconfdirVar epgsearchconfdirVar;

    cTimerDateVar timerDateVar;
    cTimerStartVar timerStartVar;
    cTimerStopVar timerStopVar;
    cTimerFileVar timerFileVar;
    cTimerChnrVar timerChnrVar;
    cTimerChannelShortVar timerChShortVar;
    cTimerChannelLongVar timerChLongVar;
    cTimerSearchVar timerSearchVar; 
    cTimerSearchIDVar timerSearchIDVar; 

    map<string, cExtEPGVar*> extEPGVars;
    set<cUserVar*> userVars;
    map<string, cInternalVar*> internalVars;
    map<string, cTimerVar*> internalTimerVars;
    map<string, cSearchVar*> internalSearchVars;

    void InitInternalVars()
	{
	    internalVars[titleVar.Name()] = &titleVar;
	    internalVars[subtitleVar.Name()] = &subtitleVar;
	    internalVars[summaryVar.Name()] = &summaryVar;
	    internalVars[htmlsummaryVar.Name()] = &htmlsummaryVar;
	    internalVars[eventIDVar.Name()] = &eventIDVar;
	    internalVars[timeVar.Name()] = &timeVar;
	    internalVars[timeEndVar.Name()] = &timeEndVar;
	    internalVars[time_wVar.Name()] = &time_wVar;
	    internalVars[time_dVar.Name()] =  &time_dVar;
	    internalVars[time_lngVar.Name()] = &time_lngVar;
	    internalVars[time_spanVar.Name()] = &time_spanVar;
	    internalVars[length_Var.Name()] = &length_Var;
	    internalVars[dateVar.Name()] = &dateVar;
	    internalVars[dateShortVar.Name()] = &dateShortVar;
	    internalVars[chnrVar.Name()] = &chnrVar;
	    internalVars[chShortVar.Name()] = &chShortVar;
	    internalVars[chLongVar.Name()] = &chLongVar;
	    internalVars[chDataVar.Name()] = &chDataVar;

	    internalVars[colonVar.Name()] = &colonVar;
	    internalVars[dateNowVar.Name()] = &dateNowVar;
	    internalVars[dateShortNowVar.Name()] = &dateShortNowVar;
	    internalVars[timeNowVar.Name()] = &timeNowVar;
	    internalVars[videodirVar.Name()] = &videodirVar;
	    internalVars[plugconfdirVar.Name()] = &plugconfdirVar;
	    internalVars[epgsearchconfdirVar.Name()] = &epgsearchconfdirVar;

	    internalTimerVars[timerDateVar.Name()] = &timerDateVar;	    
	    internalTimerVars[timerStartVar.Name()] = &timerStartVar;
	    internalTimerVars[timerStopVar.Name()] = &timerStopVar;
	    internalTimerVars[timerFileVar.Name()] = &timerFileVar;
	    internalTimerVars[timerChnrVar.Name()] = &timerChnrVar;
	    internalTimerVars[timerChShortVar.Name()] = &timerChShortVar;
	    internalTimerVars[timerChLongVar.Name()] = &timerChLongVar;
	    internalTimerVars[timerSearchVar.Name()] = &timerSearchVar;
	    internalTimerVars[timerSearchIDVar.Name()] = &timerSearchIDVar;

	    internalSearchVars[searchQueryVar.Name()] = &searchQueryVar;
	}

    void InitExtEPGVars()
	{
	    cSearchExtCat* SearchExtCat = SearchExtCats.First();
	    while (SearchExtCat) 
	    {
		string varName = string("%") + SearchExtCat->name + string("%");
		std::transform(varName.begin(), varName.end(), varName.begin(), tolower);
		cExtEPGVar* extEPGVar = new cExtEPGVar(varName);
		extEPGVars[varName] =  extEPGVar;
		SearchExtCat = SearchExtCats.Next(SearchExtCat);
	    }
	}
    void ResetCache()
	{
	    cUserVar* var = First();
	    while (var) 
	    {
		var->ResetCache();
		var = Next(var);
	    }
	}
    ~cUserVars()
	{
	    std::map<string, cExtEPGVar*>::iterator evar;
	    for (evar = extEPGVars.begin(); evar != extEPGVars.end(); evar++)
		delete evar->second;
	    extEPGVars.clear();

	    std::set<cUserVar*>::iterator uvar;
	    for (uvar = userVars.begin(); uvar != userVars.end(); uvar++)
		delete (*uvar);
	    userVars.clear();
	}
    cUserVar* GetFromName(const string& varName, bool log = true);
};

extern cUserVars UserVars;

class cUserVarLine : public cListObject {
public:
  static bool Parse(char *s);
};


class cUserVarFile : public cConfig<cUserVarLine> 
{
public:
    cUserVarFile() { UserVars.Clear(); };
};

class cVarExpr 
{
    string expr;
 public:
    set<cUserVar*> usedVars;
    cVarExpr(const string& Expr) : expr(Expr) {} 
    string Evaluate(const cEvent* e = NULL);
    string Evaluate(const cTimer* t);
    string Evaluate(const cSearchExt* s);
    bool DependsOnVar(const string& varName, const cEvent* e);
};

#endif

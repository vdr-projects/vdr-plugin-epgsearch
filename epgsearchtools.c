/*
Copyright (C) 2004-2007 Christian Wieninger

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

#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <ctype.h>

#include "uservars.h"
#include "epgsearchtools.h"
#include "epgsearchext.h"
#include "epgsearchcats.h"
#include "epgsearchcfg.h"
#include "svdrpclient.h"
#include "distance.h"
#include "md5.h"
#include "afuzzy.h"
#include "i18n.h"
#include "timerstatus.h"

#ifdef HAVE_PCREPOSIX
#include <pcreposix.h>
#else
#include <regex.h>
#endif

const char AllowedChars[] = trNOOP("$ abcdefghijklmnopqrstuvwxyz0123456789-.,#~\\^$[]|()*+?{}/:%@&");

#define MATCHLIMIT 0.9

int CompareEventTime(const void *p1, const void *p2)
{
   time_t time1 = (*(cSearchResult **)p1)->event->StartTime();
   time_t time2 = (*(cSearchResult **)p2)->event->StartTime();
   if (time1 == time2)
      return (int)(ChannelNrFromEvent((*(cSearchResult **)p1)->event) - 
                   ChannelNrFromEvent((*(cSearchResult **)p2)->event));
   else
      return (int)(time1 - time2);
}

int CompareEventChannel(const void *p1, const void *p2)
{
   int ch1 = ChannelNrFromEvent((*(cSearchResult **)p1)->event);
   int ch2 = ChannelNrFromEvent((*(cSearchResult **)p2)->event);
   if (ch1 == ch2)
      return (int)((*(cSearchResult **)p1)->event->StartTime() -
                   (*(cSearchResult **)p2)->event->StartTime());
   else
      return ch1 - ch2;
}

char* IndentMenuItem(const char* szString)
{
   char* szIndented = NULL;
   msprintf(&szIndented, "  %s", szString);
   return szIndented;
}

// based on Nordlicht's EPG-Plugin
void PatchFont(eDvbFont DvbFont)
{
#if VDRVERSNUM < 10503
   const cFont *font = cFont::GetFont(DvbFont);
   int num_rows_global = font->Height();
   int num_rows = num_rows_global+2;
   int i,j,num_char;
 
   cFont::tPixelData* font_data = new cFont::tPixelData[225*(num_rows)];
 
   for (i = 0; i < 225; i++)
      for (j = 0; j < num_rows; j++)
         font_data[(i*num_rows)+j]=0x0000000000000000;
   
   font_data[0+0]=6;//font->CharData(0)->width;
   font_data[0+1]=num_rows_global;
 
   for(num_char=33, i = 1; num_char<256; i++, num_char++)
   {
      const cFont::tCharData* char_data = font->CharData(num_char);
      font_data[i*num_rows+0] = (num_char>=127 && num_char<=129)?6:char_data->width;
      font_data[i*num_rows+1] = char_data->height;

      if(num_char==130)
      {
         if(DvbFont==fontOsd)
         {
            font_data[i*num_rows+ 0]=6;
            font_data[i*num_rows+ 1]=23;
            font_data[i*num_rows+ 8]=0x3f; // ## ####
            font_data[i*num_rows+ 9]=0x3f; // ## ####
            font_data[i*num_rows+10]=0x30; // ## ....
            font_data[i*num_rows+11]=0x30; // ## ....
            font_data[i*num_rows+12]=0x30; // ## ....
            font_data[i*num_rows+13]=0x30; // ## ....
            font_data[i*num_rows+14]=0x30; // ## ....
            font_data[i*num_rows+15]=0x30; // ## ....
            font_data[i*num_rows+16]=0x30; // ## ....
            font_data[i*num_rows+17]=0x30; // ## ....
            font_data[i*num_rows+18]=0x30; // ## ....
            font_data[i*num_rows+19]=0x30; // ## ....
            font_data[i*num_rows+20]=0x30; // ## ....
            font_data[i*num_rows+21]=0x30; // ## ....
            font_data[i*num_rows+22]=0x30; // ## ....
            font_data[i*num_rows+23]=0x3f; // ## ####
            font_data[i*num_rows+24]=0x3f; // ## ####
         }
         else
         {
            font_data[i*num_rows+ 0]=5;
            font_data[i*num_rows+ 1]=19;
            font_data[i*num_rows+ 6]=0x1f; // # ####
            font_data[i*num_rows+ 7]=0x1f; // # ####
            font_data[i*num_rows+ 8]=0x18; // # #...
            font_data[i*num_rows+ 9]=0x18; // # #...
            font_data[i*num_rows+10]=0x18; // # #...
            font_data[i*num_rows+11]=0x18; // # #...
            font_data[i*num_rows+12]=0x18; // # #...
            font_data[i*num_rows+13]=0x18; // # #...
            font_data[i*num_rows+14]=0x18; // # #...
            font_data[i*num_rows+15]=0x18; // # #...
            font_data[i*num_rows+16]=0x18; // # #...
            font_data[i*num_rows+17]=0x18; // # #...
            font_data[i*num_rows+18]=0x18; // # #...
            font_data[i*num_rows+19]=0x1f; // # #...
            font_data[i*num_rows+20]=0x1f; // # ####
         }
      }
      else if(num_char==131)
      {
         if(DvbFont==fontOsd)
         {
            font_data[i*num_rows+ 0]=6;
            font_data[i*num_rows+ 1]=23;
            font_data[i*num_rows+ 8]=0x3f; // ## ####
            font_data[i*num_rows+ 9]=0x3f; // ## ####
            font_data[i*num_rows+10]=0x0;  // .. ....
            font_data[i*num_rows+11]=0x0;  // .. ....
            font_data[i*num_rows+12]=0x0;  // .. ....
            font_data[i*num_rows+13]=0x0;  // .. ....
            font_data[i*num_rows+14]=0x0;  // .. ....
            font_data[i*num_rows+15]=0x0;  // .. ....
            font_data[i*num_rows+16]=0x0;  // .. ....
            font_data[i*num_rows+17]=0x0;  // .. ....
            font_data[i*num_rows+18]=0x0;  // .. ....
            font_data[i*num_rows+19]=0x0;  // .. ....
            font_data[i*num_rows+20]=0x0;  // .. ....
            font_data[i*num_rows+21]=0x0;  // .. ....
            font_data[i*num_rows+22]=0x0;  // .. ....
            font_data[i*num_rows+23]=0x3f; // ## ####
            font_data[i*num_rows+24]=0x3f; // ## ####
         }
         else
         {
            font_data[i*num_rows+ 0]=5;
            font_data[i*num_rows+ 1]=19;
            font_data[i*num_rows+ 6]=0x1f; // # ####
            font_data[i*num_rows+ 7]=0x1f; // # ####
            font_data[i*num_rows+ 8]=0x0;  // . ....
            font_data[i*num_rows+ 9]=0x0;  // . ....
            font_data[i*num_rows+10]=0x0;  // . ....
            font_data[i*num_rows+11]=0x0;  // . ....
            font_data[i*num_rows+12]=0x0;  // . ....
            font_data[i*num_rows+13]=0x0;  // . ....
            font_data[i*num_rows+14]=0x0;  // . ....
            font_data[i*num_rows+15]=0x0;  // . ....
            font_data[i*num_rows+16]=0x0;  // . ....
            font_data[i*num_rows+17]=0x0;  // . ....
            font_data[i*num_rows+18]=0x0;  // . ....
            font_data[i*num_rows+19]=0x1f; // # ####
            font_data[i*num_rows+20]=0x1f; // # ####
         }
      }
      else if(num_char==132)
      {
         if(DvbFont==fontOsd)
         {
            font_data[i*num_rows+ 0]=6;
            font_data[i*num_rows+ 1]=23;
            font_data[i*num_rows+ 8]=0x3f; // ## ####
            font_data[i*num_rows+ 9]=0x3f; // ## ####
            font_data[i*num_rows+10]=0x03; // .. ..##
            font_data[i*num_rows+11]=0x03; // .. ..##
            font_data[i*num_rows+12]=0x03; // .. ..##
            font_data[i*num_rows+13]=0x03; // .. ..##
            font_data[i*num_rows+14]=0x03; // .. ..##
            font_data[i*num_rows+15]=0x03; // .. ..##
            font_data[i*num_rows+16]=0x03; // .. ..##
            font_data[i*num_rows+17]=0x03; // .. ..##
            font_data[i*num_rows+18]=0x03; // .. ..##
            font_data[i*num_rows+19]=0x03; // .. ..##
            font_data[i*num_rows+20]=0x03; // .. ..##
            font_data[i*num_rows+21]=0x03; // .. ..##
            font_data[i*num_rows+22]=0x03; // .. ..##
            font_data[i*num_rows+23]=0x3f; // ## ####
            font_data[i*num_rows+24]=0x3f; // ## ####
         }
         else
         {
            font_data[i*num_rows+ 0]=5;
            font_data[i*num_rows+ 1]=19;
            font_data[i*num_rows+ 6]=0x1f; // # ####
            font_data[i*num_rows+ 7]=0x1f; // # ####
            font_data[i*num_rows+ 8]=0x03; // . ..##
            font_data[i*num_rows+ 9]=0x03; // . ..##
            font_data[i*num_rows+10]=0x03; // . ..##
            font_data[i*num_rows+11]=0x03; // . ..##
            font_data[i*num_rows+12]=0x03; // . ..##
            font_data[i*num_rows+13]=0x03; // . ..##
            font_data[i*num_rows+14]=0x03; // . ..##
            font_data[i*num_rows+15]=0x03; // . ..##
            font_data[i*num_rows+16]=0x03; // . ..##
            font_data[i*num_rows+17]=0x03; // . ..##
            font_data[i*num_rows+18]=0x03; // . ..##
            font_data[i*num_rows+19]=0x1f; // # ####
            font_data[i*num_rows+20]=0x1f; // # ####
         }
      }
      else if(num_char==127)
      {
         if(DvbFont==fontOsd)
         {
            font_data[i*num_rows+ 0]=6;
            font_data[i*num_rows+ 1]=23;
            font_data[i*num_rows+ 8]=0x3f; // ## ####
            font_data[i*num_rows+ 9]=0x3f; // ## ####
            font_data[i*num_rows+10]=0x3f; // ## ####
            font_data[i*num_rows+11]=0x3f; // ## ####
            font_data[i*num_rows+12]=0x3f; // ## ####
            font_data[i*num_rows+13]=0x3f; // ## ####
            font_data[i*num_rows+14]=0x3f; // ## ####
            font_data[i*num_rows+15]=0x3f; // ## ####
            font_data[i*num_rows+16]=0x3f; // ## ####
            font_data[i*num_rows+17]=0x3f; // ## ####
            font_data[i*num_rows+18]=0x3f; // ## ####
            font_data[i*num_rows+19]=0x3f; // ## ####
            font_data[i*num_rows+20]=0x3f; // ## ####
            font_data[i*num_rows+21]=0x3f; // ## ####
            font_data[i*num_rows+22]=0x3f; // ## ####
            font_data[i*num_rows+23]=0x3f; // ## ####
            font_data[i*num_rows+24]=0x3f; // ## ####
         }
         else
         {
            font_data[i*num_rows+ 0]=5;
            font_data[i*num_rows+ 1]=19;
            font_data[i*num_rows+ 6]=0x1f; // ######
            font_data[i*num_rows+ 7]=0x1f; // # ####
            font_data[i*num_rows+ 8]=0x1f; // # ####
            font_data[i*num_rows+ 9]=0x1f; // # ####
            font_data[i*num_rows+10]=0x1f; // # ####
            font_data[i*num_rows+11]=0x1f; // # ####
            font_data[i*num_rows+12]=0x1f; // # ####
            font_data[i*num_rows+13]=0x1f; // # ####
            font_data[i*num_rows+14]=0x1f; // # ####
            font_data[i*num_rows+15]=0x1f; // # ####
            font_data[i*num_rows+16]=0x1f; // # ####
            font_data[i*num_rows+17]=0x1f; // # ####
            font_data[i*num_rows+18]=0x1f; // # ####
            font_data[i*num_rows+19]=0x1f; // # ####
            font_data[i*num_rows+20]=0x1f; // # ####
         }
      }
      else if (char_data->width > 0)
      {
         for(j = 0; j<(int)char_data->height; j++)
         {
            cFont::tPixelData Data = (cFont::tPixelData)char_data->lines[j];    
            font_data[(i*num_rows)+2+j] = Data;
         }
      }
   }
 
   cFont::SetFont(DvbFont, font_data);
#endif
}


bool MatchesSearchMode(const char* szTest, const char* searchText, int mode, const char* delim, int tolerance)
{
   if (szTest && *szTest)
   {
      if (mode == 0) // substring
         return (strstr(szTest, searchText) != NULL); 
      else if (mode == 1 || mode == 2) // AND or OR
      { 
         bool bTesting = false;
         char *pstrSearchToken, *pptr;
         bool bFirst=true;
         char *pstrSearch=strdup(searchText);
         pstrSearchToken=strtok_r(pstrSearch, delim, &pptr);
         while(pstrSearchToken) 
         {
            if(szTest && strstr(szTest, skipspace(pstrSearchToken))) 
            { 
               if(mode==1) 
               { // means AND
                  if(bFirst) 
                  {
                     bTesting=true;
                     bFirst=false;
                  } 
                  else
                     bTesting&=true;
               }
               else
                  bTesting|=true;
            } 
            else 
            {// not found!!
               if(mode==1) 
               { // means AND
                  bTesting=false;
                  bFirst=false;
               }
            }
            pstrSearchToken=strtok_r(NULL, delim, &pptr);
         }
         free(pstrSearch);
         return bTesting;
      }
      else if (mode == 3) // match exactly
      {
         if (strcmp(szTest, searchText) == 0)
            return true;
         else
            return false;
      } 
      else if (mode == 4) // regexp
      {
         regex_t re;

         if ( 0 == regcomp(&re, searchText, REG_EXTENDED) )
         {
            int status = regexec( &re, szTest, 0, NULL, 0);
            regfree(&re);
            return (status == 0);
         }
         return false;
      }	
      else if (mode == 5) // fuzzy
      {
         AFUZZY af = { NULL, NULL, NULL, NULL, NULL, NULL };
         string query = searchText?searchText:"";
         if (query.size() > 32) query = query.substr(0, 32);
         afuzzy_init(query.c_str(), tolerance, 0, &af);
         /* Checking substring */
         int res = afuzzy_checkSUB(szTest, &af);
         afuzzy_free(&af);
         return (res > 0);
      }
      else if (mode >= 10 && mode <= 15) 
      {
         int testvalue = atoi(szTest);
         int value = atoi(searchText);
	 if (value == 0) return true;

         if (mode == 10) // less 
            return testvalue < value;
         else if (mode == 11) // less or equal
            return testvalue <= value;
         else if (mode == 12) // greater
            return testvalue > value;
         else if (mode == 13) // greater or equal
            return testvalue >= value;
         else if (mode == 14) // equal
            return testvalue == value;
         else if (mode == 15) // not equal
            return testvalue != value;
      }
   }
   return false;
}

void ToLower(char* szText)
{
   if (!szText)
      return;
   for (int loop = 0; szText[loop] !=0; loop++)
     szText[loop] = tolower(szText[loop]);
}

char* GetExtEPGValue(const cEvent* e, cSearchExtCat* SearchExtCat)
{
   if (!e || !SearchExtCat)
      return NULL;
   return GetExtEPGValue(e->Description(), SearchExtCat->name);
}

char* GetExtEPGValue(const char* description, const char* catname)
{
   if (isempty(description))
      return NULL;
   char* tmp = NULL;
    
   // search the category, must be at beginnig of a line
   msprintf(&tmp, "\n%s: ", catname);
   char* descr = strdup(description);
   char* cat = NULL;
   if ((cat = strstr(descr, tmp)) == NULL)
   {
      free(descr);
      free(tmp);
      return NULL;
   }
   else
      cat++; // skip linefeed

   // search the value to appear before the next line feed or end 
   char* end = strchr(cat, '\n');
   int endpos = strlen(cat);
   if (end)
      endpos = end - cat;
   cat[endpos] = 0;

   char* value = NULL;
   msprintf(&value, "%s", cat + strlen(tmp)-1);
   free(descr);
   free(tmp);

   return value;
}

char* GetAuxValue(const char* aux, const char* name)
{
   if (isempty(aux))
      return NULL;
    
   char* descr = strdup(aux);
   char* beginaux = strstr(descr, "<epgsearch>");
   char* endaux = strstr(descr, "</epgsearch>");
   if (!beginaux || !endaux) return NULL;
   strn0cpy(descr, beginaux + strlen("<epgsearch>"), endaux - (beginaux + strlen("<epgsearch>")) + 1);
   if (strcmp(name, "epgsearch") == 0) return descr; // full aux

   char catname[100] = "";
   sprintf(catname, "<%s>", name);

   char* cat = NULL;
   int iPosCat = FindIgnoreCase(descr, catname);
   if (iPosCat < 0) return NULL;
   cat = descr + iPosCat;

   char* end = strstr(cat, "</");
   int endpos = strlen(cat);
   if (end)
      endpos = end - cat;
   else
   {
      free(descr);	
      return NULL;
   }
   cat[endpos] = 0;
	
   char* value = NULL;
   msprintf(&value, "%s", cat + strlen(name)+2);
   free(descr);
   
   return value;
}

char* GetAuxValue(const cRecording *recording, const char* name)
{
   if (!recording || !recording->Info()) return NULL;
   return GetAuxValue(recording->Info()->Aux(), name);
}

char* GetAuxValue(const cTimer *timer, const char* name)
{
   if (!timer || !timer->Aux()) return NULL;
   return GetAuxValue(timer->Aux(), name);
}

string UpdateAuxValue(string aux, string section, long num)
{
   return UpdateAuxValue(aux, section, NumToString(num));
}

string UpdateAuxValue(string aux, string section, string value)
{
   string secStart = "<" + section + ">";
   string secEnd = "</" + section + ">";
   int valueStartPos = aux.find(secStart);
   int valueEndPos = aux.find(secEnd);
   if (valueStartPos >= 0 && valueEndPos >= 0)
      aux.replace(valueStartPos + secStart.size(), valueEndPos - valueStartPos - secStart.size(), value);
   else
      aux += secStart + value + secEnd;
   return aux;
}

// replace s1 with s2 in s ignoring the case of s1
char *strreplacei(char *s, const char *s1, const char *s2)
{
   char *p = strcasestr(s, s1);
   if (p) {
      int of = p - s;
      int l  = 0;
      if (s)
        l = strlen(s);
      int l1 = 0;
      if (s1)
        l1 = strlen(s1);
      int l2 = 0;
      if (s2)
        l2 = strlen(s2);
      if (l2 > l1)
         s = (char *)realloc(s, strlen(s) + l2 - l1 + 1);
      if (l2 != l1)
         memmove(s + of + l2, s + of + l1, l - of - l1 + 1);
      memcpy(s + of, s2, l2);
   }
   return s;
}

unsigned int strcharcount(char* source, char ch)
{
   unsigned int count;
    
   for (count = 0; *source; source++)
      if (*source == ch)
         count++;
   return (count);
}

char *strreplaceall(char* source, char ch, const char* replacement)
{
   int		replen;
   int		srclen;
   unsigned int	chnum;
   register char	*sptr, *nptr;
   char		*newval;
    
   chnum = strcharcount(source, ch);
    
   if (chnum == 0)
      return (NULL);
  
   srclen = strlen(source);
   replen = strlen(replacement);
   newval = (char*) malloc(srclen + (chnum * replen) + 1);
   nptr = newval;
   for (sptr = source; chnum > 0; sptr++)
      if (*sptr == ch) {
         memcpy(nptr, replacement, replen);
         nptr += replen;
         chnum--;        
      }
      else
         *nptr++ = *sptr;
   memcpy(nptr, sptr, srclen - (sptr - source) + 1);
   return (newval);
}

void sleepMSec(long ms)
{
   cCondWait::SleepMs(ms);
}

void sleepSec(long s)
{
   sleepMSec(s * 1000);
}

bool SendViaSVDRP(cString SVDRPcmd) 
{
   bool bSuccess = true;
   cString cmdbuf;
   if (EPGSearchConfig.useExternalSVDRP)
   {
     cmdbuf = cString::sprintf("%s -p %d \"%s\"", 
               cSVDRPClient::SVDRPSendCmd,
               EPGSearchConfig.SVDRPPort,
               *SVDRPcmd);

      FILE *p = popen(cmdbuf, "r");
      if (p)
         pclose(p);
      else
      {
         LogFile.eSysLog("can't open pipe for command '%s'", *cmdbuf);
         bSuccess = false;
      }
   }
   else
   {
      cmdbuf = SVDRPcmd;
      cSVDRPClient client(EPGSearchConfig.SVDRPPort);
      if (!client.SendCmd(*cmdbuf))
      {
         LogFile.eSysLog("command '%s' failed", *cmdbuf);
         bSuccess = false;
      }
   }
	    
   return bSuccess;
}

int SendMsg(cString Message, bool confirm, int seconds) 
{
   int Keys = Skins.QueueMessage(mtInfo, Message, seconds, confirm?seconds+2:0);
   return Keys;
}


bool InEditMode(const char* ItemText, const char* ItemName, const char* ItemValue)
{
   bool bEditMode = true;
   // ugly solution to detect, if in edit mode
   char* value = strdup(ItemText);
   strreplace(value, ItemName, "");
   strreplace(value, ":\t", "");
   // for bigpatch
   strreplace(value, "\t", "");
   if (strlen(value) == strlen(ItemValue))
      bEditMode = false;
   free(value);
   return bEditMode;
}

// checks if the timer was triggered from a search timer and return a pointer to the search
cSearchExt* TriggeredFromSearchTimer(const cTimer* timer)
{
   char* searchID = GetAuxValue(timer, "s-id");
   if (!searchID)
      return NULL;
    
   cSearchExt* search = SearchExts.GetSearchFromID(atoi(searchID));
   free(searchID);
   return search;
}

int TriggeredFromSearchTimerID(const cTimer* timer)
{
   cSearchExt* trigger = TriggeredFromSearchTimer(timer);
   if (trigger) 
      return trigger->ID;
   else
      return -1;
}

double FuzzyMatch(const char* s1, const char* s2, int maxLength)
{
   Distance D;
   int dist = D.LD (s1, s2, maxLength);
   double fMaxLength = max(strlen(s1), strlen(s2));
   return (fMaxLength - dist)/fMaxLength;
}

bool DescriptionMatches(const char* eDescr, const char* rDescr)
{
   if (eDescr == NULL && rDescr == NULL) return true;
   if (eDescr == NULL && rDescr != NULL) return false;
   if (eDescr != NULL && rDescr == NULL) return false;
   int l_eDescr = strlen(eDescr);
   int l_rDescr = strlen(rDescr);
   if (l_eDescr == l_rDescr && strcmp(eDescr, rDescr) == 0) return true;

   // partial match:
   // first check the length, should only be different at MATCHLIMIT
   int minLength = min(l_eDescr, l_rDescr);
   int maxLength = max(l_eDescr, l_rDescr);
   if (double(minLength)/double(maxLength) < MATCHLIMIT)
      return false;

   // last try with Levenshtein Distance, only compare the first 1000 chars 
   double fMatch = FuzzyMatch(eDescr, rDescr, 1000);
   double tmp_matchlimit = MATCHLIMIT;
   if(maxLength - minLength < 5)
   {
      tmp_matchlimit = 0.95;
      LogFile.Log(2,"difference between both descriptions is < 5 setting matchlimit to: %.2f %%", tmp_matchlimit*100);
   }
   if (fMatch > tmp_matchlimit)
   {
      LogFile.Log(2,"match is: %.2f %%", fMatch*100);
      return true;
   }
   return false;
}

const cEvent* GetEvent(cTimer* timer)
{
   const cEvent* event = NULL;
   const cChannel *channel = timer->Channel();
   time_t Time = timer->StartTime() + (timer->StopTime() - timer->StartTime()) / 2;
   for (int seconds = 0; seconds <= 3; seconds++) 
   {
      {
         cSchedulesLock SchedulesLock;
         const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
         if (Schedules) {
            const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
            if (Schedule) {
               event = Schedule->GetEventAround(Time);
               if (event) return event; 
            }
         }
      }
      if (seconds == 0)
         LogFile.Log(2,"waiting for EPG info...");
      sleepSec(1);
   }
   LogFile.Log(1,"no EPG info available");
   return NULL;
}

// this extracts the real description from a given epg entry cutting all that looks like a category line
// we asume, that a category has a name not longer than MAXCATNAMELENGTH and a value not longer than 
// MAXCATVALUELENGTH (so in most cases e.g. the 'cast' category will stay part of the description). name and 
// value are separated with ': '
#define MAXCATNAMELENGTH 40
#define MAXCATVALUELENGTH 60

char* GetRawDescription(const char* descr)
{
   if (!descr || !*descr) return NULL;

   char* rawDescr = (char*) calloc(strlen(descr)+1, sizeof(char));
   
   const char* tmp = descr;    
   while(tmp)
   {
      // extract a single line
      char* lf = strchr(tmp, '\n');
      char* line = NULL;
      if (lf)
	line = strndup(tmp, lf-tmp);
      else
	line = strdup(tmp);
      //      if (lf) *lf = 0;
      // check for category
      char* delim = strstr(line, ": ");
      if (!delim || 
          (delim && (delim - line > MAXCATNAMELENGTH || strlen(line) - (delim - line) + 2 > MAXCATVALUELENGTH)))
         if (*line)
	   strcat(rawDescr, line);
	
      if (lf) tmp += strlen(line)+1; else tmp = NULL;
      free(line);
   }
   return rawDescr;
}

void PrepareTimerFile(const cEvent* event, cTimer* timer)
{
   if (!event) return;
   if (EPGSearchConfig.addSubtitleToTimer == addSubtitleNever && strlen(EPGSearchConfig.defrecdir) == 0) // nothing to do
      return;
   if (!isempty(event->ShortText())) // add subtitle if present
   {
      bool addSubtitle = (EPGSearchConfig.addSubtitleToTimer != addSubtitleNever);
      if (EPGSearchConfig.addSubtitleToTimer == addSubtitleSmart)
         if (event->Duration() > 80*60)
            addSubtitle = false;

      if (addSubtitle)
      {
         char tmp[MaxFileName] = "";
         sprintf(tmp, "%s~%s", event->Title(), event->ShortText());
         timer->SetFile(tmp);
      }
   }
   if (strlen(EPGSearchConfig.defrecdir) > 0)
   {
      char directory[MaxFileName] = "";
      strn0cpy(directory, EPGSearchConfig.defrecdir,sizeof(directory));
      cVarExpr varExprDir(directory);
      if (!varExprDir.DependsOnVar("%title%", event))
      {	    
         strcat(directory, "~");
         strcat(directory, timer->File());
      }
      // parse the epxression and evaluate it
      cVarExpr varExpr(directory);
      strcpy(directory, varExpr.Evaluate(event).c_str());
      if (strchr(directory, '%') == NULL) // only set directory to new value if all categories could have been replaced
         timer->SetFile(directory);
   }
}

bool EventsMatch(const cEvent* event1, const cEvent* event2, bool compareTitle, bool compareSubtitle, bool compareSummary, unsigned long catvaluesAvoidRepeat)
{
   if (!event1 || !event2) return false;
   if (event1 == event2) return true;

   // only compare the alphanumeric portions
   string Title1 = "";
   string Title2 = "";
   if (compareTitle)
   {
      string s1 = event1->Title()?event1->Title():"";
      string s2 = event2->Title()?event2->Title():"";
      Title1 = GetAlNum(s1);
      Title2 = GetAlNum(s2);
      std::transform(Title1.begin(), Title1.end(), Title1.begin(), tolower);
      std::transform(Title2.begin(), Title2.end(), Title2.begin(), tolower);
   }
   string Subtitle1 = "";
   string Subtitle2 = "";
   if (compareSubtitle)
   {
      string s1 = event1->ShortText()?event1->ShortText():"";
      string s2 = event2->ShortText()?event2->ShortText():"";
      Subtitle1 = GetAlNum(s1);
      Subtitle2 = GetAlNum(s2);
      std::transform(Subtitle1.begin(), Subtitle1.end(), Subtitle1.begin(), tolower);
      std::transform(Subtitle2.begin(), Subtitle2.end(), Subtitle2.begin(), tolower);
   }

   bool match = false;
   if ((!compareTitle || Title1 == Title2) &&
       (!compareSubtitle || Subtitle1 == Subtitle2))
   {	
      const char* Descr1    = event1->Description();
      const char* Descr2    = event2->Description();
      if (compareSummary)
      {
         char* rawDescr1 = GetRawDescription(Descr1);
         char* rawDescr2 = GetRawDescription(Descr2);
         match = DescriptionMatches(rawDescr1, rawDescr2);
         free(rawDescr1);
         free(rawDescr2);
         if (!match) return false;
      }
      if (catvaluesAvoidRepeat != 0) // check categories
      {
         bool bCatMatch = ((Descr1 && Descr2) || (!Descr1 && !Descr2));
         cSearchExtCat *SearchExtCat = SearchExtCats.First();
         int index = 0;
         while (catvaluesAvoidRepeat > 0 && SearchExtCat && bCatMatch) 	    
         {
            if (catvaluesAvoidRepeat & (1<<index))
            {
               char* CatValue1 = GetExtEPGValue(Descr1, SearchExtCat->name);
               char* CatValue2 = GetExtEPGValue(Descr2, SearchExtCat->name);
               if ((!CatValue1 && CatValue2) ||
                   (!CatValue2 && CatValue1) ||
                   (CatValue1 && CatValue2 && strcmp(CatValue1, CatValue2) != 0))
                  bCatMatch = false;
               free(CatValue1);
               free(CatValue2);
            }
            SearchExtCat = SearchExtCats.Next(SearchExtCat);
            index++;
         }		    
         if (bCatMatch)
            match = true;
      }
      else
         match = true;
   }
   return match;
}

int ChannelNrFromEvent(const cEvent* pEvent)
{
   if (!pEvent)
      return -1;
   cChannel* channel = Channels.GetByChannelID(pEvent->ChannelID(), true, true);
   if (!channel)
      return -1;
   else
      return channel->Number();						
}

void DelTimer(int index)
{
  cString cmdbuf = cString::sprintf("DELT %d", index);    
  LogFile.Log(2, "delete timer %d", index);
  SendViaSVDRP(cmdbuf);
  gl_timerStatusMonitor->SetConflictCheckAdvised(); 
}

char* FixSeparators(char* buffer, char sep)
{
   char tmp[5];
   sprintf(tmp, "%c\t", sep);
   buffer = strreplacei(buffer, tmp, "\t");
   sprintf(tmp, "%c \t", sep);
   buffer = strreplacei(buffer, tmp, "\t");
   sprintf(tmp, "%c ", sep);
   if (strstr(buffer, tmp) == buffer + strlen(buffer) - 2) buffer[strlen(buffer) - 2] = 0;
   if (buffer[strlen(buffer)-1] == sep) buffer[strlen(buffer) - 1] = 0;

   sprintf(tmp, "\t%c", sep);
   buffer = strreplacei(buffer, tmp, "\t");
   sprintf(tmp, "\t %c", sep);
   buffer = strreplacei(buffer, tmp, "\t");
   sprintf(tmp, "\t  %c", sep);
   buffer = strreplacei(buffer, tmp, "\t");
   return buffer;
}

cString DateTime(time_t t)
{
   char buffer[32];
   if (t == 0)
      time(&t);
   struct tm tm_r;
   tm *tm = localtime_r(&t, &tm_r);
   snprintf(buffer, sizeof(buffer), "%02d.%02d %02d:%02d", tm->tm_mday, tm->tm_mon + 1, tm->tm_hour, tm->tm_min);
   return buffer;
}

string NumToString(long num)
{
   ostringstream os;
   os << num;
   return os.str();
}

int FindIgnoreCase(const string& expr, const string& query)
{
  icstring exprIC(expr.c_str());
  icstring queryIC(query.c_str());
  int pos = exprIC.find(queryIC);
  return pos;
}

bool EqualsNoCase(const string& a, const string& b)
{
   return (FindIgnoreCase(a,b) == 0 && a.size() == b.size());
}

string Strip(const string& input)
{
  string str = input;
  string::size_type pos = str.find_last_not_of(' ');
  if(pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
  return str;
}

string GetAlNum(const string& s)
{
   string res;
   for(unsigned int i=0; i<s.size(); i++)
      if (isalnum(s[i]))
         res += s[i];
   return res;
}

string ReplaceAll(const string& input, const string& what, const string& with)
{
   string result = input;
   int pos = 0;

   while((pos = FindIgnoreCase(result, what)) >= 0)
      result.replace(pos, what.size(), with);
   return result;
}

string EscapeString(const string& S)
{
   string tmp = S;
   int apostrophPos = 0;
   int apostrophTempPos = 0;
   while((apostrophPos = tmp.find("'", apostrophTempPos)) >= apostrophTempPos) {
      tmp.replace(apostrophPos, 1, "'\"'\"'");
      apostrophTempPos = apostrophPos + 5;
   }
   return tmp;
}

string QuoteApostroph(const string& S)
{
   string tmp = S;
   int apostrophPos = 0;
   int apostrophTempPos = 0;
   while((apostrophPos = tmp.find("\"", apostrophTempPos)) >= apostrophTempPos) {
      tmp.replace(apostrophPos, 1, "\\\"");
      apostrophTempPos = apostrophPos + 2;
   }
   return tmp;
}

string MD5(const string& input)
{
   char* szInput = strdup(input.c_str());
   if (!szInput) return "";
   char* szRes = MD5String(szInput);
   string res = szRes;
   free(szRes);
   return res;
}

void SetAux(cTimer* timer, string aux)
{
   if (!timer) return;
   string timerText = *timer->ToText();
   string::size_type auxPos = string::npos;
   for(int i=0; i<=7; i++) // get aux value
      auxPos = timerText.find(":", auxPos == string::npos?0:auxPos+1);
   if (auxPos == string::npos) return;
   timerText.replace(auxPos+1, timerText.size()-auxPos+1, aux);
   timer->Parse(timerText.c_str());
}

int msprintf(char **strp, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  int res=vasprintf (strp, fmt, ap);
  va_end (ap);
  return res;
}

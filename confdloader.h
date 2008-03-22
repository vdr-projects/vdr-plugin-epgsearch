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
#include "log.h"
#include "uservars.h"
#include "menu_dirselect.h"
#include "templatefile.h"
#include "epgsearchcats.h"

class cConfDLoader {  
 public:
  cConfDLoader(const char* FileName) { Load(FileName); }
  bool Load(const char *FileName)
  {
    if (FileName && access(FileName, F_OK) == 0) {
      LogFile.Log(1, "loading %s", FileName);
      FILE *f = fopen(FileName, "r");
      if (f) {
	char *s;
	int line = 0;
	cReadLine ReadLine;
	std::string section;
	while ((s = ReadLine.Read(f)) != NULL) {
	  line++;
	  char *p = strchr(s, '#');
	  if (p)
	    *p = 0;
	  stripspace(s);
	  if (!isempty(s)) {
	    if (*s == '[' && *(s+strlen(s)-1)==']') // Section?
	      section = s;
	    else {
	      if (EqualsNoCase(section, "[epgsearchuservars]"))
		  cUserVarLine::Parse(s);
	      if (EqualsNoCase(section, "[epgsearchdirs]"))
		{
		  cDirExt* D = new cDirExt;
		  if (D && D->Parse(s))
		    ConfDDirExts.Add(D);
		  else
		    delete D;
		}
	      if (EqualsNoCase(section, "[epgsearchmenu]"))
		{
		  cTemplLine T;
		  if (T.Parse(s))
		    cTemplFile::Parse(T.Name(), T.Value());
		}
	      if (EqualsNoCase(section, "[epgsearchcats]"))
		{
		  cSearchExtCat* cat = new cSearchExtCat;
		  if (cat && cat->Parse(s))
		    SearchExtCats.Add(cat);
		  else
		    delete cat;
		}
	    }
	  }
	}
      }
      fclose(f);
      return true;
    }
    else 
      {
	LOG_ERROR_STR(FileName);
	return false;
      }
  }
};

/*
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

#include <string>
#include <list>
#include <vdr/plugin.h>
#if APIVERSNUM < 10507
#include "i18n.h"
#define trNOOP(s) (s)
#endif
#include "services.h"
#include "mainmenushortcut.h"

static const char VERSION[]       = "0.0.1";
static const char DESCRIPTION[]   = trNOOP("Direct access to epgsearch's conflict check menu");
static const char MAINMENUENTRY[] = trNOOP("Timer conflicts");
static const char SETUPTEXT[]     = trNOOP("Conflict info in main menu");

#if APIVERSNUM < 10507
const tI18nPhrase PhrasesLocal[] = {
  {
   "Direct access to epgsearch's conflict check menu",
   "Direkter Zugriff auf epgsearch's Konflikt-Prüfungs-Menü",	// Deutsch
   "",				// TODO 
   "",				// TODO
   "",				// TODO
   "",				// TODO 
   "",				// TODO
   "",				// TODO 
   "Suoratoiminto EPGSearch-laajennoksen ajastimien tarkistukselle",
   "",				// TODO Polski
   "Acceso directo al menú de conflictos EPGSearch",			// Español
   "",				// TODO ÅëëçíéêÜ (Greek)
   "",				// TODO Svenska
   "",				// TODO Romaneste
   "",				// TODO Magyar
   "",				// TODO Català
   "",				// TODO ÀãááÚØÙ (Russian)
   "",				// TODO Hrvatski (Croatian)
   "",				// Eesti
   "",				// Dansk
   "",				// Czech
   },
  {
   "Timer conflicts",
   "Timer-Konflikte",
   "",				// TODO
   "",				// TODO
   "",
   "",				// TODO
   "",
   "",				// TODO
   "Ajastimien päällekkäisyydet",
   "",				// TODO
   "Conflictos de programación",
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// Eesti
   "",				// Dansk
   "",				// Czech
   },
  {
   "next",
   "nächster",
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "seuraava",
   "",				// TODO
   "siguiente",
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// Eesti
   "",				// Dansk
   "",				// Czech
   },
  {
   "Conflict info in main menu",
   "Konflikt-Info im Hauptmenü",
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "Näytä päällekkäisyydet päävalikossa",
   "",				// TODO
   "Mostrar conflictos en menú principal",
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// TODO
   "",				// Eesti
   "",				// Dansk
   "",				// Czech
   },
  {NULL}
};
#endif

cString DateTime(time_t t)
{
  char buffer[32];
  if (t == 0) {
    time(&t);
  }
  struct tm tm_r;
  tm *tm = localtime_r(&t, &tm_r);
  snprintf(buffer, sizeof(buffer), "%02d.%02d %02d:%02d", tm->tm_mday,
	   tm->tm_mon + 1, tm->tm_hour, tm->tm_min);
  return buffer;
}

class cPluginConflictcheckonly:public cMainMenuShortcut {
private:
  char *_menuText;

public:
   cPluginConflictcheckonly();
  ~cPluginConflictcheckonly();
  virtual const char *Version() {
    return VERSION;
  }
#if APIVERSNUM < 10507
  virtual const char *Description() {
    return tr(DESCRIPTION);
  }
#else
  virtual const char *Description() {
    return I18nTranslate(DESCRIPTION, I18nEpgsearch);
  }
#endif
  virtual bool Initialize();
  virtual cOsdObject *MainMenuAction() {
    return GetEpgSearchMenu("Epgsearch-conflictmenu-v1.0");
  }

protected:
#if APIVERSNUM < 10507
  virtual const char *SetupText() {
    return tr(SETUPTEXT);
  }
#else
  virtual const char *SetupText() {
    return I18nTranslate(SETUPTEXT, I18nEpgsearch);
  }
#endif
  virtual const char *MainMenuText(void);
};

cPluginConflictcheckonly::cPluginConflictcheckonly():_menuText(NULL)
{
}

cPluginConflictcheckonly::~cPluginConflictcheckonly()
{
  free(_menuText);
}

const char *cPluginConflictcheckonly::MainMenuText(void)
{
#if APIVERSNUM < 10507
  const char *menuText = tr(MAINMENUENTRY);
#else
  const char *menuText = I18nTranslate(MAINMENUENTRY, I18nEpgsearch);
#endif

  cPlugin *epgSearchPlugin = cPluginManager::GetPlugin("epgsearch");
  if (epgSearchPlugin) {
    Epgsearch_lastconflictinfo_v1_0 *serviceData = new Epgsearch_lastconflictinfo_v1_0;
    if (epgSearchPlugin->Service("Epgsearch-lastconflictinfo-v1.0", serviceData)) {
      if (serviceData->relevantConflicts > 0) {
	free(_menuText);
	if (asprintf(&_menuText, "%s (%d, %s: %s)", menuText, serviceData->relevantConflicts,
#if APIVERSNUM < 10507
		     tr("next"), *DateTime(serviceData->nextConflict)))
#else
	    I18nTranslate("next", I18nEpgsearch), *DateTime(serviceData->nextConflict)))
#endif	
            menuText = _menuText;
      }
    }
    delete serviceData;
  }
  return menuText;
}

bool cPluginConflictcheckonly::Initialize(void)
{
#if APIVERSNUM < 10507
  RegisterI18n(PhrasesLocal);
#endif
  return cMainMenuShortcut::Initialize();
}

VDRPLUGINCREATOR(cPluginConflictcheckonly);	// Don't touch this!

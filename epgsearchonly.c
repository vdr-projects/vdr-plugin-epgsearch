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

#include <string>
#include <list>
#include <vdr/plugin.h>
#if APIVERSNUM < 10507
#include <vdr/i18n.h>
#define trNOOP(s) (s)
#endif
#include "services.h"
#include "mainmenushortcut.h"

static const char *VERSION       = "0.0.1";
static const char *DESCRIPTION   = trNOOP("Direct access to epgsearch's search menu");
static const char *MAINMENUENTRY = trNOOP("Search");
static const char *SETUPTEXT     = trNOOP("EpgSearch-Search in main menu");

#if APIVERSNUM < 10507
const tI18nPhrase PhrasesLocal[] = {
  {
   "Direct access to epgsearch's search menu",
   "Direkter Zugriff auf epgsearch's Suchenmenu",	// Deutsch
   "",				// TODO 
   "",				// TODO
   "",				// TODO
   "",				// TODO 
   "",				// TODO
   "",				// TODO 
   "Suoratoiminto EPGSearch-laajennoksen haulle",
   "",				// TODO Polski
   "Acceso directo al menú de búsqueda EPGSearch",	// Español
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
   "Search",
   "Suche",
   "",				// TODO
   "Ricerca",
   "Zoek",
   "",				// TODO
   "Recherche",
   "",				// TODO
   "Etsi ohjelmaoppaasta",
   "",				// TODO
   "Buscar",
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
   "EpgSearch-Search in main menu",
   "EpgSearch-Suche im Hauptmenü",	// Deutsch
   "",				// TODO 
   "",				// TODO
   "",				// TODO
   "",				// TODO 
   "",				// TODO
   "",				// TODO 
   "EpgSearch-laajennoksen hakutoiminto päävalikossa",
   "",				// TODO Polski
   "Buscar EPG en menú principal",
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
  {NULL}
};
#endif

class cPluginEpgsearchonly:public cMainMenuShortcut {
public:
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
    return GetEpgSearchMenu("Epgsearch-searchmenu-v1.0");
  };

protected:
#if APIVERSNUM < 10507
  virtual const char *SetupText() {
    return tr(SETUPTEXT);
  }
  virtual const char *MainMenuText() {
    return tr(MAINMENUENTRY);
  };
#else
  virtual const char *SetupText() {
    return I18nTranslate(SETUPTEXT, I18nEpgsearch);
  }
  virtual const char *MainMenuText() {
    return I18nTranslate(MAINMENUENTRY, I18nEpgsearch);
  }
#endif
};

bool cPluginEpgsearchonly::Initialize()
{
#if APIVERSNUM < 10507
  RegisterI18n(PhrasesLocal);
#endif
  return cMainMenuShortcut::Initialize();
}

VDRPLUGINCREATOR(cPluginEpgsearchonly);	// Don't touch this!

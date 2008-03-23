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

#include <string>
#include <list>
#include <vdr/plugin.h>
#include "i18n.h"
#include "services.h"
#include "mainmenushortcut.h"

static const char VERSION[]        = "0.0.1";
static const char DESCRIPTION[]    = trNOOP("Quick search for broadcasts");
static const char MAINMENUENTRY[]  = trNOOP("Quick search");
static const char SETUPTEXT[]      = trNOOP("Show in main menu");

#if APIVERSNUM < 10507
const tI18nPhrase PhrasesLocal[] =
{
    {
        "Quick search for broadcasts",
        "Schnelle Suche nach Sendungen",// Deutsch
        "",// TODO 
        "",// TODO
        "",// TODO
        "",// TODO 
        "",// TODO
        "",// TODO 
        "Pikahaku ohjelmaoppaalle",// suomi
        "",// TODO Polski
        "Búsqueda rápida de emisiones",// Español
        "",// TODO ÅëëçíéêÜ (Greek)
        "",// TODO Svenska
        "",// TODO Romaneste
        "",// TODO Magyar
        "",// TODO Català
        "",// TODO ÀãááÚØÙ (Russian)
        "",// TODO Hrvatski (Croatian)
        "", // Eesti
        "", // Dansk
        "", // Czech
    },
    {
        "Quick search",
        "Schnellsuche",
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "Pikahaku",
        "",// TODO
        "Búsqueda rápida",
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "", // Eesti
        "", // Dansk
        "", // Czech
    },
    {
        "Show in main menu",
        "Im Hauptmenü anzeigen",
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "Näytä valinta päävalikossa",
        "",// TODO
        "Mostrar en menú principal",
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "",// TODO
        "", // Eesti
        "", // Dansk
        "", // Czech
    },
    { NULL }
};
#endif
  
class cPluginQuicksearch : public cMainMenuShortcut
{
    public:
        virtual const char* Version() { return VERSION; }
#if APIVERSNUM < 10507
        virtual const char* Description() { return tr(DESCRIPTION); }
#else
        virtual const char* Description() { return I18nTranslate(DESCRIPTION, I18nEpgsearch); }
#endif
        virtual bool Initialize();
        virtual cOsdObject* MainMenuAction() { return GetEpgSearchMenu("Epgsearch-quicksearch-v1.0"); };

    protected:
#if APIVERSNUM < 10507
        virtual const char* SetupText() { return tr(SETUPTEXT); }
        virtual const char* MainMenuText() { return tr(MAINMENUENTRY); };
#else
        virtual const char* SetupText() { return I18nTranslate(SETUPTEXT, I18nEpgsearch); }
        virtual const char* MainMenuText() { return I18nTranslate(MAINMENUENTRY, I18nEpgsearch); }
#endif
};

bool cPluginQuicksearch::Initialize()
{
#if APIVERSNUM < 10507
    RegisterI18n(PhrasesLocal);
#endif
    return cMainMenuShortcut::Initialize();
}

VDRPLUGINCREATOR(cPluginQuicksearch); // Don't touch this!

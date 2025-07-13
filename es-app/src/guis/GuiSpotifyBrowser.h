#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "SpotifyManager.h" // Per le struct SpotifyPlaylist/Track

class GuiSpotifyBrowser : public GuiComponent
{
public:
    GuiSpotifyBrowser(Window* window);

private:
    void loadPlaylists();
    // ! SICUREZZA !: Accetta l'ID per valore per evitare problemi di memoria
    void loadTracks(std::string id); 

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
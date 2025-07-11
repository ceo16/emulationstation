#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "SpotifyManager.h"

class GuiSpotifyBrowser : public GuiComponent
{
public:
    GuiSpotifyBrowser(Window* window);

private:
    void loadPlaylists();
    void loadTracks(const std::string& playlist_id);
    std::vector<SpotifyTrack> getPlaylistTracks(const std::string& playlist_id); // <-- Funzione dichiarata qui

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
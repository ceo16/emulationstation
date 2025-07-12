#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "SpotifyManager.h" // Includiamo SpotifyManager
#include "guis/GuiMsgBox.h" // Necessario per GuiMsgBox nel .cpp

class GuiSpotifyBrowser : public GuiComponent
{
public:
    GuiSpotifyBrowser(Window* window);

private:
    void loadPlaylists();
    void loadTracks(std::string playlist_id);

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "SpotifyManager.h" // per SpotifyTrack/Playlist
#include "HelpPrompt.h"     // per getHelpPrompts
#include "InputConfig.h"    // per input()

class GuiSpotifyBrowser : public GuiComponent
{
public:
    explicit GuiSpotifyBrowser(Window* window);

    // input() per gestire “back” / “start”
   bool input(InputConfig* config, Input input) override;

    // i help‑prompt in basso
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void loadPlaylists();
    void loadTracks(std::string id);
    void centerMenu();

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H

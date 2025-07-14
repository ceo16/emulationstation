#pragma once
#include "views/gamelist/BasicGameListView.h"

class SpotifyPlaylistView : public BasicGameListView {
public:
    SpotifyPlaylistView(Window* window, SystemData* system);
    void launch(const GameLaunchData& gameData) override;
};

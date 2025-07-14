#pragma once
#include "views/gamelist/BasicGameListView.h"

class SpotifySongListView : public BasicGameListView {
public:
    SpotifySongListView(Window* window, FileData* playlistFile);
    void launch(const GameLaunchData& gameData) override;
};

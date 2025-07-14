#pragma once
#include "views/gamelist/BasicGameListView.h"

class SpotifyHomeView : public BasicGameListView {
public:
    SpotifyHomeView(Window* window, SystemData* system)
      : BasicGameListView(window, system) {
        // Popola con raccomandazioni (API: mgr.getRecommended())
    }
};

#include "views/spotify/SpotifyPlaylistView.h"
#include "views/spotify/SpotifySongListView.h"

SpotifyPlaylistView::SpotifyPlaylistView(Window* window, SystemData* system)
  : BasicGameListView(window, system) {
    // il root folder è già popolato da populateSpotifyVirtual
}

void SpotifyPlaylistView::launch(const GameLaunchData& gameData) {
    // Push della view dei brani senza invocare LaunchGameCmd
    mWindow->pushGui(std::make_shared<SpotifySongListView>(mWindow, &gameData.game));
}

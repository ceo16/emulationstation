#include "views/spotify/SpotifySongListView.h"
#include "SpotifyManager.h"

SpotifySongListView::SpotifySongListView(Window* window, FileData* playlistFile)
  : BasicGameListView(window, nullptr) {
    auto& mgr = SpotifyManager::getInstance();
    auto songs = mgr.getPlaylistTracks(playlistFile->getPath());
    for (auto& track : songs) {
        std::string uri = "spotify://track/" + track.id;
        FileData* fd = new FileData(FileType::GAME, uri, nullptr);
        fd->metadata.set(MetaDataId::Name, track.name);
        getRootFolder()->addChild(fd, false);
    }
}

void SpotifySongListView::launch(const GameLaunchData& gameData) {
    auto& mgr = SpotifyManager::getInstance();
    mgr.playTrack(gameData.game.getPath());
}

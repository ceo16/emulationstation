#include "views/spotify/SpotifyHomeView.h"
#include "SpotifyManager.h"

SpotifyHomeView::SpotifyHomeView(Window* window, SystemData* system)
  : BasicGameListView(window, system) {
    auto& mgr = SpotifyManager::getInstance();
    auto recs = mgr.getRecommended();
    for (auto& track : recs) {
        std::string uri = "spotify://track/" + track.id;
        FileData* fd = new FileData(FileType::GAME, uri, nullptr);
        fd->metadata.set(MetaDataId::Name, track.name);
        getRootFolder()->addChild(fd, false);
    }
}

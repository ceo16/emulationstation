#include "views/spotify/SpotifyPhotoGalleryView.h"
#include "components/ImageComponent.h"
#include "SpotifyManager.h"

SpotifyPhotoGalleryView::SpotifyPhotoGalleryView(Window* window, SystemData* system)
  : GuiComponent(window) {
    auto& mgr = SpotifyManager::getInstance();
    auto images = mgr.getArtistImages(); // or appropriate API
    int x = 0, y = 0;
    for (auto& img : images) {
        auto imageComp = std::make_shared<ImageComponent>(window);
        imageComp->setImage(img.url);
        imageComp->setPosition(x, y);
        addChild(imageComp.get());
        x += imageComp->getSize().x() + 10;
        if (x > window->getSize().x()) {
            x = 0;
            y += imageComp->getSize().y() + 10;
        }
    }
}

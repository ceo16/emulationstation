#include "views/spotify/SpotifyArtistDetailView.h"
#include "components/TextComponent.h"
#include "components/ImageComponent.h"
#include "SpotifyManager.h"

SpotifyArtistDetailView::SpotifyArtistDetailView(Window* window, void* artist)
  : GuiComponent(window) {
    // stub for artist details
    auto nameComp = std::make_shared<TextComponent>(window, "Artist Name", 32);
    addChild(nameComp.get());
    // add image, bio, etc.
}

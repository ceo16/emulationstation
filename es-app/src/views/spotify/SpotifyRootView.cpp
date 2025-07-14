#include "views/spotify/SpotifyRootView.h"
#include "views/spotify/SpotifyHomeView.h"
#include "views/spotify/SpotifySearchView.h"
#include "views/spotify/SpotifyPlaylistView.h"
#include "views/spotify/SpotifyPhotoGalleryView.h"

SpotifyRootView::SpotifyRootView(Window* window, SystemData* system)
  : GuiComponent(window),
    mMenu(window, (int)Font::get(FONT_SIZE_MEDIUM)) {
    mMenu.addEntry("Home", 0, true);
    mMenu.addEntry("Cerca Artisti", 1, false);
    mMenu.addEntry("Playlist", 2, false);
    mMenu.addEntry("Foto", 3, false);

    mActions = {
        [window, system] {
            window->pushGui(std::make_shared<SpotifyHomeView>(window, system));
        },
        [window, system] {
            window->pushGui(std::make_shared<SpotifySearchView>(window, system));
        },
        [window, system] {
            window->pushGui(std::make_shared<SpotifyPlaylistView>(window, system));
        },
        [window, system] {
            window->pushGui(std::make_shared<SpotifyPhotoGalleryView>(window, system));
        }
    };

    addChild(&mMenu);
}

void SpotifyRootView::render(const Transform4x4f& parentTrans) {
    mMenu.render(parentTrans);
}

bool SpotifyRootView::input(InputConfig* config, Input input) {
    if (mMenu.input(config, input)) {
        int idx = mMenu.getSelectedId();
        mActions[idx]();
        return true;
    }
    return GuiComponent::input(config, input);
}

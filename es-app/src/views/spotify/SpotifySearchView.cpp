#include "views/spotify/SpotifySearchView.h"
#include "SpotifyManager.h"

SpotifySearchView::SpotifySearchView(Window* window, SystemData* system)
  : GuiComponent(window),
    mPrompt(window, "Cerca artista:", 24),
    mResults(window, (int)Font::get(FONT_SIZE_SMALL)) {
    addChild(&mPrompt);
    addChild(&mResults);
}

bool SpotifySearchView::input(InputConfig* config, Input input) {
    if (input.value && input.id == /* your input ID for search */ 0) {
        mQuery = Window::showTextInput("Digita nome artista");
        auto& mgr = SpotifyManager::getInstance();
        auto artists = mgr.searchArtists(mQuery);
        mResults.clear();
        for (auto& art : artists) {
            mResults.addEntry(art.name, 0, [this, art] {
                window->pushGui(std::make_shared<SpotifyArtistDetailView>(window, art));
            });
        }
        return true;
    }
    if (mResults.input(config, input)) return true;
    return GuiComponent::input(config, input);
}

void SpotifySearchView::render(const Transform4x4f& parentTrans) {
    mPrompt.render(parentTrans);
    mResults.render(parentTrans);
}

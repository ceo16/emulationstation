#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "SpotifyManager.h"
#include "HelpPrompt.h"
#include "InputConfig.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "json.hpp" // Necessario per nlohmann::json

// --- COMPONENTE PER LA RIGA (INVARIATO) ---
class SpotifyItemComponent : public GuiComponent
{
public:
    SpotifyItemComponent(Window* window, const std::string& text, const std::string& imageUrl);
    void render(const Transform4x4f& parentTrans) override;
    void onSizeChanged() override;
private:
    ImageComponent* mImage;
    TextComponent* mText;
    std::string mTextStr;
    std::string mImageUrl;
    bool mInitialized;
};


// --- CLASSE PRINCIPALE DEL BROWSER ---
class GuiSpotifyBrowser : public GuiComponent
{
public:
    explicit GuiSpotifyBrowser(Window* window);

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    // --- DICHIARAZIONI COMPLETE DI TUTTE LE FUNZIONI ---
    void openMainMenu();
    void openSearchMenu();
    void openSearch(const std::string& type);
    void openPlaylists();
    void openTracks(const std::string& playlistId, const std::string& playlistName);
    void openArtistTopTracks(const std::string& artistId, const std::string& artistName);
    void openLikedSongs();
    
    void showTrackResults(const nlohmann::json& results);
    void showArtistResults(const nlohmann::json& results);

    void centerMenu();

    // --- STATO ---
    enum class SpotifyViewState { MainMenu, SearchMenu, Playlists, Tracks, LikedSongs, SearchResults, ArtistTopTracks };
    SpotifyViewState mState;

    std::string mCurrentPlaylistId;
    std::string mCurrentPlaylistName;

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
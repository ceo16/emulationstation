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
#include "json.hpp"
#include <memory>
#include <vector>

// --- SpotifyItemComponent (invariato) ---
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

// Aggiunta per coerenza, anche se usata solo qui
struct SpotifyArtist {
    std::string name;
    std::string id;
    std::string image_url;
};


// --- CLASSE PRINCIPALE DEL BROWSER ---
class GuiSpotifyBrowser : public GuiComponent
{
public:
    explicit GuiSpotifyBrowser(Window* window);

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void openMainMenu();
    void openSearchMenu();
    void openSearch(const std::string& type);
    void openPlaylists();
    void openLikedSongs();
	void openFeaturedPlaylists();
	void openCategories();
    void openCategoryPlaylists(const std::string& categoryId, const std::string& categoryName);

    void openMyPlaylistTracks(const std::string& playlistId, const std::string& playlistName);
    void openSearchPlaylistTracks(const std::string& playlistId, const std::string& playlistName);
    void openAlbumTracks(const std::string& albumId, const std::string& albumName, const std::string& albumImageUrl);
    void openArtistTopTracks(const std::string& artistId, const std::string& artistName);

    void showTrackResults(const nlohmann::json& results);
    void showArtistResults(const nlohmann::json& results);
    void showAlbumResults(const nlohmann::json& results);
    void showPlaylistResults(const nlohmann::json& results);
	    void openDeviceList();


    void centerMenu();

    enum class SpotifyViewState { MainMenu, SearchMenu, Playlists, MyPlaylistTracks, LikedSongs, SearchResults, ArtistTopTracks, AlbumTracks, SearchPlaylistTracks, FeaturedPlaylists, Categories, CategoryPlaylists, DeviceList };
    SpotifyViewState mState;

    MenuComponent mMenu;

    // Variabili separate per ogni tipo di lista
    std::vector<SpotifyPlaylist> mLoadedPlaylists;
    std::vector<SpotifyPlaylist> mFoundPlaylists;
    std::vector<SpotifyAlbum>    mLoadedAlbums;
	std::vector<SpotifyPlaylist> mFeaturedPlaylists;
    std::vector<SpotifyArtist>   mFoundArtists; // Variabile dedicata per gli artisti
	std::vector<SpotifyCategory> mCategories; // Aggiunto
	std::vector<SpotifyPlaylist> mCategoryPlaylists; // Aggiunto
    std::vector<SpotifyDevice>   mDevices;
    nlohmann::json mLastSearchResults;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
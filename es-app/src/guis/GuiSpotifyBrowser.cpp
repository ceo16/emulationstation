#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "SpotifyManager.h"
#include "guis/GuiMsgBox.h"
#include "renderers/Renderer.h"
#include "InputConfig.h"
#include "ThemeData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Paths.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include <thread>
#include <fstream>
#include <memory>

// --- SpotifyItemComponent (implementazione invariata) ---
namespace {
    std::string getSpotifyImagePath() {
        std::string path = Paths::getUserEmulationStationPath() + "/roms/spotify/images";
        return path;
    }
}

SpotifyItemComponent::SpotifyItemComponent(Window* window, const std::string& text, const std::string& imageUrl)
    : GuiComponent(window), mImage(nullptr), mText(nullptr),
      mTextStr(text), mImageUrl(imageUrl), mInitialized(false)
{
}

void SpotifyItemComponent::render(const Transform4x4f& parentTrans)
{
    if (!mInitialized)
    {
        mInitialized = true;
        mImage = new ImageComponent(mWindow);
        mText = new TextComponent(mWindow, mTextStr, ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
        mText->setVerticalAlignment(ALIGN_CENTER);
        addChild(mImage);
        addChild(mText);

        if (!mImageUrl.empty())
        {
            std::string safeName = Utils::String::replace(mImageUrl, "https://", "");
            safeName = Utils::String::replace(safeName, "/", "_");
            safeName = Utils::String::replace(safeName, ":", "_");
            safeName = Utils::String::replace(safeName, "?", "_");
            std::string tempPath = getSpotifyImagePath() + "/" + safeName + ".jpg";

            if (Utils::FileSystem::exists(tempPath)) {
                mImage->setImage(tempPath);
            } else {
                std::thread([this, tempPath]() {
                    HttpReq req(mImageUrl);
                    req.wait();
                    if (req.status() == HttpReq::Status::REQ_SUCCESS)
                    {
                        std::ofstream file(tempPath, std::ios::binary);
                        if (file) {
                            file.write(req.getContent().c_str(), req.getContent().length());
                            file.close();
                            mWindow->postToUiThread([this, tempPath]() {
                                if(mImage) mImage->setImage(tempPath);
                            });
                        }
                    }
                }).detach();
            }
        }
        setSize(mSize);
    }
    GuiComponent::render(parentTrans);
}

void SpotifyItemComponent::onSizeChanged()
{
    if (!mInitialized) return;
    const float imageSize = 64.0f;
    if (mImage) {
        mImage->setOrigin(0, 0.5f);
        mImage->setPosition(10, mSize.y() / 2);
        mImage->setMaxSize(Vector2f(imageSize, imageSize));
    }
    if (mText) {
        const float textX = imageSize + 20;
        mText->setOrigin(0, 0.5f);
        mText->setPosition(textX, mSize.y() / 2);
        mText->setSize(mSize.x() - textX, mText->getFont()->getLetterHeight());
    }
}

// --- GuiSpotifyBrowser (implementazione completa e corretta) ---
GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window)
  : GuiComponent(window),
    mMenu(window, "SPOTIFY"),
    mState(SpotifyViewState::MainMenu)
{
    std::string path = getSpotifyImagePath();
    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);
    addChild(&mMenu);
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    openMainMenu();
}

void GuiSpotifyBrowser::centerMenu()
{
    float sw = Renderer::getScreenWidth();
    float sh = Renderer::getScreenHeight();
    auto sz = mMenu.getSize();
    mMenu.setPosition((sw - sz.x()) * 0.5f, (sh - sz.y()) * 0.5f);
}

void GuiSpotifyBrowser::openMainMenu() {
    mState = SpotifyViewState::MainMenu;
    mMenu.clear();
    mMenu.setTitle("SPOTIFY");
    mMenu.addEntry("RICERCA", true, [this] { openSearchMenu(); });
    mMenu.addEntry("LE TUE PLAYLIST", true, [this] { openPlaylists(); });
    mMenu.addEntry("BRANI CHE TI PIACCIONO", true, [this] { openLikedSongs(); });
    centerMenu();
}

void GuiSpotifyBrowser::openSearchMenu() {
    mState = SpotifyViewState::SearchMenu;
    mMenu.clear();
    mMenu.setTitle("COSA VUOI CERCARE?");
    mMenu.addEntry("BRANI", true, [this] { openSearch("track"); });
    mMenu.addEntry("ARTISTI", true, [this] { openSearch("artist"); });
    mMenu.addEntry("ALBUM", true, [this] { openSearch("album"); });
    mMenu.addEntry("PLAYLIST", true, [this] { openSearch("playlist"); });
    centerMenu();
}

void GuiSpotifyBrowser::openSearch(const std::string& type) {
    std::string title = "CERCA " + Utils::String::toUpper(type);
    auto keyboard = new GuiTextEditPopupKeyboard(mWindow, title, "", [this, type](const std::string& query) {
        if (!query.empty()) {
            mMenu.clear();
            mMenu.setTitle("RICERCA IN CORSO...");
            centerMenu();
            SpotifyManager::getInstance(mWindow)->search(query, type, [this, type](const nlohmann::json& results) {
                mLastSearchResults = results;
                if (type == "track") showTrackResults(results);
                else if (type == "artist") showArtistResults(results);
                else if (type == "album") showAlbumResults(results);
                else if (type == "playlist") showPlaylistResults(results);
            });
        }
    }, false);
    mWindow->pushGui(keyboard);
}

void GuiSpotifyBrowser::openPlaylists() {
    mState = SpotifyViewState::Playlists;
    mMenu.clear();
    mMenu.setTitle("LE TUE PLAYLIST");
    mMenu.addEntry("Caricamento...", false, nullptr);
    centerMenu();
    
    SpotifyManager::getInstance(mWindow)->getUserPlaylists([this](const std::vector<SpotifyPlaylist>& playlists) {
        mLoadedPlaylists = playlists;
        mMenu.clear();
        mMenu.setTitle("LE TUE PLAYLIST");
        if (mLoadedPlaylists.empty()) {
            mMenu.addEntry("Nessuna playlist trovata.", false, nullptr);
        } else {
            for (size_t i = 0; i < mLoadedPlaylists.size(); ++i) {
                const auto& p = mLoadedPlaylists[i];
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, p.name, p.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                
                row.makeAcceptInputHandler([this, index = i] {
                    if (index < mLoadedPlaylists.size()) {
                        const auto& clickedPlaylist = mLoadedPlaylists[index];
                        openMyPlaylistTracks(clickedPlaylist.id, clickedPlaylist.name);
                    }
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openLikedSongs() {
    mState = SpotifyViewState::LikedSongs;
    mMenu.clear();
    mMenu.setTitle("BRANI CHE TI PIACCIONO");
    mMenu.addEntry("Caricamento...", false, nullptr);
    centerMenu();
    SpotifyManager::getInstance(mWindow)->getUserLikedSongs([this](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle("BRANI CHE TI PIACCIONO");
        if (tracks.empty()) {
            mMenu.addEntry("Nessun brano trovato.", false, nullptr);
        } else {
            for (const auto& t : tracks) {
                auto uriPtr = std::make_shared<std::string>(t.uri);
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, t.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([uriPtr] {
                    if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr);
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openMyPlaylistTracks(const std::string& playlistId, const std::string& playlistName) {
    mState = SpotifyViewState::MyPlaylistTracks;
    mMenu.clear();
    mMenu.setTitle(Utils::String::toUpper(playlistName));
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();

    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(playlistId, [this, playlistName](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle(Utils::String::toUpper(playlistName));
        if (tracks.empty()) {
            mMenu.addEntry("Nessuna traccia trovata.", false, nullptr);
        } else {
            for (const auto& t : tracks) {
                auto uriPtr = std::make_shared<std::string>(t.uri);
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, t.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([uriPtr] {
                    if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr);
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openSearchPlaylistTracks(const std::string& playlistId, const std::string& playlistName) {
    mState = SpotifyViewState::SearchPlaylistTracks;
    mMenu.clear();
    mMenu.setTitle(Utils::String::toUpper(playlistName));
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();

    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(playlistId, [this, playlistName](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle(Utils::String::toUpper(playlistName));
        if (tracks.empty()) {
            mMenu.addEntry("Nessuna traccia trovata.", false, nullptr);
        } else {
            for (const auto& t : tracks) {
                auto uriPtr = std::make_shared<std::string>(t.uri);
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, t.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([uriPtr] {
                    if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr);
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openAlbumTracks(const std::string& albumId, const std::string& albumName, const std::string& albumImageUrl) {
    mState = SpotifyViewState::AlbumTracks;
    mMenu.clear();
    mMenu.setTitle(Utils::String::toUpper(albumName));
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();
    
    SpotifyManager::getInstance(mWindow)->getAlbumTracks(albumId, [this, albumName, albumImageUrl](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle(Utils::String::toUpper(albumName));
        if (tracks.empty()) {
            mMenu.addEntry("Nessuna traccia trovata.", false, nullptr);
        } else {
            for (const auto& t : tracks) {
                auto uriPtr = std::make_shared<std::string>(t.uri);
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, albumImageUrl);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([uriPtr] {
                    if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr);
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openArtistTopTracks(const std::string& artistId, const std::string& artistName) {
    mState = SpotifyViewState::ArtistTopTracks;
    mMenu.clear();
    mMenu.setTitle(Utils::String::toUpper(artistName));
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();
    
    SpotifyManager::getInstance(mWindow)->getArtistTopTracks(artistId, [this, artistName](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle(Utils::String::toUpper(artistName));
        if (tracks.empty()) {
            mMenu.addEntry("Nessuna traccia trovata per questo artista.", false, nullptr);
        } else {
            for (const auto& t : tracks) {
                auto uriPtr = std::make_shared<std::string>(t.uri);
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name, t.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([uriPtr] { 
                    if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr);
                });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::showTrackResults(const nlohmann::json& results) {
    mState = SpotifyViewState::SearchResults;
    mMenu.clear();
    mMenu.setTitle("RISULTATI RICERCA BRANI");
    if (!results.contains("tracks") || !results["tracks"].contains("items") || results["tracks"]["items"].empty()) {
        mMenu.addEntry("Nessun brano trovato.", false, nullptr);
    } else {
        for (const auto& tr : results["tracks"]["items"]) {
            if (tr.is_null()) continue;
            auto uriPtr = std::make_shared<std::string>(tr.value("uri", ""));
            std::string label = tr.value("name", "?") + " — " + (tr["artists"][0].value("name", "?"));
            std::string imageUrl = (tr.contains("album") && !tr["album"]["images"].empty()) ? tr["album"]["images"][0].value("url", "") : "";
            
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, label, imageUrl);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            row.makeAcceptInputHandler([uriPtr] { 
                if (!uriPtr->empty()) SpotifyManager::getInstance()->startPlayback(*uriPtr); 
            });
            mMenu.addRow(row);
        }
    }
    centerMenu();
}

void GuiSpotifyBrowser::showArtistResults(const nlohmann::json& results) {
    mState = SpotifyViewState::SearchResults;
    mMenu.clear();
    mMenu.setTitle("RISULTATI RICERCA ARTISTI");

    // Usiamo la variabile dedicata mFoundArtists
    mFoundArtists.clear();
    if (results.contains("artists") && results["artists"].contains("items")) {
        for (const auto& artistJson : results["artists"]["items"]) {
            if (artistJson.is_null()) continue;
            mFoundArtists.push_back({
                artistJson.value("name", "?"),
                artistJson.value("id", ""),
                (!artistJson["images"].empty()) ? artistJson["images"][0].value("url", "") : ""
            });
        }
    }

    if (mFoundArtists.empty()) {
        mMenu.addEntry("Nessun artista trovato.", false, nullptr);
    } else {
        // Cicliamo sulla nuova variabile con un indice
        for (size_t i = 0; i < mFoundArtists.size(); ++i) {
            const auto& artist = mFoundArtists[i];
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, artist.name, artist.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            
            // La logica basata su indice garantisce che i dati corretti vengano recuperati
            row.makeAcceptInputHandler([this, index = i] {
                if (index < mFoundArtists.size()) {
                    const auto& clickedArtist = mFoundArtists[index];
                    LOG(LogInfo) << "[GuiSpotifyBrowser] Click su artista cercato (indice " << index 
                                 << "). Recuperato artista '" << clickedArtist.name 
                                 << "' con ID '" << clickedArtist.id << "'";
                    openArtistTopTracks(clickedArtist.id, clickedArtist.name);
                } else {
                    LOG(LogError) << "[GuiSpotifyBrowser] ERRORE: Indice di ricerca artista " << index << " fuori dai limiti!";
                }
            });
            mMenu.addRow(row);
        }
    }
    centerMenu();
}

void GuiSpotifyBrowser::showAlbumResults(const nlohmann::json& results) {
    mState = SpotifyViewState::SearchResults;
    mMenu.clear();
    mMenu.setTitle("RISULTATI RICERCA ALBUM");

    mLoadedAlbums.clear();
    if (results.contains("albums") && results["albums"].contains("items")) {
        for (const auto& albumJson : results["albums"]["items"]) {
            if (albumJson.is_null()) continue;
            mLoadedAlbums.push_back({
                albumJson.value("name", "?"),
                albumJson.value("id", ""),
                (!albumJson["images"].empty()) ? albumJson["images"][0].value("url", "") : ""
            });
        }
    }

    if (mLoadedAlbums.empty()) {
        mMenu.addEntry("Nessun album trovato.", false, nullptr);
    } else {
        for (size_t i = 0; i < mLoadedAlbums.size(); ++i) {
            const auto& album = mLoadedAlbums[i];
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, album.name, album.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            
            row.makeAcceptInputHandler([this, index = i] {
                if (index < mLoadedAlbums.size()) {
                    const auto& clickedAlbum = mLoadedAlbums[index];
                    openAlbumTracks(clickedAlbum.id, clickedAlbum.name, clickedAlbum.image_url);
                }
            });
            mMenu.addRow(row);
        }
    }
    centerMenu();
}

void GuiSpotifyBrowser::showPlaylistResults(const nlohmann::json& results) {
    mState = SpotifyViewState::SearchResults;
    mMenu.clear();
    mMenu.setTitle("RISULTATI RICERCA PLAYLIST");

    mFoundPlaylists.clear();
    if (results.contains("playlists") && results["playlists"].contains("items")) {
        for (const auto& p : results["playlists"]["items"]) {
            if (p.is_null()) continue;
            mFoundPlaylists.push_back({
                p.value("name", "?"),
                p.value("id", ""),
                (!p["images"].empty()) ? p["images"][0].value("url", "") : ""
            });
        }
    }

    if (mFoundPlaylists.empty()) {
        mMenu.addEntry("Nessuna playlist trovata.", false, nullptr);
    } else {
        for (size_t i = 0; i < mFoundPlaylists.size(); ++i) {
            const auto& p = mFoundPlaylists[i];
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, p.name, p.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            
            row.makeAcceptInputHandler([this, index = i] {
                if (index < mFoundPlaylists.size()) {
                    const auto& clickedPlaylist = mFoundPlaylists[index];
                    openSearchPlaylistTracks(clickedPlaylist.id, clickedPlaylist.name);
                }
            });
            mMenu.addRow(row);
        }
    }
    centerMenu();
}

bool GuiSpotifyBrowser::input(InputConfig* config, Input input) {
    if (GuiComponent::input(config, input)) return true;
    if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0) {
        switch (mState) {
            case SpotifyViewState::MyPlaylistTracks:
                openPlaylists();
                break;

            case SpotifyViewState::SearchPlaylistTracks:
                showPlaylistResults(mLastSearchResults);
                break;

            case SpotifyViewState::AlbumTracks:
                showAlbumResults(mLastSearchResults);
                break;
            
            case SpotifyViewState::ArtistTopTracks:
                showArtistResults(mLastSearchResults);
                break;

            case SpotifyViewState::SearchResults:
                openSearchMenu();
                break;

            case SpotifyViewState::Playlists:
            case SpotifyViewState::LikedSongs:
            case SpotifyViewState::SearchMenu:
                openMainMenu();
                break;

            case SpotifyViewState::MainMenu:
            default:
                delete this;
                break;
        }
        return true;
    }
    return false;
}

std::vector<HelpPrompt> GuiSpotifyBrowser::getHelpPrompts() {
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
    prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
    if (mState != SpotifyViewState::MainMenu) {
        prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
    } else {
        prompts.push_back(HelpPrompt("start", _("CLOSE")));
    }
    return prompts;
}
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

// --- SpotifyItemComponent (invariato) ---
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
            // --- INIZIO BLOCCO CORRETTO PER IL NOME FILE ---
            // Crea un nome di file sicuro dall'URL
            std::string safeName = Utils::String::replace(mImageUrl, "https://", "");
            safeName = Utils::String::replace(safeName, "/", "_");
            safeName = Utils::String::replace(safeName, ":", "_");
            safeName = Utils::String::replace(safeName, "?", "_"); // Rimuovi anche i punti interrogativi
            
            // Definiamo un'estensione fissa, dato che gli URL di Spotify non ce l'hanno
            std::string tempPath = getSpotifyImagePath() + "/" + safeName + ".jpg";
            // --- FINE BLOCCO CORRETTO ---

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
                    } else {
                        LOG(LogError) << "Download fallito per: " << mImageUrl;
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


GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window)
  : GuiComponent(window),
    mMenu(window, "SPOTIFY"),
    mState(SpotifyViewState::MainMenu) // Iniziamo dal menu principale
{
    std::string path = getSpotifyImagePath();
    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    addChild(&mMenu);
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    openMainMenu(); // La prima schermata è il menu principale
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
    updateHelpPrompts();
}

void GuiSpotifyBrowser::openSearchMenu() {
    mState = SpotifyViewState::SearchMenu;
    mMenu.clear();
    mMenu.setTitle("COSA VUOI CERCARE?");
    mMenu.addEntry("BRANI", true, [this] { openSearch("track"); });
    mMenu.addEntry("ARTISTI", true, [this] { openSearch("artist"); });
    centerMenu();
    updateHelpPrompts();
}

void GuiSpotifyBrowser::openSearch(const std::string& type) {
    std::string title = (type == "track") ? "CERCA UN BRANO" : "CERCA UN ARTISTA";
    auto keyboard = new GuiTextEditPopupKeyboard(mWindow, title, "", [this, type](const std::string& query) {
        if (!query.empty()) {
            mMenu.clear();
            mMenu.setTitle("RICERCA IN CORSO...");
            mMenu.addEntry("...", false, nullptr);
            centerMenu();
            SpotifyManager::getInstance(mWindow)->search(query, type, [this, type](const nlohmann::json& results) {
                if (type == "track") showTrackResults(results);
                else if (type == "artist") showArtistResults(results);
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
    updateHelpPrompts();
    SpotifyManager::getInstance(mWindow)->getUserPlaylists([this](const std::vector<SpotifyPlaylist>& playlists) {
        mMenu.clear();
        mMenu.setTitle("LE TUE PLAYLIST");
        if (playlists.empty()) mMenu.addEntry("Nessuna playlist trovata.", false, nullptr);
        else for (auto& p : playlists) {
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, p.name, p.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            row.makeAcceptInputHandler([this, id = p.id, name = p.name] { openTracks(id, name); });
            mMenu.addRow(row);
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::openTracks(const std::string& playlistId, const std::string& playlistName) {
    mState = SpotifyViewState::Tracks;
    mCurrentPlaylistId = playlistId;
    mCurrentPlaylistName = playlistName;
    mMenu.clear();
    mMenu.setTitle(playlistName);
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();
    updateHelpPrompts();
    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(playlistId, [this](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle(mCurrentPlaylistName);
        if (tracks.empty()) mMenu.addEntry("Nessuna traccia trovata.", false, nullptr);
        else for (const auto& t : tracks) {
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, t.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            row.makeAcceptInputHandler([uri = t.uri] { SpotifyManager::getInstance()->startPlayback(uri); });
            mMenu.addRow(row);
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
    updateHelpPrompts();
    SpotifyManager::getInstance(mWindow)->getUserLikedSongs([this](const std::vector<SpotifyTrack>& tracks) {
        mMenu.clear();
        mMenu.setTitle("BRANI CHE TI PIACCIONO");
        if (tracks.empty()) mMenu.addEntry("Nessun brano trovato.", false, nullptr);
        else for (const auto& t : tracks) {
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name + " — " + t.artist, t.image_url);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            row.makeAcceptInputHandler([uri = t.uri] { SpotifyManager::getInstance()->startPlayback(uri); });
            mMenu.addRow(row);
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
    } else for (const auto& tr : results["tracks"]["items"]) {
        if (!tr.is_null()) {
            std::string label = tr.value("name", "?") + " — " + (tr["artists"][0].value("name", "?"));
            std::string imageUrl = (tr.contains("album") && !tr["album"]["images"].empty()) ? tr["album"]["images"][0].value("url", "") : "";
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, label, imageUrl);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            row.makeAcceptInputHandler([uri = tr.value("uri", "")] { SpotifyManager::getInstance()->startPlayback(uri); });
            mMenu.addRow(row);
        }
    }
    centerMenu();
    updateHelpPrompts();
}

void GuiSpotifyBrowser::showArtistResults(const nlohmann::json& results)
{
    mState = SpotifyViewState::SearchResults;
    mMenu.clear();
    mMenu.setTitle("RISULTATI RICERCA ARTISTI");

    if (!results.contains("artists") || !results["artists"].contains("items") || results["artists"]["items"].empty()) {
        mMenu.addEntry("Nessun artista trovato.", false, nullptr);
    } else {
        for (const auto& artist : results["artists"]["items"]) {
            if (artist.is_null()) continue;

            // ==========================================================
            // ===      LA SOLUZIONE DEFINITIVA È QUI                 ===
            // ==========================================================
            // Creiamo puntatori intelligenti per garantire che le stringhe
            // esistano ancora quando l'utente cliccherà.
            auto artistNamePtr = std::make_shared<std::string>(artist.value("name", "?"));
            auto artistIdPtr = std::make_shared<std::string>(artist.value("id", ""));
            auto imageUrl = (!artist["images"].empty()) ? artist["images"][0].value("url", "") : "";
            
            ComponentListRow row;
            auto item = std::make_shared<SpotifyItemComponent>(mWindow, *artistNamePtr, imageUrl);
            row.addElement(item, true);
            item->setSize(mMenu.getSize().x(), 74.0f);
            
            // Catturiamo i puntatori. Ora siamo sicuri che i dati saranno validi.
            row.makeAcceptInputHandler([this, artistIdPtr, artistNamePtr] {
                LOG(LogInfo) << "CLICK RILEVATO per l'artista: " << *artistNamePtr << " con ID: " << *artistIdPtr;
                if (!artistIdPtr->empty()) {
                    openArtistTopTracks(*artistIdPtr, *artistNamePtr);
                }
            });
            mMenu.addRow(row);
        }
    }
    centerMenu();
    updateHelpPrompts();
}

void GuiSpotifyBrowser::openArtistTopTracks(const std::string& artistId, const std::string& artistName)
{
    // Log di debug per confermare l'esecuzione della funzione
    LOG(LogInfo) << "Funzione openArtistTopTracks avviata per l'artista: " << artistName;

    mState = SpotifyViewState::ArtistTopTracks;
    mMenu.clear();
    mMenu.setTitle(artistName);
    mMenu.addEntry("Caricamento tracce...", false, nullptr);
    centerMenu();
    updateHelpPrompts();

    SpotifyManager::getInstance(mWindow)->getArtistTopTracks(artistId,
        [this, artistName](const std::vector<SpotifyTrack>& tracks)
        {
            mMenu.clear();
            mMenu.setTitle(artistName);

            if (tracks.empty()) {
                mMenu.addEntry("Nessuna traccia trovata per questo artista.", false, nullptr);
            } else {
                for (const auto& t : tracks) {
                    ComponentListRow row;
                    auto item = std::make_shared<SpotifyItemComponent>(mWindow, t.name, t.image_url);
                    row.addElement(item, true);
                    item->setSize(mMenu.getSize().x(), 74.0f);
                    row.makeAcceptInputHandler([uri = t.uri] { SpotifyManager::getInstance()->startPlayback(uri); });
                    mMenu.addRow(row);
                }
            }
            centerMenu();
        }
    );
}

bool GuiSpotifyBrowser::input(InputConfig* config, Input input) {
    if (GuiComponent::input(config, input)) return true;
    if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0) {
        switch (mState) {
            case SpotifyViewState::Tracks: openPlaylists(); break;
            case SpotifyViewState::ArtistTopTracks: openSearchMenu(); break;
            case SpotifyViewState::Playlists: case SpotifyViewState::LikedSongs: case SpotifyViewState::SearchResults: case SpotifyViewState::SearchMenu: openMainMenu(); break;
            case SpotifyViewState::MainMenu: default: delete this; break;
        }
        return true;
    }
    return false;
}

std::vector<HelpPrompt> GuiSpotifyBrowser::getHelpPrompts() {
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
    prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
    if (mState != SpotifyViewState::MainMenu) prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
    else prompts.push_back(HelpPrompt("start", _("CLOSE")));
    return prompts;
}
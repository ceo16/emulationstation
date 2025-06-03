// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesStore.cpp
#include "GameStore/EAGames/EAGamesStore.h"
#include "GameStore/EAGames/EAGamesAuth.h"
#include "GameStore/EAGames/EAGamesAPI.h"
#include "GameStore/EAGames/EAGamesScanner.h"
#include "Log.h"
#include "Window.h"
#include "Settings.h"
#include "FileData.h"
#include "SystemData.h"
#include "MetaData.h"
#include "utils/StringUtil.h"
#include "utils/Platform.h"
#include "utils/FileSystemUtil.h"
#include "LocaleES.h"
#include "Paths.h"
#include "views/ViewController.h"
#include "GameStore/EAGames/EAGamesUI.h"
#include "scrapers/Scraper.h"

#include <algorithm>
#include <map>
#include <thread>
#include <memory> // Per std::make_shared

const std::string EAGamesStore::STORE_ID = "EAGamesStore";

EAGamesStore::EAGamesStore(Window* window)
    : mWindow(window),
      mAuth(std::make_unique<EAGames::EAGamesAuth>(mWindow)),
      mApi(std::make_unique<EAGames::EAGamesAPI>(mAuth.get())),
      mScanner(std::make_unique<EAGames::EAGamesScanner>()),
      mGamesCacheDirty(true),
      mFetchingGamesInProgress(false),
      mActiveScrapeCounter(0)
{
    LOG(LogInfo) << "EAGamesStore: Constructor completed.";
}

EAGamesStore::~EAGamesStore() {
    LOG(LogInfo) << "EAGamesStore: Destructor";
}

bool EAGamesStore::init(Window* /*window_param*/) {
    LOG(LogInfo) << "EAGamesStore: Initializing...";
    if (mAuth && !mAuth->isUserLoggedIn() && !mAuth->getRefreshToken().empty()) {
        LOG(LogDebug) << "EAGamesStore: Attempting initial token refresh.";
        mAuth->RefreshTokens([this](bool success, const std::string& message){
            if(success) {
                LOG(LogInfo) << "EAGamesStore: Auto token refresh successful on init.";
                this->mGamesCacheDirty = true;
            } else {
                LOG(LogWarning) << "EAGamesStore: Auto token refresh failed on init. Message: " << message;
            }
        });
    }
    this->_initialized = true;
    return true;
}

void EAGamesStore::showStoreUI(Window* window) {
    LOG(LogError) << "EAGamesStore::showStoreUI - INIZIO";
    if (!window) {
        LOG(LogError) << "EAGamesStore::showStoreUI - ERRORE: window è nullptr!";
        return;
    }
    LOG(LogError) << "EAGamesStore::showStoreUI - Sto per fare new EAGamesUI(window)";

    EAGamesUI* ui = new EAGamesUI(window);

    LOG(LogError) << "EAGamesStore::showStoreUI - new EAGamesUI(window) ESEGUITO, ui pointer: " << ui;
    if (!ui) {
         LOG(LogError) << "EAGamesStore::showStoreUI - ERRORE: new EAGamesUI(window) ha restituito nullptr!";
         return;
    }
    window->pushGui(ui);
    LOG(LogError) << "EAGamesStore::showStoreUI - window->pushGui(ui) ESEGUITO - FINE";
}

std::string EAGamesStore::getStoreName() const {
    return "EA Games";
}

bool EAGamesStore::launchGame(const std::string& gameId) {
    LOG(LogInfo) << "EAGamesStore: Attempting to launch game with ID: " << gameId;
    #ifndef _WIN32
        LOG(LogError) << "EAGamesStore: Game launching currently only supported on Windows.";
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Avvio supportato solo su Windows."));
        return false;
    #endif

    if (gameId.empty()) {
        LOG(LogError) << "EAGamesStore: Game ID for URI launch is empty.";
        return false;
    }
    std::string uri = "origin://launchgame/" + gameId;
    LOG(LogInfo) << "EAGamesStore: Launching game via URI: " << uri;
    std::string command = "explorer.exe \"" + uri + "\"";
   if (Utils::Platform::ProcessStartInfo(command).run() == 0) {
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Avvio gioco EA...") + " (" + gameId + ")");
        return true;
    }
    LOG(LogError) << "EAGamesStore: Failed to execute command: " << command;
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Fallimento avvio comando."));
    return false;
}

void EAGamesStore::shutdown() {
    LOG(LogInfo) << "EAGamesStore: Shutting down...";
    this->_initialized = false;
}

std::vector<FileData*> EAGamesStore::getGamesList() {
    LOG(LogDebug) << "EAGamesStore::getGamesList called. Cache dirty: " << mGamesCacheDirty << ", Fetching: " << mFetchingGamesInProgress;
    if (IsUserLoggedIn()) {
        if (mGamesCacheDirty && !mFetchingGamesInProgress) {
            LOG(LogInfo) << "EAGamesStore: Cache is dirty, initiating sync.";
            SyncGames([this](bool success){
                LOG(LogInfo) << "EAGamesStore: Background sync completed from getGamesList, success: " << success;
                if (this->mWindow && Settings::getInstance()->getBool("StorePopups")) {
                    this->mWindow->displayNotificationMessage(success ? _("Sincronizzazione EA Games completata.") : _("Errore sincronizzazione EA Games."));
                }
                if (success && this->mWindow && ViewController::get()) {
                    ViewController::get()->reloadAll(mWindow);
                }
            });
        }
    } else {
        LOG(LogWarning) << "EAGamesStore::getGamesList - User not logged in.";
        if (!mCachedGameFileDatas.empty()) {
           mCachedGameFileDatas.clear();
           rebuildReturnableGameList();
           if (mWindow && ViewController::get()) ViewController::get()->reloadAll(mWindow);
        }
    }

    if (mReturnableGameList.empty() && !mCachedGameFileDatas.empty()) {
        rebuildReturnableGameList();
    }
    return mReturnableGameList;
}

bool EAGamesStore::installGame(const std::string& gameId) {
    LOG(LogInfo) << "EAGamesStore: Attempting to install game with ID: " << gameId;
    #ifndef _WIN32
        LOG(LogError) << "EAGamesStore: Installazione supportata solo su Windows."; return false;
    #endif
    if (gameId.empty()) return false;
    std::string uri = "origin://downloadgame/" + gameId;
    std::string command = "explorer.exe \"" + uri + "\"";
    if (Utils::Platform::ProcessStartInfo(command).run() == 0) {
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Apertura client EA per installare ") + "(" + gameId + ")");
        return true;
    }
    return false;
}

bool EAGamesStore::uninstallGame(const std::string& gameId) {
    LOG(LogInfo) << "EAGamesStore: Attempting to uninstall game with ID: " << gameId;
    #ifndef _WIN32
        LOG(LogError) << "EAGamesStore: Disinstallazione supportata solo su Windows."; return false;
    #endif
    if (gameId.empty()) return false;
    std::string uri = "origin://uninstall/" + gameId;
    std::string command = "explorer.exe \"" + uri + "\"";
     if (Utils::Platform::ProcessStartInfo(command).run() == 0) {
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Apertura client EA per disinstallare ") + "(" + gameId + ")");
        mGamesCacheDirty = true;
        return true;
    }
    return false;
}

bool EAGamesStore::updateGame(const std::string& gameId) {
    LOG(LogWarning) << "EAGamesStore: updateGame for ID " << gameId << " - Client gestisce aggiornamenti.";
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Aggiornamenti gestiti dal client EA."));
    return launchGame(gameId);
}

bool EAGamesStore::IsUserLoggedIn() {
    return mAuth && mAuth->isUserLoggedIn();
}

void EAGamesStore::Login(std::function<void(bool success, const std::string& message)> callback) {
    if (mAuth) {
        mAuth->StartLoginFlow([this, callback](bool flowSuccess, const std::string& flowMessage) {
            if (flowSuccess) this->mGamesCacheDirty = true;
            if (callback) {
                if (this->mWindow) this->mWindow->postToUiThread([callback, flowSuccess, flowMessage] { callback(flowSuccess, flowMessage); });
                else callback(flowSuccess, flowMessage);
            }
        });
    } else if (callback) {
        std::string errorMsg = _("Modulo Auth EA non inizializzato.");
        if (this->mWindow) this->mWindow->postToUiThread([callback, errorMsg] { callback(false, errorMsg); });
        else callback(false, errorMsg);
    }
}

void EAGamesStore::Logout() {
    if (mAuth) mAuth->logout();
    mCachedGameFileDatas.clear();
    rebuildReturnableGameList();
    mGamesCacheDirty = true;
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Logout EA effettuato."));
    if (mWindow && ViewController::get()) ViewController::get()->reloadAll(mWindow);
}

void EAGamesStore::GetUsername(std::function<void(const std::string& username)> callback) {
    if (IsUserLoggedIn() && mAuth) {
        std::string pid = mAuth->getPidId();
        if (callback) {
            std::string usernameToShow = !pid.empty() ? (mAuth->getUserName().empty() ? "EA User (PID: " + pid + ")" : mAuth->getUserName()) : _("Utente EA");
            if (this->mWindow) this->mWindow->postToUiThread([callback, usernameToShow] { callback(usernameToShow); });
            else callback(usernameToShow);
        }
    } else if (callback) {
        if (this->mWindow) this->mWindow->postToUiThread([callback] { callback(""); });
        else callback("");
    }
}

void EAGamesStore::incrementActiveScrape() {
    mActiveScrapeCounter++;
    LOG(LogDebug) << "EAGamesStore: Scrape counter incremented to " << mActiveScrapeCounter;
}

void EAGamesStore::decrementActiveScrape() {
    mActiveScrapeCounter--;
    LOG(LogDebug) << "EAGamesStore: Scrape counter decremented to " << mActiveScrapeCounter;
}

void EAGamesStore::SyncGames(std::function<void(bool success)> callback) {
    if (mFetchingGamesInProgress) {
        LOG(LogInfo) << "EAGamesStore: SyncGames - Fetch già in corso.";
        if (callback) callback(false);
        return;
    }
    if (mActiveScrapeCounter > 0) {
        LOG(LogWarning) << "EAGamesStore: SyncGames - Sincronizzazione bloccata, scraping attivo (" << mActiveScrapeCounter << " in corso). Riprovare più tardi.";
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) {
            mWindow->displayNotificationMessage(_("EA Games: Sincronizzazione bloccata da scraping attivo. Riprova."));
        }
        if (callback) callback(false);
        return;
    }

    if (!IsUserLoggedIn()) {
        LOG(LogWarning) << "EAGamesStore::SyncGames - Utente non loggato.";
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Effettua il login per sincronizzare."));
        if (callback) callback(false);
        return;
    }
    LOG(LogInfo) << "EAGamesStore: SyncGames - Avvio fetch...";
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Sincronizzazione giochi EA..."));
    mFetchingGamesInProgress = true;

    std::thread([this, callback] {
        std::vector<EAGames::InstalledGameInfo> installedGames;
        if (mScanner) installedGames = mScanner->scanForInstalledGames();
        else LOG(LogError) << "EAGamesStore::SyncGames - mScanner è null!";

        if (mApi) {
            mApi->getOwnedGames([this, installedGames, callback](std::vector<EAGames::GameEntitlement> onlineGames, bool successApi) {
                mFetchingGamesInProgress = false;
                if (!successApi) {
                    LOG(LogError) << "EAGamesStore: SyncGames - Fallimento fetch giochi online.";
                    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Errore libreria online."));
                    if (callback) {
                        if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(false); });
                        else callback(false);
                    }
                    return;
                }
                processAndCacheGames(onlineGames, installedGames);

                if (callback) {
                     if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(true); });
                     else callback(true);
                }
            });
        } else {
            LOG(LogError) << "EAGamesStore::SyncGames - mApi è null!";
            mFetchingGamesInProgress = false;
            if (callback) {
                if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(false); });
                else callback(false);
            }
        }
    }).detach();
}

void EAGamesStore::StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished) {
    if (mAuth) {
        mAuth->StartLoginFlow([this, onFlowFinished](bool success, const std::string& msg) {
            if (success) {
                 this->mGamesCacheDirty = true;
            }
            if (onFlowFinished) {
                if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, success, msg] { onFlowFinished(success, msg); });
                else onFlowFinished(success, msg);
            }
        });
    } else if (onFlowFinished) {
        std::string errorMsg = _("Modulo Auth EA non inizializzato.");
        if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, errorMsg] { onFlowFinished(false, errorMsg); });
        else onFlowFinished(false, errorMsg);
    }
}

unsigned short EAGamesStore::GetLocalRedirectPort() {
    return EAGames::EAGamesAuth::GetLocalRedirectPort();
}

void EAGamesStore::processAndCacheGames(
    const std::vector<EAGames::GameEntitlement>& onlineGames,
    const std::vector<EAGames::InstalledGameInfo>& installedScannedGames)
{
    LOG(LogInfo) << "EA Store: Processing " << installedScannedGames.size() << " installed, " << onlineGames.size() << " online games from API.";
    mCachedGameFileDatas.clear();
    std::map<std::string, std::unique_ptr<FileData>> gameDataMap;

    SystemData* eaSystem = SystemData::getSystem(EAGamesStore::STORE_ID);

    if (!eaSystem) {
        LOG(LogWarning) << "EA Store: System with STORE_ID '" << EAGamesStore::STORE_ID << "' not found directly. Trying common names for EA system.";
        std::vector<std::string> eaSystemCommonNames = {"eagames", "eapc", "eao", "eaapp", Utils::String::toLower(EAGamesStore::STORE_ID)};
        for (SystemData* sys : SystemData::sSystemVector) {
            const std::string& sysNameLower = Utils::String::toLower(sys->getName());
            if (std::find(eaSystemCommonNames.begin(), eaSystemCommonNames.end(), sysNameLower) != eaSystemCommonNames.end()) {
                eaSystem = sys;
                LOG(LogInfo) << "EA Store: Found EA system using common name: " << sys->getName();
                break;
            }
        }
    }

    if (!eaSystem) {
        LOG(LogError) << "EA Store: System for EA Games (ID: " << EAGamesStore::STORE_ID << " or common names) not found in SystemData vector! Cannot create FileData.";
        mGamesCacheDirty = false;
        rebuildReturnableGameList();
        if (mWindow) mWindow->displayNotificationMessage(_("Sistema EA non configurato o non trovato in EmulationStation! Impossibile mostrare i giochi."));
        return;
    }

    for (const auto& installedInfo : installedScannedGames) {
        if (installedInfo.id.empty()) {
            LOG(LogWarning) << "EA Store: Installed game found with empty ID. Skipping. Name: " << installedInfo.name;
            continue;
        }
        std::string fdPath = installedInfo.executablePath;
        if (fdPath.empty() || !Utils::FileSystem::exists(fdPath)) {
            fdPath = installedInfo.installPath;
            if (fdPath.empty() || !Utils::FileSystem::isAbsolute(fdPath)) {
                 LOG(LogWarning) << "EA Store: Installed game '" << installedInfo.name << "' has invalid or non-existent path. Skipping. Path: " << fdPath;
                 continue;
            }
        }
        std::string uniqueGameKey = installedInfo.id;
        auto fd = std::make_unique<FileData>(FileType::GAME, fdPath, eaSystem);
        fd->getMetadata().set(MetaDataId::Name, installedInfo.name.empty() ? ("EA Game " + uniqueGameKey) : installedInfo.name);
        fd->getMetadata().set(MetaDataId::Installed, "true");
        fd->getMetadata().set(MetaDataId::IsOwned, "false");

        fd->getMetadata().set(MetaDataId::EaOfferId, installedInfo.id);
        if (!installedInfo.multiplayerId.empty())
             fd->getMetadata().set(MetaDataId::EaMultiplayerId, installedInfo.multiplayerId);
        fd->getMetadata().set(MetaDataId::InstallDir, installedInfo.installPath);

        gameDataMap[uniqueGameKey] = std::move(fd);
    }

    for (const auto& entitlement : onlineGames) {
        std::string uniqueGameKey = entitlement.originOfferId;
        if (uniqueGameKey.empty()) {
            LOG(LogWarning) << "EA Store: Online entitlement found with empty originOfferId. Title: " << entitlement.title << ". Skipping.";
            continue;
        }

        if (gameDataMap.count(uniqueGameKey)) {
            FileData* fd_ptr = gameDataMap[uniqueGameKey].get();
            fd_ptr->getMetadata().set(MetaDataId::IsOwned, "true");

            std::string currentName = fd_ptr->getMetadata().get(MetaDataId::Name);
            if (!entitlement.title.empty() && (currentName.empty() || Utils::String::startsWith(currentName, "EA Game "))) {
                fd_ptr->getMetadata().set(MetaDataId::Name, entitlement.title);
            }
            fd_ptr->getMetadata().set(MetaDataId::EaOfferId, entitlement.originOfferId);
            if (!entitlement.productId.empty()) {
                 fd_ptr->getMetadata().set(MetaDataId::EaMasterTitleId, entitlement.productId);
            }
        } else {
            std::string virtualPath = "ea://game/" + uniqueGameKey;
            auto fd = std::make_unique<FileData>(FileType::GAME, virtualPath, eaSystem);

            std::string gameName = entitlement.title;
            if (gameName.empty()) gameName = "EA Game (" + uniqueGameKey + ")";

            fd->getMetadata().set(MetaDataId::Name, gameName);
            fd->getMetadata().set(MetaDataId::Installed, "false");
            fd->getMetadata().set(MetaDataId::IsOwned, "true");
            fd->getMetadata().set(MetaDataId::EaOfferId, entitlement.originOfferId);
            if (!entitlement.productId.empty()) {
                 fd->getMetadata().set(MetaDataId::EaMasterTitleId, entitlement.productId);
            }
            gameDataMap[uniqueGameKey] = std::move(fd);
        }
    }

    for (auto& pair : gameDataMap) {
        mCachedGameFileDatas.push_back(std::move(pair.second));
    }

    std::sort(mCachedGameFileDatas.begin(), mCachedGameFileDatas.end(),
        [](const std::unique_ptr<FileData>& a, const std::unique_ptr<FileData>& b) {
        return Utils::String::toLower(a->getName()) < Utils::String::toLower(b->getName());
    });

    mGamesCacheDirty = false;
    rebuildReturnableGameList();
    LOG(LogInfo) << "EA Store: Cached " << mCachedGameFileDatas.size() << " EA games processed.";
}

void EAGamesStore::rebuildReturnableGameList() {
    mReturnableGameList.clear();
    mReturnableGameList.reserve(mCachedGameFileDatas.size());
    for (const auto& fd_ptr : mCachedGameFileDatas) {
        mReturnableGameList.push_back(fd_ptr.get());
    }
}

void EAGamesStore::GetGameArtwork(const FileData* game, const std::string& artworkType, ArtworkFetchedCallbackStore callback) {
    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Richiesta artwork di tipo '" << artworkType << "' per il gioco: " << (game ? game->getName() : "GIOCO_NULLO");

    if (!game) {
        LOG(LogError) << "EAGamesStore::GetGameArtwork - Puntatore al gioco (game) è nullo.";
        if (callback) callback("", false);
        return;
    }

    std::string gameNameForLog = game->getName();
    std::string offerId = game->getMetadata().get(MetaDataId::EaOfferId);
    std::string masterId = game->getMetadata().get(MetaDataId::EaMasterTitleId);
    std::string idToUseForApi = !offerId.empty() ? offerId : masterId;

    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Info gioco: Nome='" << gameNameForLog << "', OfferID='" << offerId << "', MasterID='" << masterId << "', ID da usare='" << idToUseForApi << "'";

    if (!mApi) {
        LOG(LogError) << "EAGamesStore::GetGameArtwork - mApi (unique_ptr) è nullo per il gioco: " << gameNameForLog;
        if (callback) callback("", false);
        return;
    }
    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - mApi unique_ptr è valido. Procedo con Settings.";

    if (idToUseForApi.empty()) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Nessun ID API (OfferID/MasterID) disponibile per il gioco: " << gameNameForLog;
        if (callback) callback("", false);
        return;
    }

    Settings* settings = Settings::getInstance();
    if (!settings) {
        LOG(LogError) << "EAGamesStore::GetGameArtwork - FATALE: Settings::getInstance() ha restituito null!";
        if (callback) callback("", false);
        return;
    }
    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Istanza Settings ottenuta. Acquisisco country/locale.";

    std::string country;
    try {
        country = settings->getString("ThemeRegion");
    } catch (const std::exception& e) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Eccezione leggendo ThemeRegion: " << e.what() << ". Uso fallback 'US'.";
        country = "US";
    } catch (...) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Eccezione sconosciuta leggendo ThemeRegion. Uso fallback 'US'.";
        country = "US";
    }
    if (country.empty() || country.length() != 2) country = "US";

    std::string langLocale;
    try {
        langLocale = settings->getString("Language");
    } catch (const std::exception& e) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Eccezione leggendo Language: " << e.what() << ". Uso fallback 'en_US'.";
        langLocale = "en_US";
    } catch (...) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Eccezione sconosciuta leggendo Language. Uso fallback 'en_US'.";
        langLocale = "en_US";
    }
    std::string apiLocale = langLocale;
    if (apiLocale.length() == 2) apiLocale = Utils::String::toLower(apiLocale) + "_" + Utils::String::toUpper(country);
    else if (apiLocale.empty()) apiLocale = "en_US";

    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Paese finale: " << country << ", Locale API finale: " << apiLocale;

    auto apiCallbackAdapter = [artworkType, callback, gameNameCapture = gameNameForLog](EAGames::GameStoreData metadata, bool success) mutable {
        if (success && (!metadata.title.empty() || !metadata.imageUrl.empty() || !metadata.backgroundImageUrl.empty())) {
            std::string url;
            if (artworkType == "boxart" || artworkType == "image") url = metadata.imageUrl;
            else if (artworkType == "background" || artworkType == "fanart") url = metadata.backgroundImageUrl;

            if (!url.empty()) {
                LOG(LogInfo) << "EAGamesStore::GetGameArtwork - Trovato artwork '" << artworkType << "' per '" << gameNameCapture << "': " << url;
                if (callback) callback(url, true);
            } else {
                LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Tipo artwork '" << artworkType << "' non trovato nei metadati per '" << gameNameCapture << "'.";
                if (callback) callback("", false);
            }
        } else {
            LOG(LogError) << "EAGamesStore::GetGameArtwork - Chiamata API fallita o nessun dato utile per '" << gameNameCapture << "'. Successo API: " << success;
            if (callback) callback("", false);
        }
    };

    LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Chiamo API per artwork per ID '" << idToUseForApi << "', gioco '" << gameNameForLog << "'.";
    if (!offerId.empty()) {
        mApi->getOfferStoreData(offerId, country, apiLocale, apiCallbackAdapter);
    } else if (!masterId.empty()) {
        mApi->getMasterTitleStoreData(masterId, country, apiLocale, apiCallbackAdapter);
    }
    return;
}

void EAGamesStore::GetGameMetadata(const FileData* game, MetadataFetchedCallbackStore callback) {
    LOG(LogError) << "!!!! EAGamesStore::GetGameMetadata - INIZIO FUNZIONE !!!!";

    if (game == nullptr) {
        LOG(LogError) << "EAGamesStore::GetGameMetadata - ERRORE CRITICO: 'game' è nullptr.";
        if (callback) callback({}, false);
        return;
    }

    LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Puntatore 'game' NON è nullptr. Indirizzo: " << static_cast<const void*>(game);

    std::string gameNameStr;
    std::string offerIdStr;
    std::string masterIdStr;

    try {
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Tentativo di accedere a game->getName()...";
        gameNameStr = game->getName();
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - game->getName() OK. Nome: " << gameNameStr;

        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Tentativo di accedere a game->getMetadata()...";
        const auto& metadata = game->getMetadata();
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - metadata.get(EaOfferId) OK. OfferID: " << offerIdStr;
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Tentativo di get EaMasterTitleId...";
        masterIdStr = metadata.get(MetaDataId::EaMasterTitleId);
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - metadata.get(EaMasterTitleId) OK. MasterID: " << masterIdStr;
    } catch (const std::exception& e) {
        LOG(LogError) << "EAGamesStore::GetGameMetadata - ECCEZIONE durante l'accesso ai metadati iniziali: " << e.what();
        if (callback) callback({}, false);
        return;
    } catch (...) {
        LOG(LogError) << "EAGamesStore::GetGameMetadata - ECCEZIONE SCONOSCIUTA durante l'accesso ai metadati iniziali!";
        if (callback) callback({}, false);
        return;
    }

    std::string gameNameForLog = gameNameStr;
    std::string initialOfferId = offerIdStr;
    std::string initialMasterId = masterIdStr;
    std::string idToUseForApi = !initialOfferId.empty() ? initialOfferId : initialMasterId;

    LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Info gioco post-controlli: Nome='" << gameNameForLog << "', OfferID='" << initialOfferId << "', MasterID='" << initialMasterId << "', ID da usare='" << idToUseForApi << "'";

    if (!mApi) {
        LOG(LogError) << "EAGamesStore::GetGameMetadata - mApi (unique_ptr) è nullo per il gioco: " << gameNameForLog;
        if (callback) callback({}, false);
        return;
    }
    LOG(LogDebug) << "EAGamesStore::GetGameMetadata - mApi unique_ptr è valido. Procedo con Settings.";

    if (idToUseForApi.empty()) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Nessun ID API (OfferID/MasterTitleId) disponibile per il gioco: " << gameNameForLog;
        if (callback) callback({}, false);
        return;
    }

    Settings* settings = Settings::getInstance();
    if (!settings) {
        LOG(LogError) << "EAGamesStore::GetGameMetadata - FATALE: Settings::getInstance() ha restituito null!";
        if (callback) callback({}, false);
        return;
    }
    LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Istanza Settings ottenuta. Acquisisco country/locale.";

    std::string country;
    try {
        country = settings->getString("ThemeRegion");
    } catch (const std::exception& e) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Eccezione leggendo ThemeRegion: " << e.what() << ". Uso fallback 'US'.";
        country = "US";
    } catch (...) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Eccezione sconosciuta leggendo ThemeRegion. Uso fallback 'US'.";
        country = "US";
    }
    if (country.empty() || country.length() != 2) country = "US";

    std::string langLocale;
    try {
        langLocale = settings->getString("Language");
    } catch (const std::exception& e) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Eccezione leggendo Language: " << e.what() << ". Uso fallback 'en_US'.";
        langLocale = "en_US";
    } catch (...) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Eccezione sconosciuta leggendo Language. Uso fallback 'en_US'.";
        langLocale = "en_US";
    }
    std::string apiLocale = langLocale;
    if (apiLocale.length() == 2) apiLocale = Utils::String::toLower(apiLocale) + "_" + Utils::String::toUpper(country);
    else if (apiLocale.empty()) apiLocale = "en_US";

    LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Paese finale: " << country << ", Locale API finale: " << apiLocale;

    // Usiamo un shared_ptr per gestire lo stato tra le chiamate asincrone
    struct MetadataFetchState {
        EAGamesStore::EAGameData resultData;
        std::string initialGameName;
        MetadataFetchedCallbackStore finalCallback;
        std::string country;
        std::string locale;
    };
    auto state = std::make_shared<MetadataFetchState>();
    state->initialGameName = gameNameForLog;
    state->finalCallback = callback;
    state->country = country;
    state->locale = apiLocale;
    state->resultData.id = idToUseForApi;
    state->resultData.name = gameNameForLog;
    state->resultData.offerId = initialOfferId;
    state->resultData.masterTitleId = initialMasterId;


    // Callback per la prima e unica chiamata API
    auto primaryApiCallback = [this, state](EAGames::GameStoreData storeApiData, bool success) {
        if (success && (!storeApiData.title.empty() || !storeApiData.masterTitleId.empty() || !storeApiData.offerId.empty())) {
            state->resultData.offerId = storeApiData.offerId;
            state->resultData.masterTitleId = storeApiData.masterTitleId;
            state->resultData.name = storeApiData.title;

            // Questi campi rimarranno vuoti poiché non sono forniti dalla query attuale
            state->resultData.description = "";
            state->resultData.developer = "";
            state->resultData.publisher = "";
            state->resultData.releaseDate = "";
            state->resultData.genre = "";
            state->resultData.imageUrl = "";
            state->resultData.backgroundUrl = "";

            LOG(LogInfo) << "EAGamesStore::GetGameMetadata (PrimaryAPIAdapter) - Metadati recuperati con successo per '" << state->initialGameName << "'. Titolo: " << state->resultData.name;
            if (state->finalCallback) state->finalCallback(state->resultData, true);
        } else {
            LOG(LogError) << "EAGamesStore::GetGameMetadata (PrimaryAPIAdapter) - Chiamata API fallita o nessun dato utile per '" << state->initialGameName << "'. Successo API: " << success;
            if (state->finalCallback) state->finalCallback(state->resultData, false);
        }
    };

    // Esegui la prima e unica chiamata API
    if (!initialOfferId.empty()) {
        mApi->getOfferStoreData(initialOfferId, country, apiLocale, primaryApiCallback);
    } else if (!initialMasterId.empty()) {
        // Se disponibile solo MasterID, usa getMasterTitleStoreData, ma sappi che fornisce solo dati base ora
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata: OfferID vuoto, usando MasterID per la chiamata: " << initialMasterId;
        mApi->getMasterTitleStoreData(initialMasterId, country, apiLocale, primaryApiCallback);
    } else {
        LOG(LogError) << "EAGamesStore::GetGameMetadata: Impossibile avviare lo scraping, OfferID e MasterID sono entrambi vuoti.";
        if (callback) callback({}, false);
    }
}
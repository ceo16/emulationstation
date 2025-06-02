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

#include <algorithm>
#include <map>
#include <thread>

const std::string EAGamesStore::STORE_ID = "EAGamesStore";

EAGamesStore::EAGamesStore(Window* window)
    : mWindow(window),
      mAuth(std::make_unique<EAGames::EAGamesAuth>(mWindow)),
      mApi(std::make_unique<EAGames::EAGamesAPI>(mAuth.get())),
      mScanner(std::make_unique<EAGames::EAGamesScanner>()),
      mGamesCacheDirty(true),
      mFetchingGamesInProgress(false)
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
    // Il tuo codice esistente per showStoreUI va bene
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
    // Il tuo codice esistente per launchGame va bene
    // Nota: gameId qui è probabilmente l'OfferID o un ID che il client EA capisce
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
    // L'URI scheme "origin://" potrebbe essere obsoleto. "ea://", "eadt://", o "electron://" sono usati da EA App.
    // Dovrai determinare l'URI corretto per lanciare giochi con EA App.
    // Esempio (ipotetico, da verificare): "eadt://launch?offerid=" + gameId + "&pid=" + mAuth->getPidId();
    std::string uri = "origin://launchgame/" + gameId; // <<< POTREBBE ESSERE DA CAMBIARE PER EA APP
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
    // Il tuo codice esistente per getGamesList va bene
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
    // Il tuo codice esistente per installGame va bene (ma vedi nota su URI scheme per launchGame)
    LOG(LogInfo) << "EAGamesStore: Attempting to install game with ID: " << gameId;
    #ifndef _WIN32
        LOG(LogError) << "EAGamesStore: Installazione supportata solo su Windows."; return false;
    #endif
    if (gameId.empty()) return false;
    std::string uri = "origin://downloadgame/" + gameId; // <<< POTREBBE ESSERE DA CAMBIARE PER EA APP
    std::string command = "explorer.exe \"" + uri + "\"";
    if (Utils::Platform::ProcessStartInfo(command).run() == 0) {
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Apertura client EA per installare ") + "(" + gameId + ")");
        return true;
    }
    return false;
}

bool EAGamesStore::uninstallGame(const std::string& gameId) {
    // Il tuo codice esistente per uninstallGame va bene (ma vedi nota su URI scheme per launchGame)
    LOG(LogInfo) << "EAGamesStore: Attempting to uninstall game with ID: " << gameId;
    #ifndef _WIN32
        LOG(LogError) << "EAGamesStore: Disinstallazione supportata solo su Windows."; return false;
    #endif
    if (gameId.empty()) return false;
    std::string uri = "origin://uninstall/" + gameId; // <<< POTREBBE ESSERE DA CAMBIARE PER EA APP
    std::string command = "explorer.exe \"" + uri + "\"";
     if (Utils::Platform::ProcessStartInfo(command).run() == 0) {
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Apertura client EA per disinstallare ") + "(" + gameId + ")");
        mGamesCacheDirty = true;
        return true;
    }
    return false;
}

bool EAGamesStore::updateGame(const std::string& gameId) {
    // Il tuo codice esistente per updateGame va bene
    LOG(LogWarning) << "EAGamesStore: updateGame for ID " << gameId << " - Client gestisce aggiornamenti.";
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Aggiornamenti gestiti dal client EA."));
    return launchGame(gameId); // Prova a lanciare il gioco, il client EA dovrebbe gestire l'aggiornamento se necessario
}

bool EAGamesStore::IsUserLoggedIn() {
    return mAuth && mAuth->isUserLoggedIn();
}

void EAGamesStore::Login(std::function<void(bool success, const std::string& message)> callback) {
    // Il tuo codice esistente per Login va bene
    if (mAuth) {
        mAuth->StartLoginFlow([this, callback](bool flowSuccess, const std::string& flowMessage) { 
            if (flowSuccess) this->mGamesCacheDirty = true;
            if (callback) { // Invia sempre al thread UI
                if (this->mWindow) this->mWindow->postToUiThread([callback, flowSuccess, flowMessage] { callback(flowSuccess, flowMessage); });
                else callback(flowSuccess, flowMessage); // Fallback
            }
        });
    } else if (callback) {
        // Assicurati che anche questo callback venga eseguito sul thread UI se necessario
        std::string errorMsg = _("Modulo Auth EA non inizializzato.");
        if (this->mWindow) this->mWindow->postToUiThread([callback, errorMsg] { callback(false, errorMsg); });
        else callback(false, errorMsg); // Fallback
    }
}

void EAGamesStore::Logout() {
    // Il tuo codice esistente per Logout va bene
    if (mAuth) mAuth->logout();
    mCachedGameFileDatas.clear();
    rebuildReturnableGameList();
    mGamesCacheDirty = true;
    if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("Logout EA effettuato."));
    if (mWindow && ViewController::get()) ViewController::get()->reloadAll(mWindow);
}

void EAGamesStore::GetUsername(std::function<void(const std::string& username)> callback) {
    // Il tuo codice esistente per GetUsername va bene
    if (IsUserLoggedIn() && mAuth) {
        // Usa mAuth->getUserName() se l'hai popolato in EAGamesAuth::fetchUserIdentity
        // std::string displayName = mAuth->getUserName();
        // if (callback) callback(!displayName.empty() ? displayName : _("Utente EA"));
        // Oppure continua a usare PID se preferisci per ora:
        std::string pid = mAuth->getPidId();
        if (callback) {
            std::string usernameToShow = !pid.empty() ? (mAuth->getUserName().empty() ? "EA User (PID: " + pid + ")" : mAuth->getUserName()) : _("Utente EA");
            if (this->mWindow) this->mWindow->postToUiThread([callback, usernameToShow] { callback(usernameToShow); });
            else callback(usernameToShow); // Fallback
        }
    } else if (callback) {
        if (this->mWindow) this->mWindow->postToUiThread([callback] { callback(""); });
        else callback(""); // Fallback
    }
}

void EAGamesStore::SyncGames(std::function<void(bool success)> callback) {
    // Il tuo codice esistente per SyncGames (la logica del thread) va bene.
    // Le modifiche al parsing avvengono in EAGamesAPI e EAGamesModels.
    if (mFetchingGamesInProgress) {
        LOG(LogInfo) << "EAGamesStore: SyncGames - Fetch già in corso.";
        if (callback) callback(false); // Considera postToUiThread se il callback aggiorna la UI
        return;
    }
    if (!IsUserLoggedIn()) {
        LOG(LogWarning) << "EAGamesStore: SyncGames - Utente non loggato.";
        if (mWindow && Settings::getInstance()->getBool("StorePopups")) mWindow->displayNotificationMessage(_("EA Games: Effettua il login per sincronizzare."));
        if (callback) callback(false); // Considera postToUiThread
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
                    // Callback deve essere eseguito sul thread UI se aggiorna la UI
                    if (callback) {
                        if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(false); });
                        else callback(false);
                    }
                    return;
                }
                processAndCacheGames(onlineGames, installedGames); // Questa ora usa i campi corretti (sotto)
                
                // Il callback finale di SyncGames
                if (callback) {
                     if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(true); });
                     else callback(true);
                }
                // L'aggiornamento della UI è già in getGamesList, ma se SyncGames è chiamato da altre parti, potrebbe servire qui.
                // if (mWindow && ViewController::get()) ViewController::get()->reloadAll(mWindow); // Già fatto da getGamesList
            });
        } else {
            LOG(LogError) << "EAGamesStore::SyncGames - mApi è null!";
            mFetchingGamesInProgress = false;
            if (callback) { // Callback deve essere eseguito sul thread UI
                if (this->mWindow) this->mWindow->postToUiThread([callback]{ callback(false); });
                else callback(false);
            }
        }
    }).detach();
}

void EAGamesStore::StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished) {
    // Il tuo codice esistente per StartLoginFlow va bene
    if (mAuth) {
        mAuth->StartLoginFlow([this, onFlowFinished](bool success, const std::string& msg) {
            if (success) {
                 this->mGamesCacheDirty = true;
            }
            if (onFlowFinished) { // Invia sempre al thread UI
                if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, success, msg] { onFlowFinished(success, msg); });
                else onFlowFinished(success, msg); // Fallback
            }
        });
    } else if (onFlowFinished) {
        std::string errorMsg = _("Modulo Auth EA non inizializzato.");
        if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, errorMsg] { onFlowFinished(false, errorMsg); });
        else onFlowFinished(false, errorMsg); // Fallback
    }
}

unsigned short EAGamesStore::GetLocalRedirectPort() {
    return EAGames::EAGamesAuth::GetLocalRedirectPort();
}

// === VERSIONE MODIFICATA DI processAndCacheGames ===
void EAGamesStore::processAndCacheGames(
    const std::vector<EAGames::GameEntitlement>& onlineGames,
    const std::vector<EAGames::InstalledGameInfo>& installedScannedGames)
{
    LOG(LogInfo) << "EA Store: Processing " << installedScannedGames.size() << " installed, " << onlineGames.size() << " online games from API.";
    mCachedGameFileDatas.clear();
    std::map<std::string, std::unique_ptr<FileData>> gameDataMap;

    SystemData* eaSystem = SystemData::getSystem(EAGamesStore::STORE_ID); // Utilizza STORE_ID definito in EAGamesStore.h

    if (!eaSystem) {
        // Fallback a nomi comuni se l'ID specifico non viene trovato.
        // Questo è una misura di sicurezza; il sistema dovrebbe essere presente se creato dinamicamente correttamente.
        LOG(LogWarning) << "EA Store: System with STORE_ID '" << EAGamesStore::STORE_ID << "' not found directly. Trying common names for EA system.";
        // Assicurati che l'elenco di fallback includa la versione minuscola del tuo STORE_ID
        std::vector<std::string> eaSystemCommonNames = {"eagames", "eapc", "origin", "eaapp", Utils::String::toLower(EAGamesStore::STORE_ID)};
        for (SystemData* sys : SystemData::sSystemVector) { // Itera su sSystemVector definito in SystemData.cpp
            const std::string& sysNameLower = Utils::String::toLower(sys->getName());
            if (std::find(eaSystemCommonNames.begin(), eaSystemCommonNames.end(), sysNameLower) != eaSystemCommonNames.end()) {
                eaSystem = sys;
                LOG(LogInfo) << "EA Store: Found EA system using common name: " << sys->getName();
                break;
            }
        }
    }

    if (!eaSystem) {
        LOG(LogError) << "EA Store: System for EA Games (ID: " << EAGamesStore::STORE_ID << " or common names) not found in SystemData vector! Cannot create FileData."; //
        mGamesCacheDirty = false; 
        rebuildReturnableGameList(); 
        if (mWindow) mWindow->displayNotificationMessage(_("Sistema EA non configurato o non trovato in EmulationStation! Impossibile mostrare i giochi."));
        return;
    }

    // Processa giochi installati (come prima)
    for (const auto& installedInfo : installedScannedGames) {
        if (installedInfo.id.empty()) { // L'ID dello scanner è probabilmente l'OfferID o MasterTitleID
            LOG(LogWarning) << "EA Store: Installed game found with empty ID. Skipping. Name: " << installedInfo.name;
            continue;
        }
        std::string fdPath = installedInfo.executablePath;
        if (fdPath.empty() || !Utils::FileSystem::exists(fdPath)) {
            fdPath = installedInfo.installPath;
            if (fdPath.empty() || !Utils::FileSystem::isAbsolute(fdPath)) { // Assicurati che il percorso sia assoluto se non è un eseguibile
                 LOG(LogWarning) << "EA Store: Installed game '" << installedInfo.name << "' has invalid or non-existent path. Skipping. Path: " << fdPath;
                 continue;
            }
        }
        // Usa installedInfo.id (che dovrebbe essere l'Offer ID o un ID consistente) come chiave
        std::string uniqueGameKey = installedInfo.id; 
        auto fd = std::make_unique<FileData>(FileType::GAME, fdPath, eaSystem);
        fd->getMetadata().set(MetaDataId::Name, installedInfo.name.empty() ? ("EA Game " + uniqueGameKey) : installedInfo.name);
        fd->getMetadata().set(MetaDataId::Installed, "true");
        fd->getMetadata().set(MetaDataId::IsOwned, "false"); // Sarà true se trovato online
        
        // Aggiungi gli ID specifici di EA se disponibili dallo scanner
        fd->getMetadata().set(MetaDataId::EaOfferId, installedInfo.id); // Assumendo che installedInfo.id sia l'OfferID
        // Se lo scanner fornisce anche un MasterTitleID o ProductID, impostalo
        // fd->getMetadata().set(MetaDataId::EaMasterTitleId, installedInfo.masterTitleId_o_productId); 
        if (!installedInfo.multiplayerId.empty())
             fd->getMetadata().set(MetaDataId::EaMultiplayerId, installedInfo.multiplayerId);
        fd->getMetadata().set(MetaDataId::InstallDir, installedInfo.installPath);
        
        gameDataMap[uniqueGameKey] = std::move(fd);
    }

    // Processa giochi online (da GraphQL)
    for (const auto& entitlement : onlineGames) {
        // Dalla struct GameEntitlement aggiornata (in EAGamesModels.h):
        // - entitlement.originOfferId (era offerId)
        // - entitlement.title (era product.name)
        // - entitlement.productId (era product.id)
        // - entitlement.gameSlug (era product.gameSlug)
        // - entitlement.gameType (era product.baseItem.gameType)
        // - masterTitleId e offerPath NON sono più direttamente popolati in GameEntitlement dal parser GraphQL

        std::string uniqueGameKey = entitlement.originOfferId; // Usa originOfferId come chiave primaria
        if (uniqueGameKey.empty()) {
             // Potresti usare entitlement.productId come fallback se originOfferId fosse vuoto,
             // ma originOfferId dovrebbe essere l'identificatore principale per la licenza.
            LOG(LogWarning) << "EA Store: Online entitlement found with empty originOfferId. Title: " << entitlement.title << ". Skipping.";
            continue;
        }

        if (gameDataMap.count(uniqueGameKey)) { // Gioco trovato anche tra quelli installati
            FileData* fd_ptr = gameDataMap[uniqueGameKey].get();
            fd_ptr->getMetadata().set(MetaDataId::IsOwned, "true"); // Ora sappiamo che è posseduto
            
            // Aggiorna il nome se quello online è più completo e quello attuale è un placeholder
            std::string currentName = fd_ptr->getMetadata().get(MetaDataId::Name);
            if (!entitlement.title.empty() && (currentName.empty() || Utils::String::startsWith(currentName, "EA Game "))) {
                fd_ptr->getMetadata().set(MetaDataId::Name, entitlement.title);
            }
            // Aggiorna gli ID specifici di EA
            fd_ptr->getMetadata().set(MetaDataId::EaOfferId, entitlement.originOfferId);
            if (!entitlement.productId.empty()) { // Se abbiamo un Product ID dal parser GraphQL
                 fd_ptr->getMetadata().set(MetaDataId::EaMasterTitleId, entitlement.productId); // Usiamo productId come master/product ID
            }
            // gameSlug e gameType sono ora in entitlement, potresti salvarli se hai campi metadati adatti
            // fd_ptr->getMetadata().set(MetaDataId::EaGameSlug, entitlement.gameSlug);
            // fd_ptr->getMetadata().set(MetaDataId::GameType, entitlement.gameType);

        } else { // Gioco posseduto online ma non trovato tra quelli installati
            std::string virtualPath = "ea://game/" + uniqueGameKey; // Percorso virtuale per giochi non installati
            auto fd = std::make_unique<FileData>(FileType::GAME, virtualPath, eaSystem);
            
            std::string gameName = entitlement.title;
            if (gameName.empty()) gameName = "EA Game (" + uniqueGameKey + ")"; // Fallback name
            
            fd->getMetadata().set(MetaDataId::Name, gameName);
            fd->getMetadata().set(MetaDataId::Installed, "false");
            fd->getMetadata().set(MetaDataId::IsOwned, "true");
            fd->getMetadata().set(MetaDataId::EaOfferId, entitlement.originOfferId);
            if (!entitlement.productId.empty()) {
                 fd->getMetadata().set(MetaDataId::EaMasterTitleId, entitlement.productId);
            }
            // fd->getMetadata().set(MetaDataId::EaGameSlug, entitlement.gameSlug);
            // fd->getMetadata().set(MetaDataId::GameType, entitlement.gameType);

            gameDataMap[uniqueGameKey] = std::move(fd);
        }
    }

    // Popola la cache finale
    for (auto& pair : gameDataMap) {
        mCachedGameFileDatas.push_back(std::move(pair.second));
    }

    // Ordina la lista
    std::sort(mCachedGameFileDatas.begin(), mCachedGameFileDatas.end(),
        [](const std::unique_ptr<FileData>& a, const std::unique_ptr<FileData>& b) {
        return Utils::String::toLower(a->getName()) < Utils::String::toLower(b->getName());
    });

    mGamesCacheDirty = false; // La cache è ora aggiornata
    rebuildReturnableGameList(); // Aggiorna la lista che verrà effettivamente mostrata
    LOG(LogInfo) << "EA Store: Cached " << mCachedGameFileDatas.size() << " EA games processed.";
}

void EAGamesStore::rebuildReturnableGameList() {
    // Il tuo codice esistente per rebuildReturnableGameList va bene
    mReturnableGameList.clear();
    mReturnableGameList.reserve(mCachedGameFileDatas.size());
    for (const auto& fd_ptr : mCachedGameFileDatas) {
        mReturnableGameList.push_back(fd_ptr.get());
    }
}

void EAGamesStore::GetGameArtwork(const FileData* game, const std::string& artworkType, ArtworkFetchedCallbackStore callback) {
    // Il tuo codice esistente per GetGameArtwork va bene
    // Assicurati che MetaDataId::EaOfferId e MetaDataId::EaMasterTitleId usati qui
    // corrispondano a come li stai impostando in processAndCacheGames
    if (!game) { if (callback) callback("", false); return; }
    std::string offerId = game->getMetadata().get(MetaDataId::EaOfferId); // Questo dovrebbe essere originOfferId
    std::string masterId = game->getMetadata().get(MetaDataId::EaMasterTitleId); // Questo dovrebbe essere productId
    
    // Usa l'OfferID (originOfferId) come primario per l'artwork se disponibile
    std::string idToUseForApi = !offerId.empty() ? offerId : masterId; 
    std::string gameNameForLog = game->getName(); 

    if (idToUseForApi.empty() || !mApi) {
        LOG(LogWarning) << "EAGamesStore::GetGameArtwork - No ID or API for game: " << gameNameForLog;
        if (callback) callback("", false); return;
    }
    // ... (resto del tuo codice per GetGameArtwork, assicurati che mApi->getOfferStoreData/getMasterTitleStoreData
    // usino endpoint che funzionano, altrimenti non troveranno artwork)
    // Per ora lascio il resto invariato, ma quegli endpoint API sono probabilmente deprecati.
    std::string country = Settings::getInstance()->getString("ThemeRegion");
    if (country.empty() || country.length() != 2) country = "US";
    std::string langLocale = Settings::getInstance()->getString("Language");
	std::string apiLocale = langLocale;
	if (apiLocale.length() == 2) apiLocale = Utils::String::toLower(apiLocale) + "_" + Utils::String::toUpper(country);
	else if (apiLocale.empty()) apiLocale = "en_US";

    auto apiCallback = [artworkType, callback, gameNameCapture = gameNameForLog](EAGames::GameStoreData metadata, bool success) mutable {
        if (success && (!metadata.title.empty() || !metadata.imageUrl.empty() || !metadata.backgroundImageUrl.empty())) {
            std::string url;
            if (artworkType == "boxart" || artworkType == "image") url = metadata.imageUrl;
            else if (artworkType == "background" || artworkType == "fanart") url = metadata.backgroundImageUrl;
            if (!url.empty()) {
                LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Found " << artworkType << " for " << gameNameCapture << ": " << url;
                if (callback) callback(url, true);
            } else {
                LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Artwork type " << artworkType << " not found in metadata for " << gameNameCapture;
                if (callback) callback("", false);
            }
        } else {
            LOG(LogError) << "EAGamesStore::GetGameArtwork - API call failed or no useful data for " << gameNameCapture;
            if (callback) callback("", false);
        }
    };
    // Le seguenti chiamate a getOfferStoreData/getMasterTitleStoreData useranno gli endpoint di Origin deprecati
    // e probabilmente falliranno. Dovrai trovare endpoint EA App equivalenti per i metadati dei giochi.
    if (!offerId.empty()) { // Qui offerId è l'originOfferId
        LOG(LogDebug) << "EAGamesStore::GetGameArtwork - Using OfferID " << offerId << " for game " << gameNameForLog;
        mApi->getOfferStoreData(offerId, country, apiLocale, apiCallback);
    } else if (!masterId.empty()) { // Qui masterId è il productId
        LOG(LogDebug) << "EAGamesStore::GetGameArtwork - OfferID empty, using MasterTitleID " << masterId << " for game " << gameNameForLog;
        mApi->getMasterTitleStoreData(masterId, country, apiLocale, apiCallback);
    } else {
         LOG(LogWarning) << "EAGamesStore::GetGameArtwork - Both OfferID and MasterTitleID are empty for " << gameNameForLog;
         if (callback) callback("", false);
    }
}

void EAGamesStore::GetGameMetadata(const FileData* game, MetadataFetchedCallbackStore callback) {
    // Il tuo codice esistente per GetGameMetadata va bene
    // Stesse considerazioni di GetGameArtwork per gli ID e gli endpoint API
    if (!game) { if (callback) callback({}, false); return; }
    std::string offerId = game->getMetadata().get(MetaDataId::EaOfferId);
    std::string masterId = game->getMetadata().get(MetaDataId::EaMasterTitleId);
    std::string idToUseForApi = !offerId.empty() ? offerId : masterId;
    std::string gameNameForLog = game->getName();

    if (idToUseForApi.empty() || !mApi) {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - No ID or API for game: " << gameNameForLog;
        if (callback) callback({}, false); return;
    }
    // ... (resto del tuo codice, soggetto agli stessi problemi di endpoint deprecati per i metadati) ...
    std::string country = Settings::getInstance()->getString("ThemeRegion");
	if (country.empty() || country.length() != 2) country = "US";
    std::string langLocale = Settings::getInstance()->getString("Language");
	std::string apiLocale = langLocale;
	if (apiLocale.length() == 2) apiLocale = Utils::String::toLower(apiLocale) + "_" + Utils::String::toUpper(country);
	else if (apiLocale.empty()) apiLocale = "en_US";

    auto apiCallback = [callback, gameNameCapture = gameNameForLog](EAGames::GameStoreData storeApiData, bool success) mutable {
        EAGameData resultData; // Assicurati che EAGameData sia definita da qualche parte
        if (success && (!storeApiData.title.empty() || !storeApiData.masterTitleId.empty() || !storeApiData.offerId.empty())) {
            resultData.id = !storeApiData.masterTitleId.empty() ? storeApiData.masterTitleId : storeApiData.offerId;
            resultData.name = storeApiData.title;
            resultData.description = storeApiData.description;
            resultData.developer = storeApiData.developer;
            resultData.publisher = storeApiData.publisher;
            resultData.releaseDate = storeApiData.releaseDate;
            if (!storeApiData.genres.empty()) resultData.genre = Utils::String::vectorToCommaString(storeApiData.genres);
            resultData.imageUrl = storeApiData.imageUrl;
            resultData.backgroundUrl = storeApiData.backgroundImageUrl;
            LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Successfully fetched metadata for " << gameNameCapture;
            if (callback) callback(resultData, true);
        } else {
            LOG(LogError) << "EAGamesStore::GetGameMetadata - API call failed or no useful data for " << gameNameCapture;
            if (callback) callback(resultData, false); // Restituisci resultData vuoto ma indica fallimento
        }
    };
     // Le seguenti chiamate a getOfferStoreData/getMasterTitleStoreData useranno gli endpoint di Origin deprecati
    if (!offerId.empty()) {
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - Using OfferID " << offerId << " for game " << gameNameForLog;
        mApi->getOfferStoreData(offerId, country, apiLocale, apiCallback);
    } else if (!masterId.empty()) {
        LOG(LogDebug) << "EAGamesStore::GetGameMetadata - OfferID empty, using MasterTitleID " << masterId << " for game " << gameNameForLog;
        mApi->getMasterTitleStoreData(masterId, country, apiLocale, apiCallback);
    } else {
        LOG(LogWarning) << "EAGamesStore::GetGameMetadata - Both OfferID and MasterTitleID are empty for " << gameNameForLog;
        if (callback) callback({}, false);
    }
}

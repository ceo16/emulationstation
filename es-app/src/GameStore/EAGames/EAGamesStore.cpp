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
#include "Gamelist.h"
#include "views/gamelist/IGameListView.h"
#include "FileSorts.h"

#include <algorithm>
#include <map>
#include <thread>
#include <memory> // Per std::make_shared
#include <vector>

namespace {
    int levenshtein_distance(const std::string &s1, const std::string &s2) {
        const std::size_t len1 = s1.size(), len2 = s2.size();
        std::vector<unsigned int> col(len2 + 1), prevCol(len2 + 1);
        for (unsigned int i = 0; i < prevCol.size(); i++) prevCol[i] = i;
        for (unsigned int i = 0; i < len1; i++) {
            col[0] = i + 1;
            for (unsigned int j = 0; j < len2; j++)
                col[j + 1] = std::min({ prevCol[j + 1] + 1, col[j] + 1, prevCol[j] + (s1[i] == s2[j] ? 0 : 1) });
            col.swap(prevCol);
        }
        return prevCol[len2];
    }

    std::string normalizeGameName(const std::string& name) {
        std::string lowerName = Utils::String::toLower(name);
        std::string result = "";
        for (char c : lowerName) {
            if (isalnum(static_cast<unsigned char>(c))) result += c;
        }
        result = Utils::String::replace(result, "gameoftheyearedition", "");
        result = Utils::String::replace(result, "goty", "");
        result = Utils::String::replace(result, "edition", "");
        if (result.find("piantecontrozombi") != std::string::npos || result.find("plantsvszombies") != std::string::npos) {
            return "plantsvszombies";
        }
        if (result.find("easportsfc24") != std::string::npos) {
            return "easportsfc24";
        }
        return result;
    }
}

const std::string EAGamesStore::STORE_ID = "EAGamesStore";

EAGamesStore::EAGamesStore(Window* window)
    : GameStore(),
      mWindow(window),
      mAuth(std::make_unique<EAGames::EAGamesAuth>(mWindow)),
      mApi(std::make_unique<EAGames::EAGamesAPI>(mAuth.get())),
      mScanner(std::make_unique<EAGames::EAGamesScanner>()),
      mGamesCacheDirty(true),
      mFetchingGamesInProgress(false),
      mSubscriptionDetailsInitialized(false),
      mSubscriptionDetailsFetching(false),
      mEAPlayCatalogInitialized(false)
{}

EAGamesStore::~EAGamesStore() {}

std::vector<EAGames::InstalledGameInfo> EAGamesStore::getInstalledGames()
{
    if (mScanner) 
        return mScanner->scanForInstalledGames();
    
    return {};
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
    return STORE_ID;
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
    if (IsUserLoggedIn() && mGamesCacheDirty && !mFetchingGamesInProgress) {
        LOG(LogInfo) << "EAGamesStore: Cache is dirty, initiating sync.";
        SyncGames([this](bool success){
            if (success) ViewController::get()->reloadAll(mWindow, false);
        });
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

void EAGamesStore::getSubscriptionDetails(std::function<void(const EAGames::SubscriptionDetails& details)> callback)
{
    // Controlla prima la cache in modo sicuro
    {
        std::lock_guard<std::mutex> lock(mCacheMutex);
        if (mSubscriptionDetailsInitialized) {
            if (callback) {
                // Posta sempre sul thread UI per sicurezza, non costa nulla
                mWindow->postToUiThread([callback, details = mSubscriptionDetails] { callback(details); });
            }
            return;
        }

        // Se un'altra richiesta è già partita, non fare nulla.
        // La richiesta originale notificherà tutti quando avrà finito.
        // NOTA: Questa logica semplice "perde" le richieste successive. Per ora va bene per risolvere il bug.
        if (mSubscriptionDetailsFetching) {
            return;
        }
        
        // Imposta il flag per bloccare altre richieste
        mSubscriptionDetailsFetching = true;
    }

    // Esegui la chiamata API
    mApi->getSubscriptions([this, callback](EAGames::SubscriptionDetails details, bool success) {
        // Questa callback viene eseguita sul thread della UI (grazie ad EAGamesAPI)
        LOG(LogDebug) << "Store Callback: Ricevuto da API. Successo: " << (success ? "true" : "false") << ". Valore di details.isActive: " << (details.isActive ? "true" : "false");

        // Aggiorna la cache
        std::lock_guard<std::mutex> lock(mCacheMutex);
        mSubscriptionDetails = success ? details : EAGames::SubscriptionDetails{};
        mSubscriptionDetailsInitialized = true;
        mSubscriptionDetailsFetching = false; // Sblocca per richieste future

        // Esegui il callback originale passato dalla UI
        if (callback) {
            // Non c'è bisogno di ri-postare, siamo già sul thread corretto.
            callback(mSubscriptionDetails);
        }
    });
}


void EAGamesStore::getEAPlayCatalog(std::function<void(const std::vector<EAGames::SubscriptionGame>& catalog)> callback) {
    {
        std::lock_guard<std::mutex> lock(mCacheMutex);
        if (mEAPlayCatalogInitialized) {
            if (callback) mWindow->postToUiThread([this, callback] { callback(mEAPlayCatalog); });
            return;
        }
    }

    getSubscriptionDetails([this, callback](const EAGames::SubscriptionDetails& details) {
        if (!details.isActive) {
            LOG(LogInfo) << "Nessun abbonamento EA Play attivo, il catalogo è vuoto.";
            std::lock_guard<std::mutex> lock(mCacheMutex);
            mEAPlayCatalogInitialized = true;
            mEAPlayCatalog.clear();
            mEAPlayOfferIds.clear();
            if (callback) mWindow->postToUiThread([callback] { callback({}); });
            return;
        }

        mApi->getSubscriptionGameSlugs(details.tier, [this, callback](std::vector<std::string> allSlugs, bool success) {
            if (!success || allSlugs.empty()) {
                LOG(LogError) << "Fallito il recupero degli slug del catalogo EA Play.";
                if (callback) mWindow->postToUiThread([callback] { callback({}); });
                return;
            }

            //================================================================
            // --- INIZIO NUOVA LOGICA DI PAGINAZIONE (CHUNKING) ---
            //================================================================

            // 1. Prepara le variabili per gestire le chiamate multiple
            auto allSubscriptionGames = std::make_shared<std::vector<EAGames::SubscriptionGame>>();
            auto outstandingCalls = std::make_shared<std::atomic<int>>(0);
            auto resultsMutex = std::make_shared<std::mutex>();
            const size_t slugLimit = 99; // Limite imposto da EA (usiamo 99 per sicurezza)

            // Controlla se ci sono slug da processare
            if (allSlugs.empty()) {
                if (callback) mWindow->postToUiThread([callback] { callback({}); });
                return;
            }

            // 2. Itera sulla lista di slug creando pacchetti (chunk)
            for (size_t i = 0; i < allSlugs.size(); i += slugLimit) {
                (*outstandingCalls)++; // Incrementa il contatore per ogni chiamata che stiamo per fare

                // Crea il pacchetto di slug
                auto start_it = allSlugs.begin() + i;
                auto end_it = allSlugs.begin() + std::min(i + slugLimit, allSlugs.size());
                std::vector<std::string> slugChunk(start_it, end_it);

                // 3. Chiama l'API con il pacchetto di slug corrente
                mApi->getGamesDetailsBySlug(slugChunk, [this, callback, allSubscriptionGames, outstandingCalls, resultsMutex](std::vector<EAGames::SubscriptionGame> gamesChunk, bool chunkSuccess) {
                    
                    if (chunkSuccess) {
                        // Se la chiamata ha successo, aggiungi i risultati al vettore finale in modo sicuro
                        std::lock_guard<std::mutex> lock(*resultsMutex);
                        allSubscriptionGames->insert(allSubscriptionGames->end(), gamesChunk.begin(), gamesChunk.end());
                    }

                    // Decrementa il contatore perché questa chiamata è finita
                    (*outstandingCalls)--; 

                    // 4. Se questa era l'ultima chiamata in sospeso, abbiamo finito
                    if (*outstandingCalls == 0) {
                        LOG(LogInfo) << "[PAGINAZIONE] Tutte le chiamate per i dettagli del catalogo EA Play sono terminate. Giochi totali recuperati: " << allSubscriptionGames->size();
                        
                        // Ora possiamo procedere con il caching e il callback finale
                        std::lock_guard<std::mutex> lock(*resultsMutex);
                        mEAPlayCatalog = *allSubscriptionGames;
                        mEAPlayOfferIds.clear();

                        for(const auto& game : mEAPlayCatalog) { 
                            mEAPlayOfferIds.insert(game.offerId); 
                        }
                        mEAPlayCatalogInitialized = true;

                        // Esegui il callback finale sul thread principale
                        if (callback) mWindow->postToUiThread([callback, finalCatalog = mEAPlayCatalog] { callback(finalCatalog); });
                    }
                });
            }
            //================================================================
            // --- FINE NUOVA LOGICA DI PAGINAZIONE (CHUNKING) ---
            //================================================================
        });
    });
}


void EAGamesStore::fetchDetailsForGames(
    const std::vector<EAGames::GameEntitlement>& onlineGames,
    const std::vector<EAGames::SubscriptionGame>& catalogGames,
    std::function<void(bool, const std::map<std::string, EAGames::GameStoreData>&)> on_complete)
{
    std::set<std::string> uniqueOfferIds;
    for (const auto& game : onlineGames) {
        if (!game.originOfferId.empty()) uniqueOfferIds.insert(game.originOfferId);
    }
    for (const auto& game : catalogGames) {
        if (!game.offerId.empty()) uniqueOfferIds.insert(game.offerId);
    }

    if (uniqueOfferIds.empty()) {
        if (on_complete) mWindow->postToUiThread([on_complete] { on_complete(true, {}); });
        return;
    }

    LOG(LogInfo) << "[fetchDetails] Fetching details for " << uniqueOfferIds.size() << " unique games.";

    auto detailedDataMap = std::make_shared<std::map<std::string, EAGames::GameStoreData>>();
    auto outstandingCalls = std::make_shared<std::atomic<int>>(uniqueOfferIds.size());
    auto resultsMutex = std::make_shared<std::mutex>();

    for (const std::string& offerId : uniqueOfferIds) {
        mApi->getOfferStoreData(offerId, "", "", 
            [this, offerId, detailedDataMap, outstandingCalls, resultsMutex, on_complete]
            (EAGames::GameStoreData details, bool success) {
            
            if (success && !details.contentId.empty()) {
                std::lock_guard<std::mutex> lock(*resultsMutex);
                detailedDataMap->emplace(offerId, details);
            }
            
            if (--(*outstandingCalls) == 0) {
                LOG(LogInfo) << "[fetchDetails] Detail fetching complete.";
                mWindow->postToUiThread([on_complete, detailedDataMap] { 
                    on_complete(true, *detailedDataMap); 
                });
            }
        });
    }
}


void EAGamesStore::SyncGames(std::function<void(bool success)> callback) {
    if (mFetchingGamesInProgress || !IsUserLoggedIn()) {
        if (callback) callback(false);
        return;
    }
    mFetchingGamesInProgress = true;
    LOG(LogInfo) << "[SyncGames] Starting Phase 1: Fetching base lists...";

    auto ownedGames = std::make_shared<std::vector<EAGames::GameEntitlement>>();
    auto catalogGames = std::make_shared<std::vector<EAGames::SubscriptionGame>>();
    auto callsDone = std::make_shared<std::atomic<int>>(0);

    auto onPhase1Complete = [this, ownedGames, catalogGames, callback]() {
        LOG(LogInfo) << "[SyncGames] Phase 1 complete. Owned: " << ownedGames->size() << ", Catalog: " << catalogGames->size();
        
        fetchDetailsForGames(*ownedGames, *catalogGames, 
            [this, ownedGames, catalogGames, callback]
            (bool detailsSuccess, const std::map<std::string, EAGames::GameStoreData>& detailedDataMap) {
            
            if (detailsSuccess) {
                LOG(LogInfo) << "[SyncGames] Phase 2 complete. Merging and caching...";
                auto installedGames = mScanner->scanForInstalledGames();
                processAndCacheGames(*ownedGames, installedGames, *catalogGames, detailedDataMap);
            }
            
            mFetchingGamesInProgress = false;
            if (callback) mWindow->postToUiThread([callback, detailsSuccess] { callback(detailsSuccess); });
        });
    };

    // Chiamata per i giochi posseduti
    mApi->getOwnedGames([ownedGames, callsDone, onPhase1Complete](std::vector<EAGames::GameEntitlement> result, bool success) {
        if (success) *ownedGames = result;
        if (++(*callsDone) == 2) onPhase1Complete();
    });

    // Chiamata per i giochi del catalogo
    if (Settings::getInstance()->getBool("EAPlay.Enabled")) {
        getEAPlayCatalog([catalogGames, callsDone, onPhase1Complete](const std::vector<EAGames::SubscriptionGame>& result) {
            *catalogGames = result;
            if (++(*callsDone) == 2) onPhase1Complete();
        });
    } else {
        if (++(*callsDone) == 2) onPhase1Complete();
    }
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


// FASE 3: Usa tutti i dati raccolti per costruire la lista finale
void EAGamesStore::processAndCacheGames(
    const std::vector<EAGames::GameEntitlement>& onlineGames,
    const std::vector<EAGames::InstalledGameInfo>& installedGames,
    const std::vector<EAGames::SubscriptionGame>& catalogGames,
    const std::map<std::string, EAGames::GameStoreData>& detailedDataMap)
{
    LOG(LogInfo) << "[processAndCacheGames] Merging: " << installedGames.size() << " installed, " 
                 << onlineGames.size() << " owned, " << catalogGames.size() << " catalog, "
                 << detailedDataMap.size() << " details fetched.";

    mCachedGameFileDatas.clear();
    SystemData* eaSystem = SystemData::getSystem(STORE_ID);
    if (!eaSystem) return;

    std::map<std::string, std::unique_ptr<FileData>> processedGames;

	// --- PRIORITÀ 1: GIOCHI INSTALLATI ---
	// La logica di fuzzy matching cerca di associare i giochi installati localmente
	// con le voci della libreria online per ottenere metadati accurati.
	auto onlineGamesPool = onlineGames; // Copia per poter rimuovere elementi

	for (const auto& installedInfo : installedGames) {
		std::string normalizedLocalName = normalizeGameName(installedInfo.name);
		double bestMatchScore = 0.85; // Soglia di corrispondenza
		auto bestMatchIterator = onlineGamesPool.end();

		// Cerca la migliore corrispondenza nella libreria online
		for (auto it = onlineGamesPool.begin(); it != onlineGamesPool.end(); ++it) {
			std::string normalizedOnlineName = normalizeGameName(it->title);
			double score = 1.0 - (double)levenshtein_distance(normalizedLocalName, normalizedOnlineName) / std::max(normalizedLocalName.length(), normalizedOnlineName.length());
			if (score > bestMatchScore) {
				bestMatchScore = score;
				bestMatchIterator = it;
			}
		}

		if (bestMatchIterator != onlineGamesPool.end()) {
			// --- CORRISPONDENZA TROVATA ---
			LOG(LogInfo) << "[processAndCacheGames] Matched installed game '" << installedInfo.name << "' with library game '" << bestMatchIterator->title << "'";
			
			std::string offerId = bestMatchIterator->originOfferId;
			auto fd = std::make_unique<FileData>(FileType::GAME, "ea_installed:/" + offerId, eaSystem);
			MetaDataList& mdl = fd->getMetadata();
			
			mdl.set(MetaDataId::Name, bestMatchIterator->title);
			mdl.set(MetaDataId::Installed, "true");
			mdl.set(MetaDataId::Virtual, "false");
			mdl.set("ea_offerid", offerId);
			mdl.set("ea_mastertitleid", bestMatchIterator->productId);
			mdl.set("gameid", offerId);
			
			// Per un gioco installato, il comando di lancio punta al suo eseguibile locale.
			mdl.set(MetaDataId::LaunchCommand, "\"" + installedInfo.executablePath + "\" " + installedInfo.launchParameters);
			
			// Arricchiamo comunque i metadati con il contentId per completezza.
			auto detailIt = detailedDataMap.find(offerId);
			if (detailIt != detailedDataMap.end()) {
				mdl.set("ea_contentid", detailIt->second.contentId);
			}
			
			mdl.set("eaplay", "false"); 
			
			processedGames[offerId] = std::move(fd);
			onlineGamesPool.erase(bestMatchIterator); // Rimuovi per non riprocessarlo dopo.

		} else {
			// --- NESSUNA CORRISPONDENZA TROVATA ---
			// Aggiungiamo il gioco usando solo le informazioni locali.
			LOG(LogWarning) << "[processAndCacheGames] No library match for installed game '" << installedInfo.name << "'";
			auto fd = std::make_unique<FileData>(FileType::GAME, "ea_installed_local:/" + (installedInfo.id.empty() ? installedInfo.name : installedInfo.id), eaSystem);
			fd->getMetadata().set(MetaDataId::Name, installedInfo.name);
			fd->getMetadata().set(MetaDataId::Installed, "true");
			fd->getMetadata().set(MetaDataId::LaunchCommand, "\"" + installedInfo.executablePath + "\" " + installedInfo.launchParameters);
			processedGames[installedInfo.name] = std::move(fd);
		}
	}

    // GIOCHI POSSEDUTI (non installati)
    for (const auto& entitlement : onlineGames) {
        if (processedGames.find(entitlement.originOfferId) != processedGames.end()) continue;

        auto fd = std::make_unique<FileData>(FileType::GAME, "ea_virtual:/" + entitlement.originOfferId, eaSystem);
        MetaDataList& mdl = fd->getMetadata();
        mdl.set(MetaDataId::Name, entitlement.title);
        mdl.set(MetaDataId::Installed, "false");
        mdl.set(MetaDataId::Virtual, "true");
        mdl.set("ea_offerid", entitlement.originOfferId);
        
        std::string launchId = "";
        auto it = detailedDataMap.find(entitlement.originOfferId);
        if (it != detailedDataMap.end()) launchId = it->second.contentId;
        
        mdl.set(MetaDataId::LaunchCommand, "origin2://game/launch?offerIds=" + launchId);
        processedGames[entitlement.originOfferId] = std::move(fd);
    }

    // GIOCHI DEL CATALOGO EA PLAY
    for (const auto& catalogGame : catalogGames) {
        if (processedGames.find(catalogGame.offerId) == processedGames.end()) {
            auto fd = std::make_unique<FileData>(FileType::GAME, "eaplay:/" + catalogGame.offerId, eaSystem);
            MetaDataList& mdl = fd->getMetadata();
            mdl.set(MetaDataId::Name, catalogGame.name);
            mdl.set(MetaDataId::Installed, "false");
            mdl.set(MetaDataId::Virtual, "true");
            mdl.set("ea_offerid", catalogGame.offerId);
            mdl.set("eaplay", "true");

            std::string launchId = "";
            auto it = detailedDataMap.find(catalogGame.offerId);
            if (it != detailedDataMap.end()) launchId = it->second.contentId;

            mdl.set(MetaDataId::LaunchCommand, "origin2://game/launch?offerIds=" + launchId);
            processedGames[catalogGame.offerId] = std::move(fd);
        }
    }

    for (auto& pair : processedGames) {
        mCachedGameFileDatas.push_back(std::move(pair.second));
    }

    rebuildAndSortCache();
    updateGamelist(eaSystem);
}

void EAGamesStore::rebuildAndSortCache() {
    std::sort(mCachedGameFileDatas.begin(), mCachedGameFileDatas.end(),
        [](const std::unique_ptr<FileData>& a, const std::unique_ptr<FileData>& b) {
        return Utils::String::toLower(a->getName()) < Utils::String::toLower(b->getName());
    });
    mGamesCacheDirty = false;
    rebuildReturnableGameList();
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
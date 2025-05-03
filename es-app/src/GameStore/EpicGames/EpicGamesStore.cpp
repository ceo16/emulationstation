#include "GameStore/EpicGames/EpicGamesStore.h"
#include "Log.h"
#include "guis/GuiMsgBox.h"
#include "utils/Platform.h"
#include "../../es-core/src/Window.h"
#include "GameStore/EpicGames/EpicGamesUI.h"
#include "services/HttpServerThread.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h> // Per API Registro e Conversione Stringhe
#include <ShlObj.h>  // Per SHGetFolderPath
#include <sstream>
#include "json.hpp"
#include "../../es-app/src/FileData.h"
#include "../../es-app/src/SystemData.h"
#include "utils/StringUtil.h"
#include <locale>   // <<< AGGIUNGI
#include <codecvt>  // <<< AGGIUNGI (anche se non usiamo più wstring_convert)
#include "GameStore/EpicGames/EpicGamesAuth.h" // Necessario per EpicGamesAuth
#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "guis/GuiMsgBox.h"
#include <thread>   // Per std::thread
#include <chrono>   // Per std::chrono
#include "SystemData.h" // Per accedere a SystemData
#include "FileData.h"   // Per accedere a FileData
#include "MetaData.h"   // Per MetaDataId
#include "GameStore/EpicGames/EpicGamesModels.h"
#include <map>
#include <set> // Useremo un set per cercare velocemente gli installati
#include "utils/FileSystemUtil.h" 
#include "ApiSystem.h" 
#include "HttpReq.h"
#include "views/ViewController.h"
#include "views/gamelist/IGameListView.h"
#include "GameStore/EpicGames/EpicGamesParser.h" // <--- Assicurati che ci sia!


using json = nlohmann::json;
namespace fs = std::filesystem; // Alias per filesyste

// Funzione da eseguire in background
void updateEpicGamesMetadata(Window* window, EpicGamesStoreAPI* api) { // Accetta direttamente EpicGamesAuth*
    if (!window || !api) {
    LOG(LogError) << "updateEpicGamesMetadata: Window or API pointer is null.";
    return;
}

    LOG(LogInfo) << "Background Epic Metadata Update Thread started.";

    // Pausa iniziale per non sovraccaricare l'avvio
    std::this_thread::sleep_for(std::chrono::seconds(15));

    // Verifica autenticazione (modo semplice tramite token)
    EpicGamesAuth* auth = api->getAuth(); // ===> NUOVO: Assumiamo esista un getter per mAuth in API
    if (auth->getAccessToken().empty()) {
     LOG(LogWarning) << "BG_Meta: Not authenticated. Exiting metadata update thread.";
     return;
}

    std::vector<EpicGames::Asset> assets;
    std::map<std::string, std::string> catalogIdToAppName;
    try {
        LOG(LogInfo) << "BG_Meta: Calling GetAllAssetsAsync...";
        std::future<std::vector<EpicGames::Asset>> assetsFuture = api->GetAllAssetsAsync();
        assets = assetsFuture.get();
        LOG(LogInfo) << "BG_Meta: Found " << assets.size() << " assets.";
        if (assets.empty()) return;

        // Popola mappa per riferimento futuro
        for(const auto& asset : assets) {
             if (!asset.catalogItemId.empty() && !asset.appName.empty()) {
                 catalogIdToAppName[asset.catalogItemId] = asset.appName;
             }
        }

    } catch (const std::exception& e) {
        LOG(LogError) << "BG_Meta: Exception fetching assets: " << e.what();
        return;
    }

    // Prepara lista per GetCatalogItems
    std::vector<std::pair<std::string, std::string>> itemsToFetch;
    itemsToFetch.reserve(assets.size());
    for (const auto& asset : assets) {
        if (!asset.ns.empty() && !asset.catalogItemId.empty()) {
            itemsToFetch.push_back({asset.ns, asset.catalogItemId});
        }
    }
    if (itemsToFetch.empty()) { LOG(LogInfo) << "BG_Meta: No valid items to fetch catalog data for."; return; }

    // Recupera Dati Catalogo
    std::map<std::string, EpicGames::CatalogItem> catalogItems;
    try {
        LOG(LogInfo) << "BG_Meta: Calling GetCatalogItemsAsync for " << itemsToFetch.size() << " items...";
        std::future<std::map<std::string, EpicGames::CatalogItem>> itemsFuture = api->GetCatalogItemsAsync(itemsToFetch);
        catalogItems = itemsFuture.get();
        LOG(LogInfo) << "BG_Meta: Received " << catalogItems.size() << " catalog item results.";
    } catch (const std::exception& e) {
        LOG(LogError) << "BG_Meta: Exception fetching catalog items: " << e.what();
        // Potremmo decidere di continuare con i dati parziali, ma per ora usciamo se fallisce
        return;
    }
    if (catalogItems.empty()) { LOG(LogWarning) << "BG_Meta: No catalog item data retrieved."; return; }


    // --- Qui inizia la nuova logica di aggiornamento FileData ---

    LOG(LogInfo) << "BG_Meta: Starting metadata application to FileData...";

    // Ottieni il SystemData per epicgamestore
    SystemData* epicSystem = SystemData::getSystem("epicgamestore");
    if (!epicSystem) {
        LOG(LogError) << "BG_Meta: Could not find 'epicgamestore' SystemData.";
        return;
    }

    // Crea una mappa AppName -> FileData* per accesso rapido
    std::map<std::string, FileData*> fileDataMap;
    std::vector<FileData*> gameList = epicSystem->getRootFolder()->getFilesRecursive(GAME); // Ottieni tutti i FileData
    for (FileData* fd : gameList) {
        // Usiamo EpicId (che corrisponde ad AppName) come chiave
        std::string epicId = fd->getMetadata().get(MetaDataId::EpicId);
        if (!epicId.empty()) {
            fileDataMap[epicId] = fd;
        }
    }
    LOG(LogDebug) << "BG_Meta: Created map with " << fileDataMap.size() << " FileData entries.";


    bool metadataChanged = false; // Flag per sapere se salvare gamelist

    // Itera sui risultati del catalogo ottenuti
    for (auto const& [catalogId, catalogItem] : catalogItems) {
        // Trova l'AppName corrispondente a questo CatalogId usando la mappa creata prima
        auto appNameIt = catalogIdToAppName.find(catalogId);
        if (appNameIt == catalogIdToAppName.end()) {
             LOG(LogWarning) << "BG_Meta: Could not find AppName for CatalogID: " << catalogId;
             continue;
        }
        std::string appName = appNameIt->second;

        // Trova il FileData corrispondente usando l'AppName
        auto fdIt = fileDataMap.find(appName);
        if (fdIt == fileDataMap.end()) {
             LOG(LogWarning) << "BG_Meta: Could not find FileData for AppName: " << appName << " (CatalogID: " << catalogId << ")";
             continue; // Salta se non troviamo il FileData
        }
        FileData* gameFile = fdIt->second;
        LOG(LogDebug) << "BG_Meta: Processing metadata for: " << gameFile->getName() << " (App: " << appName << ")";

        // --- Applica i Metadati al FileData ---
        // Controlla se il dato è cambiato prima di impostarlo per ottimizzare
        // (get() restituisce stringa vuota se non esiste)

        if (gameFile->getMetadata().get(MetaDataId::Name) != catalogItem.title && !catalogItem.title.empty()) {
            gameFile->setMetadata(MetaDataId::Name, catalogItem.title); metadataChanged = true;
        }
        if (gameFile->getMetadata().get(MetaDataId::Desc) != catalogItem.description && !catalogItem.description.empty()) {
            gameFile->setMetadata(MetaDataId::Desc, catalogItem.description); metadataChanged = true;
        }
        if (gameFile->getMetadata().get(MetaDataId::Developer) != catalogItem.developer && !catalogItem.developer.empty()) {
            gameFile->setMetadata(MetaDataId::Developer, catalogItem.developer); metadataChanged = true;
        }
        if (gameFile->getMetadata().get(MetaDataId::Publisher) != catalogItem.publisher && !catalogItem.publisher.empty()) {
            gameFile->setMetadata(MetaDataId::Publisher, catalogItem.publisher); metadataChanged = true;
        }

        // Gestione Data Rilascio (parsing ISO 8601 -> YYYYMMDD T000000)
        if (!catalogItem.releaseDate.empty()) {
            std::string currentReleaseDate = gameFile->getMetadata().get(MetaDataId::ReleaseDate);
            time_t releaseTime = Utils::Time::iso8601ToTime(catalogItem.releaseDate); // Nuova funzione helper da aggiungere a TimeUtil
            std::string newReleaseDateStr = Utils::Time::timeToMetaDataString(releaseTime); // Converte in YYYYMMDD T000000
            if (currentReleaseDate != newReleaseDateStr) {
                gameFile->setMetadata(MetaDataId::ReleaseDate, newReleaseDateStr); metadataChanged = true;
                LOG(LogDebug) << "  Updated ReleaseDate to: " << newReleaseDateStr;
            }
        }

        // TODO: Gestione Genere (da catalogItem.categories)
        // std::string genres = ...; // Estrai/concatena da categories
        // if (gameFile->getMetadata().get(MetaDataId::Genre) != genres) {
        //     gameFile->setMetadata(MetaDataId::Genre, genres); metadataChanged = true;
        // }

        // TODO: Gestione Rating (da customAttributes?) - Richiede parsing e conversione in float 0.0-1.0
        // float rating = ...;
        // if (gameFile->getMetadata().getFloat(MetaDataId::Rating) != rating) {
        //    gameFile->setMetadata(MetaDataId::Rating, std::to_string(rating)); metadataChanged = true;
        // }

        // TODO: Gestione Players (da customAttributes?)
        // std::string players = ...;
        // if (gameFile->getMetadata().get(MetaDataId::Players) != players) {
        //     gameFile->setMetadata(MetaDataId::Players, players); metadataChanged = true;
        // }


        // --- Gestione Immagini (Semplificata - TODO: Download Asincrono) ---
        std::string imageUrl = "";
        std::string imageType = "";
        // Cerca un'immagine adatta (es. wide o thumbnail)
        for (const auto& img : catalogItem.keyImages) {
            if (img.type == "OfferImageWide" || img.type == "DieselStoreFrontWide" || img.type == "VaultClosed" || img.type == "Thumbnail") { // Aggiunti altri tipi comuni
                 imageUrl = img.url;
                 imageType = img.type;
                 break;
            }
        }
        if (imageUrl.empty() && !catalogItem.keyImages.empty()) { // Fallback alla prima
             imageUrl = catalogItem.keyImages[0].url;
             imageType = catalogItem.keyImages[0].type;
        }

        if (!imageUrl.empty()) {
             LOG(LogDebug) << "  Found image URL (" << imageType << "): " << imageUrl;
             // Per ora, salviamo solo l'URL. Il download va implementato.
             // TODO: Implementare download asincrono di 'imageUrl' in un percorso locale
             //       e poi salvare il percorso locale in MetaDataId::Image.
             // Esempio concettuale:
             // std::string localImagePath = getLocalMediaPathForGame(gameFile, "image"); // Funzione helper
             // startAsyncDownload(imageUrl, localImagePath, [gameFile, localImagePath, &metadataChanged](bool success) {
             //     if (success && gameFile->getMetadata().get(MetaDataId::Image) != localImagePath) {
             //         gameFile->setMetadata(MetaDataId::Image, localImagePath);
             //         metadataChanged = true; // Attenzione: accesso concorrente a metadataChanged! Serve mutex o atomico.
             //     }
             // });

             // SALVATAGGIO TEMPORANEO URL IN UN CAMPO CUSTOM (per debug)
             // if (gameFile->getMetadata().get("EpicImageURL") != imageUrl) { // Usa un nome custom
             //     gameFile->setMetadata("EpicImageURL", imageUrl); metadataChanged = true;
             // }
        }

        // TODO: Gestione Video (simile a immagini)

    } // Fine loop sui catalogItems

    // 7. Salva gamelist.xml SE qualcosa è cambiato
    if (metadataChanged) {
        LOG(LogInfo) << "BG_Meta: Metadata changed, updating gamelist for " << epicSystem->getName();
        // TODO: Assicurarsi che questo sia thread-safe o venga chiamato nel thread UI
        // NotificationManager::getInstance()->Subject("update-gamelist")->Notify(epicSystem); // Notifica per UI
        // SystemData::instance()->writeMetaData(); // Salva tutti i gamelist
        // O forse: epicSystem->getRootFolder()->saveToGamelistRecovery(); // Salva solo recovery
        // Potrebbe essere necessario un meccanismo più robusto per salvare da un thread background.
        LOG(LogWarning) << "BG_Meta: Gamelist saving from background thread not fully implemented yet!";
    } else {
        LOG(LogInfo) << "BG_Meta: No metadata changes detected.";
    }

    LOG(LogInfo) << "Background Epic Metadata Update Thread finished.";
}

// --- getEpicLauncherConfigPath() DEFINED OUTSIDE the class ---
std::string getEpicLauncherConfigPath() {
    return "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\EMS\\EpicGamesLauncher";
}

EpicGamesStore::EpicGamesStore(EpicGamesAuth* auth)
  : mAPI(nullptr), mUI(), mAuth(auth), mWindow(nullptr), _initialized(false) // Inizializza mAPI a nullptr
 {
  LOG(LogDebug) << "EpicGamesStore: Constructor (with auth)";
  if (mAuth == nullptr) {
  LOG(LogError) << "EpicGamesStore: Received null auth pointer in constructor!";
  // Gestisci l'errore se necessario, es. non creare mAPI
  } else {
  // Crea mAPI qui, passando mAuth
  mAPI = new EpicGamesStoreAPI(mAuth); // <<< CREA mAPI con new, passando mAuth
  if (mAPI == nullptr) {
  LOG(LogError) << "EpicGamesStore: Failed to allocate EpicGamesStoreAPI!";
  }
  }
  // testFindInstalledGames(); // Forse chiamare dopo init?
 }

 EpicGamesStore::EpicGamesStore()
  : mAPI(nullptr), mUI(), mAuth(nullptr), mWindow(nullptr), _initialized(false) // Inizializza mAPI a nullptr
 {
  LOG(LogWarning) << "EpicGamesStore: Constructor (default) - mAuth is null!";
  // Se usi questo costruttore, mAuth sarà nullo. Come ottieni l'auth?
  // Potresti creare mAPI qui con nullptr, ma le chiamate API falliranno.
  // O potresti ritardare la creazione di mAPI a quando l'auth è disponibile.
  // ATTENZIONE: Questa configurazione richiede che mAuth sia impostato altrove prima di usare mAPI.
  // È più sicuro usare SOLO il costruttore che richiede mAuth.
  mAPI = new EpicGamesStoreAPI(nullptr); // Crea con nullptr, ma non funzionerà senza auth
  // testFindInstalledGames();
 }

 EpicGamesStore::~EpicGamesStore() {
  LOG(LogDebug) << "EpicGamesStore: Destructor";
  shutdown();
  delete mAPI; // <<< CANCELLA mAPI nel distruttore
  mAPI = nullptr; // Buona norma impostare a nullptr dopo delete
 }

bool EpicGamesStore::init(Window* window) {
    // ... (codice esistente che assicura la creazione di mAPI) ...

    _initialized = true;
    LOG(LogDebug) << "EpicGamesStore: Initialization successful (API object ready).";

    // --- CODICE DI TEST AGGIORNATO per GetAllAssets e GetCatalogItems ---
    if (mAPI && mAuth && !mAuth->getAccessToken().empty()) {
        LOG(LogInfo) << "[TEST] Eseguendo test API Epic Games...";
        try {
            // 1. Chiama GetAllAssets
            LOG(LogInfo) << "[TEST] Chiamando GetAllAssets()...";
            std::vector<EpicGames::Asset> assets = mAPI->GetAllAssets();
            LOG(LogInfo) << "[TEST] GetAllAssets() terminata. Asset trovati: " << assets.size();

            if (!assets.empty()) {
                // 2. Prepara la lista di item da recuperare per GetCatalogItems
                std::vector<std::pair<std::string, std::string>> itemsToFetch;
                itemsToFetch.reserve(assets.size()); // Riserva spazio
                for (const auto& asset : assets) {
                    // Assicurati che namespace e catalogItemId non siano vuoti
                    if (!asset.ns.empty() && !asset.catalogItemId.empty()) {
                        itemsToFetch.push_back({asset.ns, asset.catalogItemId});
                        LOG(LogDebug) << "  [TEST] Preparato per fetch: NS=" << asset.ns << ", CatalogID=" << asset.catalogItemId << " (App: " << asset.appName << ")";
                    } else {
                        LOG(LogWarning) << "  [TEST] Asset saltato (NS o CatalogID vuoto): AppName=" << asset.appName;
                    }
                }

                // 3. Chiama GetCatalogItems se ci sono item validi da recuperare
                if (!itemsToFetch.empty()) {
                    LOG(LogInfo) << "[TEST] Chiamando GetCatalogItems() per " << itemsToFetch.size() << " items... (potrebbe richiedere tempo)";
                    // Chiama la versione sincrona per test, passando la lista
                    std::map<std::string, EpicGames::CatalogItem> catalogItems = mAPI->GetCatalogItems(itemsToFetch); // Usa i default per country/locale
                    LOG(LogInfo) << "[TEST] GetCatalogItems() terminata. Risultati ottenuti: " << catalogItems.size();

                    // 4. Logga i dettagli dei risultati ottenuti
                    for (const auto& pair : catalogItems) {
                        const std::string& catalogId = pair.first;
                        const EpicGames::CatalogItem& item = pair.second;
                        LOG(LogInfo) << "  [TEST] Risultato per CatalogID: " << catalogId;
                        LOG(LogInfo) << "      Titolo:       " << item.title;
                        LOG(LogInfo) << "      Descrizione:  " << (item.description.length() > 100 ? item.description.substr(0, 100) + "..." : item.description); // Tronca descrizione lunga
                        LOG(LogInfo) << "      Sviluppatore: " << item.developer;
                        LOG(LogInfo) << "      Editore:    " << item.publisher;
                        LOG(LogInfo) << "      Data Ril.:  " << item.releaseDate;
                        // Logga URL della prima immagine trovata (es. Thumbnail o OfferImageWide)
                        std::string imageUrl = "N/A";
                        for (const auto& img : item.keyImages) {
                             if (img.type == "OfferImageWide" || img.type == "Thumbnail" || img.type == "DieselStoreFrontWide") { // Cerca tipi comuni
                                 imageUrl = img.url;
                                 LOG(LogDebug) << "      Immagine trovata (" << img.type << "): " << imageUrl;
                                 break; // Prendi la prima utile
                             }
                        }
                         if (imageUrl == "N/A" && !item.keyImages.empty()) imageUrl = item.keyImages[0].url; // Fallback alla prima immagine
                         LOG(LogInfo) << "      URL Immagine: " << imageUrl;

                    }
                } else {
                    LOG(LogWarning) << "[TEST] Nessun item valido da passare a GetCatalogItems.";
                }
            } else {
                LOG(LogWarning) << "[TEST] GetAllAssets() non ha restituito asset, impossibile testare GetCatalogItems.";
            }

        } catch (const std::exception& e) {
            LOG(LogError) << "[TEST] Eccezione durante il test API: " << e.what();
        }
    } else {
         LOG(LogWarning) << "[TEST] Salto test API - API non pronta o non autenticato.";
    }
    // --- FINE CODICE DI TEST AGGIORNATO ---

    return true; // Ritorno originale della funzione init
 }

void EpicGamesStore::showStoreUI(Window* window) {
    LOG(LogDebug) << "EpicGamesStore: Showing store UI";
    mUI.showMainMenu(window, this);
}

std::string EpicGamesStore::getStoreName() const {
    return "EpicGamesStore";
}

 void EpicGamesStore::shutdown() {
  LOG(LogDebug) << "EpicGamesStore: Shutting down";
  _initialized = false;

 }
std::string EpicGamesStore::getGameLaunchUrl(const EpicGames::Asset& asset) const
{
    if (asset.ns.empty() || asset.catalogItemId.empty() || asset.appName.empty()) {
         return "com.epicgames.launcher://apps/" + HttpReq::urlEncode(asset.appName) + "?action=launch&silent=true"; // Fallback
    }
    return "com.epicgames.launcher://apps/" +
        HttpReq::urlEncode(asset.ns) + "%3A" +             // <--- MODIFICATO
        HttpReq::urlEncode(asset.catalogItemId) + "%3A" +  // <--- MODIFICATO
        HttpReq::urlEncode(asset.appName) +                // <--- MODIFICATO
        "?action=launch&silent=true";+
        "?action=launch&silent=true";
}

// Usa il nome corretto per entry.extension() se diverso! (es. getExtension()?)
// Usa il tipo corretto EpicGames::Asset
bool EpicGamesStore::checkInstallationStatus(const EpicGames::Asset& asset)
{
     try {
        const std::string manifestFolder = "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests";
        if (!Utils::FileSystem::exists(manifestFolder)) { return false; }
        for (const Utils::FileSystem::FileInfo& entry : Utils::FileSystem::getDirectoryFiles(manifestFolder)) {
            std::string fileExtension = Utils::FileSystem::getExtension(entry.path); // Ottieni l'estensione usando la funzione statica
if (Utils::String::toLower(fileExtension) == ".item") { // Confronta l'estensione ottenuta
                 std::string filePath = entry.path;
                 try {
                     std::ifstream f(filePath);
                     if (!f.is_open()) continue;
                     nlohmann::json manifestData = nlohmann::json::parse(f, nullptr, false);
                     f.close();
                     if (manifestData.is_discarded() || !manifestData.is_object()) continue;
                     if (manifestData.contains("AppName") && manifestData["AppName"].is_string() &&
                         manifestData["AppName"].get<std::string>() == asset.appName) {
                         if (/* ... controlli installazione come prima ... */ true ) { return true; } else { return false; }
                     }
                 } catch (...) { /* Gestione errori */ }
            }
        }
    } catch(...) { /* Gestione errori */ }
    return false;
}

// --------- FUNZIONE getGamesList() CORRETTA (?) ---------
std::vector<FileData*> EpicGamesStore::getGamesList()
{
    LOG(LogDebug) << "EpicGamesStore::getGamesList() - STARTING (Shows ALL owned games)";
    std::vector<FileData*> gameList;

    if (!mAPI || !mAuth || mAuth->getAccessToken().empty()) { /* Log e return */ return gameList;}

    LOG(LogDebug) << "EpicGamesStore::getGamesList() - Calling GetAllAssets...";
    std::vector<EpicGames::Asset> ownedAssets = mAPI->GetAllAssets();
    if (ownedAssets.empty()) { /* Log e return */ return gameList; }
    LOG(LogDebug) << "EpicGamesStore::getGamesList() - Fetched " << ownedAssets.size() << " owned assets from API.";

    LOG(LogDebug) << "EpicGamesStore::getGamesList() - Calling getInstalledEpicGamesWithDetails...";
    std::vector<EpicGamesStore::EpicGameInfo> installedGamesDetails = getInstalledEpicGamesWithDetails();
    LOG(LogDebug) << "EpicGamesStore::getGamesList() - Found " << installedGamesDetails.size() << " installed games via helper.";
    std::map<std::string, EpicGamesStore::EpicGameInfo> installedGamesMap;
    for (const auto& installedGame : installedGamesDetails) { /* ... popola mappa ... */ }

    // --- Usa mWindow->getCurrentSystem() per ottenere SystemData* ---
    SystemData* system = nullptr;
    if (mWindow) {
        system = mWindow->getCurrentSystem(); // <--- RIPRISTINATO
    }
    if (!system) {
        LOG(LogError) << "EpicGamesStore::getGamesList() - Could not get current SystemData via mWindow! Trying static fallback...";
        system = SystemData::getSystem("epicgamestore"); // Usa il nome corretto
         if (!system) {
             LOG(LogError) << "EpicGamesStore::getGamesList() - Static fallback failed! Aborting.";
             return gameList;
         }
    }
    // --- Fine ottenimento SystemData* ---

    LOG(LogDebug) << "EpicGamesStore::getGamesList() - Processing all " << ownedAssets.size() << " owned assets...";
    for (const auto& asset : ownedAssets)
    {
        if (asset.appName.empty()) { continue; }
        LOG(LogDebug) << "Processing asset: AppName=" << asset.appName;

        bool isInstalled = checkInstallationStatus(asset);
        std::string gameDisplayName = asset.appName;
        std::string launchPath = "";
        std::string installCommandUri = "";

        if (isInstalled) {
            launchPath = getGameLaunchUrl(asset);
            auto installedIter = installedGamesMap.find(asset.appName);
            if (installedIter != installedGamesMap.end() && !installedIter->second.name.empty()) {
                gameDisplayName = installedIter->second.name;
            }
            LOG(LogDebug) << "Game '" << gameDisplayName << "' (AppName: " << asset.appName << ") is installed. Launch URI: " << launchPath;
        } else {
            // Costruisci URI installazione (usa nome corretto per urlEncode!)
                 if (!asset.ns.empty() && !asset.catalogItemId.empty() && !asset.appName.empty()) {
                installCommandUri = "com.epicgames.launcher://apps/";
                installCommandUri += HttpReq::urlEncode(asset.ns) + "%3A";            // <--- MODIFICATO
                installCommandUri += HttpReq::urlEncode(asset.catalogItemId) + "%3A"; // <--- MODIFICATO
                installCommandUri += HttpReq::urlEncode(asset.appName);               // <--- MODIFICATO
                installCommandUri += "?action=install&silent=false";
                launchPath = installCommandUri;
                LOG(LogDebug) << "Game '" << gameDisplayName << "' is NOT installed. Install URI: " << installCommandUri;
            } else {
                launchPath = asset.appName; // Fallback
                LOG(LogWarning) << "Missing IDs for install URI for " << asset.appName << ". Using appName as path.";
            }
        }

        if (launchPath.empty()) { continue; }

        // --- Usa 'system' come terzo argomento per il costruttore ---
        FileData* newGame = new FileData(FileType::GAME, launchPath, system); // <--- CORRETTO

        // --- Usa setMetadata SOLO con MetaDataId ---
        newGame->setMetadata(MetaDataId::Name, gameDisplayName); // OK

        // RIMOSSI tutti i tentativi di usare setMetadata con chiavi stringa
        // RIMOSSI setInstalled / setInstallCommand perché probabilmente non esistono

        // Aggiungi metadati standard qui se/quando li recuperi (Desc, Image, etc.)
        // newGame->setMetadata(MetaDataId::Desc, ...);
        // newGame->setMetadata(MetaDataId::Image, ...);

        gameList.push_back(newGame);
    } // Fine ciclo for

    LOG(LogInfo) << "EpicGamesStore::getGamesList() - END, returning " << gameList.size() << " games (including not installed).";
    return gameList;
}
bool EpicGamesStore::installGame(const std::string& gameId) {
    LOG(LogDebug) << "EpicGamesStore::installGame (placeholder)";
    return true;
}

bool EpicGamesStore::uninstallGame(const std::string& gameId) {
    LOG(LogDebug) << "EpicGamesStore::uninstallGame (placeholder)";
    return true;
}

bool EpicGamesStore::updateGame(const std::string& gameId) {
    LOG(LogDebug) << "EpicGamesStore::updateGame (placeholder)";
    return true;
}

void EpicGamesStore::startLoginFlow() {
    LOG(LogDebug) << "EpicGamesStore::startLoginFlow - Using Alternative Flow (Manual Code Entry)";

    // URL diretto che mostra il codice all'utente dopo il login
    // Usa il Client ID di Playnite
    std::string directAuthUrl = "https://www.epicgames.com/id/api/redirect?clientId=34a02cf8f4414e29b15921876da36f9a&responseType=code";

    LOG(LogInfo) << "Opening browser for Epic Games login at: " << directAuthUrl;
    // Apri l'URL nel browser esterno
    Utils::Platform::openUrl(directAuthUrl);

    // Non facciamo altro qui, l'UI gestirà l'input del codice.
}


void EpicGamesStore::processAuthCode(const std::string& authCode) {
    LOG(LogDebug) << "EpicGamesStore: Processing auth code: " << authCode;
    std::string accessToken; // Variabile per ricevere il token

    // Verifica che mAuth sia valido
    if (!mAuth) {
        LOG(LogError) << "EpicGamesStore::processAuthCode - mAuth is null!";
         if(mWindow) mWindow->pushGui(new GuiMsgBox(mWindow, "Errore interno: Oggetto autenticazione mancante.", "OK"));
        return;
    }

    // Chiama getAccessToken (che ora usa le credenziali Playnite)
    bool success = mAuth->getAccessToken(authCode, accessToken);

    // Informa l'utente del risultato
    if (success && !accessToken.empty()) {
        LOG(LogInfo) << "Epic Login Successful!";
        if(mWindow) mWindow->pushGui(new GuiMsgBox(mWindow, "Login a Epic Games completato con successo!", "OK", [this] {
             // Azione opzionale dopo successo, es. tornare al menu o mostrare libreria
             // showGameList(mWindow, this); // Esempio
        }));
    } else {
        LOG(LogError) << "Epic Login Failed.";
        if(mWindow) mWindow->pushGui(new GuiMsgBox(mWindow, "Login a Epic Games fallito.\nControlla il codice inserito o riprova.", "OK"));
    }
}

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string EpicGamesStore::getLauncherInstalledDatPath() {
    return "C:\\ProgramData\\Epic\\UnrealEngineLauncher\\LauncherInstalled.dat";
}

std::vector<std::string> EpicGamesStore::findInstalledEpicGames() {
    std::vector<std::string> installedGames;
    std::filesystem::path launcherInstalledDatPath = getLauncherInstalledDatPath();

    // 1. Try to read from LauncherInstalled.dat
    if (std::filesystem::exists(launcherInstalledDatPath)) {
        std::ifstream datFile(launcherInstalledDatPath);
        if (datFile.is_open()) {
            LOG(LogDebug) << "Reading game paths from LauncherInstalled.dat";
            std::stringstream datContents;
            std::string datLine;
            while (std::getline(datFile, datLine)) {
                datContents << datLine;
            }
            datFile.close();

            try {
                json parsedData = json::parse(datContents.str());
                if (parsedData.contains("InstallationList")) {  //  Check if "InstallationList" exists
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("InstallLocation")) {  //  Check if "InstallLocation" exists
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            installedGames.push_back(installLocation);
                            LOG(LogInfo) << "  Found game from dat: " << installLocation;
                        } else {
                            LOG(LogWarning) << "  Entry missing InstallLocation, skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found in LauncherInstalled.dat, falling back to directory scan.";
                    installedGames.clear();
                }
            } catch (const json::parse_error& e) {
                LOG(LogError) << "  Error parsing LauncherInstalled.dat: " << e.what();
                LOG(LogWarning) << "  Falling back to directory scan.";
                installedGames.clear();
            }
        } else {
            LOG(LogError) << "  Failed to open LauncherInstalled.dat";
            LOG(LogWarning) << "  Falling back to directory scan.";
        }
    } else {
        LOG(LogWarning) << "  LauncherInstalled.dat not found. Falling back to directory scan.";
    }

    // 2. Fallback to directory scan (if LauncherInstalled.dat failed or was empty)
    if (installedGames.empty()) {
        std::vector<std::string> defaultInstallLocations = {
            "C:\\Program Files\\Epic Games",
            "C:\\Program Files (x86)\\Epic Games"
        };

        for (const auto& installLocation : defaultInstallLocations) {
            if (std::filesystem::exists(installLocation) && std::filesystem::is_directory(installLocation)) {
                LOG(LogDebug) << "  Checking for games in: " << installLocation;
                for (const auto& entry : std::filesystem::directory_iterator(installLocation)) {
                    if (entry.is_directory()) {
                        std::filesystem::path gameDir = entry.path();
                        LOG(LogDebug) << "   Checking directory: " << gameDir;
                        std::filesystem::path egstorePath = gameDir / ".egstore";
                        if (std::filesystem::exists(egstorePath) && std::filesystem::is_directory(egstorePath)) {
                            bool foundGame = false;
                            for (const auto& egstoreEntry : std::filesystem::directory_iterator(egstorePath)) {
                                if (egstoreEntry.is_regular_file() && egstoreEntry.path().extension() == ".manifest") {
                                    LOG(LogInfo) << "    Found game in: " << gameDir << " (using .manifest)";
                                    installedGames.push_back(gameDir.string());
                                    foundGame = true;
                                    break;
                                }
                            }
                            if (!foundGame) {
                                for (const auto& egstoreEntry : std::filesystem::directory_iterator(egstorePath)) {
                                    if (egstoreEntry.is_regular_file() && egstoreEntry.path().extension() == ".mancpn") {
                                        LOG(LogInfo) << "    Found game in: " << gameDir << " (using .mancpn)";
                                        installedGames.push_back(gameDir.string());
                                        foundGame = true;
                                        break;
                                    } else {
                                        LOG(LogDebug) << "    Not a manifest file: " << egstoreEntry.path();
                                    }
                                }
                                if (!foundGame) {
                                    LOG(LogWarning) << "    No .manifest or .mancpn file found in .egstore directory: " << egstorePath;
                                }
                            }
                        }
                    }
                }
            } else {
                LOG(LogWarning) << "  Install location does not exist or is not a directory: " << installLocation;
            }
        }
    }
    return installedGames;
}

std::string EpicGamesStore::getEpicGameId(const std::string& path) {
    std::string gameId = "";
    std::filesystem::path launcherInstalledDatPath = getLauncherInstalledDatPath();

    if (std::filesystem::exists(launcherInstalledDatPath)) {
        std::ifstream datFile(launcherInstalledDatPath);
        if (datFile.is_open()) {
            std::stringstream datContents;
            std::string datLine;
            while (std::getline(datFile, datLine)) {
                datContents << datLine;
            }
            datFile.close();

            try {
                json parsedData = json::parse(datContents.str());
                if (parsedData.contains("InstallationList")) {  //  Check if "InstallationList" exists
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("InstallLocation") && entry.contains("AppName")) {  //  Check if keys exist
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            if (Utils::String::compareIgnoreCase(installLocation, path)) {
                                gameId = entry["AppName"].get<std::string>();
                                break;
                            }
                        } else {
                            LOG(LogWarning) << "  Entry missing InstallLocation or AppName, skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found in LauncherInstalled.dat.";
                }
            } catch (const json::parse_error& e) {
                LOG(LogError) << "Error parsing LauncherInstalled.dat: " << e.what();
            }
        } else {
            LOG(LogError) << "LauncherInstalled.dat not found.";
        }
    }
    return gameId;
}

std::vector<EpicGamesStore::EpicGameInfo> EpicGamesStore::getInstalledEpicGamesWithDetails() {
    LOG(LogDebug) << "EpicGamesStore::getInstalledEpicGamesWithDetails() - START (Using Registry & .item manifests)";
    std::vector<EpicGamesStore::EpicGameInfo> games;
    fs::path launcherInstalledDatPath = getLauncherInstalledDatPath();
    std::string metadataPath = getMetadataPathFromRegistry(); // Usa la nuova funzione per leggere dal registro

    // 1. Read LauncherInstalled.dat to map AppName -> InstallLocation
    std::map<std::string, std::string> installLocations;
    if (fs::exists(launcherInstalledDatPath)) {
        std::ifstream datFile(launcherInstalledDatPath);
        if (datFile.is_open()) {
             LOG(LogDebug) << "EpicGamesStore: Reading " << launcherInstalledDatPath;
            std::stringstream datContents;
            std::string datLine;
            while (std::getline(datFile, datLine)) {
                datContents << datLine;
            }
            datFile.close();

            try {
                json parsedData = json::parse(datContents.str());
                if (parsedData.contains("InstallationList") && parsedData["InstallationList"].is_array()) {
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("AppName") && entry["AppName"].is_string() &&
                            entry.contains("InstallLocation") && entry["InstallLocation"].is_string())
                        {
                            std::string appName = entry["AppName"].get<std::string>();
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            if (!appName.empty() && !installLocation.empty()) {
                                installLocations[appName] = installLocation;
                                LOG(LogDebug) << "  Found app from dat: " << appName << " -> " << installLocation;
                            } else {
                                 LOG(LogWarning) << "  Entry has empty AppName or InstallLocation, skipping.";
                            }
                        } else {
                            LOG(LogWarning) << "  Entry missing AppName or InstallLocation (or not strings), skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found or not an array in " << launcherInstalledDatPath;
                }
            } catch (const json::parse_error& e) {
                LOG(LogError) << "  Error parsing " << launcherInstalledDatPath << ": " << e.what();
                installLocations.clear();
            } catch (const std::exception& e) {
                 LOG(LogError) << "  Exception while processing " << launcherInstalledDatPath << ": " << e.what();
                 installLocations.clear();
            }
        } else {
            LOG(LogError) << "  Failed to open " << launcherInstalledDatPath;
        }
    } else {
        LOG(LogWarning) << "  " << launcherInstalledDatPath << " not found.";
    }

    // 2. Read .item manifests from the metadata path
    if (!metadataPath.empty() && fs::exists(metadataPath) && fs::is_directory(metadataPath)) {
         LOG(LogDebug) << "EpicGamesStore: Reading .item manifests from: " << metadataPath;
        for (const auto& entry : fs::directory_iterator(metadataPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".item") {
                std::ifstream itemFile(entry.path());
                if (itemFile.is_open()) {
                    std::stringstream itemContents;
                    // Read potentially large manifests chunk by chunk or ensure sufficient buffer
                    itemContents << itemFile.rdbuf();
                    itemFile.close();

                    try {
                        json manifest = json::parse(itemContents.str());

                        // Check essential fields
                        if (!manifest.contains("AppName") || !manifest["AppName"].is_string()) {
                             LOG(LogWarning) << "  Manifest missing AppName or not a string, skipping: " << entry.path().filename();
                             continue;
                        }
                        std::string appName = manifest["AppName"].get<std::string>();

                        // --- Filter out DLCs ---
                        if (manifest.contains("MainGameAppName") && manifest["MainGameAppName"].is_string() &&
                            manifest.contains("AppName") && manifest["AppName"].is_string() && // Check AppName again
                            manifest["AppName"].get<std::string>() != manifest["MainGameAppName"].get<std::string>())
                        {
                            LOG(LogDebug) << "  Skipping DLC: " << appName << " (MainGame: " << manifest["MainGameAppName"].get<std::string>() << ")";
                            continue;
                        }

                        // --- Filter out Plugins ---
                        if (manifest.contains("AppCategories") && manifest["AppCategories"].is_array()) {
                            bool isPlugin = false;
                            for (const auto& category : manifest["AppCategories"]) {
                                if (category.is_string()) {
                                    std::string catStr = category.get<std::string>();
                                    if (catStr == "plugins" || catStr == "plugins/engine") {
                                        isPlugin = true;
                                        LOG(LogDebug) << "  Skipping Plugin: " << appName;
                                        break;
                                    }
                                }
                            }
                            if (isPlugin) {
                                continue;
                            }
                        }

                        // --- Get Game Info ---
                        EpicGamesStore::EpicGameInfo game;
                        game.id = appName; // AppName is the primary ID
                        game.name = manifest.contains("DisplayName") && manifest["DisplayName"].is_string() ? manifest["DisplayName"].get<std::string>() : appName;
                        game.executable = manifest.contains("LaunchExecutable") && manifest["LaunchExecutable"].is_string() ? manifest["LaunchExecutable"].get<std::string>() : "";
                        game.catalogNamespace = manifest.contains("CatalogNamespace") && manifest["CatalogNamespace"].is_string() ? manifest["CatalogNamespace"].get<std::string>() : ""; // <<< Get Namespace
                        game.catalogItemId = manifest.contains("CatalogItemId") && manifest["CatalogItemId"].is_string() ? manifest["CatalogItemId"].get<std::string>() : ""; // <<< AGGIUNGI QUESTA RIGA per ItemID
                        // Get Install Directory from the map we built earlier
                        if (installLocations.count(appName)) {
                            game.installDir = installLocations[appName];
                        } else {
                             LOG(LogWarning) << "  Install location not found in LauncherInstalled.dat for AppName: " << appName << ". InstallDir will be empty.";
                             game.installDir = ""; // Set explicitly to empty if not found
                             // Optionally, skip this game if installDir is crucial
                             // continue;
                        }

                        // Construct launch command (URL)
                        game.launchCommand = "com.epicgames.launcher://apps/" + appName + "?action=launch&silent=true";

                        // Add to list if it's not already there (shouldn't happen if AppName is unique)
                        bool already_added = false;
                        for(const auto& existing_game : games) {
                            if (existing_game.id == game.id) {
                                already_added = true;
                                break;
                            }
                        }
                        if (!already_added) {
                            games.push_back(game);
                            LOG(LogInfo) << "  Found game: Name='" << game.name << "', ID='" << game.id << "', Namespace='" << game.catalogNamespace << "', InstallDir='" << game.installDir << "'";
                        }


                    } catch (const json::parse_error& e) {
                        LOG(LogError) << "  Error parsing .item manifest '" << entry.path().filename() << "': " << e.what();
                    } catch (const std::exception& e) {
                         LOG(LogError) << "  Exception while processing manifest '" << entry.path().filename() << "': " << e.what();
                    }
                } else {
                    LOG(LogError) << "  Failed to open .item manifest: " << entry.path();
                }
            }
        }
    } else {
        LOG(LogError) << "  Metadata path not found or is not a directory: '" << metadataPath << "'. Cannot read .item manifests.";
    }
	// 3. Fallback (Optional - Current directory scan is probably not useful now)
    // Consider removing this fallback if the .item manifest approach is reliable enough.
    if (games.empty()) {
        LOG(LogWarning) << "  No games found using manifests.";
        // Your existing fallback logic (findInstalledEpicGames -> directory scan) could go here,
        // but it wouldn't provide CatalogNamespace.
    }

    LOG(LogDebug) << "EpicGamesStore::getInstalledEpicGamesWithDetails() - END - Found " << games.size() << " valid games.";
    return games;
}

std::string EpicGamesStore::getMetadataPathFromRegistry() {
    HKEY hKey;
    std::string metadataPath = "";
    // Tenta di aprire la chiave di registro specifica per l'utente corrente
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Epic Games\\EOS", 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS) {
        WCHAR szBuffer[MAX_PATH * 2]; // Buffer più grande per sicurezza
        DWORD dwBufferSize = sizeof(szBuffer);
        ULONG nError;
        // Tenta di leggere il valore "ModSdkMetadataDir"
        nError = RegQueryValueExW(hKey, L"ModSdkMetadataDir", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
        if (nError == ERROR_SUCCESS) {
            // Converte il percorso da WCHAR* (UTF-16 su Windows) a std::string (UTF-8)
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, szBuffer, -1, NULL, 0, NULL, NULL);
            if (bufferSize > 0) {
                std::vector<char> utf8Buffer(bufferSize);
                WideCharToMultiByte(CP_UTF8, 0, szBuffer, -1, utf8Buffer.data(), bufferSize, NULL, NULL);
                // Assegna rimuovendo eventuali terminatori null multipli (data() può non essere null-terminated come string)
                metadataPath = std::string(utf8Buffer.data());
                 LOG(LogDebug) << "EpicGamesStore: Found ModSdkMetadataDir in registry: " << metadataPath;
            } else {
                 LOG(LogError) << "EpicGamesStore: WideCharToMultiByte failed (1). Error: " << GetLastError();
            }
        } else {
             LOG(LogWarning) << "EpicGamesStore: Could not read ModSdkMetadataDir value. Error: " << nError;
        }
        RegCloseKey(hKey);
    } else {
        LOG(LogWarning) << "EpicGamesStore: Could not open registry key Software\\Epic Games\\EOS. Error: " << lRes;
    }

    // Fallback se la chiave di registro non è stata letta
    if (metadataPath.empty()) {
         LOG(LogWarning) << "EpicGamesStore: ModSdkMetadataDir not found in registry, attempting fallback path.";
         WCHAR szPath[MAX_PATH];
         // Ottiene il percorso standard per %LOCALAPPDATA%
         if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) {
             // Converte il percorso base da WCHAR* a std::string (UTF-8)
             int bufferSize = WideCharToMultiByte(CP_UTF8, 0, szPath, -1, NULL, 0, NULL, NULL);
             std::string basePath = "";
             if (bufferSize > 0) {
                 std::vector<char> utf8Buffer(bufferSize);
                 WideCharToMultiByte(CP_UTF8, 0, szPath, -1, utf8Buffer.data(), bufferSize, NULL, NULL);
                 basePath = std::string(utf8Buffer.data());
             } else {
                  LOG(LogError) << "EpicGamesStore: WideCharToMultiByte failed (2). Error: " << GetLastError();
             }

             if (!basePath.empty()) {
                  // Costruisce il percorso completo e lo converte in stringa
                  fs::path fullPath = fs::path(basePath) / "EpicGamesLauncher" / "Saved" / "Config" / "Windows";
                  metadataPath = fullPath.string(); // <<< Usa .string() per convertire
                  LOG(LogWarning) << "EpicGamesStore: Using fallback metadata path: " << metadataPath;
             } else {
                  LOG(LogError) << "EpicGamesStore: Failed to convert Local AppData path.";
             }
         } else {
              LOG(LogError) << "EpicGamesStore: Failed to get Local AppData path using SHGetFolderPathW.";
         }
    }

    return metadataPath;
}

std::string EpicGamesStore::getMetadataPath() {
     // Restituiamo il percorso ottenuto dal registro o dal fallback
     return getMetadataPathFromRegistry();
}

void EpicGamesStore::testFindInstalledGames() {
    LOG(LogDebug) << "EpicGamesStore::testFindInstalledGames() called!";
    std::vector<std::string> games = findInstalledEpicGames();
    if (games.empty()) {
        LOG(LogWarning) << "No Epic Games found.";
    } else {
        LOG(LogInfo) << "Found Epic Games:";
        for (const auto& game : games) {
            LOG(LogInfo) << "  " << game;
        }
    }
}

std::future<void> EpicGamesStore::updateGamesMetadataAsync(SystemData* system, const std::vector<std::string>& gameIdsToUpdate) {
    // La funzione restituisce un future che esegue la lambda in un thread separato
    return std::async(std::launch::async, [this, system, gameIdsToUpdate]() {
        // Controlli preliminari (Autenticazione e API)
        if (!mAuth || mAuth->getAccessToken().empty()) {
            LOG(LogWarning) << "Epic Store: Not authenticated or Auth module missing, cannot update metadata.";
            return; // Esce dal thread se non autenticato
        }
         if (!mAPI) {
            LOG(LogWarning) << "Epic Store: API module missing, cannot update metadata.";
            return; // Esce dal thread se l'API non è disponibile
         }

        if (!mAuth || mAuth->getAccessToken().empty()) { /* ... */ return; }
        if (!mAPI) { /* ... */ return; }
        LOG(LogInfo) << "Epic Store: Starting background metadata update for " << gameIdsToUpdate.size() << " specific game paths.";
        int updatedCount = 0;
        int errorCount = 0;
        int skippedCount = 0;
        std::map<std::string, FileData*> gameMap;
        std::vector<std::pair<std::string, std::string>> itemsToFetch;
        std::map<std::string, std::string> itemKeyToGamePath;

        if (system && system->getRootFolder()) {
            // Popola gameMap (come nel tuo codice)
             for (auto* fd : system->getRootFolder()->getFilesRecursive(GAME)) { if (fd) gameMap[fd->getPath()] = fd; }
             LOG(LogDebug) << "GameMap built with " << gameMap.size() << " entries.";
             // Prepara itemsToFetch (come nel tuo codice)
             for (const std::string& gamePath : gameIdsToUpdate) {
                 auto mapIt = gameMap.find(gamePath);
                 if (mapIt == gameMap.end() || !mapIt->second) { skippedCount++; continue; }
                 FileData* fileData = mapIt->second;
                 MetaDataList& metadata = fileData->getMetadata();
                 std::string ns = metadata.get(MetaDataId::EpicNamespace);
                 std::string item = metadata.get(MetaDataId::EpicCatalogId);
                 if (ns.empty() || item.empty()) { skippedCount++; continue; }
                 itemsToFetch.push_back({ns, item});
                 itemKeyToGamePath[item] = gamePath; // Usa item (CatalogID) come chiave
             }
             LOG(LogDebug) << "Prepared " << itemsToFetch.size() << " items to fetch from API.";
        } else { /* ... log errore e return ... */ return; }

        if (itemsToFetch.empty()) { /* ... log info e return ... */ return; }

        // --- Chiama API GetCatalogItems (come nel tuo codice) ---
        LOG(LogInfo) << "Epic Store: Calling GetCatalogItems API for " << itemsToFetch.size() << " items.";
        std::map<std::string, EpicGames::CatalogItem> catalogResults;
        try {
            catalogResults = mAPI->GetCatalogItems(itemsToFetch);
             LOG(LogInfo) << "Epic Store: API call successful, received " << catalogResults.size() << " results.";
        } catch (const std::exception& apiEx) { /* ... gestione errore ... */ }
          catch (...) { /* ... gestione errore ... */ }

        // --- Processa Risultati (PARTE MODIFICATA/ESPANSIA) ---
        LOG(LogInfo) << "Epic Store: Processing " << catalogResults.size() << " results from API.";
        bool anyMetadataChanged = false; // Flag generale per sapere se ricaricare la vista

        for (const auto& pair : catalogResults) {
            const std::string& resultKey = pair.first; // Chiave API (CatalogID)
            const EpicGames::CatalogItem& details = pair.second; // Dettagli completi

            // Trova FileData corrispondente (come prima)
            auto pathIt = itemKeyToGamePath.find(resultKey);
            if (pathIt == itemKeyToGamePath.end()) { errorCount++; continue; }
            const std::string& gamePath = pathIt->second;
            auto fdIt = gameMap.find(gamePath);
            if (fdIt == gameMap.end() || !fdIt->second) { errorCount++; continue; }
            FileData* fileData = fdIt->second;
            MetaDataList& metadata = fileData->getMetadata(); // Riferimento

            LOG(LogDebug) << "Epic Store Meta: Processing details for " << gamePath << " ('" << details.title << "')";
            bool gameMetadataChanged = false; // Flag per questo gioco

            // --- INIZIO AGGIORNAMENTO METADATI DETTAGLIATI ---

            // Nome (Controlla sempre se è cambiato)
            if (!details.title.empty() && metadata.get(MetaDataId::Name) != details.title) {
                 LOG(LogDebug) << "  Updating Name to: " << details.title;
                 metadata.set(MetaDataId::Name, details.title); gameMetadataChanged = true;
            }

            // Descrizione
            if (!details.description.empty() && metadata.get(MetaDataId::Desc) != details.description) {
                 LOG(LogDebug) << "  Updating Desc.";
                 metadata.set(MetaDataId::Desc, details.description); gameMetadataChanged = true;
            }

            // Sviluppatore
            if (!details.developer.empty() && metadata.get(MetaDataId::Developer) != details.developer) {
                 LOG(LogDebug) << "  Updating Developer to: " << details.developer;
                 metadata.set(MetaDataId::Developer, details.developer); gameMetadataChanged = true;
            }

            // Editore
            if (!details.publisher.empty() && metadata.get(MetaDataId::Publisher) != details.publisher) {
                 LOG(LogDebug) << "  Updating Publisher to: " << details.publisher;
                 metadata.set(MetaDataId::Publisher, details.publisher); gameMetadataChanged = true;
            }

// Data Rilascio (con conversione formato) - BLOCCO CORRETTO
  // Data Rilascio (Logica OK)
            if (!details.releaseDate.empty()){
                time_t release_t = Utils::Time::iso8601ToTime(details.releaseDate);
                if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                    std::string esDate = Utils::Time::timeToMetaDataString(release_t);
                    if (!esDate.empty() && metadata.get(MetaDataId::ReleaseDate) != esDate) {
                         metadata.set(MetaDataId::ReleaseDate, esDate); gameMetadataChanged = true;
                         LOG(LogDebug) << "    Updated ReleaseDate to: " << esDate;
                    }
                } else { LOG(LogWarning) << "    Could not parse ISO 8601 ReleaseDate: " << details.releaseDate; }
            }

            // Genere (Esempio base)
            std::string genre = "";
            for(const auto& category : details.categories) { /* ... logica per estrarre genere ... */ } // Mantieni la logica che avevi o migliorala
            if (!genre.empty() && metadata.get(MetaDataId::Genre) != genre) {
                 LOG(LogDebug) << "  Updating Genre to: " << genre;
                 metadata.set(MetaDataId::Genre, genre); gameMetadataChanged = true;
            }

            // --- Gestione Immagine (Placeholder - Logica Download da implementare) ---
            std::string imageUrl = "";
            std::string imageType = "";
            for (const auto& img : details.keyImages) { /* ... logica per scegliere URL migliore ... */ }
            if (!imageUrl.empty()) {
                 LOG(LogDebug) << "  Found potential image URL (" << imageType << "): " << imageUrl;
                 //
                 // ***** QUI VA LA LOGICA DI DOWNLOAD ASINCRONO *****
                 // 1. Determina path locale (es. roms/epicgamestore/images/epicid.jpg)
                 // 2. Controlla se esiste già e se URL sorgente è lo stesso (per evitare re-download)
                 // 3. Se necessario, lancia download asincrono
                 // 4. Nel callback del download (che DEVE essere thread-safe per UI):
                 //    - Chiama metadata.set(MetaDataId::Image, localPath);
                 //    - Salva magari l'URL sorgente in un tag custom: metadata.set("SourceImageUrl", imageUrl);
                 //    - Notifica la UI per refresh immagine (se necessario/possibile)
                 //
                 // gameMetadataChanged = true; // Segna cambiamento se il download parte o l'immagine cambia
            }

            // --- FINE AGGIORNAMENTO METADATI DETTAGLIATI ---
			
			 // ***** INSERISCI IL CODICE DI LOGGING "DOPO" QUI *****
         // Logga lo stato DOPO aver potenzialmente chiamato set() per questo gioco
         // Assumendo che 'fileData' e 'metadata' siano validi qui
         if (fileData) { // Aggiungi un controllo per sicurezza
             LOG(LogDebug) << "EG_DEBUG: Stato Metadati DOPO il task background per gioco: " << fileData->getName();
             // Metodo Alternativo:
             LOG(LogDebug) << "  -> name = [" << metadata.get(MetaDataId::Name) << "]";
             LOG(LogDebug) << "  -> desc = [" << metadata.get(MetaDataId::Desc).substr(0, 30) << "...]";
             LOG(LogDebug) << "  -> virtual = [" << metadata.get(MetaDataId::Virtual) << "]";
             LOG(LogDebug) << "  -> epicns = [" << metadata.get(MetaDataId::EpicNamespace) << "]";
             LOG(LogDebug) << "  -> epiccstid = [" << metadata.get(MetaDataId::EpicCatalogId) << "]";
             LOG(LogDebug) << "  -> launch = [" << metadata.get(MetaDataId::LaunchCommand) << "]";
         } else {
             LOG(LogWarning) << "EG_DEBUG: fileData non valido nel punto di logging 'DOPO' per gamePath: " << gamePath;
         }
         // ***** FINE CODICE DI LOGGING "DOPO" INSERITO *****

            if (gameMetadataChanged) {
                updatedCount++;
                anyMetadataChanged = true; // Segna che almeno un gioco è cambiato
            }

        } // Fine ciclo for (pair : catalogResults)

        // Calcola errori finali (come prima)
        int notFoundInApiCount = itemsToFetch.size() - catalogResults.size();
        int finalErrorCount = errorCount + notFoundInApiCount;
        LOG(LogInfo) << "Epic Store: Background metadata update finished. Updated: " << updatedCount
                     << ", Errors/Not Found: " << finalErrorCount
                     << ", Skipped (Input/Metadata): " << skippedCount;

        // --- Ricarica la vista SOLO SE qualcosa è cambiato ---
        if (anyMetadataChanged && system) { // Usa il flag generale
            LOG(LogInfo) << "Epic Store: Metadata changed, requesting UI reload for system " << system->getName();
            if (ViewController::get()) {
                 // Usa il metodo corretto per ricaricare la vista
                 ViewController::get()->reloadGameListView(system);
            } else {
                 LOG(LogWarning) << "Epic Store: ViewController instance not available, cannot request UI reload.";
            }
            // Considera se salvare gamelist qui o affidarti al salvataggio all'uscita
            // system->saveGamelist(); // Rischioso da background thread!
        }

    }); // Fine lambda std::async
}


std::future<void> EpicGamesStore::refreshGamesListAsync() {
    return std::async(std::launch::async, [this]() {
        LOG(LogInfo) << "Epic Store: Starting background library refresh (refreshGamesListAsync v4 - Simplified)...";

        // --- Controlli preliminari ---
        if (!mAuth || mAuth->getAccessToken().empty()) { LOG(LogWarning) << "Epic Store Refresh: Not authenticated."; return; }
        if (!mAPI) { LOG(LogError) << "Epic Store Refresh: API object is null."; return; }
        SystemData* system = SystemData::getSystem("epicgamestore");
        if (!system) { LOG(LogError) << "Epic Store Refresh: Could not find SystemData for 'epicgamestore'."; return; }
        FolderData* rootFolder = system->getRootFolder();
        if (!rootFolder) { LOG(LogError) << "Epic Store Refresh: Root folder for system is null."; return; }

        // --- Ottieni lista giochi ATTUALE da SystemData (mappa per PATH) ---
        std::vector<FileData*> currentFiles = rootFolder->getFilesRecursive(GAME);
        std::map<std::string, FileData*> currentFilesMapByPath;
        for (FileData* fd : currentFiles) {
             if (fd) { currentFilesMapByPath[fd->getPath()] = fd; }
        }
        LOG(LogInfo) << "Epic Store Refresh: Found " << currentFilesMapByPath.size() << " existing game paths.";

        // --- Ottieni lista giochi AGGIORNATA da API ---
        std::vector<EpicGames::Asset> assetsFromApi;
        try {
            assetsFromApi = mAPI->GetAllAssets();
            LOG(LogInfo) << "Epic Store Refresh: Fetched " << assetsFromApi.size() << " assets from API.";
        } catch (const std::exception& e) { LOG(LogError) << "Epic Store Refresh: Exception fetching assets: " << e.what(); return; }
          catch (...) { LOG(LogError) << "Epic Store Refresh: Unknown exception fetching assets."; return; }

        // --- Confronta e Aggiungi Giochi Nuovi ---
        bool changesMade = false;
        std::vector<std::string> addedGamePathsForMetaUpdate;
        int addedCount = 0;
        const std::string VIRTUAL_EPIC_PREFIX = "epic://virtual/";

        for (const auto& asset : assetsFromApi) {
            if (asset.appName.empty()) continue;

            // Ricostruisci pseudoPath
            std::string pseudoPath = VIRTUAL_EPIC_PREFIX;
            if (!asset.ns.empty() && !asset.catalogItemId.empty()) {
                 pseudoPath += HttpReq::urlEncode(asset.appName) + "/" + HttpReq::urlEncode(asset.catalogItemId);
            } else {
                   pseudoPath += HttpReq::urlEncode(asset.appName);
            }

            if (currentFilesMapByPath.find(pseudoPath) == currentFilesMapByPath.end()) {
                // GIOCO NUOVO
                LOG(LogInfo) << "Epic Store Refresh: Adding new game -> " << pseudoPath << " (" << asset.appName << ")";
                FileData* newGame = new FileData(FileType::GAME, pseudoPath, system);
                MetaDataList& mdl = newGame->getMetadata();
                mdl.set(MetaDataId::Name, asset.appName);
                mdl.set(MetaDataId::Installed, "false");
                mdl.set(MetaDataId::Virtual, "true");
                mdl.set(MetaDataId::EpicId, asset.appName);
                mdl.set(MetaDataId::EpicNamespace, asset.ns);
                mdl.set(MetaDataId::EpicCatalogId, asset.catalogItemId);
                std::string installCommandUri = "com.epicgames.launcher://apps/";
                 if (!asset.ns.empty() && !asset.catalogItemId.empty() && !asset.appName.empty()) { /* ... costruisci URI completo ... */ } else { /* ... costruisci URI fallback ... */ }
                 mdl.set(MetaDataId::LaunchCommand, installCommandUri);

                rootFolder->addChild(newGame);
                changesMade = true;
                addedGamePathsForMetaUpdate.push_back(pseudoPath);
                addedCount++;
                FileFilterIndex* filterIndex = system->getFilterIndex();
                 if (filterIndex != nullptr) { filterIndex->addToIndex(newGame); }
            }
        } // Fine ciclo assetsFromApi

        // --- Blocco Rimozione Eliminato ---

        // --- Azioni Finali Semplificate ---
        if (changesMade) {
            LOG(LogInfo) << "Epic Store Refresh: " << addedCount << " new games added.";

            // Triggera l'aggiornamento metadati per i NUOVI giochi
            if (!addedGamePathsForMetaUpdate.empty()) {
                 LOG(LogInfo) << "Epic Store Refresh: Triggering metadata update for " << addedCount << " newly added games.";
                 this->updateGamesMetadataAsync(system, addedGamePathsForMetaUpdate);
            }

            // Aggiorna conteggio giochi
             system->updateDisplayedGameCount();

            // ** NON tentiamo di salvare o ricaricare l'UI da qui **
            // Ci affidiamo al salvataggio all'uscita di ES e al refresh
            // manuale della vista da parte dell'utente (uscendo e rientrando nel sistema).
             LOG(LogInfo) << "Epic Store Refresh: Relying on ES auto-save. UI will update on next view load.";

        } else {
            LOG(LogInfo) << "Epic Store Refresh: No new games found to add.";
        }

        LOG(LogInfo) << "Epic Store: Background library refresh finished (refreshGamesListAsync v4).";
		
		
    });
}
// --- Assicurati che l'implementazione di getGameLaunchUrl NON sia duplicata ---
// std::string EpicGamesStore::getGameLaunchUrl(const EpicGames::Asset& asset) const { ... }

 
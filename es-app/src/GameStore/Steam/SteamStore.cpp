#include "GameStore/Steam/SteamStore.h"
#include "GameStore/Steam/SteamAuth.h"
#include "GameStore/Steam/SteamStoreAPI.h"
#include "SystemData.h"
#include "FileData.h"
#include "MetaData.h"
#include "Log.h"
#include "utils/Platform.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"         // Per timeToMetaDataString, NOT_A_DATE_TIME
#include "Settings.h"
#include "views/ViewController.h"
#include "SdlEvents.h"              // Per SDL_STEAM_REFRESH_COMPLETE

#include <fstream>
#include <sstream>
#include <map>
#include <stack>
#include <set>
#include <future>
#include <thread>
#include <chrono>
#include <iomanip>                  // Per std::get_time
#include <ctime>                    // Per std::tm, mktime

#ifdef _WIN32
#include <windows.h>
#endif

// Helper Functions and Enums for VDF Parsing
enum class VdfParseState {
    LookingForToken,
    ReadingKey,
    ReadingValue
};

static std::string unescapeVdfString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            if (str[i + 1] == '\\') result += '\\';
            else if (str[i + 1] == '"') result += '"';
            else result += str[i + 1]; // Handle other escape sequences if needed
            ++i;
        } else {
            result += str[i];
        }
    }
    return result;
}

static std::map<std::string, std::string> parseVdf(const std::string& filePath) {
    std::map<std::string, std::string> data;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG(LogError) << "VDF: Failed to open file: " << filePath;
        return data;
    }

    VdfParseState state = VdfParseState::LookingForToken;
    std::stack<std::string> blockStack;
    std::string currentKey;
    std::string currentToken;
    int lineNumber = 1;
    bool inQuotes = false;

    auto processToken = [&]() {
        if (currentToken.empty()) return;
        LOG(LogDebug) << "VDF: Processing token: '" << currentToken << "' in state: " << static_cast<int>(state);
        if (state == VdfParseState::LookingForToken) {
            if (currentToken == "{") {
                if (!currentKey.empty()) {
                    blockStack.push(currentKey);
                }
            } else if (currentToken == "}") {
                if (!blockStack.empty()) {
                    blockStack.pop();
                } else {
                    LOG(LogWarning) << "VDF: Unbalanced curly braces in " << filePath << " at line " << lineNumber;
                }
            } else if (currentToken[0] == '"') {
                currentKey = unescapeVdfString(currentToken.substr(1, currentToken.length() - 2));
                state = VdfParseState::ReadingValue;
            }
        } else if (state == VdfParseState::ReadingValue) {
            if (currentToken[0] == '"') {
                data[currentKey] = unescapeVdfString(currentToken.substr(1, currentToken.length() - 2));
            } else {
                data[currentKey] = currentToken;
            }
            state = VdfParseState::LookingForToken;
            currentKey = "";
        }
        currentToken.clear();
    };

    char c;
    while (file.get(c)) {
        if (c == '"') {
            inQuotes = !inQuotes;
            if (!currentToken.empty() && state != VdfParseState::LookingForToken) {
                currentToken += c;
            } else {
                currentToken += c;
            }
        } else if (std::isspace(c) && !inQuotes) {
            processToken();
        } else if (c == '{' || c == '}') {
            if (!currentToken.empty()) {
                processToken();
            }
            currentToken += c;
            processToken();
        } else {
            currentToken += c;
        }

        if (c == '\n') lineNumber++;
    }

    processToken(); // Process any remaining token
    if (!blockStack.empty()) {
        LOG(LogWarning) << "VDF: Unclosed block in " << filePath;
    }

    file.close();
    return data;
}

// --- SteamStore Class Implementation ---

SteamStore::SteamStore(SteamAuth* auth)
    // Inizializza _initialized ereditato da GameStore
    : GameStore(), mAuth(auth), mAPI(nullptr), mWindow(nullptr) {
    LOG(LogDebug) << "SteamStore: Constructor";
    if (!mAuth) {
        LOG(LogError) << "SteamStore: Auth object is null in constructor!";
    }
    mAPI = new SteamStoreAPI(mAuth);
if (!mAPI) {
    LOG(LogError) << "SteamStore: Failed to create SteamStoreAPI object! mAPI is NULL."; // Log più specifico
} else {
    LOG(LogDebug) << "SteamStore: SteamStoreAPI object created successfully. mAPI pointer: " << mAPI; // Log di successo
}
}

SteamStore::~SteamStore() {
    LOG(LogDebug) << "SteamStore: Destructor";
    shutdown();
    delete mAPI;
    mAPI = nullptr;
}

bool SteamStore::init(Window* window) {
    mWindow = window;
    // _initialized è protetto, usa il metodo pubblico se necessario o accedi direttamente
    _initialized = true;
    LOG(LogInfo) << "SteamStore: Initialized.";
    return true;
}

void SteamStore::shutdown() {
    LOG(LogInfo) << "SteamStore: Shutdown.";
    _initialized = false;
    // TODO: Gestire la terminazione sicura dei task async se necessario
}

void SteamStore::showStoreUI(Window* window) {
    LOG(LogDebug) << "SteamStore: Showing store UI";
    mUI.showSteamSettingsMenu(window, this);
}

std::string SteamStore::getStoreName() const {
    return "SteamStore";
}

 std::string SteamStore::getGameLaunchUrl(unsigned int appId) const {
  //  return "steam://rungameid/" + std::to_string(appId);
  return "steam://launch/" + std::to_string(appId); //  or "steam://run/" + std::to_string(appId);
 }

bool SteamStore::checkInstallationStatus(unsigned int appId, const std::vector<SteamInstalledGameInfo>& installedGames) {
    for (const auto& game : installedGames) {
        if (game.appId == appId && game.fullyInstalled) {
            return true;
        }
    }
    return false;
}

std::vector<FileData*> SteamStore::getGamesList() {
  LOG(LogDebug) << "----------------------------------------------------";
  LOG(LogDebug) << "SteamStore::getGamesList() - ENTRY POINT! ";
  LOG(LogDebug) << "SteamStore: mAuth = " << mAuth << ", mAPI = " << mAPI;
  std::vector<FileData*> gameList;
 

  if (!this->_initialized || !this->mAPI) {
  LOG(LogError) << "SteamStore not initialized or API unavailable.";
  return gameList;
  }
 

  //  1. Find locally installed games
  std::vector<SteamInstalledGameInfo> installedGames = findInstalledSteamGames();
  if (installedGames.empty()) {
  LOG(LogWarning) << "No locally installed Steam games found.";
  } else {
  LOG(LogInfo) << "SteamStore: Found " << installedGames.size() << " locally installed Steam games.";
  }
 

  //  2. Get games from online library if authenticated
  std::vector<Steam::OwnedGame> onlineGames;
  if (mAuth && mAuth->isAuthenticated() && !mAuth->getSteamId().empty() && !mAuth->getApiKey().empty()) {
  onlineGames = mAPI->GetOwnedGames(mAuth->getSteamId(), mAuth->getApiKey());
  if (onlineGames.empty()) {
  LOG(LogWarning) << "No games found in the online Steam library.";
  } else {
  LOG(LogInfo) << "SteamStore: Obtained " << onlineGames.size() << " games from the online Steam library.";
  }
  } else {
  LOG(LogInfo) << "SteamStore: Not authenticated or missing credentials, online library games will not be loaded.";
  }
 

  SystemData* steamSystem = SystemData::getSystem("steam");
  LOG(LogDebug) << "SteamStore: steamSystem = " << steamSystem;
  if (!steamSystem) {
  LOG(LogError) << "SteamStore::getGamesList() - Could not find SystemData for 'steam'. Games might not have the correct system.";
  }
 

  std::map<unsigned int, FileData*> processedGames;
 

  //  First, installed games
  for (const auto& installedGame : installedGames) {
  if (installedGame.appId == 0) continue;
 

  FileData* fd = new FileData(FileType::GAME, getGameLaunchUrl(installedGame.appId), steamSystem);
 

  fd->setMetadata(MetaDataId::Name, installedGame.name);
  fd->setMetadata(MetaDataId::SteamAppId, std::to_string(installedGame.appId));
  fd->setMetadata(MetaDataId::Installed, "true");
  fd->setMetadata(MetaDataId::Virtual, "false");
  fd->setMetadata(MetaDataId::Path, installedGame.libraryFolderPath + "/common/" + installedGame.installDir);
  fd->setMetadata(MetaDataId::LaunchCommand, getGameLaunchUrl(installedGame.appId));
 

  processedGames[installedGame.appId] = fd;
  gameList.push_back(fd);
  }
 

  //  Then, online games
  for (const auto& onlineGame : onlineGames) {
  if (onlineGame.appId == 0) continue;
 

  if (processedGames.find(onlineGame.appId) == processedGames.end()) {
  FileData* fd = new FileData(FileType::GAME, getGameLaunchUrl(onlineGame.appId), steamSystem);
 

  fd->setMetadata(MetaDataId::Name, onlineGame.name);
  fd->setMetadata(MetaDataId::SteamAppId, std::to_string(onlineGame.appId));
  fd->setMetadata(MetaDataId::Installed, checkInstallationStatus(onlineGame.appId, installedGames) ? "true" : "false");
  fd->setMetadata(MetaDataId::Virtual, "true");
  fd->setMetadata(MetaDataId::LaunchCommand, getGameLaunchUrl(onlineGame.appId));
 

  processedGames[onlineGame.appId] = fd;
  gameList.push_back(fd);
  } else {
  FileData* existingFd = processedGames[onlineGame.appId];
  if (existingFd->getMetadata().get(MetaDataId::Name).empty() || existingFd->getMetadata().get(MetaDataId::Name) == "N/A") {
  existingFd->setMetadata(MetaDataId::Name, onlineGame.name);
  }
  //  TODO: Update playtime if necessary
  }
  }
 

  LOG(LogInfo) << "SteamStore::getGamesList() - End, returned " << gameList.size() << " games.";
  return gameList;
 }
 
 std::future<void> SteamStore::refreshSteamGamesListAsync() {
    // Cattura 'this' per accedere ai membri e metodi della classe
    return std::async(std::launch::async, [this]() {
        std::vector<NewSteamGameData>* payload = nullptr;
        SystemData* steamSystemForEvent = nullptr; 
        bool initialChecksOk = false;
        auto startTime = std::chrono::high_resolution_clock::now();

        LOG(LogInfo) << "Steam Store Refresh BG: Starting asynchronous refresh...";

        try {
            // --- CONTROLLI PRELIMINARI ---
            LOG(LogDebug) << "Steam Store Refresh BG: Performing pre-checks...";
            LOG(LogDebug) << "  - 'this' pointer: " << static_cast<void*>(this);
            if (!this) {
                LOG(LogError) << "Steam Store Refresh BG: 'this' pointer is NULL! Aborting.";
                // L'evento verrà inviato nel blocco 'finally' implicito con payload nullo.
                // Per sicurezza, creiamo un payload vuoto per evitare dereferenziazioni nulle nel gestore.
                payload = new std::vector<NewSteamGameData>();
                throw std::runtime_error("'this' pointer is null"); // Esce dal try, va al catch, poi invia evento.
            }
            LOG(LogDebug) << "  - this->_initialized: " << this->_initialized;
            LOG(LogDebug) << "  - this->mAuth pointer: " << static_cast<void*>(this->mAuth);
            LOG(LogDebug) << "  - this->mAPI pointer: " << static_cast<void*>(this->mAPI);

            if (!this->_initialized || !this->mAuth || !this->mAPI) {
                LOG(LogError) << "Steam Store Refresh BG: Precondition failed: Store not initialized, Auth, or API unavailable.";
                if (!this->_initialized) LOG(LogError) << "   - Reason: _initialized is false";
                if (!this->mAuth) LOG(LogError) << "   - Reason: mAuth is NULL";
                if (!this->mAPI) LOG(LogError) << "   - Reason: mAPI is NULL";
                payload = new std::vector<NewSteamGameData>(); // Payload vuoto per segnalare problema
                throw std::runtime_error("Store prerequisites not met");
            }

            if (!this->mAuth->isAuthenticated()) {
                LOG(LogError) << "Steam Store Refresh BG: Not authenticated. Aborting.";
                payload = new std::vector<NewSteamGameData>(); // Payload vuoto
                throw std::runtime_error("Not authenticated");
            }
            LOG(LogInfo) << "Steam Store Refresh BG: Authentication verified as TRUE.";

            steamSystemForEvent = SystemData::getSystem("steam");
            if (!steamSystemForEvent || !steamSystemForEvent->getRootFolder()) {
                LOG(LogError) << "Steam Store Refresh BG: Cannot find Steam system or its root folder (" << this->getStoreName() << "). Aborting.";
                payload = new std::vector<NewSteamGameData>(); // Payload vuoto
                throw std::runtime_error("Steam system or root folder not found");
            }
            initialChecksOk = true; // Tutti i controlli iniziali sono passati

            // --- RACCOLTA DATI (solo se i controlli iniziali sono OK) ---
            LOG(LogInfo) << "Steam Store Refresh BG: Collecting data...";
            std::set<unsigned int> existingAppIds;
            LOG(LogDebug) << "Steam Store Refresh BG: Collecting existing AppIDs...";
            try {
                std::vector<FileData*> currentFiles = steamSystemForEvent->getRootFolder()->getFilesRecursive(GAME);
                for (FileData* fd : currentFiles) {
                    if (!fd) continue;
                    std::string appIdStr = fd->getMetadata().get(MetaDataId::SteamAppId);
                    if (!appIdStr.empty()) {
                        try { existingAppIds.insert(std::stoul(appIdStr)); } catch(...) { /* ignora errori di conversione */ }
                    }
                }
                LOG(LogDebug) << "Steam Store Refresh BG: Found " << existingAppIds.size() << " existing AppIDs.";
            } catch (const std::exception& e) {
                LOG(LogError) << "Steam Store Refresh BG: Exception while collecting existing AppIDs: " << e.what();
                // Non rilanciare, potremmo voler comunque inviare un payload vuoto.
                // Il payload sarà vuoto se si arriva qui e non viene popolato dopo.
            }

            LOG(LogInfo) << "Steam Store Refresh BG: Searching for installed games...";
            std::vector<SteamInstalledGameInfo> installedGames = findInstalledSteamGames(); // Assicurati che questa funzione sia robusta
            LOG(LogInfo) << "Steam Store Refresh BG: Found " << installedGames.size() << " installed games.";

            std::vector<Steam::OwnedGame> onlineGames;
            LOG(LogInfo) << "Steam Store Refresh BG: Fetching online owned games...";
            std::string steamId = mAuth->getSteamId();
            std::string apiKey = mAuth->getApiKey();
            if (!steamId.empty() && !apiKey.empty()) {
                try {
                    onlineGames = this->mAPI->GetOwnedGames(steamId, apiKey, true, true); // Assumendo che `this->mAPI` sia l'oggetto corretto
                    LOG(LogInfo) << "Steam Store Refresh BG: Fetched " << onlineGames.size() << " online games.";
                } catch (const std::exception& e) {
                    LOG(LogError) << "Steam Store Refresh BG: Exception fetching owned games: " << e.what();
                    // Continua, potremmo avere giochi installati da processare
                }
            } else {
                 LOG(LogWarning) << "Steam Store Refresh BG: SteamID or API Key missing, cannot fetch online games.";
            }

            // --- PREPARAZIONE PAYLOAD ---
            LOG(LogInfo) << "Steam Store Refresh BG: Identifying new games and preparing payload...";
            payload = new std::vector<NewSteamGameData>(); // Alloca il payload qui
            std::set<unsigned int> processedForPayload; 

            for (const auto& installedGame : installedGames) {
                if (installedGame.appId == 0) continue;
                if (existingAppIds.find(installedGame.appId) == existingAppIds.end()) { 
                    if (processedForPayload.insert(installedGame.appId).second) { 
                        NewSteamGameData data;
                        data.pseudoPath = getGameLaunchUrl(installedGame.appId); 
                        data.metadataMap[MetaDataId::Name] = installedGame.name;
                        data.metadataMap[MetaDataId::SteamAppId] = std::to_string(installedGame.appId);
                        data.metadataMap[MetaDataId::Installed] = "true";
                        data.metadataMap[MetaDataId::Virtual] = "false";
                        data.metadataMap[MetaDataId::InstallDir] = installedGame.libraryFolderPath + "/common/" + installedGame.installDir;
                        data.metadataMap[MetaDataId::LaunchCommand] = data.pseudoPath;
                        payload->push_back(data);
                        LOG(LogDebug) << "  Added NEW INSTALLED to payload: " << installedGame.name;
                    }
                }
            }

            for (const auto& onlineGame : onlineGames) {
                if (onlineGame.appId == 0) continue;
                if (existingAppIds.find(onlineGame.appId) == existingAppIds.end()) { 
                    if (processedForPayload.insert(onlineGame.appId).second) { 
                        NewSteamGameData data;
                        data.pseudoPath = getGameLaunchUrl(onlineGame.appId); 
                        data.metadataMap[MetaDataId::Name] = onlineGame.name.empty() ? ("Steam Game " + std::to_string(onlineGame.appId)) : onlineGame.name;
                        data.metadataMap[MetaDataId::SteamAppId] = std::to_string(onlineGame.appId);
                        data.metadataMap[MetaDataId::Installed] = "false"; 
                        data.metadataMap[MetaDataId::Virtual] = "true";
                        data.metadataMap[MetaDataId::LaunchCommand] = data.pseudoPath;
                        payload->push_back(data);
                        LOG(LogDebug) << "  Added NEW ONLINE to payload: " << onlineGame.name;
                    }
                }
            }

            if (payload->empty()) {
                LOG(LogInfo) << "Steam Store Refresh BG: No new games found to add.";
            } else {
                LOG(LogInfo) << "Steam Store Refresh BG: Prepared payload with " << payload->size() << " new games.";
            }

        } catch (const std::exception& e) {
            LOG(LogError) << "Steam Store Refresh BG: General exception during refresh logic: " << e.what();
            if (payload == nullptr) { // Se l'eccezione è avvenuta prima dell'allocazione del payload
                payload = new std::vector<NewSteamGameData>(); // Alloca un payload vuoto
            } else {
                // Se payload era già allocato ma c'è stata un'eccezione dopo, 
                // potrebbe contenere dati parziali. Potresti volerlo svuotare.
                // payload->clear(); // Opzionale: svuota se preferisci un payload pulito in caso di errore
            }
        } catch (...) {
            LOG(LogError) << "Steam Store Refresh BG: Unknown exception during refresh logic.";
            if (payload == nullptr) {
                payload = new std::vector<NewSteamGameData>();
            }
        }

        // --- INVIO EVENTO (SEMPRE) ---
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        LOG(LogInfo) << "Steam Store Refresh BG: Operation completed. Duration: " << duration.count() << " ms. Pushing completion event.";

        SDL_Event event;
        SDL_zero(event); // o SDL_memset(&event, 0, sizeof(event));
        event.type = SDL_USEREVENT; // Assicurati che il tuo sistema registri e gestisca SDL_USEREVENT
                                    // o usa un tipo di evento utente specifico se registrato con SDL_RegisterEvents()
        event.user.code = SDL_STEAM_REFRESH_COMPLETE; // Il tuo codice specifico per questo evento
        event.user.data1 = payload;        // Passa il payload (che è SEMPRE allocato qui, anche se vuoto)
        event.user.data2 = steamSystemForEvent; // Passa il puntatore al sistema (può essere nullptr se i check iniziali sono falliti)
        
        SDL_PushEvent(&event);
        // Il gestore eventi nel thread principale è ora responsabile della deallocazione di 'payload'
        // (event.user.data1) dopo averlo usato.
    });
}

bool SteamStore::installGame(const std::string& gameId) {
    LOG(LogDebug) << "SteamStore::installGame for AppID: " << gameId;
    try {
        unsigned int appId = std::stoul(gameId);
        std::string launchUrl = "steam://run/" + gameId;
        LOG(LogInfo) << "Attempting to install/launch Steam game with URL: " << launchUrl;
        Utils::Platform::openUrl(launchUrl);
        return true;
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore::installGame - Invalid AppID error: " << e.what();
        return false;
    }
}

bool SteamStore::uninstallGame(const std::string& gameId) {
    LOG(LogDebug) << "SteamStore::uninstallGame for AppID: " << gameId;
    try {
        unsigned int appId = std::stoul(gameId);
        std::string uninstallUrl = "steam://uninstall/" + gameId;
        LOG(LogInfo) << "Attempting to uninstall Steam game with URL: " << uninstallUrl;
        Utils::Platform::openUrl(uninstallUrl);
        return true;
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore::uninstallGame - Invalid AppID error: " << e.what();
        return false;
    }
}

bool SteamStore::updateGame(const std::string& gameId) {
    LOG(LogDebug) << "SteamStore::updateGame for AppID: " << gameId;
    return installGame(gameId);
}

// --- Logic to find installed games ---

std::string SteamStore::getSteamInstallationPath() {
    #ifdef _WIN32
        std::string path_candidate_x86 = "C:/Program Files (x86)/Steam";
        LOG(LogDebug) << "SteamStore: Checking x86 path: " << path_candidate_x86;
        if (Utils::FileSystem::exists(path_candidate_x86)) {
            LOG(LogInfo) << "SteamStore: Found Steam at x86 path: " << path_candidate_x86;
            return path_candidate_x86;
        }

        std::string path_candidate_x64 = "C:/Program Files/Steam";
        LOG(LogDebug) << "SteamStore: Checking x64 path: " << path_candidate_x64;
        if (Utils::FileSystem::exists(path_candidate_x64)) {
            LOG(LogInfo) << "SteamStore: Found Steam at x64 path: " << path_candidate_x64;
            return path_candidate_x64;
        }

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char path[255];
            DWORD path_len = sizeof(path);
            DWORD type;
            if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", RRF_RT_REG_SZ, &type, path, &path_len) == ERROR_SUCCESS) {
                std::string steamPath = Utils::FileSystem::getGenericPath(std::string(path));
                RegCloseKey(hKey);
                LOG(LogInfo) << "SteamStore: Found Steam path in Registry: " << steamPath;
                return steamPath;
            } else {
                LOG(LogWarning) << "SteamStore: Registry key 'SteamPath' not found.";
            }
            RegCloseKey(hKey);
        } else {
            LOG(LogWarning) << "SteamStore: Registry key 'Software\\Valve\\Steam' not found.";
        }

        LOG(LogError) << "SteamStore: Steam path not found";
        return "";
#elif __linux__
    std::string home = Utils::FileSystem::getHomePath();
    std::string path1 = home + "/.steam/steam";
    if (Utils::FileSystem::exists(path1 + "/steamapps")) return path1;
    path1 = home + "/.steam/root";
    if (Utils::FileSystem::exists(path1 + "/steamapps")) return path1;
    std::string path2 = home + "/.local/share/Steam";
    if (Utils::FileSystem::exists(path2 + "/steamapps")) return path2;

    LOG(LogError) << "Steam path not found";
    return "";
#else
    LOG(LogError) << "Steam path detection not implemented";
    return "";
#endif
}

std::vector<std::string> SteamStore::getSteamLibraryFolders(const std::string& steamPath) {
    std::vector<std::string> libraryFolders;
    if (steamPath.empty() || !Utils::FileSystem::exists(steamPath)) {
        LOG(LogWarning) << "SteamStore: Invalid or not found Steam installation path: " << steamPath;
        return libraryFolders;
    }

    std::string mainSteamApps = steamPath + "/steamapps";
    if (Utils::FileSystem::exists(mainSteamApps) && Utils::FileSystem::isDirectory(mainSteamApps)) {
        libraryFolders.push_back(Utils::FileSystem::getGenericPath(mainSteamApps));
    }

    std::string libraryFoldersVdfPath = steamPath + "/config/libraryfolders.vdf"; // Corrected path
    LOG(LogDebug) << "SteamStore: Attempting to read libraryfolders.vdf from: " << libraryFoldersVdfPath;

    if (Utils::FileSystem::exists(libraryFoldersVdfPath)) {
        try {
            std::map<std::string, std::string> vdfData = parseVdf(libraryFoldersVdfPath);
            for (const auto& pair : vdfData) {
                if (Utils::String::isNumber(pair.first) && pair.second.find(":\\") != std::string::npos) { // Basic path check
                    std::string path = Utils::FileSystem::getGenericPath(pair.second) + "/steamapps";
                    if (Utils::FileSystem::exists(path) && Utils::FileSystem::isDirectory(path)) {
                        bool alreadyAdded = false;
                        for (const auto& existing : libraryFolders) {
                            if (Utils::FileSystem::getGenericPath(existing) == Utils::FileSystem::getGenericPath(path)) {
                                alreadyAdded = true;
                                break;
                            }
                        }
                        if (!alreadyAdded) {
                            libraryFolders.push_back(Utils::FileSystem::getGenericPath(path));
                            LOG(LogInfo) << "SteamStore: Found additional Steam library folder: " << path;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "SteamStore: Error parsing libraryfolders.vdf: " << e.what();
        }
    } else {
        LOG(LogInfo) << "SteamStore: libraryfolders.vdf not found.";
    }

    if (libraryFolders.empty()) {
        LOG(LogWarning) << "SteamStore: No Steam library folders found (not even the main one?).";
    }
    return libraryFolders;
}

std::vector<SteamInstalledGameInfo> SteamStore::findInstalledSteamGames() {
    std::vector<SteamInstalledGameInfo> installedGames;
    std::string steamInstallPath = getSteamInstallationPath();

    std::vector<std::string> libraryPaths = getSteamLibraryFolders(steamInstallPath);

    for (const auto& libPath : libraryPaths) {
        LOG(LogDebug) << "SteamStore: Scanning library folder: " << libPath;
        if (!Utils::FileSystem::exists(libPath) || !Utils::FileSystem::isDirectory(libPath)) {
            LOG(LogWarning) << "SteamStore: Invalid or inaccessible library folder: " << libPath;
            continue;
        }

        auto filesInDir = Utils::FileSystem::getDirectoryFiles(libPath);

        for (const auto& fileEntry : filesInDir) {
            const std::string& currentPath = fileEntry.path;

            if (Utils::FileSystem::isRegularFile(currentPath) &&
                Utils::String::startsWith(Utils::FileSystem::getFileName(currentPath), "appmanifest_") &&
                Utils::String::toLower(Utils::FileSystem::getExtension(currentPath)) == ".acf") {
                LOG(LogDebug) << "SteamStore: Found manifest file: " << currentPath;
                try {
                    SteamInstalledGameInfo gameInfo = parseAppManifest(currentPath);
                    if (gameInfo.appId != 0 && gameInfo.fullyInstalled) {
                        gameInfo.libraryFolderPath = Utils::FileSystem::getGenericPath(libPath);
                        installedGames.push_back(gameInfo);
                        LOG(LogInfo) << "SteamStore: Installed game detected: " << gameInfo.name << " (AppID: " << gameInfo.appId << ") in " << libPath;
                    }
                } catch (const std::exception& e) {
                    LOG(LogError) << "SteamStore: Error parsing manifest " << currentPath << ": " << e.what();
                }
            }
        }
    }
    return installedGames;
}

SteamInstalledGameInfo SteamStore::parseAppManifest(const std::string& acfFilePath) {
    SteamInstalledGameInfo gameInfo;
    gameInfo.appId = 0;
    gameInfo.fullyInstalled = false;

    try {
        std::map<std::string, std::string> appStateValues = parseVdf(acfFilePath);

        if (appStateValues.count("appid")) gameInfo.appId = static_cast<unsigned int>(std::stoul(appStateValues["appid"]));
        if (appStateValues.count("name")) gameInfo.name = appStateValues["name"];
        if (appStateValues.count("installdir")) gameInfo.installDir = appStateValues["installdir"];
        if (appStateValues.count("StateFlags")) {
            unsigned int stateFlags = static_cast<unsigned int>(std::stoul(appStateValues["StateFlags"]));
            if ((stateFlags & 4) != 0) {
                gameInfo.fullyInstalled = true;
            }
        }

    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore: Error parsing ACF file: " << acfFilePath << " - " << e.what();
        throw; // Re-throw the exception to be caught in findInstalledSteamGames
    }

    if (gameInfo.appId == 0 || gameInfo.name.empty() || gameInfo.installDir.empty()) {
        LOG(LogWarning) << "SteamStore: Incomplete data in manifest ACF: " << acfFilePath;
        gameInfo.fullyInstalled = false; // Ensure it's marked as not fully installed
    }
    return gameInfo;
}
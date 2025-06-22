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

#include "json.hpp" // Per nlohmann/json
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
#include <mutex> 
#include "../../es-core/src/AppContext.h"

#ifdef _WIN32
#include <windows.h>
#endif

struct SteamScrapedGame {
    unsigned int appId;
    std::string name;
    unsigned int playtimeForever;
    time_t rtimeLastPlayed;
};

struct BasicGameInfo {
    std::string path;
    std::string appIdStr;
    bool isInstalled;
    std::string name;
};

// Helper Functions and Enums for VDF Parsing
enum class VdfParseState {
    LookingForToken,
    ReadingKey,
    ReadingValue
};

// NUOVO: unescapeVdfString più robusto
static std::string unescapeVdfString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; break;
                case '"':  result += '"';  break;
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                default:   result += str[i + 1]; break;
            }
            ++i;
        } else {
            result += str[i];
        }
    }
    return result;
}

// MODIFICATO: Parser VDF con debugging estremamente dettagliato e logica più robusta.
static std::map<std::string, std::string> parseVdf(const std::string& filePath) {
    std::map<std::string, std::string> data;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG(LogError) << "VDF_DEBUG: Failed to open file: " << filePath;
        return data;
    }

    std::stack<std::string> keyStack;
    std::string currentKey;
    std::string currentValue;
    std::string tempToken;
    bool inQuotes = false;
    bool inLineComment = false;
    bool inBlockComment = false; // Non usato ma presente nelle precedenti versioni, per coerenza.
    char c;
    int lineNumber = 1; // <<<< MODIFICATO: DICHIARAZIONE DI lineNumber QUI

    auto trimQuotes = [](std::string s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"') {
            return s.substr(1, s.length() - 2);
        }
        return s;
    };

    while (file.get(c)) {
        if (c == '\n') lineNumber++; // Aggiorna il numero di riga qui, all'inizio del loop.

        // ... (resto della logica di parseVdf, inclusi gestione commenti, virgolette, ecc.) ...
        // Le righe che fanno riferimento a 'lineNumber' dovrebbero essere ora riconosciute.

        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
            }
            continue;
        }
        if (inBlockComment) {
            if (c == '*' && file.peek() == '/') {
                inBlockComment = false;
                file.get(c);
            }
            continue;
        }

        if (c == '/' && file.peek() == '/') {
            inLineComment = true;
            file.get(c);
            continue;
        }
        if (c == '/' && file.peek() == '*') {
            inBlockComment = true;
            file.get(c);
            continue;
        }

        if (c == '"') {
            inQuotes = !inQuotes;
            if (!inQuotes) {
                if (!currentKey.empty() && currentKey.front() == '"') {
                    currentKey = unescapeVdfString(trimQuotes(currentKey));
                } else if (!currentValue.empty() && currentValue.front() == '"') {
                    currentValue = unescapeVdfString(trimQuotes(currentValue));
                    data[keyStack.empty() ? currentKey : keyStack.top() + "/" + currentKey] = currentValue;
                    currentKey.clear();
                    currentValue.clear();
                }
            } else {
                tempToken.clear();
            }
            continue;
        }

        if (inQuotes) {
            tempToken += c;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}') {
            if (!tempToken.empty()) {
                if (currentKey.empty()) {
                    currentKey = tempToken;
                } else {
                    currentValue = tempToken;
                }
                tempToken.clear();
            }

            if (c == '{') {
                if (!currentKey.empty()) {
                    keyStack.push(currentKey);
                    currentKey.clear();
                } else {
                    LOG(LogWarning) << "VDF_DEBUG: Unexpected '{' at top level or without a key in " << filePath << ". Line: " << lineNumber << ". Assuming anonymous block.";
                    keyStack.push("");
                }
            } else if (c == '}') {
                if (!keyStack.empty()) {
                    keyStack.pop();
                } else {
                    LOG(LogWarning) << "VDF_DEBUG: Unbalanced '}' in " << filePath << ". Line: " << lineNumber << ". Too many closing braces.";
                }
            }

            if (!currentKey.empty() && !currentValue.empty()) {
                std::string fullKey = currentKey;
                if (!keyStack.empty()) {
                    fullKey = keyStack.top() + "/" + currentKey;
                }
                data[fullKey] = currentValue;
                currentKey.clear();
                currentValue.clear();
            }
            continue;
        }

        tempToken += c;
    }

    if (!tempToken.empty()) {
        if (currentKey.empty()) {
            currentKey = tempToken;
        } else {
            currentValue = tempToken;
        }
    }

    if (!currentKey.empty() && !currentValue.empty()) {
        std::string fullKey = currentKey;
        if (!keyStack.empty()) {
            fullKey = keyStack.top() + "/" + currentKey;
        }
        data[fullKey] = currentValue;
    }
    
    if (!keyStack.empty()) {
        LOG(LogWarning) << "VDF_DEBUG: Unclosed block(s) in " << filePath << ". Remaining blocks on stack.";
    }

    file.close();
    LOG(LogInfo) << "VDF_DEBUG: Parser finished for file: " << filePath;
    return data;
}

// --- SteamStore Class Implementation ---
SteamStore::SteamStore(SteamAuth* auth, Window* window_param)
    : GameStore(), mAuth(auth), mAPI(nullptr), mWindow(window_param) // Inizializza mWindow
{
    LOG(LogDebug) << "SteamStore: Constructor";
    if (!mAuth) {
        LOG(LogError) << "SteamStore: Auth object is null in constructor!";
    }
    mAPI = new SteamStoreAPI(mAuth);
    if (!mAPI) {
        LOG(LogError) << "SteamStore: Failed to create SteamStoreAPI object! mAPI is NULL.";
    } else {
        LOG(LogDebug) << "SteamStore: SteamStoreAPI object created successfully. mAPI pointer: " << mAPI;
    }
    _initialized = false; // Imposta _initialized qui, poi init lo metterà a true.
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
    LOG(LogDebug) << "SteamStore::getGamesList() - Lettura giochi esistenti dal SystemData.";
    std::vector<FileData*> gameFiles;
    SystemData* steamSystem = SystemData::getSystem("steam");
    if (!steamSystem) {
        LOG(LogError) << "SteamStore::getGamesList() - Sistema 'steam' non trovato!";
        return gameFiles;
    }

    // Restituisce semplicemente i giochi già presenti nel SystemData per "steam".
    // I giochi vengono aggiunti/aggiornati in refreshSteamGamesListAsync.
    if (steamSystem->getRootFolder()) {
        gameFiles = steamSystem->getRootFolder()->getFilesRecursive(GAME, true);
    }
    
    LOG(LogInfo) << "SteamStore::getGamesList() - Fine, restituiti " << gameFiles.size() << " giochi esistenti.";
    return gameFiles;
}
std::future<void> SteamStore::refreshSteamGamesListAsync() {
    return std::async(std::launch::async, [this]() {
        LOG(LogInfo) << "Steam Store Refresh BG: Avvio refresh asincrono (Parte 1)...";

        // --- FASE 1: RACCOLTA DATI ESISTENTI E GIOCHI INSTALLATI (come prima) ---
        std::vector<BasicGameInfo> copiedExistingGamesInfo;
        std::map<std::string, BasicGameInfo> copiedGamesMap;

        try {
            { // Blocco per il lock
                std::lock_guard<std::mutex> lock(g_systemDataMutex);
                SystemData* steamSystem = SystemData::getSystem("steam");
                if (!steamSystem || !steamSystem->getRootFolder()) {
                    LOG(LogError) << "Steam Store Refresh BG: Sistema Steam non trovato. Annullamento.";
                    return;
                }
                if (!mAuth || !mAuth->isAuthenticated()) {
                    LOG(LogError) << "Steam Store Refresh BG: Non autenticato. Annullamento.";
                    return;
                }

                std::vector<FileData*> currentSystemGames = steamSystem->getRootFolder()->getFilesRecursive(GAME, true);
                for (FileData* fd : currentSystemGames) {
                    if (!fd) continue;
                    BasicGameInfo info;
                    info.path = fd->getPath();
                    info.appIdStr = fd->getMetadata().get(MetaDataId::SteamAppId);
                    info.isInstalled = (fd->getMetadata().get(MetaDataId::Installed) == "true");
                    info.name = fd->getName();
                    copiedExistingGamesInfo.push_back(info);
                }
            } // Fine lock

            for (const auto& gameInfo : copiedExistingGamesInfo) {
                if (!gameInfo.appIdStr.empty()) copiedGamesMap[gameInfo.appIdStr] = gameInfo;
                else copiedGamesMap[gameInfo.path] = gameInfo;
            }

            LOG(LogInfo) << "Steam Store Refresh BG: Ricerca giochi Steam installati localmente...";
            std::vector<SteamInstalledGameInfo> installedGames = findInstalledSteamGames();
            LOG(LogInfo) << "Steam Store Refresh BG: Trovati " << installedGames.size() << " giochi installati.";

            // --- FASE 2: DEFINIZIONE DEL LAVORO DA FARE DOPO LO SCRAPING ---
            // Questo blocco di codice verrà eseguito DOPO che la WebView ha finito.
            auto processScrapedData = [this, copiedGamesMap, installedGames](const std::string& scrapedGameDataJson) {
                LOG(LogInfo) << "Steam Store Refresh BG: Avvio elaborazione dati (Parte 2)...";
                std::vector<NewSteamGameData> newGamesPayload;
                bool metadataChangedForExisting = false; // Traccia se i metadati cambiano

                // (Qui andrebbe tutta la logica di parsing del JSON e confronto con i giochi installati.
                // Per ora, ci concentriamo sul far funzionare il flusso.
                // Se lo scraping ha successo, questo log apparirà)
                if (!scrapedGameDataJson.empty() && scrapedGameDataJson != "null")
{
    LOG(LogInfo) << "Steam Store Refresh BG: Scraping riuscito! Inizio il parsing del JSON...";
    try
    {
        nlohmann::json profileJson = nlohmann::json::parse(scrapedGameDataJson);
        
        // IL CAMBIAMENTO CHIAVE E' QUI: cerchiamo la lista dentro l'oggetto del profilo.
        // La pagina dei giochi di Steam la chiama "rgGames".
        if (profileJson.is_object() && profileJson.contains("rgGames") && profileJson["rgGames"].is_array())
        {
            nlohmann::json gamesJson = profileJson["rgGames"];
            LOG(LogInfo) << "Steam Store Refresh BG: Trovata la lista 'rgGames' con " << gamesJson.size() << " giochi.";

            for (const auto& gameNode : gamesJson)
            {
                if (!gameNode.is_object() || !gameNode.contains("appid") || !gameNode.contains("name")) {
                    continue;
                }

                unsigned int appId = gameNode.value("appid", 0);
                if (appId == 0 || appId == 228980) continue; 

                std::string appIdStr = std::to_string(appId);
                if (copiedGamesMap.find(appIdStr) != copiedGamesMap.end()) {
                    continue; // Gioco già presente
                }

                NewSteamGameData data;
                data.pseudoPath = "steam_online_appid://" + appIdStr;
                data.metadataMap[MetaDataId::Name] = gameNode.value("name", "Unknown Steam Game");
                data.metadataMap[MetaDataId::SteamAppId] = appIdStr;
                data.metadataMap[MetaDataId::Installed] = "false";
                data.metadataMap[MetaDataId::Virtual] = "true";
                data.metadataMap[MetaDataId::LaunchCommand] = "steam://rungameid/" + appIdStr;

                unsigned int playtimeMinutes = gameNode.value("playtime_forever", 0);
            //    data.metadataMap[MetaDataId::PlayTime] = std::to_string(playtimeMinutes * 60);

                time_t lastPlayedTimestamp = gameNode.value("rtime_last_played", 0);
                if (lastPlayedTimestamp > 0) {
                     data.metadataMap[MetaDataId::LastPlayed] = Utils::Time::timeToMetaDataString(lastPlayedTimestamp);
                }

                newGamesPayload.push_back(data);
            }
            LOG(LogInfo) << "Steam Store Refresh BG: Parsing completato. Aggiunti " << newGamesPayload.size() << " nuovi giochi online.";
        }
        else
        {
            LOG(LogError) << "Steam Store Refresh BG: Il JSON ricevuto non contiene la lista 'rgGames' attesa.";
        }
    }
    catch (const nlohmann::json::parse_error& e) {
        LOG(LogError) << "Steam Store Refresh BG: Errore fatale durante il parsing del JSON dei giochi: " << e.what();
    }
}
else {
    LOG(LogWarning) << "Steam Store Refresh BG: Nessun dato valido ricevuto dallo scraping.";
}

                // --- FASE FINALE: INVIO EVENTO ALL'INTERFACCIA ---
                SystemData* steamSystemForEvent = SystemData::getSystem("steam");
                if(steamSystemForEvent) {
                    SDL_Event event;
                    SDL_zero(event);
                    event.type = SDL_USEREVENT;
                    // !!! QUESTA E' LA CORREZIONE DEL PASSO 2 !!!
                    event.user.code = SDL_STEAM_REFRESH_COMPLETE;
                    event.user.data1 = new std::vector<NewSteamGameData>(newGamesPayload);
                    event.user.data2 = steamSystemForEvent;
                    SDL_PushEvent(&event);
                    LOG(LogInfo) << "Steam Store Refresh BG: Lavoro completato. Evento di fine inviato all'interfaccia.";
                }
            };

            // --- FASE 3: AVVIO DELLO SCRAPING SUL THREAD UI ---
            LOG(LogInfo) << "Steam Store Refresh BG: Invio richiesta di scraping al thread UI...";
            std::string steamId = mAuth->getSteamId();
            mWindow->postToUiThread([this, steamId, processScrapedData]() {
                mAPI->getOwnedGamesViaScraping(mWindow, steamId, [processScrapedData](bool success, const std::string& gameDataJson) {
                    // Quando la webview ha finito, chiama la funzione che abbiamo definito prima
                    if (success) {
                        processScrapedData(gameDataJson);
                    } else {
                        LOG(LogError) << "Scraping fallito, chiamo il processo di elaborazione con dati vuoti.";
                        processScrapedData(""); // Gestisce il fallimento
                    }
                });
            });

        } catch (const std::exception& e) {
            LOG(LogError) << "Steam Store Refresh BG: Eccezione generale: " << e.what();
        }
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
        LOG(LogWarning) << "SteamStore: Percorso di installazione Steam non valido o non trovato: " << steamPath;
        return libraryFolders;
    }

    std::string mainSteamApps = steamPath + "/steamapps";
    if (Utils::FileSystem::exists(mainSteamApps) && Utils::FileSystem::isDirectory(mainSteamApps)) {
        libraryFolders.push_back(Utils::FileSystem::getGenericPath(mainSteamApps));
    }

    std::string libraryFoldersVdfPath = steamPath + "/config/libraryfolders.vdf";
    LOG(LogDebug) << "SteamStore: Tentativo di leggere libraryfolders.vdf da: " << libraryFoldersVdfPath;

    if (Utils::FileSystem::exists(libraryFoldersVdfPath)) {
        try {
            std::map<std::string, std::string> vdfData = parseVdf(libraryFoldersVdfPath);
            // MODIFICATO: Accedi alle cartelle della libreria usando le chiavi gerarchiche (es. "0/path", "1/path")
            for (int i = 0; ; ++i) { // Cicla per trovare tutte le cartelle numerate (0, 1, 2, ...)
                std::string pathKey = std::to_string(i) + "/path";
                if (vdfData.count(pathKey)) {
                    std::string path = Utils::FileSystem::getGenericPath(vdfData[pathKey]) + "/steamapps";
                    if (Utils::FileSystem::exists(path) && Utils::FileSystem::isDirectory(path)) {
                        bool alreadyAdded = false;
                        for (const auto& existing : libraryFolders) {
                            // Confronta i percorsi genericizzati per evitare duplicati.
                            if (Utils::FileSystem::getGenericPath(existing) == Utils::FileSystem::getGenericPath(path)) {
                                alreadyAdded = true;
                                break;
                            }
                        }
                        if (!alreadyAdded) {
                            libraryFolders.push_back(Utils::FileSystem::getGenericPath(path));
                            LOG(LogInfo) << "SteamStore: Trovata cartella libreria Steam aggiuntiva: " << path;
                        }
                    }
                } else {
                    // Se la chiave numerica non esiste, significa che non ci sono più cartelle da aggiungere.
                    break;
                }
            }

        } catch (const std::exception& e) {
            LOG(LogError) << "SteamStore: Errore durante il parsing di libraryfolders.vdf: " << e.what();
        }
    } else {
        LOG(LogInfo) << "SteamStore: libraryfolders.vdf non trovato.";
    }

    if (libraryFolders.empty()) {
        LOG(LogWarning) << "SteamStore: Nessuna cartella libreria Steam trovata (nemmeno la principale?).";
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

        // MODIFICATO: Accedi alle chiavi usando il percorso completo (ad esempio, "AppState/appid")
        // in base alla struttura VDF tipica dei file .acf.
        if (appStateValues.count("AppState/appid")) gameInfo.appId = static_cast<unsigned int>(std::stoul(appStateValues["AppState/appid"]));
        if (appStateValues.count("AppState/name")) gameInfo.name = appStateValues["AppState/name"];
        if (appStateValues.count("AppState/installdir")) gameInfo.installDir = appStateValues["AppState/installdir"];
        if (appStateValues.count("AppState/StateFlags")) {
            unsigned int stateFlags = static_cast<unsigned int>(std::stoul(appStateValues["AppState/StateFlags"]));
            if ((stateFlags & 4) != 0) { // StateFlags == 4 significa "Fully Installed"
                gameInfo.fullyInstalled = true;
            }
        }
        // Aggiungi altre chiavi se necessario, ad esempio "AppState/LastOwner"
     //   if (appStateValues.count("AppState/LastOwner")) gameInfo.lastOwnerId = appStateValues["AppState/LastOwner"];
        // ... (altre chiavi utili da ACF, come "Universe", "buildid", "LastPlayed")

    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore: Errore parsing ACF file: " << acfFilePath << " - " << e.what();
        // Nota: Il LOG qui è generico. Il log precedente mostrava un LogError diverso per il refresh.
        // Questo è il log specifico per l'eccezione interna del parsing ACF.
        throw; // Rilancia l'eccezione per essere gestita a un livello superiore.
    }

    if (gameInfo.appId == 0 || gameInfo.name.empty() || gameInfo.installDir.empty()) {
        LOG(LogWarning) << "SteamStore: Dati incompleti nel manifest ACF: " << acfFilePath;
        gameInfo.fullyInstalled = false; // Assicurati che sia marcato come non completamente installato
    }
    return gameInfo;
}
#include "GameStore/Steam/SteamStore.h"
#include "guis/GuiMsgBox.h"
#include "utils/Platform.h"       // <--- VERIFICA QUESTO INCLUDE E NAMESPACE
#include "utils/FileSystemUtil.h" // <--- Assicurati che sia incluso
#include "utils/StringUtil.h"
#include "Settings.h"
#include "SystemData.h"         // <--- Assicurati che sia incluso per SystemData::getSystem
#include "views/ViewController.h"     // <--- Includi se vuoi usare reloadGameListView etc.

#include <fstream>
#include <sstream>
#include <map> // Per std::map in getGamesList

// TODO: Includere un parser VDF (Valve Data Format).
// #include "VdfParser.h"


SteamStore::SteamStore(SteamAuth* auth)
    : mAuth(auth), mAPI(nullptr), mWindow(nullptr), mInitialized(false)
{
    LOG(LogDebug) << "SteamStore: Constructor";
    if (!mAuth) {
        LOG(LogError) << "SteamStore: Auth object is null in constructor!";
    }
    mAPI = new SteamStoreAPI(mAuth);
}

SteamStore::~SteamStore()
{
    LOG(LogDebug) << "SteamStore: Destructor";
    shutdown();
    delete mAPI;
    mAPI = nullptr;
}

bool SteamStore::init(Window* window)
{
    mWindow = window;
    LOG(LogInfo) << "SteamStore: Inizializzato.";
    mInitialized = true;
    return true;
}

void SteamStore::shutdown()
{
    LOG(LogInfo) << "SteamStore: Shutdown.";
    mInitialized = false;
}

void SteamStore::showStoreUI(Window* window)
{
    LOG(LogDebug) << "SteamStore: Showing store UI";
    // Assumendo che hai aggiunto 'SteamUI mUI;' come membro in SteamStore.h
    // e hai incluso "SteamUI.h"
    mUI.showSteamSettingsMenu(window, this); // Chiama direttamente la UI di Steam
}

std::string SteamStore::getStoreName() const
{
    return "SteamStore";
}

std::string SteamStore::getGameLaunchUrl(unsigned int appId) const
{
    return "steam://rungameid/" + std::to_string(appId);
}

bool SteamStore::checkInstallationStatus(unsigned int appId, const std::vector<SteamInstalledGameInfo>& installedGames)
{
    for (const auto& game : installedGames) {
        if (game.appId == appId && game.fullyInstalled) {
            return true;
        }
    }
    return false;
}

std::vector<FileData*> SteamStore::getGamesList()
{
    LOG(LogDebug) << "SteamStore::getGamesList() - Inizio";
    std::vector<FileData*> gameList;

    if (!mInitialized || !mAPI) {
        LOG(LogError) << "SteamStore non inizializzato o API non disponibile.";
        return gameList;
    }

    // 1. Trova giochi installati localmente
    std::vector<SteamInstalledGameInfo> installedGames = findInstalledSteamGames();
    LOG(LogInfo) << "SteamStore: Trovati " << installedGames.size() << " giochi Steam installati localmente.";

    // 2. Ottieni giochi dalla libreria online se autenticato
    std::vector<Steam::OwnedGame> onlineGames;
    if (mAuth && mAuth->isAuthenticated() && !mAuth->getSteamId().empty() && !mAuth->getApiKey().empty()) {
        onlineGames = mAPI->GetOwnedGames(mAuth->getSteamId(), mAuth->getApiKey());
        LOG(LogInfo) << "SteamStore: Ottenuti " << onlineGames.size() << " giochi dalla libreria online di Steam.";
    } else {
        LOG(LogInfo) << "SteamStore: Non autenticato o credenziali mancanti, non verranno caricati giochi dalla libreria online.";
    }

    SystemData* steamSystem = SystemData::getSystem("steam");
    if (!steamSystem) {
        LOG(LogError) << "SteamStore::getGamesList() - Impossibile trovare SystemData per 'steam'. I giochi potrebbero non avere il sistema corretto.";
        // Continua comunque, FileData accetta system nullo, ma è meglio averlo.
    }

    std::map<unsigned int, FileData*> processedGames;

    // Prima i giochi installati
    for (const auto& installedGame : installedGames) {
        if (installedGame.appId == 0 || !installedGame.fullyInstalled) continue;

        std::string pseudoPath = "steam://game/" + std::to_string(installedGame.appId);
        FileData* fd = new FileData(FileType::GAME, pseudoPath, steamSystem);

        fd->setMetadata(MetaDataId::Name, installedGame.name);
        fd->setMetadata(MetaDataId::SteamAppId, std::to_string(installedGame.appId));
        fd->setMetadata(MetaDataId::Installed, "true");
        fd->setMetadata(MetaDataId::Virtual, "false");
        // Correggi errore C2838/C2065: Assicurati che MetaDataId::Path esista!
        // Se esiste, la chiamata è corretta.
        fd->setMetadata(MetaDataId::Path, installedGame.libraryFolderPath + "/common/" + installedGame.installDir); // Percorso fisico
        fd->setMetadata(MetaDataId::LaunchCommand, getGameLaunchUrl(installedGame.appId));

        processedGames[installedGame.appId] = fd;
        gameList.push_back(fd);
    }

    // Poi i giochi online
    for (const auto& onlineGame : onlineGames) {
        if (onlineGame.appId == 0) continue;

        if (processedGames.find(onlineGame.appId) == processedGames.end()) {
            std::string pseudoPath = "steam://game/" + std::to_string(onlineGame.appId);
            FileData* fd = new FileData(FileType::GAME, pseudoPath, steamSystem);

            fd->setMetadata(MetaDataId::Name, onlineGame.name);
            fd->setMetadata(MetaDataId::SteamAppId, std::to_string(onlineGame.appId));
            fd->setMetadata(MetaDataId::Installed, "false");
            fd->setMetadata(MetaDataId::Virtual, "true");
            // fd->setMetadata(MetaDataId::Path, ""); // Path non applicabile per giochi solo virtuali
            fd->setMetadata(MetaDataId::LaunchCommand, getGameLaunchUrl(onlineGame.appId));

            processedGames[onlineGame.appId] = fd;
            gameList.push_back(fd);
        } else {
            FileData* existingFd = processedGames[onlineGame.appId];
            if (existingFd->getMetadata().getName().empty() || existingFd->getMetadata().getName() == "N/A") {
                 existingFd->setMetadata(MetaDataId::Name, onlineGame.name);
            }
            // TODO: Aggiorna playtime se necessario
        }
    }

    LOG(LogInfo) << "SteamStore::getGamesList() - Fine, restituiti " << gameList.size() << " giochi.";
    return gameList;
}


bool SteamStore::installGame(const std::string& gameId)
{
    LOG(LogDebug) << "SteamStore::installGame per AppID: " << gameId;
    try {
        unsigned int appId = std::stoul(gameId);
       std::string launchUrl = "steam://run/" + gameId;
    LOG(LogInfo) << "Tentativo di installare/lanciare gioco Steam con URL: " << launchUrl;
    // Platform::openUrl(launchUrl); // Non usare if (!) o return !
    Utils::Platform::openUrl(launchUrl); // <<--- CORREZIONE IMPORTANTE: Usa Utils::Platform::openUrl
    return true; // Indica che il comando è stato inviato
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore::installGame - Errore AppID non valido: " << e.what();
        return false;
    }
}

bool SteamStore::uninstallGame(const std::string& gameId)
{
    LOG(LogDebug) << "SteamStore::uninstallGame per AppID: " << gameId;
    try {
        unsigned int appId = std::stoul(gameId);
        std::string uninstallUrl = "steam://uninstall/" + gameId;
    LOG(LogInfo) << "Tentativo di disinstallare gioco Steam con URL: " << uninstallUrl;
    // Platform::openUrl(uninstallUrl); // Non usare if (!) o return !
    Utils::Platform::openUrl(uninstallUrl); // <<--- CORREZIONE IMPORTANTE: Usa Utils::Platform::openUrl
    return true; // Indica che il comando è stato inviato
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamStore::uninstallGame - Errore AppID non valido: " << e.what();
        return false;
    }
}

bool SteamStore::updateGame(const std::string& gameId)
{
    LOG(LogDebug) << "SteamStore::updateGame per AppID: " << gameId;
    return installGame(gameId);
}

// --- Logica per trovare giochi installati ---

std::string SteamStore::getSteamInstallationPath() {
    // ... (logica precedente, assicurati che funzioni o usa un percorso fisso per test) ...
    #ifdef _WIN32
        // ... logica Windows ...
        // Esempio placeholder:
         std::string path_candidate = "C:/Program Files (x86)/Steam";
         if (Utils::FileSystem::exists(path_candidate)) return path_candidate;
         path_candidate = "C:/Program Files/Steam";
         if (Utils::FileSystem::exists(path_candidate)) return path_candidate;
         LOG(LogError) << "Steam path non trovato"; return "";
    #elif __linux__
        // ... logica Linux ...
        // Esempio placeholder:
        std::string home = Utils::FileSystem::getHomePath();
        std::string path1 = home + "/.steam/steam"; // Spesso è qui che si trova steamapps su Linux
        if (Utils::FileSystem::exists(path1 + "/steamapps")) return path1;
        path1 = home + "/.steam/root";
         if (Utils::FileSystem::exists(path1 + "/steamapps")) return path1;
        std::string path2 = home + "/.local/share/Steam";
         if (Utils::FileSystem::exists(path2 + "/steamapps")) return path2;
        LOG(LogError) << "Steam path non trovato"; return "";
    #else
        LOG(LogError) << "Steam path detection non implementato"; return "";
    #endif
}

std::vector<std::string> SteamStore::getSteamLibraryFolders(const std::string& steamPath) {
    std::vector<std::string> libraryFolders;
    if (steamPath.empty() || !Utils::FileSystem::exists(steamPath)) {
        LOG(LogWarning) << "SteamStore: Percorso installazione Steam non valido o non trovato: " << steamPath;
        return libraryFolders;
    }

    std::string mainSteamApps = steamPath + "/steamapps";
    if (Utils::FileSystem::exists(mainSteamApps) && Utils::FileSystem::isDirectory(mainSteamApps)) {
         // Correggi errori C2039/C3861: Usa la funzione corretta se resolvePath non esiste
         // Se non è necessario, rimuovi la chiamata. getGenericPath potrebbe essere sufficiente.
         // libraryFolders.push_back(Utils::FileSystem::resolvePath(mainSteamApps));
         libraryFolders.push_back(Utils::FileSystem::getGenericPath(mainSteamApps)); // <--- CORREZIONE (esempio)
    }


    std::string libraryFoldersVdfPath = mainSteamApps + "/libraryfolders.vdf";
    LOG(LogDebug) << "SteamStore: Tento di leggere libraryfolders.vdf da: " << libraryFoldersVdfPath;

    if (Utils::FileSystem::exists(libraryFoldersVdfPath)) {
        // TODO: Implementare un parser VDF robusto qui.
        // La logica placeholder che segue è INAFFIDABILE.
        std::ifstream vdfFile(libraryFoldersVdfPath);
        std::string line;
        if (vdfFile.is_open()) {
            while (getline(vdfFile, line)) {
                line = Utils::String::trim(line);
                if (line.rfind("\"path\"", 0) == 0) {
                    size_t firstQuote = line.find('\"', 6);
                    size_t secondQuote = line.find('\"', firstQuote + 1);
                    if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                        std::string path = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                        path = Utils::String::replace(path, "\\\\", "/"); // Normalizza separatori
                        path = Utils::FileSystem::getGenericPath(path); // <--- Usa getGenericPath

                        if (Utils::FileSystem::exists(path)) {
                            std::string steamAppsPath = path + "/steamapps";
                             if (Utils::FileSystem::exists(steamAppsPath) && Utils::FileSystem::isDirectory(steamAppsPath)) {
                                bool alreadyAdded = false;
                                for(const auto& existing : libraryFolders) {
                                    // Correggi errori C2039/C3861: Usa la funzione corretta
                                    // if (Utils::FileSystem::arePathsEquivalent(existing, steamAppsPath)) { // <--- Funzione problematica
                                    // Alternativa: confronto stringhe dopo normalizzazione
                                     if (Utils::FileSystem::getGenericPath(existing) == Utils::FileSystem::getGenericPath(steamAppsPath)) { // <--- CORREZIONE (esempio)
                                        alreadyAdded = true;
                                        break;
                                    }
                                }
                                if (!alreadyAdded) {
                                    libraryFolders.push_back(Utils::FileSystem::getGenericPath(steamAppsPath)); // <--- CORREZIONE (esempio)
                                    LOG(LogInfo) << "SteamStore: Trovata cartella libreria Steam aggiuntiva: " << steamAppsPath;
                                }
                            }
                        }
                    }
                }
            }
            vdfFile.close();
        } else {
            LOG(LogError) << "SteamStore: Impossibile aprire libraryfolders.vdf: " << libraryFoldersVdfPath;
        }
    } else {
        LOG(LogInfo) << "SteamStore: libraryfolders.vdf non trovato.";
    }

    if (libraryFolders.empty()) {
         LOG(LogWarning) << "SteamStore: Nessuna cartella libreria Steam trovata (nemmeno quella principale?).";
    }
    return libraryFolders;
}

std::vector<SteamInstalledGameInfo> SteamStore::findInstalledSteamGames() {
    std::vector<SteamInstalledGameInfo> installedGames;
    std::string steamInstallPath = getSteamInstallationPath();
    // Non è necessario controllare se è vuoto qui, getSteamLibraryFolders lo gestisce

    std::vector<std::string> libraryPaths = getSteamLibraryFolders(steamInstallPath);

    for (const auto& libPath : libraryPaths) { // libPath è già il percorso a ".../steamapps"
        LOG(LogDebug) << "SteamStore: Scansione cartella libreria: " << libPath;
        if (!Utils::FileSystem::exists(libPath) || !Utils::FileSystem::isDirectory(libPath)) {
            LOG(LogWarning) << "SteamStore: Cartella libreria " << libPath << " non valida o non accessibile.";
            continue;
        }

        // CORREZIONE per errori C2039/C2672/C2100 usando getDirectoryFiles come suggerito
        // Assumendo che Utils::FileSystem::FileInfo esista e abbia il membro .path
        auto filesInDir = Utils::FileSystem::getDirectoryFiles(libPath); // <--- CORREZIONE

        for (const auto& fileEntry : filesInDir) { // <--- CORREZIONE: Itera sulla lista restituita
            const std::string& currentPath = fileEntry.path; // <--- CORREZIONE: Ottieni il percorso

            // Assicurati che le funzioni isRegularFile, getFileName, getExtension accettino std::string
            if (Utils::FileSystem::isRegularFile(currentPath) &&
                Utils::String::startsWith(Utils::FileSystem::getFileName(currentPath), "appmanifest_") &&
                Utils::String::toLower(Utils::FileSystem::getExtension(currentPath)) == ".acf")
            {
                LOG(LogDebug) << "SteamStore: Trovato file manifest: " << currentPath;
                SteamInstalledGameInfo gameInfo = parseAppManifest(currentPath);
                if (gameInfo.appId != 0 && gameInfo.fullyInstalled) {
                    // Correzione errore C2039/C3861: getGenericPath invece di resolvePath se necessario
                    gameInfo.libraryFolderPath = Utils::FileSystem::getGenericPath(libPath); // <--- CORREZIONE (esempio)
                    installedGames.push_back(gameInfo);
                    LOG(LogInfo) << "SteamStore: Gioco installato rilevato: " << gameInfo.name << " (AppID: " << gameInfo.appId << ") in " << libPath;
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

    // TODO: Implementare un parser VDF robusto qui.
    // La logica placeholder che segue è INAFFIDABILE e potrebbe non funzionare correttamente.
    std::ifstream acfFile(acfFilePath);
    std::string line;
    std::map<std::string, std::string> appStateValues;

    if (acfFile.is_open()) {
        // Parser VDF MOLTO semplificato (cerca righe chiave/valore tra virgolette)
        while (getline(acfFile, line)) {
            line = Utils::String::trim(line);
            if (line.length() > 4 && line[0] == '"') { // Se inizia con "
                size_t keyEnd = line.find('"', 1);
                if (keyEnd != std::string::npos && keyEnd + 1 < line.length()) {
                    size_t valStart = line.find('"', keyEnd + 1);
                    if (valStart != std::string::npos && valStart + 1 < line.length()) {
                        size_t valEnd = line.find('"', valStart + 1);
                        if (valEnd != std::string::npos) {
                            std::string key = line.substr(1, keyEnd - 1);
                            std::string value = line.substr(valStart + 1, valEnd - valStart - 1);
                            appStateValues[key] = value;
                        }
                    }
                }
            }
        }
        acfFile.close();

        try { // Aggiungi try-catch per stoul
            if (appStateValues.count("appid")) gameInfo.appId = static_cast<unsigned int>(std::stoul(appStateValues["appid"]));
            if (appStateValues.count("name")) gameInfo.name = appStateValues["name"];
            if (appStateValues.count("installdir")) gameInfo.installDir = appStateValues["installdir"];
            if (appStateValues.count("StateFlags")) {
                unsigned int stateFlags = static_cast<unsigned int>(std::stoul(appStateValues["StateFlags"]));
                // Flag 4 = Fully installed (da Playnite)
                if ((stateFlags & 4) != 0) {
                     gameInfo.fullyInstalled = true;
                }
            }
        } catch (const std::invalid_argument& ia) {
            LOG(LogError) << "SteamStore: Errore conversione numero nel manifest ACF: " << acfFilePath << " - " << ia.what();
            gameInfo.appId = 0; // Invalida se c'è errore
            gameInfo.fullyInstalled = false;
        } catch (const std::out_of_range& oor) {
             LOG(LogError) << "SteamStore: Numero fuori range nel manifest ACF: " << acfFilePath << " - " << oor.what();
             gameInfo.appId = 0;
             gameInfo.fullyInstalled = false;
        }

    } else {
        LOG(LogError) << "SteamStore: Impossibile aprire il file manifest ACF: " << acfFilePath;
    }

    if (gameInfo.appId == 0 || gameInfo.name.empty() || gameInfo.installDir.empty()) {
        // Non loggare come warning se il parser è un placeholder, altrimenti avrai molti warning
        // LOG(LogWarning) << "SteamStore: Parsing fallito o dati incompleti per manifest ACF: " << acfFilePath;
        gameInfo.fullyInstalled = false;
    }
    return gameInfo;
}
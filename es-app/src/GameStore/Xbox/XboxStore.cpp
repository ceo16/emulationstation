#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/Xbox/XboxUI.h" 
#include "Log.h"
#include "SystemData.h"
#include "FileData.h"   
#include "MetaData.h"   
#include "Window.h"
#include "Settings.h"   
#include "SdlEvents.h"  // Per SDL_XBOX_REFRESH_COMPLETE e SDL_GAMELIST_UPDATED
#include "utils/StringUtil.h" 
#include "utils/Platform.h" 
#include "utils/TimeUtil.h"   
#include "PlatformId.h" 
#include "guis/GuiMsgBox.h" 
#include "LocaleES.h" 
#include "views/ViewController.h" 

#include <set> 
#include <algorithm> 
#include <future> 

#ifdef _WIN32
#include <Windows.h>
#include <appmodel.h> 
#include <PathCch.h>  
#pragma comment(lib, "Pathcch.lib")

std::string ConvertWideToUtf8_XboxStore_Unique(const WCHAR* wideString) { 
    if (wideString == nullptr) return "";
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) return "";
    std::string utf8String(bufferSize -1, 0); 
    WideCharToMultiByte(CP_UTF8, 0, wideString, -1, &utf8String[0], bufferSize, NULL, NULL);
    return utf8String;
}
#endif

XboxStore::XboxStore(XboxAuth* auth, Window* window_param) // window_param per chiarezza
    : GameStore(), // << Chiama il costruttore di default di GameStore
      mAuth(auth),
      mAPI(nullptr),
      mInstanceWindow(window_param), // << Inizializza il membro di XboxStore
      _initialized(false)
{
    if (mAuth) {
        mAPI = new XboxStoreAPI(mAuth);
    } else {
        LOG(LogError) << "XboxStore created with null XboxAuth pointer!";
    }
    LOG(LogDebug) << "XboxStore constructor finished.";
}

XboxStore::~XboxStore() {
    delete mAPI;
    mAPI = nullptr;
    LOG(LogDebug) << "XboxStore destructor finished.";
}

bool XboxStore::init(Window* window_param_from_manager) {
    // mInstanceWindow è già stato impostato dal costruttore di XboxStore.
    // Il parametro window_param_from_manager qui è quello passato da GameStoreManager::init.
    // Idealmente, dovrebbe essere lo stesso. Puoi aggiungere un controllo o decidere quale usare.
    if (!this->mInstanceWindow && window_param_from_manager) {
        LOG(LogWarning) << "XboxStore::init - mInstanceWindow era nullo, usando il parametro window da init.";
        this->mInstanceWindow = window_param_from_manager; // Fallback
    } else if (window_param_from_manager && this->mInstanceWindow != window_param_from_manager) {
        LOG(LogWarning) << "XboxStore::init - Il parametro window di init è diverso da mInstanceWindow. Si usa mInstanceWindow.";
    }

    if (!mAuth || !mAPI) {
        LOG(LogError) << "XboxStore::init - Moduli Auth o API non disponibili.";
        _initialized = false;
        return false;
    }
    if (!this->mInstanceWindow) { // Controlla il membro di XboxStore
        LOG(LogError) << "XboxStore::init - Riferimento alla finestra nullo!";
        _initialized = false;
        return false;
    }
    LOG(LogInfo) << "XboxStore inizializzato con successo.";
    _initialized = true;
    return true;
}

void XboxStore::showStoreUI(Window* window_context_param) {
    // Usa principalmente mInstanceWindow. Il parametro window_context_param potrebbe essere
    // ridondante o usato come fallback/verifica.
    Window* targetWindow = this->mInstanceWindow;

    if (!targetWindow) {
         if (window_context_param) {
            LOG(LogWarning) << "XboxStore::showStoreUI - mInstanceWindow era nullo, usando il parametro window_context_param.";
            targetWindow = window_context_param;
         } else {
            LOG(LogError) << "XboxStore::showStoreUI - Contesto Window nullo! Impossibile mostrare Xbox UI.";
            return;
         }
    } else if (window_context_param && targetWindow != window_context_param) {
         LOG(LogWarning) << "XboxStore::showStoreUI - window_context_param diverso da mInstanceWindow. Si usa mInstanceWindow.";
    }

    if (!_initialized) {
         LOG(LogError) << "XboxStore::showStoreUI - Store non inizializzato!";
         if (targetWindow) // Solo se abbiamo una finestra per mostrare il messaggio
            targetWindow->pushGui(new GuiMsgBox(targetWindow, _("ERRORE XBOX") + std::string("\n") + _("Il negozio Xbox non è stato inizializzato correttamente."), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
         return;
    }
    // ... (resto della logica come prima, usando targetWindow)
    targetWindow->pushGui(new XboxUI(targetWindow, this));
}
std::string XboxStore::getStoreName() const {
    return "XboxStore";
}

std::string XboxStore::getGameLaunchCommand(const std::string& pfn) {
    if (pfn.empty()) return "";
    return "explorer.exe shell:appsFolder\\" + pfn + "!App";
}

std::vector<Xbox::InstalledXboxGameInfo> XboxStore::findInstalledXboxGames() {
    std::vector<Xbox::InstalledXboxGameInfo> installedGames;
#ifdef _WIN32
    LOG(LogDebug) << "XboxStore: Searching for installed UWP games...";
    LOG(LogWarning) << "XboxStore::findInstalledXboxGames() - UWP game detection for Windows is a placeholder and not fully implemented.";
#else
    LOG(LogWarning) << "XboxStore::findInstalledXboxGames() - UWP game detection is only available on Windows.";
#endif
    return installedGames;
}

std::vector<FileData*> XboxStore::getGamesList() {
    LOG(LogDebug) << "XboxStore::getGamesList() called.";
    std::vector<FileData*> gameFiles;
    SystemData* system = SystemData::getSystem("xbox"); 
    if (!system) {
        LOG(LogError) << "XboxStore::getGamesList - System 'xbox' not found!";
        return gameFiles;
    }
    if (!mAuth || !mAPI) {
         LOG(LogError) << "XboxStore::getGamesList - Auth or API module not initialized!";
        return gameFiles;
    }

    std::map<std::string, FileData*> existingGamesByPfn;
    try {
        for (auto* fd_base : system->getRootFolder()->getChildren()) { 
            FileData* fd = dynamic_cast<FileData*>(fd_base); 
            if (fd && fd->getType() == GAME) { 
                std::string pfn = fd->getMetadata().get(MetaDataId::XboxPfn);
                if (!pfn.empty()) {
                    existingGamesByPfn[pfn] = fd;
                }
            }
        }
        LOG(LogDebug) << "XboxStore::getGamesList - Found " << existingGamesByPfn.size() << " existing games by PFN in system 'xbox'.";
    } catch (const std::exception& e) {
        LOG(LogError) << "XboxStore::getGamesList - Exception getting existing games: " << e.what();
    }

    std::vector<Xbox::InstalledXboxGameInfo> installedApps = findInstalledXboxGames();
    LOG(LogInfo) << "XboxStore::getGamesList - Found " << installedApps.size() << " potentially installed UWP apps.";
    for (const auto& installedApp : installedApps) {
        if (installedApp.pfn.empty()) continue;

        auto it = existingGamesByPfn.find(installedApp.pfn);
        if (it != existingGamesByPfn.end()) {
            FileData* fd = it->second;
            LOG(LogDebug) << "XboxStore: Updating existing installed game: " << installedApp.displayName;
            fd->getMetadata().set(MetaDataId::Name, installedApp.displayName); 
            fd->getMetadata().set(MetaDataId::Installed, "true");
            fd->getMetadata().set(MetaDataId::Virtual, "false"); 
            fd->getMetadata().set(MetaDataId::Path, XboxStore::getGameLaunchCommand(installedApp.pfn)); 
            fd->getMetadata().set(MetaDataId::LaunchCommand, XboxStore::getGameLaunchCommand(installedApp.pfn));
            fd->getMetadata().setDirty();
            gameFiles.push_back(fd);
            existingGamesByPfn.erase(it); 
        } else {
            LOG(LogInfo) << "XboxStore: Adding new installed game to list: " << installedApp.displayName;
            std::string pseudoPath = "xbox://pfn/" + installedApp.pfn;
            FileData* newGame = new FileData(FileType::GAME, pseudoPath, system);
            newGame->setMetadata(MetaDataId::Name, installedApp.displayName);
            newGame->setMetadata(MetaDataId::XboxPfn, installedApp.pfn);
            newGame->setMetadata(MetaDataId::Installed, "true");
            newGame->setMetadata(MetaDataId::Virtual, "false");
            newGame->setMetadata(MetaDataId::LaunchCommand, XboxStore::getGameLaunchCommand(installedApp.pfn));
            newGame->setMetadata(MetaDataId::Path, XboxStore::getGameLaunchCommand(installedApp.pfn)); 
            newGame->getMetadata().setDirty();
            gameFiles.push_back(newGame);
        }
    }

    if (mAuth->isAuthenticated()) {
        LOG(LogDebug) << "XboxStore: Fetching online library titles...";
        std::vector<Xbox::OnlineTitleInfo> onlineTitles = mAPI->GetLibraryTitles();
        LOG(LogDebug) << "XboxStore: Fetched " << onlineTitles.size() << " titles from online library.";

        for (const auto& title : onlineTitles) {
            if (title.pfn.empty() && (title.type != "Game" && title.mediaItemType != "DGame")) { 
                LOG(LogDebug) << "XboxStore: Skipping online title due to missing PFN and not being a Game type: " << title.name;
                continue;
            }
            bool isPCGame = false;
            for (const std::string& device : title.devices) {
                if (device == "PC") {
                    isPCGame = true;
                    break;
                }
            }
            if (!isPCGame) {
                 LOG(LogDebug) << "XboxStore: Skipping online title as it's not marked for PC: " << title.name;
                continue;
            }

            auto it = existingGamesByPfn.find(title.pfn);
            if (it != existingGamesByPfn.end()) {
                FileData* fd = it->second;
                LOG(LogDebug) << "XboxStore: Online title already exists: " << title.name;
                if (fd->getMetadata().get(MetaDataId::Name).empty() || fd->getMetadata().get(MetaDataId::Name) == title.pfn) {
                    fd->setMetadata(MetaDataId::Name, title.name.empty() ? title.pfn : title.name);
                }
                if (fd->getMetadata().get(MetaDataId::Developer).empty() && !title.detail.developerName.empty()) 
                    fd->setMetadata(MetaDataId::Developer, title.detail.developerName);
                if (fd->getMetadata().get(MetaDataId::Publisher).empty() && !title.detail.publisherName.empty()) 
                    fd->setMetadata(MetaDataId::Publisher, title.detail.publisherName);
                if (fd->getMetadata().get(MetaDataId::ReleaseDate).empty() && !title.detail.releaseDate.empty()) {
                     time_t release_t = Utils::Time::iso8601ToTime(title.detail.releaseDate);
                     if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                         fd->setMetadata(MetaDataId::ReleaseDate, Utils::Time::timeToMetaDataString(release_t));
                     }
                }
                fd->getMetadata().setDirty();
                if (std::find(gameFiles.begin(), gameFiles.end(), fd) == gameFiles.end()) { 
                     gameFiles.push_back(fd);
                }
                existingGamesByPfn.erase(it);
            } else {
                LOG(LogInfo) << "XboxStore: Adding new library game to list: " << title.name;
                std::string pseudoPath = "xbox://pfn/" + title.pfn;
                FileData* newGame = new FileData(FileType::GAME, pseudoPath, system);
                newGame->setMetadata(MetaDataId::Name, title.name.empty() ? title.pfn : title.name);
                newGame->setMetadata(MetaDataId::XboxPfn, title.pfn);
                newGame->setMetadata(MetaDataId::XboxTitleId, title.titleId); 
                newGame->setMetadata(MetaDataId::Installed, "false"); 
                newGame->setMetadata(MetaDataId::Virtual, "true");
                newGame->setMetadata(MetaDataId::LaunchCommand, XboxStore::getGameLaunchCommand(title.pfn));
                newGame->setMetadata(MetaDataId::Path, XboxStore::getGameLaunchCommand(title.pfn)); 

                if (!title.detail.developerName.empty()) newGame->setMetadata(MetaDataId::Developer, title.detail.developerName);
                if (!title.detail.publisherName.empty()) newGame->setMetadata(MetaDataId::Publisher, title.detail.publisherName);
                if (!title.detail.releaseDate.empty()) {
                     time_t release_t = Utils::Time::iso8601ToTime(title.detail.releaseDate);
                     if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                         newGame->setMetadata(MetaDataId::ReleaseDate, Utils::Time::timeToMetaDataString(release_t));
                     }
                }
                if (!title.detail.description.empty()) newGame->setMetadata(MetaDataId::Desc, title.detail.description);
                if (!title.mediaItemType.empty()) newGame->setMetadata(MetaDataId::XboxMediaType, title.mediaItemType);
                else if (!title.type.empty()) newGame->setMetadata(MetaDataId::XboxMediaType, title.type);

                if (!title.devices.empty()) {
                    std::string devicesStr;
                    for (size_t i = 0; i < title.devices.size(); ++i) {
                        devicesStr += title.devices[i] + (i < title.devices.size() - 1 ? ", " : "");
                    }
                    newGame->setMetadata(MetaDataId::XboxDevices, devicesStr);
                }
                newGame->getMetadata().setDirty();
                gameFiles.push_back(newGame);
            }
        }
    } else {
        LOG(LogWarning) << "XboxStore::getGamesList - Not authenticated, skipping online library.";
    }

    for (const auto& pair : existingGamesByPfn) {
        LOG(LogDebug) << "XboxStore: Including game from gamelist.xml not found elsewhere: " << pair.second->getName();
        if (std::find(gameFiles.begin(), gameFiles.end(), pair.second) == gameFiles.end()) {
            gameFiles.push_back(pair.second);
        }
    }

    LOG(LogInfo) << "XboxStore::getGamesList - Returning " << gameFiles.size() << " FileData entries for Xbox system.";
    return gameFiles;
}

void XboxStore::shutdown() {
    LOG(LogInfo) << "XboxStore shutting down.";
    // Eventuale pulizia specifica di XboxStore, se mAPI o mAuth necessitano
    // di essere deallocati o chiusi in modo specifico qui.
    // Se mAPI e mAuth sono gestiti dal distruttore, questo metodo potrebbe essere semplice.
    // delete mAPI; // Se non lo fa già il distruttore e deve essere fatto prima
    // mAPI = nullptr;
    // delete mAuth; // Se XboxStore è responsabile della sua deallocazione e non lo fa il distruttore
    // mAuth = nullptr;

    _initialized = false; // Imposta lo stato a non inizializzato
}

std::future<void> XboxStore::refreshGamesListAsync() {
    return std::async(std::launch::async, [this]() {
        LOG(LogInfo) << "Xbox Store Refresh BG: Starting refreshGamesListAsync...";
        SystemData* xboxSystem = SystemData::getSystem("xbox"); 

        if (!_initialized || !mAuth || !mAPI) {
            LOG(LogError) << "Xbox Store Refresh BG: Store not ready (uninitialized, no auth, or no API).";
            if (xboxSystem) { 
                auto emptyPayload = new std::vector<NewXboxGameData>();
                SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
                event.user.code = SDL_XBOX_REFRESH_COMPLETE;
                event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
                SDL_PushEvent(&event);
            }
            return;
        }
        if (!mAuth->isAuthenticated()) {
            LOG(LogWarning) << "Xbox Store Refresh BG: Not authenticated. Aborting refresh.";
            if (xboxSystem) { 
                auto emptyPayload = new std::vector<NewXboxGameData>();
                SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
                event.user.code = SDL_XBOX_REFRESH_COMPLETE;
                event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
                SDL_PushEvent(&event);
            }
            return;
        }

        if (!xboxSystem || !xboxSystem->getRootFolder()) {
            LOG(LogError) << "Xbox Store Refresh BG: Cannot find Xbox system or its root folder.";
            return;
        }

        std::set<std::string> existingPfnsInSystem; 
        std::map<std::string, FileData*> fileDataMapByPfn; 

        try {
             std::vector<FileData*> currentSystemGames = xboxSystem->getRootFolder()->getFilesRecursive(GAME, true);
             for (FileData* fd : currentSystemGames) {
                 if (fd) {
                     std::string pfn = fd->getMetadata().get(MetaDataId::XboxPfn);
                     if (!pfn.empty()) {
                         existingPfnsInSystem.insert(pfn);
                         fileDataMapByPfn[pfn] = fd;
                     }
                 }
             }
             LOG(LogDebug) << "Xbox Store Refresh BG: Found " << existingPfnsInSystem.size() << " existing PFNs in system '" << xboxSystem->getName() << "'.";
        } catch (const std::exception& e) {
            LOG(LogError) << "Xbox Store Refresh BG: Exception collecting existing PFNs: " << e.what();
            auto emptyPayload = new std::vector<NewXboxGameData>();
            SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
            event.user.code = SDL_XBOX_REFRESH_COMPLETE;
            event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
            SDL_PushEvent(&event);
            return;
        }

        auto newGamesPayload = new std::vector<NewXboxGameData>();
        bool metadataPotentiallyChanged = false;

        std::vector<Xbox::InstalledXboxGameInfo> installedGames = findInstalledXboxGames();
        LOG(LogInfo) << "Xbox Store Refresh BG: Found " << installedGames.size() << " installed UWP apps.";
        for (const auto& installedGame : installedGames) {
            if (installedGame.pfn.empty()) continue;

            if (existingPfnsInSystem.find(installedGame.pfn) == existingPfnsInSystem.end()) { 
                NewXboxGameData data;
                data.pfn = installedGame.pfn;
                data.pseudoPath = "xbox://pfn/" + installedGame.pfn;
                data.metadataMap[MetaDataId::Name] = installedGame.displayName;
                data.metadataMap[MetaDataId::XboxPfn] = installedGame.pfn;
                data.metadataMap[MetaDataId::Installed] = "true";
                data.metadataMap[MetaDataId::Virtual] = "false";
                data.metadataMap[MetaDataId::LaunchCommand] = XboxStore::getGameLaunchCommand(installedGame.pfn);
                data.metadataMap[MetaDataId::Path] = XboxStore::getGameLaunchCommand(installedGame.pfn);
                newGamesPayload->push_back(data);
                LOG(LogDebug) << "  Payload Add (Installed New): " << installedGame.displayName;
            } else { 
                FileData* fd = fileDataMapByPfn[installedGame.pfn];
                if (fd && fd->getMetadata().get(MetaDataId::Installed) != "true") {
                    fd->getMetadata().set(MetaDataId::Installed, "true");
                    fd->getMetadata().set(MetaDataId::Virtual, "false"); 
                    fd->getMetadata().setDirty();
                    metadataPotentiallyChanged = true;
                    LOG(LogDebug) << "  Marked existing game as Installed: " << installedGame.displayName;
                }
            }
        }

        std::vector<Xbox::OnlineTitleInfo> onlineTitles = mAPI->GetLibraryTitles();
        LOG(LogInfo) << "Xbox Store Refresh BG: Fetched " << onlineTitles.size() << " titles from online library.";
        std::set<std::string> pfnsFromOnlineLibrary;

        for (const auto& onlineGame : onlineTitles) {
            if (onlineGame.pfn.empty() || (onlineGame.type != "Game" && onlineGame.mediaItemType != "DGame")) continue;
            
            pfnsFromOnlineLibrary.insert(onlineGame.pfn); 

            bool isPC = false;
            for(const auto& dev : onlineGame.devices) if(dev == "PC") isPC = true;
            if (!isPC) continue; 

            bool alreadyInPayloadAsInstalled = false;
            for(const auto& pl_game : *newGamesPayload) {
                if(pl_game.pfn == onlineGame.pfn && pl_game.metadataMap.count(MetaDataId::Installed) && pl_game.metadataMap.at(MetaDataId::Installed) == "true") {
                    alreadyInPayloadAsInstalled = true;
                    break;
                }
            }

            if (existingPfnsInSystem.find(onlineGame.pfn) == existingPfnsInSystem.end() && !alreadyInPayloadAsInstalled) {
                NewXboxGameData data;
                data.pfn = onlineGame.pfn;
                data.pseudoPath = "xbox://pfn/" + onlineGame.pfn;
                data.metadataMap[MetaDataId::Name] = onlineGame.name.empty() ? onlineGame.pfn : onlineGame.name;
                data.metadataMap[MetaDataId::XboxPfn] = onlineGame.pfn;
                data.metadataMap[MetaDataId::XboxTitleId] = onlineGame.titleId;
                data.metadataMap[MetaDataId::Installed] = "false"; 
                data.metadataMap[MetaDataId::Virtual] = "true";
                data.metadataMap[MetaDataId::LaunchCommand] = XboxStore::getGameLaunchCommand(onlineGame.pfn);
                data.metadataMap[MetaDataId::Path] = XboxStore::getGameLaunchCommand(onlineGame.pfn);
                if (!onlineGame.detail.developerName.empty()) data.metadataMap[MetaDataId::Developer] = onlineGame.detail.developerName;
                if (!onlineGame.detail.publisherName.empty()) data.metadataMap[MetaDataId::Publisher] = onlineGame.detail.publisherName;
                if (!onlineGame.detail.releaseDate.empty()) {
                     time_t release_t = Utils::Time::iso8601ToTime(onlineGame.detail.releaseDate);
                     if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                         data.metadataMap[MetaDataId::ReleaseDate] = Utils::Time::timeToMetaDataString(release_t);
                     }
                }
                if (!onlineGame.detail.description.empty()) data.metadataMap[MetaDataId::Desc] = onlineGame.detail.description;
                if (!onlineGame.mediaItemType.empty()) data.metadataMap[MetaDataId::XboxMediaType] = onlineGame.mediaItemType;
                else if (!onlineGame.type.empty()) data.metadataMap[MetaDataId::XboxMediaType] = onlineGame.type;
                if (!onlineGame.devices.empty()) {
                    std::string devicesStr;
                    for (size_t i = 0; i < onlineGame.devices.size(); ++i) {
                        devicesStr += onlineGame.devices[i] + (i < onlineGame.devices.size() - 1 ? ", " : "");
                    }
                    data.metadataMap[MetaDataId::XboxDevices] = devicesStr;
                }
                newGamesPayload->push_back(data);
                LOG(LogDebug) << "  Payload Add (Online New): " << onlineGame.name;
            } else if (existingPfnsInSystem.count(onlineGame.pfn)) { 
                 FileData* fd = fileDataMapByPfn[onlineGame.pfn];
                 if (fd) {
                    bool gameMetaChanged = false;
                    if (fd->getMetadata().get(MetaDataId::Desc).empty() && !onlineGame.detail.description.empty()) {
                        fd->setMetadata(MetaDataId::Desc, onlineGame.detail.description); gameMetaChanged = true;
                    }
                    if (gameMetaChanged) {
                        fd->getMetadata().setDirty();
                        metadataPotentiallyChanged = true;
                        LOG(LogDebug) << "  Updated metadata for existing online game: " << onlineGame.name;
                    }
                 }
            }
        }
        
        for (const auto& pfn_in_es : existingPfnsInSystem) {
            bool foundAsInstalled = false;
            for(const auto& installed : installedGames) if(installed.pfn == pfn_in_es) { foundAsInstalled = true; break; }

            bool foundInOnlineLib = pfnsFromOnlineLibrary.count(pfn_in_es);

            if (!foundAsInstalled && fileDataMapByPfn.count(pfn_in_es)) {
                FileData* fd = fileDataMapByPfn[pfn_in_es];
                if (fd && fd->getMetadata().get(MetaDataId::Installed) == "true") { 
                    LOG(LogInfo) << "Xbox Store Refresh BG: Game " << fd->getName() << " (PFN: " << pfn_in_es << ") is no longer installed. Marking as not installed.";
                    fd->getMetadata().set(MetaDataId::Installed, "false");
                    if (!foundInOnlineLib) {
                         fd->getMetadata().set(MetaDataId::Virtual, "true"); 
                         LOG(LogInfo) << "Xbox Store Refresh BG: Game " << fd->getName() << " also not in online library.";
                    }
                    fd->getMetadata().setDirty();
                    metadataPotentiallyChanged = true;
                }
            }
        }

        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_USEREVENT;
        event.user.code = SDL_XBOX_REFRESH_COMPLETE;
        event.user.data1 = newGamesPayload; 
        event.user.data2 = xboxSystem; 
        SDL_PushEvent(&event);

        if (newGamesPayload->empty() && metadataPotentiallyChanged && ViewController::get()) { 
             LOG(LogInfo) << "Xbox Store Refresh BG: Only metadata changed for existing games. Requesting UI reload.";
             SDL_Event meta_event;
             SDL_zero(meta_event);
             meta_event.type = SDL_USEREVENT;
             meta_event.user.code = SDL_GAMELIST_UPDATED; 
             meta_event.user.data1 = xboxSystem;
             meta_event.user.data2 = nullptr; 
             SDL_PushEvent(&meta_event);
        }

        LOG(LogInfo) << "Xbox Store Refresh BG: Finished. Pushed " << newGamesPayload->size() << " new games. Metadata changed: " << metadataPotentiallyChanged;
    });
}

std::future<void> XboxStore::updateGamesMetadataAsync(SystemData* system, const std::vector<std::string>& gamePfnsToUpdate) {
     return std::async(std::launch::async, [this, system, gamePfnsToUpdate]() {
        LOG(LogInfo) << "Xbox Store MetaUpdate BG: Starting for " << gamePfnsToUpdate.size() << " PFNs.";
        if (!_initialized || !mAuth || !mAPI || !system) {
            LOG(LogError) << "Xbox Store MetaUpdate BG: Store not ready or system invalid.";
            return;
        }
        if (!mAuth->isAuthenticated()) {
            LOG(LogWarning) << "Xbox Store MetaUpdate BG: Not authenticated.";
            return;
        }

        bool anyMetadataChanged = false;
        int successCount = 0;

        for (const std::string& pfn : gamePfnsToUpdate) {
            if (pfn.empty()) continue;

            FileData* gameFile = nullptr;
            std::string pseudoPathToFind = "xbox://pfn/" + pfn;
            for (auto child : system->getRootFolder()->getChildren()) {
                FileData* fd_child = dynamic_cast<FileData*>(child);
                if (fd_child && fd_child->getPath() == pseudoPathToFind) {
                    gameFile = fd_child;
                    break;
                }
            }
            
            if (!gameFile) { 
                 LOG(LogWarning) << "Xbox Store MetaUpdate BG: FileData not found for PFN via pseudoPath: " << pfn << ". Trying by metadata.";
                 auto allGames = system->getRootFolder()->getFilesRecursive(GAME, true);
                 for(auto* fd : allGames) {
                     if (fd && fd->getMetadata().get(MetaDataId::XboxPfn) == pfn) {
                         gameFile = fd;
                         LOG(LogDebug) << "Xbox Store MetaUpdate BG: Found FileData by metadata PFN: " << pfn;
                         break;
                     }
                 }
            }

            if (!gameFile) {
                LOG(LogWarning) << "Xbox Store MetaUpdate BG: FileData still not found for PFN: " << pfn;
                continue;
            }

            LOG(LogDebug) << "Xbox Store MetaUpdate BG: Fetching details for PFN: " << pfn << " (Game: " << gameFile->getName() << ")";
            Xbox::OnlineTitleInfo details = mAPI->GetTitleInfo(pfn); 

            if (details.pfn.empty() && details.name.empty()) { 
                LOG(LogWarning) << "Xbox Store MetaUpdate BG: No details returned from API for PFN: " << pfn;
                continue;
            }

            MetaDataList& mdl = gameFile->getMetadata();
            bool gameChanged = false;

            std::string apiName = details.name.empty() ? details.pfn : details.name;
            if (mdl.get(MetaDataId::Name) != apiName) {
                mdl.set(MetaDataId::Name, apiName); gameChanged = true;
            }
            if (!details.detail.description.empty() && mdl.get(MetaDataId::Desc) != details.detail.description) {
                mdl.set(MetaDataId::Desc, details.detail.description); gameChanged = true;
            }
            if (!details.detail.developerName.empty() && mdl.get(MetaDataId::Developer) != details.detail.developerName) {
                mdl.set(MetaDataId::Developer, details.detail.developerName); gameChanged = true;
            }
            if (!details.detail.publisherName.empty() && mdl.get(MetaDataId::Publisher) != details.detail.publisherName) {
                mdl.set(MetaDataId::Publisher, details.detail.publisherName); gameChanged = true;
            }
            if (!details.detail.releaseDate.empty()) {
                time_t release_t = Utils::Time::iso8601ToTime(details.detail.releaseDate);
                if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                    std::string esDate = Utils::Time::timeToMetaDataString(release_t);
                    if (!esDate.empty() && mdl.get(MetaDataId::ReleaseDate) != esDate) {
                        mdl.set(MetaDataId::ReleaseDate, esDate); gameChanged = true;
                    }
                } else { LOG(LogWarning) << "Xbox Meta BG: Could not parse release date: " << details.detail.releaseDate; }
            }
            std::string apiMediaType = details.mediaItemType.empty() ? details.type : details.mediaItemType;
            if (!apiMediaType.empty() && mdl.get(MetaDataId::XboxMediaType) != apiMediaType) {
                mdl.set(MetaDataId::XboxMediaType, apiMediaType); gameChanged = true;
            }
            if (!details.devices.empty()) {
                std::string devicesStr;
                for (size_t i = 0; i < details.devices.size(); ++i) {
                    devicesStr += details.devices[i] + (i < details.devices.size() - 1 ? ", " : "");
                }
                if (mdl.get(MetaDataId::XboxDevices) != devicesStr) {
                    mdl.set(MetaDataId::XboxDevices, devicesStr); gameChanged = true;
                }
            }

            if (gameChanged) {
                LOG(LogInfo) << "Xbox Store MetaUpdate BG: Updated metadata for " << apiName;
                anyMetadataChanged = true;
                mdl.setDirty(); 
                successCount++;
            }
        } 

        LOG(LogInfo) << "Xbox Store MetaUpdate BG: Finished. Successfully updated metadata for " << successCount << " PFN(s).";

        if (anyMetadataChanged && ViewController::get()) { 
            LOG(LogInfo) << "Xbox Store MetaUpdate BG: Metadata changed, requesting UI reload for system " << system->getName();
            SDL_Event event;
            SDL_zero(event);
            event.type = SDL_USEREVENT; 
            event.user.code = SDL_GAMELIST_UPDATED; 
            event.user.data1 = system; 
            event.user.data2 = nullptr; 
            SDL_PushEvent(&event);
        }
     });
}

bool XboxStore::installGame(const std::string& pfn) {
    LOG(LogDebug) << "XboxStore::installGame called for PFN: " << pfn;
    std::string storeUrl = "ms-windows-store://pdp/?PFN=" + pfn;
    LOG(LogInfo) << "Attempting to open Microsoft Store page: " << storeUrl;
    Utils::Platform::openUrl(storeUrl); 
    return true; 
}

bool XboxStore::uninstallGame(const std::string& pfn) {
    LOG(LogDebug) << "XboxStore::uninstallGame called for PFN: " << pfn;
    LOG(LogInfo) << "Opening Apps & Features settings page for manual uninstallation of PFN: " << pfn;
    Utils::Platform::openUrl("ms-settings:appsfeatures"); 
    return true;
}

bool XboxStore::updateGame(const std::string& pfn) {
    LOG(LogDebug) << "XboxStore::updateGame called for PFN: " << pfn;
    return installGame(pfn); 
}

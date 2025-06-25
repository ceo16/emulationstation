// emulationstation-master/es-app/src/GameStore/GameStoreManager.cpp
#include "GameStore/GameStoreManager.h"
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "GameStore/Steam/SteamStore.h"
#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/EAGames/EAGamesStore.h" // Includi per la definizione completa

#include "GameStore/EpicGames/EpicGamesAuth.h" 
#include "GameStore/Steam/SteamAuth.h"       
#include "GameStore/Xbox/XboxAuth.h"    
#include "GameStore/Amazon/AmazonGamesStore.h"     
#include "GameStore/GOG/GogGamesStore.h"
// EAGamesAuth è gestito internamente da EAGamesStore

#include "guis/GuiMenu.h" // Necessario per GuiMenu::openQuitMenu_static (se showStoreSelectionUI usa una logica simile)
#include "Log.h"
#include "Window.h"       
#include "Settings.h"     
// #include "HttpReq.h" // Non più necessario per HttpReq::Manager

// CORREZIONE: Definizione di sInstance
GameStoreManager* GameStoreManager::sInstance = nullptr;

// CORREZIONE: Implementazione di getInstance
GameStoreManager* GameStoreManager::getInstance(Window* window) {
    if (sInstance == nullptr) {
        if (window == nullptr && sInstance == nullptr) { 
            LOG(LogError) << "GameStoreManager::getInstance chiamato per la prima volta con window nullptr!";
            return nullptr; 
        }
        sInstance = new GameStoreManager(window);
    }
    return sInstance;
}
GameStore* GameStoreManager::getStore(const std::string& storeName) {
    auto it = mStores.find(storeName);
    if (it != mStores.end()) {
        return it->second;
    }
    LOG(LogWarning) << "GameStoreManager: Store not found: " << storeName;
    return nullptr;
}

GameStoreManager::GameStoreManager(Window* window) : mWindow(window) {
    LOG(LogDebug) << "GameStoreManager: Constructor";

    // Modello per la registrazione di ogni store
    auto registerStore = [&](GameStore* store) {
        if (store && store->init(mWindow)) {
            mStores[store->getStoreName()] = store;
            LOG(LogInfo) << "GameStoreManager: " << store->getStoreName() << " registrato con successo.";
        } else {
            LOG(LogError) << "GameStoreManager: Impossibile inizializzare lo store " << (store ? store->getStoreName() : "sconosciuto");
            if (store) {
                delete store;
            }
        }
    };

    if (Settings::getInstance()->getBool("EnableAmazonGames")) {
        registerStore(new AmazonGamesStore(mWindow));
    }

    if (Settings::getInstance()->getBool("EnableEpicGamesStore")) {
        EpicGamesAuth* epicAuth = new EpicGamesAuth();
        registerStore(new EpicGamesStore(epicAuth));
    }

    if (Settings::getInstance()->getBool("EnableGogStore")) {
        registerStore(new GogGamesStore(mWindow));
    }

    if (Settings::getInstance()->getBool("EnableSteamStore")) {
        SteamAuth* steamAuth = new SteamAuth();
        registerStore(new SteamStore(steamAuth, mWindow));
    }
    
    if (Settings::getInstance()->getBool("EnableXboxStore")) {
        std::function<void(const std::string&)> xboxStateCb = [](const std::string&){ /*...*/ };
        XboxAuth* xboxAuth = new XboxAuth(xboxStateCb);
        registerStore(new XboxStore(xboxAuth, mWindow));
    }

    if (Settings::getInstance()->getBool("EnableEAGamesStore")) {
        registerStore(new EAGamesStore(mWindow));
    }
}

// CORREZIONE: initAllStores non prende argomenti (usa mWindow membro)
// Assicurati che la dichiarazione in GameStoreManager.h sia void initAllStores();
void GameStoreManager::initAllStores() { 
    LOG(LogInfo) << "GameStoreManager: Initializing all registered stores (" << mStores.size() << " stores).";
    for (auto& pair : mStores) { 
        if (pair.second) {
            LOG(LogDebug) << "GameStoreManager: Initializing store: " << pair.first;
            // CORREZIONE: GameStore::init prende solo Window*
            // Rimuovi l'argomento HttpReq::Manager.
            if (!pair.second->init(mWindow)) { 
                LOG(LogError) << "GameStoreManager: Failed to initialize store " << pair.first;
            }
        }
    }
    LOG(LogInfo) << "GameStoreManager: Finished initializing all stores.";
}

// Rimuovi la funzione run() e setSetStateCallback se non sono definite/usate correttamente.
// L'errore C2039 per setSetStateCallback è in HttpServerThread.cpp che chiama
// mGameStoreManager->setSetStateCallback(...). Se mSetStateCallback è un membro pubblico in GameStoreManager.h,
// allora HttpServerThread.cpp dovrebbe fare `mGameStoreManager->mSetStateCallback = ...`.
// Altrimenti, GameStoreManager deve definire un metodo `setSetStateCallback`.
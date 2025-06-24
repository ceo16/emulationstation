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

    // Rimuovi HttpReq::Manager* httpManager e Settings::getInstance()->getHttpManager()
    // poiché non esistono e non sono più passati.
	
	if (Settings::getInstance()->getBool("EnableAmazonGames")) { // Usa un'impostazione per abilitarlo
    auto store = new AmazonGamesStore(mWindow);
    store->init(mWindow);
    mStores[store->getStoreName()] = store;
    LOG(LogInfo) << "GameStoreManager: Amazon Games Store registrato.";
}

    if (Settings::getInstance()->getBool("EnableEpicGamesStore")) {
        LOG(LogDebug) << "GameStoreManager: Attempting to register EpicGamesStore.";
        // Il tuo EpicGamesStore.h ha EpicGamesStore(EpicGamesAuth* auth);
        EpicGamesAuth* epicAuth = new EpicGamesAuth(/* parametri per EpicGamesAuth? */); 
        mStores["EpicGamesStore"] = new EpicGamesStore(epicAuth); 
        LOG(LogInfo) << "GameStoreManager: Epic Games Store registered.";
    }

    if (Settings::getInstance()->getBool("EnableSteamStore")) {
        LOG(LogDebug) << "GameStoreManager: Attempting to register SteamStore.";
        // Il tuo SteamStore.h ha SteamStore(SteamAuth* auth); e uno che prendeva HttpManager.
        // Usa quello che prende solo SteamAuth*.
        SteamAuth* steamAuth = new SteamAuth(/* params per SteamAuth? */);
        mStores["SteamStore"] = new SteamStore(steamAuth, mWindow); // CORREZIONE: Assumendo che prenda solo SteamAuth*
        LOG(LogInfo) << "GameStoreManager: SteamStore registered successfully.";
    }
	
	    if (Settings::getInstance()->getBool("EnableGogStore")) {
        LOG(LogDebug) << "GameStoreManager: Attempting to register GogGamesStore.";
        auto store = new GogGamesStore(mWindow);
        // La chiave "gog" deve corrispondere a getStoreName() in GogGamesStore.cpp
        mStores[store->getStoreName()] = store; 
        LOG(LogInfo) << "GameStoreManager: GOG.com Store registered.";
    }
    
    if (Settings::getInstance()->getBool("EnableXboxStore")) {
        LOG(LogDebug) << "GameStoreManager: Attempting to register XboxStore.";
        // Il tuo XboxStore.h ha XboxStore(XboxAuth* auth, Window* window);
        std::function<void(const std::string&)> xboxStateCb = [](const std::string&){ /*...*/ };
        XboxAuth* xboxAuth = new XboxAuth(xboxStateCb); 
        mStores["XboxStore"] = new XboxStore(xboxAuth, mWindow); // CORREZIONE
        LOG(LogInfo) << "GameStoreManager: XboxStore registered.";
    }

if (Settings::getInstance()->getBool("EnableEAGamesStore")) {
    LOG(LogDebug) << "GameStoreManager: Attempting to register EAGamesStore.";
    // USA LA COSTANTE STORE_ID PER LA CHIAVE
    mStores[EAGamesStore::STORE_ID] = new EAGamesStore(mWindow); 
    LOG(LogInfo) << "GameStoreManager: EAGamesStore registered successfully with ID: " << EAGamesStore::STORE_ID; // Log più preciso
}
}
// ... (destructor e altri metodi come getStore, registerStore come nel tuo file) ...
// Rimuovi showStoreSelectionUI e showIndividualStoreUI se sono state spostate o sono obsolete
// La logica di GuiMenu era problematica (errore C2061, C3536, C2664).
// Se devi mantenerle, la creazione del menu e l'aggiunta di entry va corretta.
// Esempio di come potrebbe essere (ma va adattato):
/*
void GameStoreManager::showStoreSelectionUI(Window* window) {
    auto s = new GuiSettings(window, _("NEGOZI ONLINE")); // Esempio, usa una GUI appropriata

    for (auto const& [storeName, storeInstance] : mStores) {
        if (storeInstance) {
            s->addEntry(storeInstance->getStoreName(), true, [window, storeInstance]() {
                storeInstance->showStoreUI(window);
            });
        }
    }
    window->pushGui(s);
}
*/

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
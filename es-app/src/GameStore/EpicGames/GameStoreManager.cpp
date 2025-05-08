#include "GameStore/EpicGames/GameStoreManager.h"
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "GameStore/EpicGames/PlaceholderStore.h"
#include "GameStore/EpicGames/EpicGamesStore.h" // Includi EpicGamesStore
#include "GameStore/Steam/SteamStore.h"
#include "guis/GuiMenu.h"
#include "Log.h"
#include "Window.h"

GameStoreManager* GameStoreManager::sInstance = nullptr;

GameStoreManager* GameStoreManager::get() {
    if (!sInstance) {
        sInstance = new GameStoreManager(nullptr); // Back to simpler constructor
    }
    return sInstance;
}

GameStoreManager::GameStoreManager(std::function<void(const std::string&)> setStateCallback) : setStateCallback(setStateCallback) {
    LOG(LogDebug) << "GameStoreManager: Constructor (with callback)";
 LOG(LogDebug) << "GameStoreManager: Attempting to register SteamStore.";
    SteamAuth* steamAuthInstance = new SteamAuth();
    SteamStore* steamStoreInstance = new SteamStore(steamAuthInstance);
    registerStore(steamStoreInstance); // Solo registra
    LOG(LogInfo) << "GameStoreManager: SteamStore registered successfully.";
}

GameStoreManager::~GameStoreManager() {
    LOG(LogDebug) << "GameStoreManager: Destructor";
    shutdownAllStores();
    for (auto& pair : mStores) {
        delete pair.second;
    }
    mStores.clear();
}


void GameStoreManager::registerStore(GameStore* store) { // Back to raw pointer
    LOG(LogDebug) << "GameStoreManager: Registering store " << store->getStoreName();
    mStores[store->getStoreName()] = store; // Back to raw pointer
}

GameStore* GameStoreManager::getStore(const std::string& storeName) {
    auto it = mStores.find(storeName);
    if (it != mStores.end()) {
        return it->second;
    }
    return nullptr;
}

void GameStoreManager::showStoreSelectionUI(Window* window) {
    LOG(LogDebug) << "GameStoreManager: Showing Store Selection UI";
    auto menu = new GuiMenu(window, false);

    menu->addEntry("Game Store", true, [window] {
        GameStoreManager::get()->showIndividualStoreUI(window);
    }, "iconFolder");

    menu->addEntry("Epic Games Store", true, [window] {
        GameStoreManager* manager = GameStoreManager::get(); // Be explicit
        manager->getStore("EpicGamesStore")->showStoreUI(window);
    }, "iconGames");

    window->pushGui(menu);
}

void GameStoreManager::showIndividualStoreUI(Window* window) {
    auto menu = new GuiMenu(window, false);

    for (auto& pair : mStores) {
        menu->addEntry(
            pair.first,
            true,
            [pair, window] {
                if (pair.second) {
                    pair.second->showStoreUI(window);
                }
            },
            "iconGames"
        );
    }

    window->pushGui(menu);
}

void GameStoreManager::initAllStores(Window* window) {
  LOG(LogDebug) << "GameStoreManager: Initializing all stores";
  // Questo ciclo scorre TUTTI gli store registrati in mStores
  // 'pair.first' è probabilmente il nome dello store (es. "SteamStore")
  // 'pair.second' è il puntatore all'oggetto GameStore* (es. SteamStore*)
  for (auto& pair : mStores) {
   if (pair.second) { // Controlla che il puntatore sia valido
     // Chiama il metodo init() sull'oggetto store (SteamStore, EpicGamesStore, ecc.)
    if (!pair.second->init(window)) {
     LOG(LogError) << "GameStoreManager: Failed to initialize store " << pair.first;
    }
   }
  }
 }

void GameStoreManager::shutdownAllStores() {
    LOG(LogDebug) << "GameStoreManager: Shutting down all stores";
    for (auto& pair : mStores) {
        if (pair.second) {
            pair.second->shutdown();
        }
    }
}

void GameStoreManager::setSetStateCallback(std::function<void(const std::string&)> setStateCallback) {
    this->setStateCallback = setStateCallback;
}
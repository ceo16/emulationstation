#ifndef ES_APP_GAMESTORE_GAMESTOREMANAGER_H
#define ES_APP_GAMESTORE_GAMESTOREMANAGER_H

#include <map>
#include <string>
#include <functional>
#include "GameStore/GameStore.h"
#include "Window.h"

class EpicGamesStore; // Forward declaration
class PlaceholderStore; // Forward declaration

class GameStoreManager {
public:
    static GameStoreManager* get();
    GameStoreManager(std::function<void(const std::string&)> setStateCallback);
    ~GameStoreManager();

    void registerStore(GameStore* store); // Back to raw pointer
    GameStore* getStore(const std::string& storeName);
	std::map<std::string, GameStore*> getStores() const { return mStores; } //  Add this method
    void showStoreSelectionUI(Window* window);
    void showIndividualStoreUI(Window* window);
    void initAllStores(Window* window);
    void shutdownAllStores();

    void setSetStateCallback(std::function<void(const std::string&)> setStateCallback);

private:
    std::map<std::string, GameStore*> mStores; // Back to raw pointer
    static GameStoreManager* sInstance;
    std::function<void(const std::string&)> setStateCallback;
};

#endif // ES_APP_GAMESTORE_GAMESTOREMANAGER_H
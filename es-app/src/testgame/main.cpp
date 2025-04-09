#include "testgame/EpicGamesStore.h"
#include <iostream>  // Add this line

int main() {
    EpicGamesStore store;
    store.init(nullptr);
    store.showStoreUI(nullptr);
    std::string name = store.getStoreName();
    std::cout << "Store name: " << name << std::endl;
    store.getGamesList();
    store.installGame("Game123");
    store.uninstallGame("Game123");
    store.updateGame("Game123");
    return 0;
}
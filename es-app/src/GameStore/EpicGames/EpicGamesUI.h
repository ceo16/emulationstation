#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESUI_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESUI_H

#include "GuiComponent.h"

class EpicGamesStore;

class EpicGamesUI {
public:
    EpicGamesUI();
    ~EpicGamesUI();

    void showMainMenu(Window* window, EpicGamesStore* store);
    void showLogin(Window* window, EpicGamesStore* store);
    void showGameList(Window* window, EpicGamesStore* store);
};

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESUI_H
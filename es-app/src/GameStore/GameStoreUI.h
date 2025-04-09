#ifndef ES_APP_GAMESTORE_GAMESTOREUI_H
#define ES_APP_GAMESTORE_GAMESTOREUI_H

class Window;
class GameStore;

class GameStoreUI {
public:
    virtual ~GameStoreUI() = default;
    virtual void showMainMenu(Window* window, GameStore* store) = 0;
    // Altri metodi comuni per la gestione dell'UI
};

#endif // ES_APP_GAMESTORE_GAMESTOREUI_H
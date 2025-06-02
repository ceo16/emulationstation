// emulationstation-master/es-app/src/GameStore/GameStoreManager.h
#ifndef ES_APP_GAMESTORE_GAMESTOREMANAGER_H
#define ES_APP_GAMESTORE_GAMESTOREMANAGER_H

#include <map>
#include <string>
#include <functional>
// #include "GameStore/GameStore.h" // Includi il path corretto se non è globale
#include "GameStore.h" // Assumendo che GameStore.h sia in GameStore/ e non GameStore/GameStore.h
#include "Window.h"

// Forward declarations
class EpicGamesStore;
class SteamStore;
class XboxStore;
class EAGamesStore;
// class PlaceholderStore; // Rimuovi se non più usato

class GameStoreManager {
public:
    // CORREZIONE: Cambiato get() in getInstance(Window* window)
    static GameStoreManager* getInstance(Window* window); 
    ~GameStoreManager();

    void registerStore(GameStore* store); // Questo metodo sembra ridondante se gli store sono creati nel costruttore
    GameStore* getStore(const std::string& storeName);
    std::map<std::string, GameStore*> getStores() const { return mStores; }

    // Rimosso showStoreSelectionUI e showIndividualStoreUI se non più usati e se GuiNegoziOnlineMenu è il sostituto
    // void showStoreSelectionUI(Window* window);
    // void showIndividualStoreUI(Window* window);     

    // CORREZIONE: initAllStores non prende argomenti (usa mWindow)
    void initAllStores(); 
    void shutdownAllStores();

    // Rimuovi mSetStateCallback se non usato o gestito diversamente.
    // Se è per HttpServerThread (la TUA versione legata a Epic), GameStoreManager non dovrebbe esporlo
    // direttamente così. Potrebbe avere un metodo specifico per Epic se necessario.
    // std::function<void(const std::string&)> mSetStateCallback; 

    Window* getWindow() const { return mWindow; }

private:
    // CORREZIONE: Il costruttore prende Window*
    GameStoreManager(Window* window); 
    
    std::map<std::string, GameStore*> mStores;
    static GameStoreManager* sInstance;
    Window* mWindow; 
};

#endif // ES_APP_GAMESTORE_GAMESTOREMANAGER_H
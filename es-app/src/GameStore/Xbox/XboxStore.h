#pragma once
#ifndef ES_APP_GAMESTORE_XBOX_XBOX_STORE_H
#define ES_APP_GAMESTORE_XBOX_XBOX_STORE_H

#include "GameStore/GameStore.h" 
#include "GameStore/Xbox/XboxAuth.h"    // Includi XboxAuth
#include "GameStore/Xbox/XboxStoreAPI.h" // Includi XboxStoreAPI
#include "GameStore/Xbox/XboxModels.h" 
// #include "GameStore/Xbox/XboxUI.h" // L'include di XboxUI non è necessario qui se la creiamo on-the-fly

#include <future> 

// Forward declaration
class FileData;
class SystemData;
class Window;
// class XboxUI; // Forward declaration se mUI fosse un membro

struct NewXboxGameData {
    std::string pseudoPath; 
    std::string pfn;
    std::map<MetaDataId, std::string> metadataMap;
};

class XboxStore : public GameStore
{
public:
    XboxStore(XboxAuth* auth, Window* window); 
    ~XboxStore() override;

    bool init(Window* window) override; 
    void shutdown() override;
    void showStoreUI(Window* window) override; // Qui useremo XboxUI
    std::string getStoreName() const override;

    std::vector<FileData*> getGamesList() override; 
    bool installGame(const std::string& gameId) override;   
    bool uninstallGame(const std::string& gameId) override; 
    bool updateGame(const std::string& gameId) override;    

    XboxAuth* getAuth() { return mAuth; }
    XboxStoreAPI* getApi() { return mAPI; }

    std::future<void> refreshGamesListAsync();
    std::future<void> updateGamesMetadataAsync(SystemData* system, const std::vector<std::string>& gamePfnsToUpdate);
    std::vector<Xbox::InstalledXboxGameInfo> findInstalledXboxGames();
    static std::string getGameLaunchCommand(const std::string& pfn);

private:
    XboxAuth* mAuth;         
    XboxStoreAPI* mAPI;      
    // XboxUI mUI; // Non usiamo un membro UI, la creiamo al momento per coerenza con EpicGamesUI
    Window* mInstanceWindow; // Riferimento alla finestra principale, se necessario per UI asincrone

    bool _initialized = false; 
};

#endif // ES_APP_GAMESTORE_XBOX_XBOX_STORE_H

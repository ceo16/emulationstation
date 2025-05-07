#pragma once
#ifndef ES_APP_GAMESTORE_STEAM_STORE_H
#define ES_APP_GAMESTORE_STEAM_STORE_H

#include "GameStore/GameStore.h"
#include "SteamAuth.h"
#include "SteamStoreAPI.h"
#include "GameStore/Steam/SteamUI.h"
#include "Window.h"
#include "Log.h"
#include "FileData.h"
#include "SystemData.h"
#include "MetaData.h"
#include <future> // Per std::future

// Helper struct per info giochi installati (simile a Playnite)
struct SteamInstalledGameInfo {
    unsigned int appId = 0;
    std::string name;
    std::string installDir; // Sottocartella in steamapps/common
    std::string libraryFolderPath; // Percorso completo della cartella libreria Steam (es. C:/Steam/steamapps)
    bool fullyInstalled = false;
};


class SteamStore : public GameStore
{
public:
    SteamStore(SteamAuth* auth);
    ~SteamStore() override;

    bool init(Window* window) override;
    void shutdown() override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;

    std::vector<FileData*> getGamesList() override; // Mostra giochi installati e/o posseduti online
    bool installGame(const std::string& gameId) override;   // gameId qui è l'appId
    bool uninstallGame(const std::string& gameId) override; // gameId qui è l'appId
    bool updateGame(const std::string& gameId) override;    // gameId qui è l'appId

    // TODO: Potrebbe servire un metodo per refreshare la lista giochi in background
    // std::future<void> refreshGamesListAsync();

    SteamAuth* getAuth() { return mAuth; }
    SteamStoreAPI* getApi() { return mAPI; }


private:
    SteamAuth* mAuth;         // Posseduto dal GameStoreManager, che lo passa qui
    SteamStoreAPI* mAPI;      // Creato e posseduto da SteamStore
    SteamUI mUI;          // TODO: Interfaccia utente specifica per Steam
    Window* mWindow;        // Non posseduto
    bool mInitialized;

    std::string getGameLaunchUrl(unsigned int appId) const;
    bool checkInstallationStatus(unsigned int appId, const std::vector<SteamInstalledGameInfo>& installedGames);

    // Logica per trovare giochi installati (da Playnite)
    std::string getSteamInstallationPath(); // Trova dove è installato Steam
    std::vector<std::string> getSteamLibraryFolders(const std::string& steamPath); // Legge libraryfolders.vdf
    std::vector<SteamInstalledGameInfo> findInstalledSteamGames(); // Cerca i file appmanifest_*.acf
    SteamInstalledGameInfo parseAppManifest(const std::string& acfFilePath); // Parsa un singolo file .acf

    // TODO: Funzione helper per convertire stringa data Steam (es. "14 Nov, 2019") in formato ES
    // std::string convertSteamDateToESFormat(const std::string& steamDate);
};

#endif // ES_APP_GAMESTORE_STEAM_STORE_H
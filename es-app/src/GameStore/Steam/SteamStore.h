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
#include "Window.h"
#include <vector>
#include <string>
#include <map>

// Helper struct per info giochi installati (simile a Playnite)
struct SteamInstalledGameInfo {
    unsigned int appId = 0;
    std::string name;
    std::string installDir; // Sottocartella in steamapps/common
    std::string libraryFolderPath; // Percorso completo della cartella libreria Steam (es. C:/Steam/steamapps)
    bool fullyInstalled = false;
};

struct NewSteamGameData {
    std::string pseudoPath;                   // Es. "steam://launch/12345"
    std::map<MetaDataId, std::string> metadataMap; // Mappa per contenere i metadati base
    // Non servono altri campi qui se sono già nella mappa
};

class SteamStore : public GameStore
{
public:
    SteamStore(SteamAuth* auth, Window* window); // MODIFICATO: Aggiunto Window*
    ~SteamStore() override;

    bool init(Window* window) override;
    void shutdown() override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;
	SteamStoreAPI* getApi() { return mAPI; } // Add this line

    std::vector<FileData*> getGamesList() override; // Mostra giochi installati e/o posseduti onlin
    bool installGame(const std::string& gameId) override;   // gameId qui è l'appId
    bool uninstallGame(const std::string& gameId) override; // gameId qui è l'appId
    bool updateGame(const std::string& gameId) override;    // gameId qui è l'appId
	std::future<void> refreshSteamGamesListAsync();

    // TODO: Potrebbe servire un metodo per refreshare la lista giochi in background
    // std::future<void> refreshGamesListAsync();

    SteamAuth* getAuth() { return mAuth; }
	std::string getGameLaunchUrl(unsigned int appId) const;
	std::vector<SteamInstalledGameInfo> findInstalledSteamGames(); // Cerca i file appmanifest_*.acf
	 bool checkInstallationStatus(unsigned int appId, const std::vector<SteamInstalledGameInfo>& installedGames);


private:
    SteamAuth* mAuth;         // Posseduto dal GameStoreManager, che lo passa qui
    SteamStoreAPI* mAPI;      // Creato e posseduto da SteamStore
    SteamUI mUI;          // TODO: Interfaccia utente specifica per Steam
    Window* mWindow;        // Non posseduto



   

    // Logica per trovare giochi installati (da Playnite)
    std::string getSteamInstallationPath(); // Trova dove è installato Steam
    std::vector<std::string> getSteamLibraryFolders(const std::string& steamPath); // Legge libraryfolders.vdf
    
    SteamInstalledGameInfo parseAppManifest(const std::string& acfFilePath); // Parsa un singolo file .acf


    // TODO: Funzione helper per convertire stringa data Steam (es. "14 Nov, 2019") in formato ES
    // std::string convertSteamDateToESFormat(const std::string& steamDate);
};

#endif // ES_APP_GAMESTORE_STEAM_STORE_H
#pragma once
#define NOMINMAX
#ifndef ES_APP_GAMESTORE_XBOX_XBOX_STORE_H
#define ES_APP_GAMESTORE_XBOX_XBOX_STORE_H

#include "GameStore/GameStore.h"
#include "GameStore/Xbox/XboxAuth.h"
#include "GameStore/Xbox/XboxStoreAPI.h"
#include "GameStore/Xbox/XboxModels.h" // Contiene Xbox::InstalledXboxGameInfo e Xbox::OnlineTitleInfo

#include <future>
#include <vector>
#include <string>
#include <map>

#ifdef _WIN32
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Gaming.Preview.GamesEnumeration.h> // Per GameListEntry
#endif

class FileData;
class SystemData;
class Window;

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
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;

    // Metodo virtuale da GameStore, deve essere implementato
    bool launchGame(const std::string& gameId) override; 

    std::vector<FileData*> getGamesList() override;
    bool installGame(const std::string& gameId) override;
	bool isXboxAppInstalled();
	void openStoreClient();
    bool uninstallGame(const std::string& gameId) override;
    bool updateGame(const std::string& gameId) override;

    XboxAuth* getAuth() { return mAuth; }
    XboxStoreAPI* getApi() { return mAPI; }

    std::future<void> refreshGamesListAsync();
    std::future<void> updateGamesMetadataAsync(SystemData* system, const std::vector<std::string>& gamePfnsToUpdate);
    
    std::vector<Xbox::InstalledXboxGameInfo> findInstalledXboxGames();
    
    static std::string getGameLaunchCommand(const std::string& aumid);
    bool launchGameByAumid(const std::string& aumid); // Dichiarazione corretta

private:
    std::vector<Xbox::InstalledXboxGameInfo> findInstalledGames_PackageManagerHelper(const std::map<std::wstring, std::wstring>* pfnAndAppIdMap); // Dichiarazione corretta

    XboxAuth* mAuth;
    XboxStoreAPI* mAPI;
    Window* mInstanceWindow;
	bool mIsXboxAppInstalled; // Per memorizzare lo stato dell'app Xbox

    bool _initialized = false;
#ifdef _WIN32
    bool mComInitialized;
#endif
};

#endif // ES_APP_GAMESTORE_XBOX_XBOX_STORE_H
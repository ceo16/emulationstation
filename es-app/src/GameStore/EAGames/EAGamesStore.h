#pragma once

#include "GameStore/GameStore.h"
#include "Window.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <mutex>
#include <set>
#include <map>
#include "GameStore/EAGames/EAGamesModels.h"

// Forward declarations
namespace EAGames {
    class EAGamesAuth;
    class EAGamesAPI;
    class EAGamesScanner;
    struct GameEntitlement;
    struct InstalledGameInfo;
    struct GameStoreData;
    struct SubscriptionDetails;
    struct SubscriptionGame;
}

class FileData;

class EAGamesStore : public GameStore
{
public:
    EAGamesStore(Window* window);
    ~EAGamesStore() override;

    // --- Interfaccia GameStore (ripristinata) ---
    bool init(Window* window) override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;
    bool launchGame(const std::string& gameId) override;
    void shutdown() override;
    std::vector<FileData*> getGamesList() override;
    bool installGame(const std::string& gameId) override;
    bool uninstallGame(const std::string& gameId) override;
    bool updateGame(const std::string& gameId) override;
    static const std::string STORE_ID;

    // --- Metodi Pubblici (ripristinati) ---
    bool IsUserLoggedIn();
    void Login(std::function<void(bool success, const std::string& message)> callback);
    void Logout();
    void GetUsername(std::function<void(const std::string& username)> callback);
    void SyncGames(std::function<void(bool success)> callback);
    void getSubscriptionDetails(std::function<void(const EAGames::SubscriptionDetails& details)> callback);
    void StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished);
    static unsigned short GetLocalRedirectPort();
    std::vector<EAGames::InstalledGameInfo> getInstalledGames();
    void incrementActiveScrape();
    void decrementActiveScrape();

    // --- Struct e Callback per lo Scraper (ripristinati) ---
    struct EAGameData {
        std::string id;
        std::string name;
        std::string description;
        std::string developer;
        std::string publisher;
        std::string releaseDate;
        std::string genre;
        std::string imageUrl;
        std::string backgroundUrl;
        std::string offerId;
        std::string masterTitleId;
    };
    using ArtworkFetchedCallbackStore = std::function<void(const std::string& url, bool success)>;
    using MetadataFetchedCallbackStore = std::function<void(const EAGameData& metadata, bool success)>;

    void GetGameArtwork(const FileData* game, const std::string& artworkType, ArtworkFetchedCallbackStore callback);
    void GetGameMetadata(const FileData* game, MetadataFetchedCallbackStore callback);

private:
    // --- NUOVA ARCHITETTURA (corretta) ---
    void fetchDetailsForGames(
        const std::vector<EAGames::GameEntitlement>& onlineGames,
        const std::vector<EAGames::SubscriptionGame>& catalogGames,
        std::function<void(bool, const std::map<std::string, EAGames::GameStoreData>&)> on_complete);

    void processAndCacheGames(
        const std::vector<EAGames::GameEntitlement>& onlineGames,
        const std::vector<EAGames::InstalledGameInfo>& installedGames,
        const std::vector<EAGames::SubscriptionGame>& catalogGames,
        const std::map<std::string, EAGames::GameStoreData>& detailedDataMap);

    // --- Funzioni private (ripristinate) ---
    void getEAPlayCatalog(std::function<void(const std::vector<EAGames::SubscriptionGame>& catalog)> callback);
    void rebuildAndSortCache();
    void rebuildReturnableGameList();

    // --- Variabili membro (ripristinate) ---
    Window* mWindow;
    std::unique_ptr<EAGames::EAGamesAuth> mAuth;
    std::unique_ptr<EAGames::EAGamesAPI> mApi;
    std::unique_ptr<EAGames::EAGamesScanner> mScanner;

    std::vector<std::unique_ptr<FileData>> mCachedGameFileDatas;
    std::vector<FileData*> mReturnableGameList;

    bool mGamesCacheDirty;
    bool mFetchingGamesInProgress;
    bool _initialized;
    int mActiveScrapeCounter;

    std::mutex mCacheMutex;
    EAGames::SubscriptionDetails mSubscriptionDetails;
    bool mSubscriptionDetailsInitialized;
    bool mSubscriptionDetailsFetching;
    std::vector<EAGames::SubscriptionGame> mEAPlayCatalog;
    std::set<std::string> mEAPlayOfferIds;
    bool mEAPlayCatalogInitialized;
};
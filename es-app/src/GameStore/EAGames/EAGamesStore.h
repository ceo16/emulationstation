// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesStore.h
#pragma once

// Assumendo che GameStore.h sia una directory sopra, o che il percorso di include sia configurato
#include "GameStore/GameStore.h" // Per la classe base 'GameStore' (globale)
#include "Window.h"
#include "HttpReq.h"     // Per la classe 'HttpReq' (globale)
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <mutex>  // <-- Aggiunto per thread-safety
#include <set>  
#include "GameStore/EAGames/EAGamesModels.h" 

// Forward declarations per i componenti interni nel namespace EAGames
namespace EAGames {
    class EAGamesAuth;
    class EAGamesAPI;
    class EAGamesScanner;
    struct GameEntitlement;
    struct InstalledGameInfo;
    struct GameStoreData; // Necessaria per la definizione della struct EAGameData, se usata lì
	struct SubscriptionDetails;
    struct SubscriptionGame;
}

class FileData;

// EAGamesStore è nel namespace globale e deriva da GameStore (globale)
class EAGamesStore : public GameStore
{
public:
    EAGamesStore(Window* window); // Non ha più HttpReq::Manager*
    ~EAGamesStore() override;

    // --- Implementazione dell'interfaccia GameStore ---
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
	void incrementActiveScrape();
    void decrementActiveScrape();
    std::vector<EAGames::InstalledGameInfo> getInstalledGames();

    // --- Metodi Pubblici Specifici di EAGamesStore ---
    bool IsUserLoggedIn();
    void Login(std::function<void(bool success, const std::string& message)> callback);
    void Logout();
    void GetUsername(std::function<void(const std::string& username)> callback);
    void SyncGames(std::function<void(bool success)> callback);
	    // Ottiene i dettagli dell'abbonamento in modo asincrono, usando la cache.
    void getSubscriptionDetails(std::function<void(const EAGames::SubscriptionDetails& details)> callback);


    void StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished);
    void HandleAuthRedirect(const std::string& redirectUrlQuery, std::function<void(bool success, const std::string& message)> onFlowFinished);
    static unsigned short GetLocalRedirectPort(); // Definito nel .cpp

    struct EAGameData {
        std::string id; // Può essere offerId o masterTitleId, a seconda del contesto che preferisci come ID primario
        std::string name;
        std::string description;
        std::string developer;
        std::string publisher;
        std::string releaseDate;
        std::string genre;
        std::string imageUrl;
        std::string backgroundUrl;
        std::string offerId;        // <--- AGGIUNGI QUESTA RIGA
        std::string masterTitleId;  // <--- AGGIUNGI QUESTA RIGA
    };
    using ArtworkFetchedCallbackStore = std::function<void(const std::string& url, bool success)>;
    using MetadataFetchedCallbackStore = std::function<void(const EAGameData& metadata, bool success)>;

    void GetGameArtwork(const FileData* game, const std::string& artworkType, ArtworkFetchedCallbackStore callback);
    void GetGameMetadata(const FileData* game, MetadataFetchedCallbackStore callback);

private:
    void processAndCacheGames(const std::vector<EAGames::GameEntitlement>& onlineGames,
                              const std::vector<EAGames::InstalledGameInfo>& installedGames,
							  const std::vector<EAGames::SubscriptionGame>& catalogGames); 
							  
    FileData* convertEaDataToGameData(const EAGames::GameEntitlement& entitlement, const EAGames::InstalledGameInfo* installedInfo);
    FileData* convertInstalledEaToGameData(const EAGames::InstalledGameInfo& installedInfo);
	bool mSubscriptionDetailsFetching = false;


	
    bool checkGameInstallation(const std::string& normalizedGameName, const std::map<std::string, EAGames::InstalledGameInfo>& installedGamesMap, EAGames::InstalledGameInfo& foundGame);
    void rebuildAndSortCache();
	
	 // Arricchisce la lista di giochi 'mCachedGameFileDatas' con il flag "eaplay".
    
    // Funzione helper asincrona per ottenere il catalogo e metterlo in cache.
    void getEAPlayCatalog(std::function<void(const std::vector<EAGames::SubscriptionGame>& catalog)> callback);

    std::mutex mCacheMutex;

    EAGames::SubscriptionDetails mSubscriptionDetails;
    bool mSubscriptionDetailsInitialized = false;

    std::vector<EAGames::SubscriptionGame> mEAPlayCatalog;
    std::set<std::string> mEAPlayOfferIds;
    bool mEAPlayCatalogInitialized = false;

    Window* mWindow;
    std::unique_ptr<EAGames::EAGamesAuth> mAuth;
    std::unique_ptr<EAGames::EAGamesAPI> mApi;
    std::unique_ptr<EAGames::EAGamesScanner> mScanner;

    std::vector<std::unique_ptr<FileData>> mCachedGameFileDatas;
    std::vector<FileData*> mReturnableGameList;
    void rebuildReturnableGameList();

    bool mGamesCacheDirty;
    bool mFetchingGamesInProgress;
	bool _initialized;
	int mActiveScrapeCounter;
};
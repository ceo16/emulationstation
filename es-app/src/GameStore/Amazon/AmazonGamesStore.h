#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_STORE_H
#define ES_APP_GAMESTORE_AMAZON_STORE_H

#include "GameStore/GameStore.h"
#include <functional>
#include <vector>
#include "GameStore/Amazon/AmazonAuth.h"
#include "GameStore/Amazon/AmazonGamesAPI.h"
#include "GameStore/Amazon/AmazonGamesScanner.h"

class FileData;
class Window;

class AmazonGamesStore : public GameStore
{
public:
    AmazonGamesStore(Window* window);
    ~AmazonGamesStore() override;

    // --- Interfaccia GameStore ---
    bool init(Window* window) override;
    void shutdown() override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;
    std::vector<FileData*> getGamesList() override;
    bool launchGame(const std::string& gameId) override;
    bool installGame(const std::string& gameId) override;
    bool uninstallGame(const std::string& gameId) override;
    bool updateGame(const std::string& gameId) override;

    // --- Metodi Pubblici Specifici ---
    bool isAuthenticated() const;
    void login(std::function<void(bool)> on_complete);
    void logout();
    void syncGames(std::function<void(bool)> on_complete);
	AmazonGamesScanner* getScanner() { return mScanner.get(); }

private:
    void processGamesList(const std::vector<Amazon::GameEntitlement>& onlineGames, const std::vector<Amazon::InstalledGameInfo>& installedGames);

    Window* mWindow;
    std::unique_ptr<AmazonAuth> mAuth;
    std::unique_ptr<AmazonGamesAPI> mApi;
    std::unique_ptr<AmazonGamesScanner> mScanner;
    
    // La cache mGames è stata rimossa per risolvere il problema di ownership
};

#endif // ES_APP_GAMESTORE_AMAZON_STORE_H
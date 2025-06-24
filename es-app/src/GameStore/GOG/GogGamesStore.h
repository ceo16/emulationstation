#pragma once
#ifndef ES_APP_GAMESTORE_GOG_STORE_H
#define ES_APP_GAMESTORE_GOG_STORE_H

#include "GameStore/GameStore.h"
#include <functional>
#include <vector>

// Forward declarations delle nostre classi
class GogAuth;
class GogGamesAPI;
class GogScanner;
class FileData;
class Window;

// Includiamo i modelli di dati di GOG
#include "GameStore/GOG/GogModels.h"

class GogGamesStore : public GameStore
{
public:
    GogGamesStore(Window* window);
    ~GogGamesStore() override;

    // --- Implementazione dell'interfaccia GameStore ---
    bool init(Window* window) override;
    void shutdown() override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;
    std::vector<FileData*> getGamesList() override;
    bool launchGame(const std::string& gameId) override;
    bool installGame(const std::string& gameId) override;
    bool uninstallGame(const std::string& gameId) override;
    bool updateGame(const std::string& gameId) override;

    // --- Metodi Pubblici Specifici per GOG ---
    bool isAuthenticated() const;
    void login(std::function<void(bool)> on_complete);
    void logout();
    void syncGames(std::function<void(bool)> on_complete);
    
    // Metodo per accedere allo scanner, utile per l'avvio del programma
    GogScanner* getScanner() { return mScanner.get(); }

private:
    // Funzione helper per combinare giochi online e installati
    void processGamesList(const std::vector<GOG::LibraryGame>& onlineGames, const std::vector<GOG::InstalledGameInfo>& installedGames);

    Window* mWindow;
    std::unique_ptr<GogAuth> mAuth;
    std::unique_ptr<GogGamesAPI> mApi;
    std::unique_ptr<GogScanner> mScanner;
};

#endif // ES_APP_GAMESTORE_GOG_STORE_H
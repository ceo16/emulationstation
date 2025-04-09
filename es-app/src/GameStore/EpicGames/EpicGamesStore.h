#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
 #define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
 
 #include <string>
 #include <vector>
 #include "GameStore/GameStore.h"
 #include "GameStore/EpicGames/EpicGamesStoreAPI.h"
 #include "GameStore/EpicGames/EpicGamesUI.h"
 #include "GameStore/EpicGames/EpicGamesAuth.h" // Include EpicGamesAuth
 #include <functional> // Include std::function
 #include "FileData.h"  // Add this line
 #include "SystemData.h" // Add this line (if needed)
 
 class EpicGamesStore : public GameStore {
 public:
     EpicGamesStore(EpicGamesAuth* auth); // Modified constructor
     EpicGamesStore(); // Keep the default constructor for GameStoreManager
     ~EpicGamesStore();
 
     bool init(Window* window) override;
     void shutdown() override;
     void showStoreUI(Window* window) override;
     std::string getStoreName() const override;
 
     std::vector<FileData*> getGamesList() override; // Return FileData*
     bool installGame(const std::string& gameId) override;
     bool uninstallGame(const std::string& gameId) override;
     bool updateGame(const std::string& gameId) override;
 
     void startLoginFlow();
     void processAuthCode(const std::string& authCode);
	 static std::string getEpicGameId(const std::string& path); // Add this li
 
 private:
     EpicGamesStoreAPI mAPI;
     EpicGamesUI mUI;
     EpicGamesAuth* mAuth;
     Window* mWindow;
 
     std::vector<std::string> findInstalledEpicGames(); // Return std::string
     void testFindInstalledGames(); // Declare test function
     std::string getEpicLauncherConfigPath(); // Declare this function
     static std::string getLauncherInstalledDatPath(); // Declare this function
 };
 
 #endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
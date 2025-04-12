#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
 #define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
 

 #include <string>
 #include <vector>
 #include "GameStore/GameStore.h"
 #include "GameStore/EpicGames/EpicGamesStoreAPI.h"
 #include "GameStore/EpicGames/EpicGamesUI.h"
 #include "GameStore/EpicGames/EpicGamesAuth.h"
 #include <functional>
 #include "FileData.h"
 #include "SystemData.h"
 

 class EpicGamesStore : public GameStore {
 public:
  // Helper struct to hold detailed game info
  struct EpicGameInfo {
  std::string id;
  std::string name;
  std::string installDir;
  std::string executable;
  std::string launchCommand;
  };
 

  EpicGamesStore(EpicGamesAuth* auth);
  EpicGamesStore();
  ~EpicGamesStore();
 

  bool init(Window* window) override;
  void shutdown() override;
  void showStoreUI(Window* window) override;
  std::string getStoreName() const override;
 

  std::vector<FileData*> getGamesList() override;
  bool installGame(const std::string& gameId) override;
  bool uninstallGame(const std::string& gameId) override;
  bool updateGame(const std::string& gameId) override;
 

  void startLoginFlow();
  void processAuthCode(const std::string& authCode);
  static std::string getEpicGameId(const std::string& path);
  std::vector<EpicGameInfo> getInstalledEpicGamesWithDetails();
 

 private:
  EpicGamesStoreAPI mAPI;
  EpicGamesUI mUI;
  EpicGamesAuth* mAuth;
  Window* mWindow;
 

  std::vector<std::string> findInstalledEpicGames();
  void testFindInstalledGames();
  std::string getEpicLauncherConfigPath();
  static std::string getLauncherInstalledDatPath();
  std::string getMetadataPath();
 };
 

 #endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTORE_H
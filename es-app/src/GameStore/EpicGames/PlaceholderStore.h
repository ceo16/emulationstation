#ifndef ES_APP_GAMESTORE_PLACEHOLDERSTORE_H
 #define ES_APP_GAMESTORE_PLACEHOLDERSTORE_H
 

 #include "GameStore/GameStore.h"
 #include <vector>
 #include <string>
 #include "FileData.h" // Include FileData
 

 class PlaceholderStore : public GameStore {
 public:
  PlaceholderStore();
  ~PlaceholderStore() override;
 

  bool init(Window* window) override;
  std::vector<FileData*> getGamesList() override; // Changed to FileData*
  bool installGame(const std::string& gameId) override;
  bool uninstallGame(const std::string& gameId) override;
  bool updateGame(const std::string& gameId) override;
  void showStoreUI(Window* window) override;
  void shutdown() override;
  std::string getStoreName() const override { return "PlaceholderStore"; }
 };
 

 #endif // ES_APP_GAMESTORE_PLACEHOLDERSTORE_H
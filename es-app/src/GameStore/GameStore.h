#ifndef ES_APP_GAMESTORE_GAMESTORE_H
 #define ES_APP_GAMESTORE_GAMESTORE_H
 

 #include <string>
 #include <vector>  // Added include
 #include "FileData.h"
 

 class Window;
 

 class GameStore {
 public:
  virtual ~GameStore() = default;
  virtual bool init(Window* window) = 0;
  virtual void showStoreUI(Window* window) = 0;
  virtual std::string getStoreName() const = 0;
  virtual void shutdown() = 0;
 

  virtual std::vector<FileData*> getGamesList() = 0; // Changed return type to std::vector<FileData*>
  virtual bool installGame(const std::string& gameId) = 0;
  virtual bool uninstallGame(const std::string& gameId) = 0;
  virtual bool updateGame(const std::string& gameId) = 0;
 };
 

 #endif // ES_APP_GAMESTORE_GAMESTORE_H
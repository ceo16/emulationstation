#ifndef ES_APP_GAMESTORE_GAMESTORE_H
 #define ES_APP_GAMESTORE_GAMESTORE_H
 

 #include <string>
 #include <vector>  // Added include
 

 class Window;
 

 class GameStore {
 public:
  virtual ~GameStore() = default;
  virtual bool init(Window* window) = 0;
  virtual void showStoreUI(Window* window) = 0;
  virtual std::string getStoreName() const = 0;
  virtual void shutdown() = 0;
 

  virtual std::vector<std::string> getGamesList() = 0;
  virtual bool installGame(const std::string& gameId) = 0;
  virtual bool uninstallGame(const std::string& gameId) = 0;
  virtual bool updateGame(const std::string& gameId) = 0;
 };
 
#ifndef ES_APP_SERVICES_HTTPSERVERTHREAD_H
 #define ES_APP_SERVICES_HTTPSERVERTHREAD_H
 

 #include "Window.h"
 #include <thread>
 #include <queue>
 #include <mutex>
 #include <condition_variable>
 #include "GameStore/EpicGames/EpicGamesAuth.h" // Percorso corretto!
 #include "GameStore/EpicGames/EpicGamesStore.h"
 #include "GameStore/GameStoreManager.h"
 #include <functional>
 

 namespace httplib
 {
  class Server;
 }
 

 class HttpServerThread
 {
 public:
 HttpServerThread(Window * window);
	virtual ~HttpServerThread();
 

  void queueTask(std::function<void()> task);
  void addEpicCallbackEndpoint(int port);
  void setExpectedState(const std::string& state);
 

  static std::string getMimeType(const std::string &path);
 

 private:
  Window* mWindow;
  bool  mRunning;
  bool  mFirstRun;
  std::thread* mThread;
 

  httplib::Server* mHttpServer;
  std::function<void(const std::string&)> mAuthCallback;
  EpicGamesAuth    mAuth;
  GameStore* mStore; // Changed to GameStore*
  GameStoreManager* mGameStoreManager;
  void run();
  std::queue<std::function<void()>> mTaskQueue;
  std::mutex mQueueMutex;
  std::condition_variable mQueueCondition;
  std::function<void(const std::string&)> mEpicLoginCallback;
  std::string mExpectedState;
 };
 

 #endif // ES_APP_SERVICES_HTTPSERVERTHREAD_H
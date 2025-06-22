#pragma once
 
 #include <memory>
 #include <mutex>
 #include "../../es-app/src/SystemData.h"
 extern std::mutex g_systemDataMutex;
 
 class AppContext {
 public:
  static AppContext& get() { //  Singleton pattern
      static AppContext instance;
      return instance;
  }
 
  std::shared_ptr<SystemData> getCurrentSystem();
  void setCurrentSystem(std::shared_ptr<SystemData> system);


 private:
  std::shared_ptr<SystemData> mCurrentSystem;
  std::mutex mCurrentSystemMutex;
  AppContext() : mCurrentSystem(nullptr) {}
  AppContext(const AppContext&) = delete;
  AppContext& operator=(const AppContext&) = delete;
 };
 

#include "AppContext.h"
 
 std::shared_ptr<SystemData> AppContext::getCurrentSystem() {
  std::lock_guard<std::mutex> lock(mCurrentSystemMutex);
  return mCurrentSystem;
 }
 
 void AppContext::setCurrentSystem(std::shared_ptr<SystemData> system) {
  std::lock_guard<std::mutex> lock(mCurrentSystemMutex);
  mCurrentSystem = system;
 }
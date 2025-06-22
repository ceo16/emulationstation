#include "AppContext.h"
#include <mutex> // Assicurati che <mutex> sia incluso qui se non lo è già per il mutex

// DEFINIZIONE DEL MUTEX GLOBALE
std::mutex g_systemDataMutex; 
 
 std::shared_ptr<SystemData> AppContext::getCurrentSystem() {
  std::lock_guard<std::mutex> lock(mCurrentSystemMutex);
  return mCurrentSystem;
 }
 
 void AppContext::setCurrentSystem(std::shared_ptr<SystemData> system) {
  std::lock_guard<std::mutex> lock(mCurrentSystemMutex);
  mCurrentSystem = system;
 }
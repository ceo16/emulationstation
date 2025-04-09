#include "GameStore/EpicGames/PlaceholderStore.h"
 #include "Log.h"
 #include "FileData.h" // Include FileData
 

 PlaceholderStore::PlaceholderStore() {
  LOG(LogDebug) << "PlaceholderStore: Constructor";
 }
 

 PlaceholderStore::~PlaceholderStore() {
  LOG(LogDebug) << "PlaceholderStore: Destructor";
  shutdown();
 }
 

 bool PlaceholderStore::init(Window* window) {
  LOG(LogDebug) << "PlaceholderStore: init";
  return true;
 }
 

 void PlaceholderStore::showStoreUI(Window* window) {
  LOG(LogDebug) << "PlaceholderStore: showStoreUI";
  // Placeholder UI logic
 }
 

 

 void PlaceholderStore::shutdown() {
  LOG(LogDebug) << "PlaceholderStore: shutdown";
 }
 

 std::vector<FileData*> PlaceholderStore::getGamesList() { // Changed to FileData*
  LOG(LogDebug) << "PlaceholderStore::getGamesList (placeholder)";
  std::vector<FileData*> placeholderGames;
  // Create placeholder FileData objects
  FileData* game1 = new FileData(GAME, "Placeholder Game 1", nullptr); // Replace nullptr if you have a SystemData
  game1->setMetadata(MetaDataId::Name, "Placeholder Game 1");
  placeholderGames.push_back(game1);
 

  FileData* game2 = new FileData(GAME, "Placeholder Game 2", nullptr); // Replace nullptr if you have a SystemData
  game2->setMetadata(MetaDataId::Name, "Placeholder Game 2");
  placeholderGames.push_back(game2);
 

  return placeholderGames;
 }
 

 bool PlaceholderStore::installGame(const std::string& gameId) {
  LOG(LogDebug) << "PlaceholderStore::installGame (placeholder)";
  return true;
 }
 

 bool PlaceholderStore::uninstallGame(const std::string& gameId) {
  LOG(LogDebug) << "PlaceholderStore::uninstallGame (placeholder)";
  return true;
 }
 

 bool PlaceholderStore::updateGame(const std::string& gameId) {
  LOG(LogDebug) << "PlaceholderStore::updateGame (placeholder)";
  return true;
 }
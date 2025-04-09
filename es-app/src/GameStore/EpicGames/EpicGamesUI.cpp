#include "GameStore/EpicGames/EpicGamesUI.h"
 #include "GameStore/EpicGames/EpicGamesStore.h"
 #include "components/ButtonComponent.h"
 #include "components/TextComponent.h"
 #include "components/ComponentList.h"
 #include "guis/GuiSettings.h"
 #include "Window.h"
 #include "Log.h"
 #include "FileData.h"  // Include FileData
 
 EpicGamesUI::EpicGamesUI() {
 }
 
 EpicGamesUI::~EpicGamesUI() {
 }
 
 void EpicGamesUI::showMainMenu(Window* window, EpicGamesStore* store) {
  LOG(LogDebug) << "EpicGamesUI::showMainMenu";
 
  auto menu = new GuiSettings(window, "Epic Games Store");
 
  menu->addEntry("Login to Epic Games", true, [window, store] {
  store->startLoginFlow();
  }, "iconFolder");
 
  window->pushGui(menu);
 }
 
 void EpicGamesUI::showLogin(Window* window, EpicGamesStore* store) {
  LOG(LogDebug) << "EpicGamesUI::showLogin";
 
  auto menu = new GuiSettings(window, "Epic Games Login");
 
  menu->addEntry("Perform Login", true, [window, store] {
  store->startLoginFlow();
  }, "iconFolder");
 
  window->pushGui(menu);
 }
 
 void EpicGamesUI::showGameList(Window* window, EpicGamesStore* store) {
  LOG(LogDebug) << "EpicGamesUI::showGameList";
 
  auto menu = new GuiSettings(window, "Epic Games Library");
 
  std::vector<FileData*> games = store->getGamesList();  // Get FileData*
  for (FileData* game : games) {
  // Access game data to display (e.g., game name)
  menu->addEntry(game->getMetadata().get(MetaDataId::Name), true, nullptr);  // Use the game name
  }
 
  window->pushGui(menu);
 }
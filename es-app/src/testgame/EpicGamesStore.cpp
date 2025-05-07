#include "EpicGamesStore.h"
 #include <iostream>
 #include <vector>
 

 EpicGamesStore::EpicGamesStore() {
  std::cout << "EpicGamesStore: Constructor" << std::endl;
 }
 

 EpicGamesStore::~EpicGamesStore() {
  std::cout << "EpicGamesStore: Destructor" << std::endl;
  shutdown();
 }
 

 bool EpicGamesStore::init(Window* window) { // Remove override
  std::cout << "EpicGamesStore::init" << std::endl;
  return true;
 }
 

 void EpicGamesStore::showStoreUI(Window* window) { // Remove override
  std::cout << "EpicGamesStore::showStoreUI" << std::endl;
 }
 

 std::string EpicGamesStore::getStoreName() const { // Remove override
  return "EpicGamesStore";
 }
 

 void EpicGamesStore::shutdown() { // Remove override
  std::cout << "EpicGamesStore::shutdown" << std::endl;
 }
 

 std::vector<std::string> EpicGamesStore::getGamesList() { // Remove override
  std::cout << "EpicGamesStore::getGamesList" << std::endl;
  return {"Game 1", "Game 2", "Game 3"};
 }
 

 bool EpicGamesStore::installGame(const std::string& gameId) { // Remove override
  std::cout << "EpicGamesStore::installGame (" << gameId << ")" << std::endl;
  return true;
 }
 

 bool EpicGamesStore::uninstallGame(const std::string& gameId) { // Remove override
  std::cout << "EpicGamesStore::uninstallGame (" << gameId << ")" << std::endl;
  return true;
 }
 

 bool EpicGamesStore::updateGame(const std::string& gameId) { // Remove override
  std::cout << "EpicGamesStore::updateGame (" << gameId << ")" << std::endl;
  return true;
 }
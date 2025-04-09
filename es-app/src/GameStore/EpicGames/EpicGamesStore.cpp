#include "GameStore/EpicGames/EpicGamesStore.h"
 #include "Log.h"
 #include "guis/GuiMsgBox.h"
 #include "utils/Platform.h"
 #include "../../es-core/src/Window.h"  // Be VERY explicit with the path
 #include "GameStore/EpicGames/EpicGamesUI.h"
 #include "services/HttpServerThread.h"
 #include <filesystem>
 #include <iostream>
 #include <fstream>
 #include <string>
 #include <vector>
 #include <Windows.h>
 #include <ShlObj.h>
 #include <sstream>
 #include "json.hpp"
 #include "../../es-app/src/FileData.h"   // Be VERY explicit with the path
 #include "../../es-app/src/SystemData.h"  // Be VERY explicit with the path
  #include "utils/StringUtil.h"
 

using json = nlohmann::json;
 

 // --- getEpicLauncherConfigPath() DEFINED OUTSIDE the class ---
 std::string getEpicLauncherConfigPath() {
  return "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\EMS\\EpicGamesLauncher";
 }
 

 EpicGamesStore::EpicGamesStore(EpicGamesAuth* auth)
  : mAPI(), mUI(), mAuth(auth), mWindow(nullptr)
 {
  LOG(LogDebug) << "EpicGamesStore: Constructor (with auth)";
  testFindInstalledGames();
 }
 

 EpicGamesStore::EpicGamesStore()
  : mAPI(), mUI(), mAuth(nullptr), mWindow(nullptr)
 {
  LOG(LogDebug) << "EpicGamesStore: Constructor";
  testFindInstalledGames();
 }
 

 EpicGamesStore::~EpicGamesStore() {
  LOG(LogDebug) << "EpicGamesStore: Destructor";
  shutdown();
 }
 

 bool EpicGamesStore::init(Window* window) {
  mWindow = window;
  if (!mAPI.initialize()) {
  LOG(LogError) << "EpicGamesStore: Failed to initialize API";
  return false;
  }
  return true;
 }
 

 void EpicGamesStore::showStoreUI(Window* window) {
  LOG(LogDebug) << "EpicGamesStore: Showing store UI";
  mUI.showMainMenu(window, this);
 }
 

 std::string EpicGamesStore::getStoreName() const {
  return "EpicGamesStore";
 }
 

 void EpicGamesStore::shutdown() {
  LOG(LogDebug) << "EpicGamesStore: Shutting down";
  mAPI.shutdown();
 }
 
std::vector<FileData*> EpicGamesStore::getGamesList() {
     LOG(LogDebug) << "EpicGamesStore::getGamesList() - START";
     std::vector<FileData*> gameList;
     std::vector<std::string> paths = findInstalledEpicGames();
     LOG(LogDebug) << "EpicGamesStore::getGamesList() - Found " << paths.size() << " game paths";
     for (const auto& path : paths) {
         SystemData* system = mWindow->getCurrentSystem();
         FileData* game = new FileData(GAME, path, system);
         std::string name = path.substr(path.find_last_of("\\") + 1);
         game->setMetadata(MetaDataId::Name, name);
         gameList.push_back(game);
     }
     LOG(LogDebug) << "EpicGamesStore::getGamesList() - END, returning " << gameList.size() << " games";
     return gameList;
 }
 

 bool EpicGamesStore::installGame(const std::string& gameId) {
  LOG(LogDebug) << "EpicGamesStore::installGame (placeholder)";
  return true;
 }
 

 bool EpicGamesStore::uninstallGame(const std::string& gameId) {
  LOG(LogDebug) << "EpicGamesStore::uninstallGame (placeholder)";
  return true;
 }
 

 bool EpicGamesStore::updateGame(const std::string& gameId) {
  LOG(LogDebug) << "EpicGamesStore::updateGame (placeholder)";
  return true;
 }
 

 void EpicGamesStore::startLoginFlow() {
  std::string state;
  std::string authUrl = mAuth->getAuthorizationUrl(state);
  Utils::Platform::openUrl(authUrl);
 }
 

 void EpicGamesStore::processAuthCode(const std::string& authCode) {
  LOG(LogDebug) << "EpicGamesStore: Processing auth code: " << authCode;
  std::string accessToken;
  if (mAuth->getAccessToken(authCode, accessToken)) {
  mWindow->pushGui(new GuiMsgBox(mWindow, "Epic Games login successful!", "OK"));
  mUI.showGameList(mWindow, this);
  } else {
  mWindow->pushGui(new GuiMsgBox(mWindow, "Epic Games login failed.", "OK"));
  }
 }
 

 // Helper function to trim whitespace
 std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  if (std::string::npos == first) {
  return "";
  }
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
 }
 

 std::string EpicGamesStore::getLauncherInstalledDatPath() {
  return "C:\\ProgramData\\Epic\\UnrealEngineLauncher\\LauncherInstalled.dat";
 }
 

 std::vector<std::string> EpicGamesStore::findInstalledEpicGames() {
  std::vector<std::string> installedGames;
  std::filesystem::path launcherInstalledDatPath = getLauncherInstalledDatPath();
 

  // 1. Try to read from LauncherInstalled.dat
  if (std::filesystem::exists(launcherInstalledDatPath)) {
  std::ifstream datFile(launcherInstalledDatPath);
  if (datFile.is_open()) {
  LOG(LogDebug) << "Reading game paths from LauncherInstalled.dat";
  std::stringstream datContents;
  std::string datLine;
  while (std::getline(datFile, datLine)) {
  datContents << datLine;
  }
  datFile.close();
 

  try {
  json parsedData = json::parse(datContents.str());
  for (const auto& entry : parsedData["InstallationList"]) {
  std::string installLocation = entry["InstallLocation"].get<std::string>();
  installedGames.push_back(installLocation);
  LOG(LogInfo) << "  Found game from dat: " << installLocation;
  }
  } catch (const json::parse_error& e) {
  LOG(LogError) << "  Error parsing LauncherInstalled.dat: " << e.what();
  LOG(LogWarning) << "  Falling back to directory scan.";
  installedGames.clear();
  }
  } else {
  LOG(LogError) << "  Failed to open LauncherInstalled.dat";
  LOG(LogWarning) << "  Falling back to directory scan.";
  }
  } else {
  LOG(LogWarning) << "  LauncherInstalled.dat not found. Falling back to directory scan.";
  }
 

  // 2. Fallback to directory scan (if LauncherInstalled.dat failed)
  if (installedGames.empty()) {
  std::vector<std::string> defaultInstallLocations = {
  "C:\\Program Files\\Epic Games",
  "C:\\Program Files (x86)\\Epic Games"
  };
 

  for (const auto& installLocation : defaultInstallLocations) {
  if (std::filesystem::exists(installLocation) && std::filesystem::is_directory(installLocation)) {
  LOG(LogDebug) << "  Checking for games in: " << installLocation;
  for (const auto& entry : std::filesystem::directory_iterator(installLocation)) {
  if (entry.is_directory()) {
  std::filesystem::path gameDir = entry.path();
  LOG(LogDebug) << "   Checking directory: " << gameDir;
  std::filesystem::path egstorePath = gameDir / ".egstore";
  if (std::filesystem::exists(egstorePath) && std::filesystem::is_directory(egstorePath)) {
  bool foundGame = false;
  for (const auto& egstoreEntry : std::filesystem::directory_iterator(egstorePath)) {
  if (egstoreEntry.is_regular_file() && egstoreEntry.path().extension() == ".manifest") {
  LOG(LogInfo) << "    Found game in: " << gameDir << " (using .manifest)";
  installedGames.push_back(gameDir.string());
  foundGame = true;
  break;
  }
  }
  if (!foundGame) {
  for (const auto& egstoreEntry : std::filesystem::directory_iterator(egstorePath)) {
  if (egstoreEntry.is_regular_file() && egstoreEntry.path().extension() == ".mancpn") {
  LOG(LogInfo) << "    Found game in: " << gameDir << " (using .mancpn)";
  installedGames.push_back(gameDir.string());
  foundGame = true;
  break;
  } else {
  LOG(LogDebug) << "    Not a manifest file: " << egstoreEntry.path();
  }
  }
  if (!foundGame) {
  LOG(LogWarning) << "    No .manifest or .mancpn file found in .egstore directory: " << egstorePath;
  }
  }
  } else {
  LOG(LogWarning) << "  Install location does not exist or is not a directory: " << installLocation;
  }
  }
  }
 

  return installedGames;
   }
  }
  }
 }
 
  std::string EpicGamesStore::getEpicGameId(const std::string& path) {
  std::string gameId = "";
  std::filesystem::path launcherInstalledDatPath = getLauncherInstalledDatPath();
 

  if (std::filesystem::exists(launcherInstalledDatPath)) {
  std::ifstream datFile(launcherInstalledDatPath);
  if (datFile.is_open()) {
  std::stringstream datContents;
  std::string datLine;
  while (std::getline(datFile, datLine)) {
  datContents << datLine;
  }
  datFile.close();
 

  try {
  json parsedData = json::parse(datContents.str());
  for (const auto& entry : parsedData["InstallationList"]) {
  std::string installLocation = entry["InstallLocation"].get<std::string>();
  if (Utils::String::compareIgnoreCase(installLocation, path)) {
  gameId = entry["AppName"].get<std::string>();
  break;
  }
  }
  } catch (const json::parse_error& e) {
  LOG(LogError) << "Error parsing LauncherInstalled.dat: " << e.what();
  }
  } else {
  LOG(LogError) << "LauncherInstalled.dat not found.";
  }
  }
 

  return gameId;
 }

 void EpicGamesStore::testFindInstalledGames() {
  LOG(LogDebug) << "EpicGamesStore::testFindInstalledGames() called!";
  std::vector<std::string> games = findInstalledEpicGames();
  if (games.empty()) {
  LOG(LogWarning) << "No Epic Games found.";
  } else {
  LOG(LogInfo) << "Found Epic Games:";
  for (const auto& game : games) {
  LOG(LogInfo) << "  " << game;
  }
  }
 }
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "Log.h"
#include "guis/GuiMsgBox.h"
#include "utils/Platform.h"
#include "../../es-core/src/Window.h"
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
#include "../../es-app/src/FileData.h"
#include "../../es-app/src/SystemData.h"
#include "utils/StringUtil.h"

using json = nlohmann::json;

// --- getEpicLauncherConfigPath() DEFINED OUTSIDE the class ---
std::string getEpicLauncherConfigPath() {
    return "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\EMS\\EpicGamesLauncher";
}

EpicGamesStore::EpicGamesStore(EpicGamesAuth* auth)
    : mAPI(), mUI(), mAuth(auth), mWindow(nullptr) {
    LOG(LogDebug) << "EpicGamesStore: Constructor (with auth)";
    testFindInstalledGames();
}

EpicGamesStore::EpicGamesStore()
    : mAPI(), mUI(), mAuth(nullptr), mWindow(nullptr) {
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
  _initialized = true;  //  Add this line
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
      std::vector<EpicGamesStore::EpicGameInfo> epicGames = getInstalledEpicGamesWithDetails();
      LOG(LogDebug) << "EpicGamesStore::getGamesList() - Found " << epicGames.size() << " games";

      SystemData* system = mWindow->getCurrentSystem();
      for (const auto& game : epicGames) {
      FileData* gameFileData = new FileData(GAME, game.launchCommand, system);  //  Use launchCommand
      gameFileData->setMetadata(MetaDataId::Name, game.name);
      gameFileData->setMetadata(MetaDataId::InstallDir, game.installDir);  //  Store install dir
      gameFileData->setMetadata(MetaDataId::Executable, game.executable);  //  Store executable
      gameFileData->setMetadata(MetaDataId::launchCommand, game.launchCommand);  //  STORE THE LAUNCH COMMAND!
      gameList.push_back(gameFileData);
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
                if (parsedData.contains("InstallationList")) {  //  Check if "InstallationList" exists
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("InstallLocation")) {  //  Check if "InstallLocation" exists
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            installedGames.push_back(installLocation);
                            LOG(LogInfo) << "  Found game from dat: " << installLocation;
                        } else {
                            LOG(LogWarning) << "  Entry missing InstallLocation, skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found in LauncherInstalled.dat, falling back to directory scan.";
                    installedGames.clear();
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

    // 2. Fallback to directory scan (if LauncherInstalled.dat failed or was empty)
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
                        }
                    }
                }
            } else {
                LOG(LogWarning) << "  Install location does not exist or is not a directory: " << installLocation;
            }
        }
    }
    return installedGames;
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
                if (parsedData.contains("InstallationList")) {  //  Check if "InstallationList" exists
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("InstallLocation") && entry.contains("AppName")) {  //  Check if keys exist
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            if (Utils::String::compareIgnoreCase(installLocation, path)) {
                                gameId = entry["AppName"].get<std::string>();
                                break;
                            }
                        } else {
                            LOG(LogWarning) << "  Entry missing InstallLocation or AppName, skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found in LauncherInstalled.dat.";
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

std::vector<EpicGamesStore::EpicGameInfo> EpicGamesStore::getInstalledEpicGamesWithDetails() {
    std::vector<EpicGamesStore::EpicGameInfo> games;
    std::filesystem::path launcherInstalledDatPath = getLauncherInstalledDatPath();
    std::string metadataPath = getMetadataPath();

    // 1. Read LauncherInstalled.dat
    std::map<std::string, std::string> installLocations;  // AppName -> InstallLocation
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
                if (parsedData.contains("InstallationList")) {  //  Check if "InstallationList" exists
                    for (const auto& entry : parsedData["InstallationList"]) {
                        if (entry.contains("AppName") && entry.contains("InstallLocation")) {  //  Check if keys exist
                            std::string appName = entry["AppName"].get<std::string>();
                            std::string installLocation = entry["InstallLocation"].get<std::string>();
                            installLocations[appName] = installLocation;
                            LOG(LogDebug) << "  Found app from dat: " << appName << " -> " << installLocation;
                        } else {
                            LOG(LogWarning) << "  Entry missing AppName or InstallLocation, skipping.";
                        }
                    }
                } else {
                    LOG(LogWarning) << "  InstallationList not found in LauncherInstalled.dat.";
                }
            } catch (const json::parse_error& e) {
                LOG(LogError) << "  Error parsing LauncherInstalled.dat: " << e.what();
                installLocations.clear();  // Clear to force fallback
            }
        } else {
            LOG(LogError) << "  Failed to open LauncherInstalled.dat";
        }
    } else {
        LOG(LogWarning) << "  LauncherInstalled.dat not found.";
    }

    // 2. Read .item manifests
    if (!metadataPath.empty() && std::filesystem::exists(metadataPath) && std::filesystem::is_directory(metadataPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(metadataPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".item") {
                std::ifstream itemFile(entry.path());
                if (itemFile.is_open()) {
                    std::stringstream itemContents;
                    std::string itemLine;
                    while (std::getline(itemFile, itemLine)) {
                        itemContents << itemLine;
                    }
                    itemFile.close();

                    try {
                        json manifest = json::parse(itemContents.str());
                        if (manifest.contains("AppName")) {  //  Check if "AppName" exists
                            std::string appName = manifest["AppName"].get<std::string>();
                            // Skip DLCs (adjust logic as needed)
                            if (manifest.contains("MainGameAppName") && manifest["AppName"] != manifest["MainGameAppName"]) {
                                continue;
                            }
                            // Skip Plugins (adjust logic as needed)
                            if (manifest.contains("AppCategories")) {
                                bool isPlugin = false;
                                for (const auto& category : manifest["AppCategories"]) {
                                    if (category.is_string()) {  //  Check if category is a string
                                        std::string catStr = category.get<std::string>();
                                        if (catStr == "plugins" || catStr == "plugins/engine") {
                                            isPlugin = true;
                                            break;
                                        }
                                    } else {
                                        LOG(LogWarning) << "  AppCategories entry is not a string, skipping.";
                                    }
                                }
                                if (isPlugin) {
                                    continue;
                                }
                            }

                            EpicGamesStore::EpicGameInfo game;
                            game.id = appName;
                            game.name = manifest.contains("DisplayName") && manifest["DisplayName"].is_string() ? manifest["DisplayName"].get<std::string>() : appName;  //  Check if "DisplayName" exists and is a string
                            game.executable = manifest.contains("LaunchExecutable") && manifest["LaunchExecutable"].is_string() ? manifest["LaunchExecutable"].get<std::string>() : "";  //  Check if "LaunchExecutable" exists and is a string
                            game.installDir = installLocations.count(appName) ? installLocations[appName] : "";

                            // Construct launch command (URL)
                            game.launchCommand = "com.epicgames.launcher://apps/" + appName + "?action=launch&silent=true";

                            games.push_back(game);
                            LOG(LogInfo) << "  Found game: " << game.name << " (" << game.id << ")";
                        } else {
                            LOG(LogWarning) << "  Manifest missing AppName, skipping.";
                        }
                    } catch (const json::parse_error& e) {
                        LOG(LogError) << "  Error parsing .item manifest: " << e.what();
                    }
                } else {
                    LOG(LogError) << "  Failed to open .item manifest: " << entry.path();
                }
            }
        }
    } else {
        LOG(LogWarning) << "  Metadata path not found: " << metadataPath;
    }

    // 3. Fallback to directory scan (less reliable, remove/adjust if needed)
    if (games.empty()) {
        LOG(LogWarning) << "  No games found using manifests, falling back to directory scan (less reliable).";
        std::vector<std::string> paths = findInstalledEpicGames();
        for (const auto& path : paths) {
            EpicGamesStore::EpicGameInfo game;
            game.name = path.substr(path.find_last_of("\\") + 1);
            game.installDir = path;
            game.launchCommand = "";  // Leave empty for now
            games.push_back(game);
        }
    }
    return games;
}

std::string EpicGamesStore::getMetadataPath() {
    std::string metadataPath;
    // [IMPLEMENT REGISTRY READING - similar to EmulatorLauncher.Common/Launchers/EpicLibrary.cs]
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Epic Games\\EOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char data[256];
        DWORD dataSize = sizeof(data);
        if (RegGetValue(hKey, NULL, "ModSdkMetadataDir", RRF_RT_REG_SZ, NULL, data, &dataSize) == ERROR_SUCCESS) {
            metadataPath = std::string(data, dataSize - 1);  // Remove null terminator
        }
        RegCloseKey(hKey);
    }
    if (metadataPath.empty()) {
        LOG(LogWarning) << "  Could not read ModSdkMetadataDir from registry.";
    }
    return metadataPath;
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
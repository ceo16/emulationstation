// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesScanner.cpp
#include "EAGamesScanner.h"
#include "Log.h"
#include "utils/FileSystemUtil.h" // Il tuo FileSystemUtil.h
#include "utils/StringUtil.h"
// ApiSystem.h non è necessario qui se usiamo #ifdef _WIN32 per i controlli OS
#include "pugixml.hpp"
#include "json.hpp"         // Per parsare il file di settings di EA Desktop
#include "Paths.h"          // Il tuo Paths.h

// Per i percorsi di sistema Windows, se Paths.h non li fornisce specificamente
#ifdef _WIN32
#include <Shlobj.h> // Per SHGetKnownFolderPath
#ifndef FOLDERID_ProgramData // Alcuni vecchi SDK potrebbero non averlo predefinito
DEFINE_KNOWN_FOLDER(FOLDERID_ProgramData, 0x62AB5D82, 0xFDC1, 0x4DC3, 0xA9, 0xDD, 0x07, 0x0D, 0x1D, 0x49, 0x5D, 0x97);
#endif
#ifndef FOLDERID_LocalAppData
DEFINE_KNOWN_FOLDER(FOLDERID_LocalAppData, 0xF1B32785, 0x6FBA, 0x4FCF, 0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x91);
#endif
#include <codecvt>
#include <locale>
#endif

namespace EAGames
{
    #ifdef _WIN32
    // Helper per convertire wstring a string (necessario per SHGetKnownFolderPath)
    std::string wstring_to_utf8_string(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        // std::wstring_convert è deprecato in C++17. Usiamo WideCharToMultiByte.
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    std::string getWindowsKnownPath(REFKNOWNFOLDERID folderId) {
        PWSTR pszPath = nullptr;
        HRESULT hr = SHGetKnownFolderPath(folderId, 0, NULL, &pszPath);
        if (SUCCEEDED(hr)) {
            std::wstring pathStr(pszPath);
            CoTaskMemFree(pszPath);
            return wstring_to_utf8_string(pathStr);
        }
        LOG(LogError) << "SHGetKnownFolderPath failed with HRESULT: " << hr;
        return "";
    }
    #endif

    EAGamesScanner::EAGamesScanner() {}

    std::string EAGamesScanner::resolveKnownFolderPath(const std::string& knownFolderToken)
    {
        // Paths.h non ha metodi diretti per ProgramData/LocalAppData
        #ifdef _WIN32
        if (knownFolderToken == "%PROGRAMDATA%") {
            return getWindowsKnownPath(FOLDERID_ProgramData);
        }
        if (knownFolderToken == "%LOCALAPPDATA%") {
            return getWindowsKnownPath(FOLDERID_LocalAppData);
        }
        #else
        // Implementazione non-Windows (se necessaria, ma EA è principalmente Windows)
        if (knownFolderToken == "%PROGRAMDATA%") return "/var/lib"; // Esempio grezzo
        if (knownFolderToken == "%LOCALAPPDATA%") {
            std::string home = Paths::getHomePath(); // Usa Paths::getHomePath()
            if (!home.empty()) return home + "/.local/share";
        }
        #endif
        LOG(LogWarning) << "EA Games Scanner: Could not resolve known folder token: " << knownFolderToken;
        return knownFolderToken;
    }

    EADesktopSettings EAGamesScanner::getEADesktopClientSettings()
    {
        EADesktopSettings settings;
        #ifndef _WIN32
        LOG(LogInfo) << "EA Games Scanner: getEADesktopClientSettings currently only supports Windows.";
        return settings;
        #endif

        // Percorso per EADesktopConfig.ini
        std::string programDataDir = resolveKnownFolderPath("%PROGRAMDATA%");
        if (programDataDir.empty() && Utils::FileSystem::exists("/usr/share/Electronic Arts/EA Desktop/Settings")) // Fallback per possibili installazioni Linux/Wine
             programDataDir = "/usr/share"; // Percorso base
        else if (programDataDir.empty())
        {
            LOG(LogError) << "EA Games Scanner: ProgramData directory could not be determined.";
            // Non possiamo continuare senza un percorso base valido su Windows
            #ifdef _WIN32
            return settings;
            #endif
        }


        std::string configIniPath = Utils::FileSystem::combine(programDataDir, "Electronic Arts/EA Desktop/Settings/EADesktopConfig.ini");
        if (Utils::FileSystem::exists(configIniPath)) {
            LOG(LogInfo) << "EA Games Scanner: Reading EA Desktop config INI: " << configIniPath;
            std::string content = Utils::FileSystem::readAllText(configIniPath);
            std::vector<std::string> lines = Utils::String::split(content, '\n');
            for (const std::string& line : lines) {
                if (Utils::String::startsWith(line, "HardwareId=")) {
                    settings.HardwareId = Utils::String::trim(line.substr(std::string("HardwareId=").length()));
                }
            }
            LOG(LogInfo) << "EA Games Scanner: EA Desktop HardwareId: " << settings.HardwareId;
        } else {
            LOG(LogWarning) << "EA Games Scanner: EA Desktop config INI not found at " << configIniPath;
        }

        // Percorso per user.config
        std::string localAppDataDir = resolveKnownFolderPath("%LOCALAPPDATA%");
         if (localAppDataDir.empty() && Utils::FileSystem::exists(Paths::getHomePath() + "/.local/share/Electronic Arts/EA Desktop")) // Fallback per possibili installazioni Linux/Wine
             localAppDataDir = Paths::getHomePath() + "/.local/share";
         else if (localAppDataDir.empty())
         {
             LOG(LogError) << "EA Games Scanner: LocalAppData directory could not be determined.";
             #ifdef _WIN32
             return settings;
             #endif
         }

        std::string userConfigPath = Utils::FileSystem::combine(localAppDataDir, "Electronic Arts/EA Desktop/user.config");
        if (Utils::FileSystem::exists(userConfigPath)) {
            LOG(LogInfo) << "EA Games Scanner: Reading EA Desktop user.config: " << userConfigPath;
            try {
                auto userJson = nlohmann::json::parse(Utils::FileSystem::readAllText(userConfigPath));
                std::string installPathKey = "contentInstallPath"; // Chiave base
                if (userJson.contains(installPathKey) && userJson[installPathKey].is_string()) {
                    std::string path = userJson[installPathKey].get<std::string>();
                    if (!path.empty() && std::find(settings.ContentInstallPath.begin(), settings.ContentInstallPath.end(), path) == settings.ContentInstallPath.end()) {
                        settings.ContentInstallPath.push_back(path);
                    }
                }
                for (int i = 2; i <= 10; ++i) { // Cerca contentInstallPath2, contentInstallPath3, ecc.
                    std::string key = installPathKey + std::to_string(i);
                    if (userJson.contains(key) && userJson[key].is_string()) {
                        std::string path = userJson[key].get<std::string>();
                         if (!path.empty() && std::find(settings.ContentInstallPath.begin(), settings.ContentInstallPath.end(), path) == settings.ContentInstallPath.end()) {
                            settings.ContentInstallPath.push_back(path);
                        }
                    } else {
                        // Se una chiave sequenziale manca, interrompi la ricerca per quella serie
                        // (EA Desktop potrebbe non usare numeri sequenziali perfetti,
                        //  ma Playnite fa così e sembra funzionare)
                        break;
                    }
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games Scanner: Failed to parse EA Desktop user.config: " << e.what();
            }
        } else {
            LOG(LogWarning) << "EA Games Scanner: EA Desktop user.config not found at " << userConfigPath;
        }

        for(const auto& path : settings.ContentInstallPath) {
            LOG(LogInfo) << "EA Games Scanner: EA Desktop Install Path found: " << path;
        }
        return settings;
    }

    InstalledGameInfo EAGamesScanner::parseOriginManifest(const std::string& filePath, pugi::xml_document& doc) {
        InstalledGameInfo gameInfo;
        pugi::xml_parse_result result = doc.load_file(filePath.c_str());

        if (!result) {
            LOG(LogError) << "EA Games Scanner: Error parsing manifest file " << filePath << ": " << result.description();
            return gameInfo;
        }

        pugi::xml_node manifestNode = doc.child("DiPManifest");
        if (!manifestNode) manifestNode = doc.child("dipmanifest");
        if (!manifestNode) {
            for (pugi::xml_node child : doc.children()) {
                if (Utils::String::toLower(child.name()).find("dipmanifest") != std::string::npos) {
                    manifestNode = child;
                    break;
                }
            }
            if (!manifestNode) {
                 LOG(LogWarning) << "EA Games Scanner: No DiPManifest root node in " << filePath;
                 return gameInfo;
            }
        }

        gameInfo.id = manifestNode.child_value("mastertitleid");
        if (gameInfo.id.empty()) gameInfo.id = manifestNode.child("contentids").child_value("contentid");
        if (gameInfo.id.empty()) gameInfo.id = manifestNode.child_value("gameid"); // Può essere OfferId

        gameInfo.multiplayerId = manifestNode.child_value("multiplayerid");

        pugi::xml_node gameTitlesNode = manifestNode.child("gameTitles");
        if (gameTitlesNode) {
            std::string defaultLang = manifestNode.child_value("defaultlanguage");
            if (defaultLang.empty()) defaultLang = "en_US";
            for (pugi::xml_node titleNode : gameTitlesNode.children("gameTitle")) {
                if (std::string(titleNode.attribute("locale").as_string()) == defaultLang) {
                    gameInfo.name = titleNode.text().as_string();
                    break;
                }
            }
            if (gameInfo.name.empty() && gameTitlesNode.first_child()) {
                gameInfo.name = gameTitlesNode.first_child().text().as_string();
            }
        }
        if (gameInfo.name.empty() && manifestNode.child("gameTitle")) {
             gameInfo.name = manifestNode.child("gameTitle").text().as_string();
        }
        if (gameInfo.name.empty() && !gameInfo.id.empty()) {
            gameInfo.name = "EA Game (" + gameInfo.id + ")";
        }

        pugi::xml_node metaDataNode = manifestNode.child("installMetaData");
        if (!metaDataNode) metaDataNode = manifestNode.child("installmetadata");

        if (metaDataNode) {
            gameInfo.installPath = metaDataNode.child_value("dipinstallpath");
            if (Utils::String::startsWith(gameInfo.installPath, "\"") && Utils::String::endsWith(gameInfo.installPath, "\"")) {
                gameInfo.installPath = gameInfo.installPath.substr(1, gameInfo.installPath.length() - 2);
            }
            if (!gameInfo.installPath.empty()) {
                 // Usa getCanonicalPath per normalizzare. Assicurati che esista e sia appropriato.
                 // Se getCanonicalPath non fa ciò che ti aspetti per la normalizzazione (es. / vs \),
                 // potresti aver bisogno di una funzione di normalizzazione personalizzata o aggiuntiva.
                 gameInfo.installPath = Utils::FileSystem::getCanonicalPath(gameInfo.installPath);
            }

            gameInfo.executablePath = metaDataNode.child_value("executePathRelative");
            if (gameInfo.executablePath.empty()) gameInfo.executablePath = metaDataNode.child_value("executepathrelative");
            
            gameInfo.launchParameters = metaDataNode.child_value("executePathArguments");
            if (gameInfo.launchParameters.empty()) gameInfo.launchParameters = metaDataNode.child_value("executepatharguments");
        }
        gameInfo.version = manifestNode.child_value("version");
        if (gameInfo.id.empty() || gameInfo.installPath.empty() || gameInfo.executablePath.empty()) {
            // LOG(LogWarning) << "Incomplete data from manifest " << filePath; // Meno verboso
            return {};
        }
        LOG(LogDebug) << "EA Games Scanner: Parsed manifest " << Utils::FileSystem::getFileName(filePath)
                     << " -> Name: " << gameInfo.name << ", ID: " << gameInfo.id;
        return gameInfo;
    }

    void EAGamesScanner::findGamesFromManifests(const std::string& manifestBaseDir, std::vector<InstalledGameInfo>& foundGames, std::vector<std::string>& processedManifestIds) {
        #ifndef _WIN32
        // Su piattaforme non Windows, questa logica potrebbe non applicarsi o fallire.
        // Ma tentiamo comunque se il percorso esiste (es. per Wine).
        #endif

        if (manifestBaseDir.empty() || !Utils::FileSystem::exists(manifestBaseDir) || !Utils::FileSystem::isDirectory(manifestBaseDir)) {
            return;
        }

        LOG(LogInfo) << "EA Games Scanner: Scanning for manifests in " << manifestBaseDir;
        
        pugi::xml_document doc; // Riusa il documento per efficienza
        // Utils::FileSystem::getDirectoryFiles restituisce std::list<FileInfo>
        Utils::FileSystem::fileList allFilesInDir = Utils::FileSystem::getDirectoryFiles(manifestBaseDir);

        for (const Utils::FileSystem::FileInfo& fileInfo : allFilesInDir) {
            if (fileInfo.directory) { // Salta le sottodirectory (getDirectoryFiles non è ricorsivo qui)
                continue;
            }

            // Filtra per estensione .mfst
            if (Utils::String::toLower(Utils::FileSystem::getExtension(fileInfo.path)) == ".mfst") {
                std::string manifestFileNameStem = Utils::FileSystem::getStem(fileInfo.path);
                if (std::find(processedManifestIds.begin(), processedManifestIds.end(), manifestFileNameStem) != processedManifestIds.end()) {
                    continue; // Già processato
                }

                InstalledGameInfo game = parseOriginManifest(fileInfo.path, doc);
                if (!game.id.empty()) {
                    bool alreadyFound = false;
                    for (const auto& existingGame : foundGames) {
                        if (existingGame.id == game.id) {
                            alreadyFound = true;
                            LOG(LogWarning) << "EA Games Scanner: Duplicate game ID found from manifest: " << game.id << " (" << game.name << "). Keeping first one.";
                            break;
                        }
                    }
                    if (!alreadyFound) {
                        foundGames.push_back(game);
                        processedManifestIds.push_back(manifestFileNameStem);
                    }
                }
            }
        }
    }

    std::vector<InstalledGameInfo> EAGamesScanner::scanForInstalledGames() {
        std::vector<InstalledGameInfo> installedGames;
        std::vector<std::string> processedManifestGameIds; // Usato per ID da manifest .mfst
        LOG(LogInfo) << "EA Games Scanner: Starting scan for installed EA games.";

        #ifndef _WIN32
        LOG(LogWarning) << "EA Games Scanner: Game scanning is primarily designed for Windows.";
        #endif

        // Percorsi dei manifest principali
        std::string programData = resolveKnownFolderPath("%PROGRAMDATA%");
        if (!programData.empty()) {
            const std::string originManifestDir = Utils::FileSystem::combine(programData, "Origin/LocalContent");
            const std::string eaDesktopManifestDir = Utils::FileSystem::combine(programData, "Electronic Arts/EA Desktop/LocalContent");
            findGamesFromManifests(originManifestDir, installedGames, processedManifestGameIds);
            findGamesFromManifests(eaDesktopManifestDir, installedGames, processedManifestGameIds);
        } else {
            LOG(LogError) << "EA Games Scanner: Could not determine ProgramData path. Skipping standard manifest scan.";
        }


        // Scansione delle librerie aggiuntive da EA Desktop Settings
        EADesktopSettings clientSettings = getEADesktopClientSettings();
        for (const std::string& installLibPath : clientSettings.ContentInstallPath) {
            if (installLibPath.empty() || !Utils::FileSystem::exists(installLibPath) || !Utils::FileSystem::isDirectory(installLibPath)) {
                continue;
            }

            LOG(LogInfo) << "EA Games Scanner: Scanning EA Desktop library path: " << installLibPath;
            // Utils::FileSystem::getDirectoryFiles restituisce una lista di tutte le voci (file e dir)
            // direttamente sotto installLibPath. Dobbiamo iterare e prendere solo le directory dei giochi.
            Utils::FileSystem::fileList contentOfInstallLib = Utils::FileSystem::getDirectoryFiles(installLibPath);
            
            for (const Utils::FileSystem::FileInfo& gameFolderInfo : contentOfInstallLib) {
                if (!gameFolderInfo.directory) { // Vogliamo solo le cartelle dei giochi
                    continue;
                }

                std::string gameFolderPath = gameFolderInfo.path;
                std::string installerXmlPath1 = Utils::FileSystem::combine(gameFolderPath, "__Installer/installerdata.xml");
                std::string installerXmlPath2 = Utils::FileSystem::combine(gameFolderPath, "__Installer/Meta/installerdata.xml");
                std::string targetXmlPath;

                if (Utils::FileSystem::exists(installerXmlPath1)) targetXmlPath = installerXmlPath1;
                else if (Utils::FileSystem::exists(installerXmlPath2)) targetXmlPath = installerXmlPath2;

                if (!targetXmlPath.empty()) {
                    pugi::xml_document doc;
                    pugi::xml_parse_result result = doc.load_file(targetXmlPath.c_str());
                    if (result) {
                        InstalledGameInfo gameInfo;
                        gameInfo.installPath = Utils::FileSystem::getCanonicalPath(gameFolderPath);
                        
                        pugi::xml_node contentIdNode = doc.select_node("//contentIDs/contentID").node();
                        if (!contentIdNode) contentIdNode = doc.select_node("//contentIds/contentId").node();

                        if (contentIdNode) {
                            gameInfo.id = contentIdNode.text().as_string(); // Questo è OfferId
                        } else {
                             LOG(LogWarning) << "EA Games Scanner: No contentID found in " << targetXmlPath;
                             continue;
                        }
                        
                        // Evita duplicati se l'ID (OfferID qui) è già stato aggiunto
                        bool alreadyInListById = false;
                        for(const auto& existingGame : installedGames) {
                            if (existingGame.id == gameInfo.id) { // Compara OfferID con ID esistenti
                                alreadyInListById = true;
                                LOG(LogDebug) << "EA Games Scanner: Game " << gameInfo.id << " from installerdata.xml already found. Skipping.";
                                break;
                            }
                        }
                        if (alreadyInListById) {
                            continue;
                        }

                        gameInfo.name = Utils::FileSystem::getFileName(gameFolderPath);
                        pugi::xml_node runtimeNode = doc.select_node("//runtime").node();
                        if (runtimeNode) {
                            std::string execPath = runtimeNode.attribute("exec").as_string();
                            if (execPath.empty()) execPath = runtimeNode.attribute("path").as_string();
                            gameInfo.executablePath = execPath;
                            gameInfo.launchParameters = runtimeNode.attribute("args").as_string();
                        }
                         if (gameInfo.executablePath.empty()) {
                             LOG(LogWarning) << "EA Games Scanner: No executable path found in " << targetXmlPath << " for " << gameInfo.name;
                             continue;
                         }

                        LOG(LogInfo) << "EA Games Scanner: Found game from installerdata.xml: " << gameInfo.name << " (ID: " << gameInfo.id << ")";
                        installedGames.push_back(gameInfo);
                    } else {
                        LOG(LogWarning) << "EA Games Scanner: Error parsing installerdata.xml: " << targetXmlPath << " - " << result.description();
                    }
                }
            }
        }
        LOG(LogInfo) << "EA Games Scanner: Found " << installedGames.size() << " installed EA games in total.";
        return installedGames;
    }

} // namespace EAGames
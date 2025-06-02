// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesScanner.h
#pragma once

#include <string>
#include <vector>
#include "EAGamesModels.h"

namespace pugi { class xml_document; } // Forward declaration

namespace EAGames
{
    class EAGamesScanner
    {
    public:
        EAGamesScanner();

        std::vector<InstalledGameInfo> scanForInstalledGames();
        EADesktopSettings getEADesktopClientSettings();

    private:
        void findGamesFromManifests(const std::string& manifestDir, std::vector<InstalledGameInfo>& foundGames, std::vector<std::string>& processedManifestIds);
        InstalledGameInfo parseOriginManifest(const std::string& filePath, pugi::xml_document& doc); // Pass doc per efficienza

        // Percorsi per EA Desktop client settings (EAC.cs)
        const std::string EA_DESKTOP_SETTINGS_PATH_LEGACY = "%PROGRAMDATA%\\Electronic Arts\\EA Desktop\\Settings\\user_%PID%.setting";
        const std::string EA_DESKTOP_SETTINGS_PATH = "%LOCALAPPDATA%\\Electronic Arts\\EA Desktop\\Settings\\user_%PID%.setting";

        std::string resolveKnownFolderPath(const std::string& knownFolder);
    };

} // namespace EAGames
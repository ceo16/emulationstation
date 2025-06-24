#include "GogScanner.h"
#include "Log.h"
#include "utils/StringUtil.h"

#ifdef _WIN32
#include <Windows.h>
#endif

GogScanner::GogScanner() {}

std::vector<GOG::InstalledGameInfo> GogScanner::findInstalledGames()
{
    std::vector<GOG::InstalledGameInfo> installedGames;
#ifdef _WIN32
    LOG(LogInfo) << "[GOG Scanner] Avvio scansione del registro per giochi installati...";
    
    const char* regKeys[] = {
        "SOFTWARE\\WOW6432Node\\GOG.com\\Games",
        "SOFTWARE\\GOG.com\\Games"
    };

    for (const char* keyPath : regKeys) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char subKeyName[255];
            DWORD subKeySize;
            DWORD index = 0;
            while (RegEnumKeyExA(hKey, index++, subKeyName, &(subKeySize = sizeof(subKeyName)), NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                
                HKEY gameKey;
                if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &gameKey) == ERROR_SUCCESS) {
                    GOG::InstalledGameInfo game;
                    game.id = subKeyName;
                    
                    char buffer[1024];
                    DWORD bufferSize = sizeof(buffer);

                    if (RegQueryValueExA(gameKey, "gameName", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                        game.name = buffer;
                    }
                    if (RegQueryValueExA(gameKey, "path", NULL, NULL, (LPBYTE)buffer, &(bufferSize = sizeof(buffer))) == ERROR_SUCCESS) {
                        game.installDirectory = buffer;
                    }

                    if (!game.id.empty() && !game.name.empty() && !game.installDirectory.empty()) {
                        LOG(LogInfo) << "[GOG Scanner] Trovato: " << game.name << " (ID: " << game.id << ")";
                        installedGames.push_back(game);
                    }
                    RegCloseKey(gameKey);
                }
            }
            RegCloseKey(hKey);
        }
    }
    LOG(LogInfo) << "[GOG Scanner] Scansione completata. Trovati " << installedGames.size() << " giochi.";
#endif
    return installedGames;
}
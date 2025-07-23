#include "GogScanner.h"
#include "Log.h"
#include "utils/StringUtil.h"
#include <fstream>
#include <filesystem>
#include "json.hpp" // Assicurati che il percorso del file json.hpp sia corretto

// Per convenienza
namespace fs = std::filesystem;
using json = nlohmann::json;

#ifdef _WIN32
#include <Windows.h>
#endif

GogScanner::GogScanner() {}

std::vector<GOG::InstalledGameInfo> GogScanner::findInstalledGames()
{
    std::vector<GOG::InstalledGameInfo> installedGames;
#ifdef _WIN32
    LOG(LogInfo) << "[GOG Scanner] Avvio scansione del registro per giochi installati (con filtro DLC)...";
    
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
                        
                        // --- INIZIO MODIFICA: LOGICA FILTRAGGIO DLC ---

                        fs::path manifestPath = fs::path(game.installDirectory) / ("goggame-" + game.id + ".info");

                        if (fs::exists(manifestPath)) {
                            try {
                                std::ifstream f(manifestPath);
                                json data = json::parse(f);

                                // Controlla se 'rootGameId' esiste ed è diverso dall'ID del gioco stesso.
                                // Se è così, questo è un DLC e va saltato.
                                if (data.contains("rootGameId") && data["rootGameId"].get<std::string>() != game.id) {
                                    LOG(LogInfo) << "[GOG Scanner] Saltato DLC '" << game.name << "' (ID: " << game.id << "). Appartiene al gioco " << data["rootGameId"].get<std::string>();
                                    RegCloseKey(gameKey);
                                    continue; // Salta al prossimo gioco nel registro
                                }
                            } catch (const json::parse_error& e) {
                                // Se il file è corrotto, logga l'errore ma procedi come se non esistesse
                                LOG(LogError) << "[GOG Scanner] Errore nel parsing del manifesto DLC " << manifestPath << ": " << e.what();
                            }
                        }
                        
                        // --- FINE MODIFICA ---

                        LOG(LogInfo) << "[GOG Scanner] Trovato: " << game.name << " (ID: " << game.id << ")";
                        installedGames.push_back(game);
                    }
                    RegCloseKey(gameKey);
                }
            }
            RegCloseKey(hKey);
        }
    }
    LOG(LogInfo) << "[GOG Scanner] Scansione completata. Trovati " << installedGames.size() << " giochi (filtrati i DLC).";
#endif
    return installedGames;
}
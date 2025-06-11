#pragma once
#ifndef ES_APP_GAME_STORE_EA_GAMES_SCANNER_H
#define ES_APP_GAME_STORE_EA_GAMES_SCANNER_H

#include "GameStore/EAGames/EAGamesModels.h" // Usa la tua struttura dati esistente
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace EAGames
{
    class EAGamesScanner
    {
    public:
        EAGamesScanner();
        std::vector<InstalledGameInfo> scanForInstalledGames();

    private:
#ifdef _WIN32
        void scanUninstallKey(HKEY rootKey, const WCHAR* keyPath, REGSAM accessFlags, std::vector<InstalledGameInfo>& games);
        bool isPublisherEA(HKEY hGameKey);
        std::optional<InstalledGameInfo> getGameInfoFromRegistry(HKEY hGameKey);
#endif
    };
}

#endif // ES_APP_GAME_STORE_EA_GAMES_SCANNER_H
#pragma once
#ifndef ES_APP_GAMESTORE_GOG_SCANNER_H
#define ES_APP_GAMESTORE_GOG_SCANNER_H

#include "GameStore/GOG/GogModels.h"
#include <vector>

class GogScanner
{
public:
    GogScanner();
    std::vector<GOG::InstalledGameInfo> findInstalledGames();
};

#endif // ES_APP_GAMESTORE_GOG_SCANNER_H
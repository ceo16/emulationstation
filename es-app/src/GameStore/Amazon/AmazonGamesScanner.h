#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_SCANNER_H
#define ES_APP_GAMESTORE_AMAZON_SCANNER_H

#include "GameStore/Amazon/AmazonGamesModels.h"
#include <vector>

class AmazonGamesScanner
{
public:
    AmazonGamesScanner();
    std::vector<Amazon::InstalledGameInfo> findInstalledGames();
};

#endif // ES_APP_GAMESTORE_AMAZON_SCANNER_H
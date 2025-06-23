#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_API_H
#define ES_APP_GAMESTORE_AMAZON_API_H

#include "GameStore/Amazon/AmazonGamesModels.h"
#include <functional>
#include <vector>

class AmazonAuth;

class AmazonGamesAPI
{
public:
    AmazonGamesAPI(AmazonAuth* auth);

    // Recupera tutti i giochi posseduti, gestendo la paginazione internamente
    void getOwnedGames(std::function<void(std::vector<Amazon::GameEntitlement> games, bool success)> on_complete);

private:
    AmazonAuth* mAuth;
};

#endif // ES_APP_GAMESTORE_AMAZON_API_H
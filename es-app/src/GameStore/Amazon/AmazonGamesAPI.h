#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_API_H
#define ES_APP_GAMESTORE_AMAZON_API_H

#include "GameStore/Amazon/AmazonGamesModels.h"
#include <functional>
#include <vector>

class AmazonAuth;
class Window; // Forward declaration

class AmazonGamesAPI
{
public:
    // Il costruttore ora accetta un puntatore a Window
    AmazonGamesAPI(Window* window, AmazonAuth* auth);

    void getOwnedGames(std::function<void(std::vector<Amazon::GameEntitlement> games, bool success)> on_complete);

private:
    Window* mWindow; // Memorizziamo qui il puntatore alla finestra
    AmazonAuth* mAuth;
	std::pair<long, std::vector<Amazon::GameEntitlement>> makeApiRequest(); 
};

#endif // ES_APP_GAMESTORE_AMAZON_API_H
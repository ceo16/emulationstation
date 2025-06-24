#pragma once
#ifndef ES_APP_GAMESTORE_GOG_API_H
#define ES_APP_GAMESTORE_GOG_API_H

#include "GameStore/GOG/GogModels.h"
#include <functional>
#include <vector>

class GogAuth;
class Window;

class GogGamesAPI
{
public:
    GogGamesAPI(Window* window, GogAuth* auth);

    // Recupera tutti i giochi posseduti, gestendo la paginazione
    void getOwnedGames(std::function<void(std::vector<GOG::LibraryGame> games, bool success)> on_complete);

private:
    Window* mWindow;
    GogAuth* mAuth;
};

#endif // ES_APP_GAMESTORE_GOG_API_H
#pragma once
#ifndef ES_APP_GAMESTORE_GOG_UI_H
#define ES_APP_GAMESTORE_GOG_UI_H

#include "guis/GuiSettings.h"

class GogGamesStore;

class GogUI : public GuiSettings
{
public:
    GogUI(Window* window);

private:
    void buildMenu();
    void syncGames();
    void performLogin();
    void performLogout();

    GogGamesStore* mStore;
};

#endif // ES_APP_GAMESTORE_GOG_UI_H
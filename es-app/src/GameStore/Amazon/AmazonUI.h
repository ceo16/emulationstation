#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_UI_H
#define ES_APP_GAMESTORE_AMAZON_UI_H

#include "guis/GuiSettings.h" // <-- Ereditiamo da GuiSettings

class AmazonGamesStore;

// La classe AmazonUI ora è un vero e proprio menu
class AmazonUI : public GuiSettings
{
public:
    AmazonUI(Window* window);

private:
    void buildMenu(); // Funzione per costruire/ricostruire le voci del menu
    void syncGames();
    void performLogin();
    void performLogout();

    AmazonGamesStore* mStore;
};

#endif // ES_APP_GAMESTORE_AMAZON_UI_H
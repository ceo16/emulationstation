#pragma once
#ifndef ES_APP_GAMESTORE_STEAM_UI_H
#define ES_APP_GAMESTORE_STEAM_UI_H

#include "Window.h" // Necessario per interagire con la finestra principale di ES
#include "guis/GuiSettings.h"

// Forward declaration per evitare dipendenze circolari o includere tutto SteamStore qui.
// SteamStore includerà SteamUI.h, e SteamUI ha solo bisogno di un puntatore a SteamStore.
class SteamStore;
class SteamAuth;
class GuiComponent; // <-- Aggiungi forward declaration per GuiComponent se non già presente

class SteamUI
{
public:
    SteamUI();
    void showSteamSettingsMenu(Window* window, SteamStore* store);

private:
    // Eventuali metodi helper privati per costruire parti specifiche della UI potrebbero andare qui,
    // ma per questo esempio, la maggior parte della logica sarà in showMainMenu.

    // Funzione helper per ricaricare la UI delle impostazioni dopo una modifica.
    // Prende il puntatore alla GuiSettings corrente (s_ptr) da chiudere,
    // la window e lo store per ricreare il menu.
static void reloadSettingsMenu(Window* window, SteamStore* store, GuiComponent* currentMenu);
};

#endif // ES_APP_GAMESTORE_STEAM_UI_H
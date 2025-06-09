#pragma once
#ifndef ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H
#define ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "Window.h"
#include "GameStore/GameStoreManager.h" // Assumendo il percorso corretto
#include "guis/GuiSettings.h" // Assicurati di includere GuiSettings

// La classe DEVE ereditare da GuiSettings per avere le funzioni che ci servono
class GuiNegoziOnlineMenu : public GuiSettings 
{
public:
	GuiNegoziOnlineMenu(Window* window);
	// Aggiungiamo la dichiarazione del distruttore
	~GuiNegoziOnlineMenu(); 
};

#endif // ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H
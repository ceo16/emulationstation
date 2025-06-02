#pragma once

#ifndef ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H
#define ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "Window.h"
#include "GameStore/GameStoreManager.h" // Assumendo il percorso corretto

class GuiNegoziOnlineMenu : public GuiComponent
{
public:
    GuiNegoziOnlineMenu(Window* window);
    ~GuiNegoziOnlineMenu();

    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    MenuComponent mMenu;
    void loadMenuEntries();
};

#endif // ES_APP_GUIS_GUI_NEGOZI_ONLINE_MENU_H
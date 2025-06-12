// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesUI.h
#pragma once
#ifndef ES_APP_GAME_STORE_EA_GAMES_UI_H
#define ES_APP_GAME_STORE_EA_GAMES_UI_H

#include "GuiComponent.h" // CORRETTO: Percorso da es-core
#include "components/MenuComponent.h"
#include "EAGamesStore.h"
#include "components/SwitchComponent.h"

class EAGamesUI : public GuiComponent // Potrebbe ereditare da GameStoreUI se questa fornisce una base utile
{
public:
    EAGamesUI(Window* window);
    // ~EAGamesUI(); // Aggiungere se si fa allocazione dinamica di membri che shared_ptr non gestisce
    ~EAGamesUI();
    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    // void update(int deltaTime) override; // Solo se necessario

private:
    void buildMenu(); // Sostituisce populateMenu per coerenza con altri StoreUI
    
    void openLoginDialog();
    // processLogin è ora gestito internamente da openLoginDialog + callback
    void onLoginFinished(bool success, const std::string& message);
    
    void processLogout();
	void addEAPlayEntries();
    
    void processImportGames(); // Per chiamare requestImportGames
    void onImportGamesFinished(bool success, const std::string& message);
	void processStartLoginFlow();

    // Componenti UI
    NinePatchComponent mBackground;
    ComponentGrid mGrid;
	std::shared_ptr<SwitchComponent> mEaPlaySwitch;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<MenuComponent> mMenu;
    std::shared_ptr<TextComponent> mVersionInfo; // Per mostrare info/errori o stato login

    EAGamesStore* mStore;
};

#endif // ES_APP_GAME_STORE_EA_GAMES_UI_H
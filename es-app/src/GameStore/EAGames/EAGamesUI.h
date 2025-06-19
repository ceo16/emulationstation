// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesUI.h
#pragma once
#ifndef ES_APP_GAME_STORE_EA_GAMES_UI_H
#define ES_APP_GAME_STORE_EA_GAMES_UI_H

#include "guis/GuiSettings.h"
#include "components/SwitchComponent.h"
#include "EAGamesStore.h"

class EAGamesUI : public GuiSettings
{
public:
    EAGamesUI(Window* window);

private:
    void initializeMenu();
    
    void onLoginFinished(bool success, const std::string& message);
    void processLogout();
    void processImportGames();
    void onImportGamesFinished(bool success, const std::string& message);
	void processStartLoginFlow();

    EAGamesStore* mStore;

    std::shared_ptr<SwitchComponent> mEaPlaySwitch;
    std::shared_ptr<TextComponent> mSubscriptionLabel;
};

#endif // ES_APP_GAME_STORE_EA_GAMES_UI_H
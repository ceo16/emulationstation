#pragma once
#ifndef ES_APP_GAMESTORE_XBOX_UI_H
#define ES_APP_GAMESTORE_XBOX_UI_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"

// Forward declarations
class Window;
class XboxStore;
class XboxAuth;

class XboxUI : public GuiComponent
{
public:
    XboxUI(Window* window, XboxStore* store);
    ~XboxUI() override = default;

    // Metodi GuiComponent standard
    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

    // Metodo pubblico per ricostruire il menu se chiamato dall'esterno (es. main.cpp)
    // Ma per ora, ci affideremo all'auto-aggiornamento o alla chiusura/riapertura della UI.
     void rebuildMenu(); 

private:
    void buildMenu(); // Costruisce/ricostruisce le voci del menu

    // Azioni del menu (SINCRONE per l'autenticazione)
    void optionLogin();
    void optionEnterAuthCodeSincrono(); // Nome esplicito
    void optionLogout();
    void optionRefreshGamesList();     // Questo può rimanere asincrono se già funziona

    XboxAuth* getAuth() const;

    Window* mWindow;
    XboxStore* mStore;
    MenuComponent mMenu;

    bool mLastAuthStatus; // Per l'auto-aggiornamento del menu in update()
};

#endif // ES_APP_GAMESTORE_XBOX_UI_H
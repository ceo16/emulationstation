#pragma once
#ifndef ES_APP_GUIS_GUIBUSYINFOPOPUP_H
#define ES_APP_GUIS_GUIBUSYINFOPOPUP_H

#include "GuiComponent.h" // Necessario per ereditare da GuiComponent
#include "Window.h"       // Necessario per passare il puntatore a Window

// Forward declaration per evitare include pesanti nell'header
class BusyComponent;
class NinePatchComponent;
class TextComponent;

class GuiBusyInfoPopup : public GuiComponent
{
public:
    GuiBusyInfoPopup(Window* window, const std::string& text);
    ~GuiBusyInfoPopup() override = default;

    // Impedisce l'input mentre è visibile
    bool input(InputConfig* config, Input input) override;

    // Update/Render come per gli altri componenti
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;

private:
    NinePatchComponent* mBackground; // Usiamo un puntatore semplice qu
    std::shared_ptr<BusyComponent> mBusyAnim;
    std::shared_ptr<TextComponent> mText; // Aggiungiamo il testo
};

#endif // ES_APP_GUIS_GUIBUSYINFOPOPUP_H
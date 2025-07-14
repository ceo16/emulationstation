#pragma once
#include "GuiComponent.h"
#include "components/TextComponent.h"
#include "components/MenuComponent.h"

class SpotifySearchView : public GuiComponent {
public:
    SpotifySearchView(Window* window, SystemData* system);
    void render(const Transform4x4f& parentTrans) override;
    bool input(InputConfig* config, Input input) override;

private:
    TextComponent  mPrompt;
    std::string    mQuery;
    MenuComponent  mResults;
};

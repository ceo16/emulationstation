#pragma once
#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include <functional>
#include <vector>

class SystemData;
class Window;

class SpotifyRootView : public GuiComponent {
public:
    SpotifyRootView(Window* window, SystemData* system);
    void render(const Transform4x4f& parentTrans) override;
    bool input(InputConfig* config, Input input) override;

private:
    MenuComponent mMenu;
    std::vector<std::function<void()>> mActions;
};

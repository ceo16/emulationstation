#pragma once
#ifndef ES_APP_GUIS_GUISPOTIFYBROWSER_H
#define ES_APP_GUIS_GUISPOTIFYBROWSER_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "SpotifyManager.h"
#include "HelpPrompt.h"
#include "InputConfig.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"

class SpotifyItemComponent : public GuiComponent
{
public:
    SpotifyItemComponent(Window* window, const std::string& text, const std::string& imageUrl);
    void render(const Transform4x4f& parentTrans) override;
    void onSizeChanged() override;

private:
    ImageComponent* mImage;
    TextComponent* mText;
    std::string mTextStr;
    std::string mImageUrl;
    bool mInitialized;
};

class GuiSpotifyBrowser : public GuiComponent
{
public:
    explicit GuiSpotifyBrowser(Window* window);

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void loadPlaylists();
    void loadTracks(std::string id);
    void centerMenu();

    // Aggiungiamo uno stato per la navigazione
    enum class SpotifyViewState { Playlists, Tracks };
    SpotifyViewState mState;

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUISPOTIFYBROWSER_H
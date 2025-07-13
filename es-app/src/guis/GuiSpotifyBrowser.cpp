#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "SpotifyManager.h"
#include "guis/GuiMsgBox.h"
#include "renderers/Renderer.h"
#include "InputConfig.h"

GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window)
  : GuiComponent(window),
    mMenu(window, _("SPOTIFY"))
{
    addChild(&mMenu);
    // facciamo sì che la Gui sappia la sua size
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    loadPlaylists();
}

void GuiSpotifyBrowser::centerMenu()
{
    float sw = Renderer::getScreenWidth();
    float sh = Renderer::getScreenHeight();
    auto sz = mMenu.getSize();
    mMenu.setPosition((sw - sz.x()) * 0.5f, (sh - sz.y()) * 0.5f);
}

void GuiSpotifyBrowser::loadPlaylists()
{
    mMenu.clear();
    mMenu.setTitle(_("LE TUE PLAYLIST"));
    // Indietro
	mMenu.addButton(_("BACK"), _("Torna indietro"), [&] { delete this; });

    // Loading placeholder
    mMenu.addEntry(_("Caricamento..."), false, nullptr);
    centerMenu();

    // Chiamata asincrona
    SpotifyManager::getInstance(mWindow)->getUserPlaylists(
      [this](const std::vector<SpotifyPlaylist>& playlists)
    {
        mMenu.clear();
        mMenu.setTitle(_("LE TUE PLAYLIST"));

        if (playlists.empty())
        {
            mMenu.addEntry(_("Nessuna playlist trovata."), false, nullptr);
        }
        else
        {
            for (auto& p : playlists)
            {
                const std::string id   = p.id;
                const std::string name = p.name;
                mMenu.addEntry(name, true, [this, id] {
                    loadTracks(id);
                });
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::loadTracks(std::string id)
{
    // 1) Prepara il menu con titolo e “torna indietro”
    mMenu.clear();
    mMenu.setTitle(_("TRACCE DELLA PLAYLIST"));
    mMenu.addEntry(_("Caricamento tracce..."), false, nullptr);
    centerMenu();

    // 2) Chiamata asincrona per ottenere le tracce
    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(
        id,
        [this, id](const std::vector<SpotifyTrack>& tracks)
        {
            // 3) Callback: ricostruisci il menu con i risultati
            mMenu.clear();
            mMenu.setTitle(_("TRACCE DELLA PLAYLIST"));


            if (tracks.empty())
            {
                mMenu.addEntry(_("Nessuna traccia trovata."), false, nullptr);
            }
            else
            {
                for (const auto& t : tracks)
                {
                    const std::string label = t.name + " — " + t.artist;
                    const std::string uri   = t.uri;
                    mMenu.addEntry(label, true, [uri] {
                        SpotifyManager::getInstance()->startPlayback(uri);
                    });
                }
            }

            // 4) Ricentra il menu ora che è popolato
            centerMenu();
        }
    );
}

bool GuiSpotifyBrowser::input(InputConfig* config, Input input)
{
    // Prima lascia che la GuiComponent gestisca frecce e A
    if (GuiComponent::input(config, input))
        return true;

    // Se premi B (BACK) o Start, chiudi questa GUI
    if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input))
        && input.value != 0)
    {
        delete this;
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiSpotifyBrowser::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    // frecce su/giu per navigare
    prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
    // A per selezionare
    prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
    // Start per uscire
    prompts.push_back(HelpPrompt("start", _("CLOSE"), [&] { delete this; }));

    return prompts;
}




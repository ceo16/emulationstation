#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"
#include "SpotifyManager.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"

GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window) : GuiComponent(window), mMenu(window, "SPOTIFY")
{
    addChild(&mMenu);
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    loadPlaylists();
}

void GuiSpotifyBrowser::loadPlaylists()
{
    mMenu.clear();
    mMenu.setTitle("LE TUE PLAYLIST");
    mMenu.addEntry("Caricamento in corso...", false, nullptr);

    // Chiama la funzione asincrona e le passa il codice da eseguire al completamento
    SpotifyManager::getInstance(mWindow)->getUserPlaylists(
        [this](const std::vector<SpotifyPlaylist>& playlists) {
            // Questo codice viene eseguito sulla UI quando le playlist sono pronte
            mMenu.clear();
            mMenu.setTitle("LE TUE PLAYLIST");
            mMenu.addEntry(".. TORNA AL MENU AUDIO", false, [this] { delete this; });

            if (playlists.empty()) {
                mMenu.addEntry("Nessuna playlist trovata.", false, nullptr);
                return;
            }

            for (const auto& p : playlists) {
                mMenu.addEntry(p.name, true, [this, id = p.id] {
                    loadTracks(id);
                });
            }
        }
    );
}

void GuiSpotifyBrowser::loadTracks(std::string id)
{
    mMenu.clear();
    mMenu.setTitle("TRACCE DELLA PLAYLIST");
    mMenu.addEntry("Caricamento tracce...", false, nullptr);

    // Chiama la funzione asincrona e le passa il codice da eseguire al completamento
    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(id,
        [this](const std::vector<SpotifyTrack>& tracks) {
            // Questo codice viene eseguito sulla UI quando le tracce sono pronte
            mMenu.clear();
            mMenu.setTitle("TRACCE DELLA PLAYLIST");
            mMenu.addEntry(".. TORNA ALLE PLAYLIST", true, [this] { loadPlaylists(); });

            if (tracks.empty()) {
                mMenu.addEntry("Nessuna traccia trovata.", false, nullptr);
                return;
            }

            for (const auto& t : tracks) {
                mMenu.addEntry(t.name + " - " + t.artist, true, [uri = t.uri] {
                    // startPlayback è già asincrono, quindi la chiamata è sicura
                    SpotifyManager::getInstance()->startPlayback(uri);
                });
            }
        }
    );
}
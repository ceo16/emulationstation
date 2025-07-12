#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"
#include "HttpReq.h" // Già presente, ma per completezza
#include "json.hpp" // Già presente, ma per completezza
#include "SpotifyManager.h" // Già presente, ma per completezza
#include "guis/GuiMsgBox.h" // Aggiunta qui e in .h

GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window) : GuiComponent(window), mMenu(window, "SPOTIFY")
{
    addChild(&mMenu);
    loadPlaylists(); // Carica le playlist all'avvio della GUI
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
}

void GuiSpotifyBrowser::loadPlaylists()
{
    mMenu.clear();
    mMenu.setTitle("LE TUE PLAYLIST");

    // L'entry per tornare al menu audio
    mMenu.addEntry(".. TORNA AL MENU AUDIO", false, [this] { delete this; });

    auto playlists = SpotifyManager::getInstance()->getUserPlaylists();

    if (playlists.empty()) {
        mMenu.addEntry("Nessuna playlist trovata o errore API", false, nullptr);
        LOG(LogWarning) << "GuiSpotifyBrowser: Nessuna playlist caricata. Controllare i log di SpotifyManager per dettagli.";
        return;
    }

for (const auto& p : playlists) {
    const std::string idCopy = p.id;
    const std::string nameCopy = p.name;
    mMenu.addEntry(nameCopy, true, [this, idCopy] {
        LOG(LogInfo) << "Playlist selezionata: idCopy = '" << idCopy << "'";
        if (idCopy.empty()) {
            mWindow->pushGui(new GuiMsgBox(mWindow, _("Errore"), _("ID playlist non valido.")));
            return;
        }
        loadTracks(idCopy);
    });
}
}

void GuiSpotifyBrowser::loadTracks(std::string id)
{
    LOG(LogInfo) << "GuiSpotifyBrowser::loadTracks chiamato con ID: '" << id << "'";
    mMenu.clear();
    mMenu.setTitle("TRACCE DELLA PLAYLIST");
    mMenu.addEntry(".. TORNA ALLE PLAYLIST", true, [this] {
        loadPlaylists();
    });

    if (id.empty()) {
        LOG(LogError) << "ID playlist in loadTracks è vuoto! Non dovrebbe accadere.";
        mMenu.addEntry("Errore: ID playlist vuoto", false, nullptr);
        return;
    }

    LOG(LogInfo) << "GuiSpotifyBrowser: Richiesta tracce per playlist ID: " << id;

    auto tracks = SpotifyManager::getInstance()->getPlaylistTracks(id);

    if (tracks.empty()) {
        mMenu.addEntry("Nessuna traccia trovata o errore API", false, nullptr);
        LOG(LogWarning) << "GuiSpotifyBrowser: Nessuna traccia caricata per playlist ID: " << id << ". Controllare log di SpotifyManager.";
        return;
    }

    for (const auto& t : tracks) {
        mMenu.addEntry(t.name + " - " + t.artist, true, [t] {
            SpotifyManager::getInstance()->startPlayback(t.uri);
        });
    }
}

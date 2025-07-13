#include "MusicStartupHelper.h"
#include "AudioManager.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "SpotifyManager.h"
#include "Log.h"
#include "Window.h"

void startBackgroundMusicBasedOnSetting(Window* window)
{
    auto& settings = *Settings::getInstance();
    if (!settings.getBool("audio.bgmusic"))
        return;

    const auto source = settings.getString("audio.musicsource");
    bool useSpotify = (source == "spotify");
    auto* spotify = SpotifyManager::getInstance(window);

    // se Spotify è selezionato ed è già autenticato
    if (useSpotify && spotify->isAuthenticated())
    {
        LOG(LogInfo) << "Avvio Spotify come musica di sottofondo.";
        if (AudioManager::isInitialized())
            AudioManager::getInstance()->stopMusic();
        spotify->startPlayback();
        return;
    }

    // fallback: musica locale (o Spotify non autenticato)
    if (useSpotify)
        LOG(LogWarning) << "Spotify selezionato ma non autenticato. Avvio musica locale.";

    auto state = ViewController::get()->getState().viewing;
    if (state == ViewController::GAME_LIST || state == ViewController::SYSTEM_SELECT)
    {
        auto theme = ViewController::get()->getState().getSystem()->getTheme();
        AudioManager::getInstance()->changePlaylist(theme);
    }
    else
    {
        AudioManager::getInstance()->playRandomMusic();
    }
}

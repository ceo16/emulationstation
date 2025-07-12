#include "MusicStartupHelper.h"
#include "AudioManager.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "SystemData.h"
#include "SpotifyManager.h"
#include "Log.h"

void startBackgroundMusicBasedOnSetting()
{
	if (!Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	if (Settings::getInstance()->getString("audio.musicsource") == "spotify") {
		if (SpotifyManager::getInstance()->isAuthenticated()) {
			LOG(LogInfo) << "Avvio Spotify come musica di sottofondo.";
            // ferma qualunque musica locale sia partita
            if (AudioManager::isInitialized())
                AudioManager::getInstance()->stopMusic();
            SpotifyManager::getInstance()->startPlayback();
		} else {
			LOG(LogWarning) << "Spotify selezionato ma non autenticato. Avvio musica locale.";
			if (ViewController::get()->getState().viewing == ViewController::GAME_LIST ||
				ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT) {
				AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme());
			} else {
				AudioManager::getInstance()->playRandomMusic();
			}
		}
	} else {
		    if (Settings::getInstance()->getString("audio.musicsource") != "spotify")
    {
        if (ViewController::get()->getState().viewing == ViewController::GAME_LIST ||
            ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
            AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme());
        else
            AudioManager::getInstance()->playRandomMusic();
    }
 }
}

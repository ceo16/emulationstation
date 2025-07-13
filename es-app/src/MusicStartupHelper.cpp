#include "MusicStartupHelper.h"
#include "AudioManager.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "SystemData.h"
#include "SpotifyManager.h"
#include "Log.h"
#include "Window.h"

void startBackgroundMusicBasedOnSetting(Window* window)
{
	if (!Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	if (Settings::getInstance()->getString("audio.musicsource") == "spotify") {
        // Passa 'window' a getInstance
		if (SpotifyManager::getInstance(window)->isAuthenticated()) {
			LOG(LogInfo) << "Avvio Spotify come musica di sottofondo.";
            // ferma qualunque musica locale sia partita
            if (AudioManager::isInitialized())
                AudioManager::getInstance()->stopMusic();
            
            // Passa 'window' a getInstance
            SpotifyManager::getInstance(window)->startPlayback();
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
        // Questa parte è per la musica locale e non necessita di SpotifyManager
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

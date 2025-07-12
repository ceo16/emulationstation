#pragma once
#ifndef ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
#define ES_CORE_MANAGERS_SPOTIFY_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include "Window.h"

// Struttura per una traccia Spotify
struct SpotifyTrack {
    std::string name;
    std::string artist;
    std::string uri; // URI di Spotify (es. "spotify:track:...")
};

// Struttura per una playlist Spotify
struct SpotifyPlaylist {
    std::string name;
    std::string id; // ID della playlist Spotify
    std::string image_url; // URL dell'immagine di copertina
};

class SpotifyManager
{
public:
    static SpotifyManager* getInstance();

    // Funzioni di autenticazione
    void exchangeCodeForTokens(Window* window, const std::string& code);
    void logout();
    bool isAuthenticated() const;
    std::string getAccessToken() const;
    bool refreshTokens(); // Resa pubblica per essere chiamata se il token scade

    // Funzioni di riproduzione
    void startPlayback(const std::string& track_uri = "");
    void pausePlayback();
    void resumePlayback(); // Funzione per riprendere la riproduzione (implementata nel .cpp)

    // Funzioni per ottenere dati da Spotify
    std::vector<SpotifyPlaylist> getUserPlaylists();
    std::vector<SpotifyTrack> getPlaylistTracks(const std::string& playlist_id); // Resa pubblica per GuiSpotifyBrowser
    SpotifyTrack getCurrentlyPlaying();
	

private:
    SpotifyManager();
    ~SpotifyManager();
    SpotifyManager(const SpotifyManager&) = delete;
    SpotifyManager& operator=(const SpotifyManager&) = delete;

    // Funzioni ausiliarie private
    std::string getActiveComputerDeviceId(); // Ottiene l'ID del dispositivo attivo (computer)

    void loadTokens();  // Carica i token dal file
    void saveTokens();  // Salva i token nel file
    void clearTokens(); // Cancella i token (logout)

    // Membri privati per i token e il percorso del file
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mTokensPath;

    static SpotifyManager* sInstance; // Singleton instance
};

#endif // ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
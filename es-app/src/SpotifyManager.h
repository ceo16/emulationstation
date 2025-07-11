#pragma once
#ifndef ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
#define ES_CORE_MANAGERS_SPOTIFY_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include "Window.h"

struct SpotifyTrack {
    std::string name;
    std::string artist;
    std::string uri;
};

struct SpotifyPlaylist {
    std::string name;
    std::string id;
    std::string image_url;
};

class SpotifyManager
{
public:
    static SpotifyManager* getInstance();

    void exchangeCodeForTokens(Window* window, const std::string& code);
    void logout();
    bool isAuthenticated() const;
    std::string getAccessToken() const;
    
    void startPlayback(const std::string& track_uri = "");
    void pausePlayback();
    void resumePlayback(); // <-- Assicurati che questa ci sia

    std::vector<SpotifyPlaylist> getUserPlaylists();
    SpotifyTrack getCurrentlyPlaying();
    bool refreshTokens(); // <-- Resa pubblica

private:
    SpotifyManager();
    ~SpotifyManager();
    SpotifyManager(const SpotifyManager&) = delete;
    SpotifyManager& operator=(const SpotifyManager&) = delete;
    std::string getActiveComputerDeviceId();

    void loadTokens();
    void saveTokens();
    void clearTokens();
    
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mTokensPath;
    
    static SpotifyManager* sInstance;
};

#endif // ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
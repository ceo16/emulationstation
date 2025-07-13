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
    static SpotifyManager* getInstance(Window* window = nullptr);

    // Funzioni aggiunte di nuovo perché usate da altre parti del codice
    void exchangeCodeForTokens(Window* window, const std::string& code);
    std::string getAccessToken() const;

    void logout();
    bool isAuthenticated() const;
    
    void startPlayback(const std::string& track_uri = "");
    void pausePlayback();
    void resumePlayback();

    void getUserPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback);
    void getPlaylistTracks(const std::string& playlist_id, const std::function<void(const std::vector<SpotifyTrack>&)>& callback);
	SpotifyTrack getCurrentlyPlaying();

private:
    SpotifyManager(Window* window);
    ~SpotifyManager();
    SpotifyManager(const SpotifyManager&) = delete;
    SpotifyManager& operator=(const SpotifyManager&) = delete;

    bool refreshTokens();
    std::string getActiveComputerDeviceId();


    void loadTokens();
    void saveTokens();
    void clearTokens();

    Window* mWindow;
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mTokensPath;
	
	static constexpr char CLIENT_ID[]    = "b0532e0f304c4fb68cb5ed528fd46b37";
    static constexpr char CLIENT_SECRET[] = "e619644b3d854b1b8056a33b73e02299";

    static SpotifyManager* sInstance;
};

#endif // ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
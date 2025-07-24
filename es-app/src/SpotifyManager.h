#pragma once
#ifndef ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
#define ES_CORE_MANAGERS_SPOTIFY_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include "Window.h"
#include "json.hpp" 
#include <thread>
#include <atomic>
#include <mutex>

struct SpotifyTrack {
    std::string name;
    std::string artist;
    std::string uri;
    std::string image_url;
};

struct SpotifyPlaylist {
    std::string name;
    std::string id;
    std::string image_url;
};

// Aggiunta la struttura per gli Album
struct SpotifyAlbum {
    std::string name;
    std::string id;
    std::string image_url;
};

struct SpotifyCategory {
    std::string name;
    std::string id;
    std::string image_url;
};

struct SpotifyDevice {
    std::string name;
    std::string id;
    std::string type;
    bool is_active;
};


class SpotifyManager
{
public:
    static SpotifyManager* getInstance(Window* window = nullptr);

    void exchangeCodeForTokens(Window* window, const std::string& code);
    std::string getAccessToken() const;
    void logout();
    bool isAuthenticated() const;
    
    void startPlayback(const std::string& track_uri = "");
    void startPlaybackContextual(const std::string& context_uri, const std::string& track_uri_to_start);

    void pausePlayback();
    void resumePlayback();

    void getUserPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback);
    // Modificata per accettare l'ID per valore e usare il market di default
    void getPlaylistTracks(std::string playlistId, const std::function<void(const std::vector<SpotifyTrack>&)>& callback, const std::string& market = "");
    void getUserLikedSongs(const std::function<void(const std::vector<SpotifyTrack>&)>& callback);
    void search(const std::string& query, const std::string& types, const std::function<void(const nlohmann::json&)>& callback, const std::string& market = "");
    void getArtistTopTracks(const std::string& artistId, const std::function<void(const std::vector<SpotifyTrack>&)>& callback, const std::string& market = "");
    // Nuova funzione per ottenere le tracce di un album
    void getAlbumTracks(const std::string& albumId, const std::function<void(const std::vector<SpotifyTrack>&)>& callback, const std::string& market = "");
	void getFeaturedPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&, const std::string&)>& callback, const std::string& market = "");
	 void getCategories(const std::function<void(const std::vector<SpotifyCategory>&)>& callback, const std::string& market = "");
    void getCategoryPlaylists(const std::string& categoryId, const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback, const std::string& market = "");
	void getAvailableDevices(const std::function<void(const std::vector<SpotifyDevice>&)>& callback);
    void transferPlayback(const std::string& deviceId);




	SpotifyTrack getCurrentlyPlaying();
	static constexpr char CLIENT_ID[]    = "b0532e0f304c4fb68cb5ed528fd46b37";
    static constexpr char CLIENT_SECRET[] = "e619644b3d854b1b8056a33b73e02299";

private:
    SpotifyManager(Window* window);
    ~SpotifyManager();
    SpotifyManager(const SpotifyManager&) = delete;
    SpotifyManager& operator=(const SpotifyManager&) = delete;

    bool refreshTokens();
    std::string getActiveComputerDeviceId();
    
    // Funzione helper per ottenere il mercato di default dalla lingua del sistema
    std::string getDefaultMarket();

    void loadTokens();
    void saveTokens();
    void clearTokens();

    Window* mWindow;
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mTokensPath;
	std::thread mPollingThread;
    std::atomic<bool> mStopPolling;
    std::string mLastTrackUri;
    std::mutex mMutex;

    // --- AGGIUNGI LA DICHIARAZIONE DI QUESTA FUNZIONE ---
    void pollingLoop(); 
	
    static SpotifyManager* sInstance;
};

#endif // ES_CORE_MANAGERS_SPOTIFY_MANAGER_H
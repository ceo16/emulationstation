#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"

// <<< AGGIUNTI GLI INCLUDE FONDAMENTALI >>>
#include "HttpReq.h"
#include "json.hpp"
#include "SpotifyManager.h"
// <<< FINE INCLUDE >>>

GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window) : GuiComponent(window), mMenu(window, "SPOTIFY")
{
    addChild(&mMenu);
    loadPlaylists();
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
}

void GuiSpotifyBrowser::loadPlaylists()
{
    mMenu.clear();
    mMenu.setTitle("LE TUE PLAYLIST");
    
    // <<< CORRETTO 'close()' con 'delete this' che è il modo giusto per chiudere una GUI >>>
    mMenu.addEntry(".. TORNA AL MENU AUDIO", false, [this] { delete this; });

    auto playlists = SpotifyManager::getInstance()->getUserPlaylists();

    if (playlists.empty()) {
        mMenu.addEntry("Nessuna playlist trovata o errore API", false, nullptr);
        return;
    }

    for (const auto& p : playlists) {
        mMenu.addEntry(p.name, true, [this, p] {
            loadTracks(p.id);
        });
    }
}

void GuiSpotifyBrowser::loadTracks(const std::string& playlist_id)
{
    mMenu.clear();
    mMenu.setTitle("TRACCE");
    mMenu.addEntry(".. TORNA ALLE PLAYLIST", true, [this] {
        loadPlaylists();
    });

    auto tracks = getPlaylistTracks(playlist_id);

    if (tracks.empty()) {
        mMenu.addEntry("Nessuna traccia trovata", false, nullptr);
        return;
    }

    for (const auto& t : tracks) {
        mMenu.addEntry(t.name + " - " + t.artist, true, [t] {
            SpotifyManager::getInstance()->startPlayback(t.uri);
        });
    }
}

std::vector<SpotifyTrack> GuiSpotifyBrowser::getPlaylistTracks(const std::string& playlist_id)
{
    if (playlist_id.empty()) {
        LOG(LogError) << "ERRORE CRITICO: L'ID della playlist è vuoto!";
        return {};
    }

    if (!SpotifyManager::getInstance()->isAuthenticated()) return {};
    
    std::vector<SpotifyTrack> tracks;
    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + SpotifyManager::getInstance()->getAccessToken());
    
    std::string url = "https://api.spotify.com/v1/playlists/" + playlist_id + "/tracks";
    
    HttpReq request(url, &options);
    request.wait();
    
    if (request.status() == 401)
    {
        if (SpotifyManager::getInstance()->refreshTokens())
        {
            HttpReqOptions newOptions;
            newOptions.customHeaders.push_back("Authorization: Bearer " + SpotifyManager::getInstance()->getAccessToken());
            
            HttpReq request_retry(url, &newOptions);
            request_retry.wait();

            if (request_retry.status() == 200) {
                try {
                    auto json = nlohmann::json::parse(request_retry.getContent());
                    if (json.contains("items")) {
                        for (const auto& item : json["items"]) {
                            if (item.contains("track") && !item["track"].is_null()) {
                                SpotifyTrack t;
                                t.name = item["track"].value("name", "N/A");
                                t.uri = item["track"].value("uri", "");
                                if (!item["track"]["artists"].empty()) { t.artist = item["track"]["artists"][0].value("name", "N/A"); }
                                tracks.push_back(t);
                            }
                        }
                    }
                } catch (const std::exception& e) { LOG(LogError) << "getPlaylistTracks (retry) JSON parse error: " << e.what(); }
            }
            return tracks;
        }
    }

    if (request.status() == 200) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("items")) {
                for (const auto& item : json["items"]) {
                    if (item.contains("track") && !item["track"].is_null()) {
                        SpotifyTrack t;
                        t.name = item["track"].value("name", "N/A");
                        t.uri = item["track"].value("uri", "");
                        if (!item["track"]["artists"].empty()) { t.artist = item["track"]["artists"][0].value("name", "N/A"); }
                        tracks.push_back(t);
                    }
                }
            }
        } catch (const std::exception& e) { LOG(LogError) << "getPlaylistTracks JSON parse error: " << e.what(); }
    }
    return tracks;
}
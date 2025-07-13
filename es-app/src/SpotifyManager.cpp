#include "SpotifyManager.h"
#include "SystemConf.h"
#include "HttpReq.h"
#include "Log.h"
#include "Paths.h"
#include "Settings.h"
#include "Window.h"
#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"
#include "views/ViewController.h"
#include <thread>
#include <json.hpp>
#include <fstream>
#include <chrono>

// --- Utility per la codifica Base64 ---
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while ((i++ < 3)) ret += '=';
    }
    return ret;
}

// Implementazione Singleton
SpotifyManager* SpotifyManager::sInstance = nullptr;
SpotifyManager* SpotifyManager::getInstance(Window* window) {
    if (sInstance == nullptr) {
        if (window == nullptr)
            throw std::runtime_error("SpotifyManager::getInstance(): window non fornita al primo avvio");
        sInstance = new SpotifyManager(window);
    } else if (window != nullptr) {
        sInstance->mWindow = window;
    }
    return sInstance;
}

SpotifyManager::SpotifyManager(Window* window) : mWindow(window) {
    std::string userPath = Paths::getUserEmulationStationPath();
    Utils::FileSystem::createDirectory(userPath + "/spotify");
    mTokensPath = userPath + "/spotify/spotify_tokens.json";
    loadTokens();
}

SpotifyManager::~SpotifyManager() {}

// --- FUNZIONI PUBBLICHE ---

void SpotifyManager::exchangeCodeForTokens(Window* window, const std::string& code) {
    // Questa funzione crea una GUI, quindi non ha bisogno di un thread separato
    // perché GuiLoading lo gestisce già internamente.
    auto SincroAuth = new GuiLoading<int>(window, "AUTHENTICATING WITH SPOTIFY...",
        [this, code](IGuiLoadingHandler* loadingInterface) -> int {
            auto loadingGui = (GuiLoading<int>*)loadingInterface;
            const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
            const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");
            const std::string SPOTIFY_REDIRECT_URI = "es-spotify://callback";

            HttpReqOptions options;
            options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
            std::string auth_str = clientId + ":" + clientSecret;
            std::string auth_b64 = base64_encode(reinterpret_cast<const unsigned char*>(auth_str.c_str()), auth_str.length());
            options.customHeaders.push_back("Authorization: Basic " + auth_b64);
            options.dataToPost = "grant_type=authorization_code&code=" + code + "&redirect_uri=" + HttpReq::urlEncode(SPOTIFY_REDIRECT_URI);
            
            HttpReq request("https://accounts.spotify.com/api/token", &options);
            request.wait();

            if (request.status() == HttpReq::Status::REQ_SUCCESS) {
                try {
                    auto responseJson = nlohmann::json::parse(request.getContent());
                    if (responseJson.contains("access_token") && responseJson.contains("refresh_token")) {
                        mAccessToken = responseJson.value("access_token", "");
                        mRefreshToken = responseJson.value("refresh_token", "");
                        saveTokens();
                        loadingGui->setText("LOGIN SUCCESSFUL!");
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    } else {
                        std::string error_desc = responseJson.value("error_description", "Unknown JSON error.");
                        loadingGui->setText("LOGIN FAILED:\n" + error_desc);
                        std::this_thread::sleep_for(std::chrono::seconds(4));
                    }
                } catch (const std::exception& e) {
                    loadingGui->setText("LOGIN FAILED:\n" + std::string(e.what()));
                    std::this_thread::sleep_for(std::chrono::seconds(4));
                }
            } else {
                loadingGui->setText("LOGIN FAILED:\n" + request.getErrorMsg());
                std::this_thread::sleep_for(std::chrono::seconds(4));
            }
            return 0;
        });
    window->pushGui(SincroAuth);
}

std::string SpotifyManager::getAccessToken() const { return mAccessToken; }
void SpotifyManager::logout() { clearTokens(); }
bool SpotifyManager::isAuthenticated() const { return !mAccessToken.empty(); }
void SpotifyManager::resumePlayback() { startPlayback(""); }

void SpotifyManager::startPlayback(const std::string& track_uri) {
    std::thread([this, track_uri]() {
        try {
            if (!isAuthenticated()) return;
            std::string deviceId = getActiveComputerDeviceId();
            if (deviceId.empty()) return;

            HttpReqOptions playOpts;
            playOpts.verb = "PUT";
            playOpts.customHeaders = {"Authorization: Bearer " + getAccessToken(), "Content-Type: application/json"};
            std::string bodyStr;
            if (!track_uri.empty()) { nlohmann::json body; body["uris"] = {track_uri}; bodyStr = body.dump(); }
            playOpts.dataToPost = bodyStr;
            playOpts.customHeaders.push_back("Content-Length: " + std::to_string(bodyStr.length()));
            std::string playUrl = "https://api.spotify.com/v1/me/player/play";
            if (!deviceId.empty()) playUrl += "?device_id=" + deviceId;
            
            HttpReq playReq(playUrl, &playOpts);
            playReq.wait();

            if (playReq.status() == 403) {
                if (mWindow) mWindow->postToUiThread([w = mWindow] { w->displayNotificationMessage("Comando Player fallito: Richiesto Account Premium"); });
                return;
            }
            if (playReq.status() != HttpReq::Status::REQ_SUCCESS && playReq.status() != 204) return;
            
            // --- MODIFICA CHIAVE ---
            // Aggiungiamo una pausa per dare tempo a Spotify di aggiornare il suo stato.
            std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // Pausa di 1.5 secondi

            if (!Settings::getInstance()->getBool("audio.display_titles")) return;
            auto info = getCurrentlyPlaying();
            if (info.name.empty()) return;

            int durationMs = Settings::getInstance()->getInt("audio.display_titles_time") * 1000;
            std::string msg = info.name + " — " + info.artist;
            if (mWindow) mWindow->postToUiThread([w = mWindow, msg, durationMs] { w->displayNotificationMessage(msg, durationMs); });
        } catch (const std::exception& e) { LOG(LogError) << "[Thread Playback] Eccezione: " << e.what(); }
    }).detach();
}

void SpotifyManager::pausePlayback() {
    std::thread([this]() {
        try {
            if (!isAuthenticated()) return;
            std::string deviceId = getActiveComputerDeviceId();
            if (deviceId.empty()) return;

            HttpReqOptions options;
            options.verb = "PUT";
            options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            options.customHeaders.push_back("Content-Length: 0");
            std::string url = "https://api.spotify.com/v1/me/player/pause";
            if (!deviceId.empty()) url += "?device_id=" + deviceId;
            
            HttpReq request(url, &options);
            request.wait();

            // ! GESTIONE MIGLIORATA DELL'ERRORE 403 !
            if (request.status() == 403) {
                if (mWindow) mWindow->postToUiThread([w = mWindow] { w->displayNotificationMessage("Comando Player fallito: Richiesto Account Premium"); });
            }
        } catch (const std::exception& e) { LOG(LogError) << "[Thread Pausa] Eccezione: " << e.what(); }
    }).detach();
}

void SpotifyManager::getUserPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback) {
    std::thread([this, callback]() {
        try {
            std::vector<SpotifyPlaylist> playlists;
            if (isAuthenticated()) {
                HttpReqOptions options; options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                std::string url = "https://api.spotify.com/v1/me/playlists";
                HttpReq request(url, &options); request.wait();
                if (request.status() == 401 && this->refreshTokens()) {
                    options.customHeaders[0] = "Authorization: Bearer " + getAccessToken();
                    HttpReq retryReq(url, &options); retryReq.wait();
                    if (retryReq.status() == HttpReq::Status::REQ_SUCCESS) {
                        auto json = nlohmann::json::parse(retryReq.getContent());
                        if (json.contains("items")) for (const auto& item : json["items"]) playlists.push_back({item.value("name", "N/A"), item.value("id", ""), (item.contains("images") && !item["images"].empty() ? item["images"][0].value("url", "") : "")});
                    }
                } else if (request.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto json = nlohmann::json::parse(request.getContent());
                    if (json.contains("items")) for (const auto& item : json["items"]) playlists.push_back({item.value("name", "N/A"), item.value("id", ""), (item.contains("images") && !item["images"].empty() ? item["images"][0].value("url", "") : "")});
                }
            }
            mWindow->postToUiThread([callback, playlists]() { callback(playlists); });
        } catch (const std::exception& e) {
            LOG(LogError) << "[Thread getUserPlaylists] Eccezione: " << e.what();
            mWindow->postToUiThread([callback]() { callback({}); });
        }
    }).detach();
}

void SpotifyManager::getPlaylistTracks(const std::string& playlist_id, const std::function<void(const std::vector<SpotifyTrack>&)>& callback) {
    std::thread([this, playlist_id, callback]() {
        try {
            std::vector<SpotifyTrack> tracks;
            if (isAuthenticated() && !playlist_id.empty()) {
                HttpReqOptions options; options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                std::string url = "https://api.spotify.com/v1/playlists/" + playlist_id + "/tracks";
                do {
                    HttpReq request(url, &options); request.wait();
                    if (request.status() == 401 && this->refreshTokens()) {
                        options.customHeaders[0] = "Authorization: Bearer " + getAccessToken();
                        continue;
                    }
                    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
                        auto json = nlohmann::json::parse(request.getContent());
                        if (json.contains("items")) for (const auto& item : json["items"]) if (item.contains("track") && !item["track"].is_null()) tracks.push_back({item["track"].value("name", "N/A"), (item["track"].contains("artists") && !item["track"]["artists"].empty() ? item["track"]["artists"][0].value("name", "N/A") : "N/A"), item["track"].value("uri", "")});
                        url = json.contains("next") && !json["next"].is_null() ? json["next"].get<std::string>() : "";
                    } else { url.clear(); }
                } while (!url.empty());
            }
            mWindow->postToUiThread([callback, tracks]() { callback(tracks); });
        } catch (const std::exception& e) {
            LOG(LogError) << "[Thread getPlaylistTracks] Eccezione: " << e.what();
            mWindow->postToUiThread([callback]() { callback({}); });
        }
    }).detach();
}

// --- FUNZIONI PRIVATE ---
// Queste sono le implementazioni bloccanti, chiamate solo dai thread.

bool SpotifyManager::refreshTokens() {
    if (mRefreshToken.empty()) return false;
    const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
    const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");
    if (clientId.empty() || clientSecret.empty()) return false;
    HttpReqOptions options;
    std::string auth_str = clientId + ":" + clientSecret;
    std::string auth_b64 = base64_encode(reinterpret_cast<const unsigned char*>(auth_str.c_str()), auth_str.length());
    options.customHeaders.push_back("Authorization: Basic " + auth_b64);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.dataToPost = "grant_type=refresh_token&refresh_token=" + mRefreshToken;
    HttpReq request("https://accounts.spotify.com/api/token", &options);
    request.wait();
    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        auto json = nlohmann::json::parse(request.getContent());
        if (json.contains("access_token")) {
            mAccessToken = json.value("access_token", "");
            if (json.contains("refresh_token")) mRefreshToken = json.value("refresh_token", "");
            saveTokens();
            return true;
        }
    }
    logout();
    return false;
}

std::string SpotifyManager::getActiveComputerDeviceId() {
    if (!isAuthenticated()) return "";
    HttpReqOptions options; options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    HttpReq request("https://api.spotify.com/v1/me/player/devices", &options);
    request.wait();
    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        auto json = nlohmann::json::parse(request.getContent());
        if (json.contains("devices")) {
            for (const auto& device : json["devices"]) {
                if (device.value("type", "") == "Computer" && device.value("is_active", false)) {
                    return device.value("id", "");
                }
            }
            for (const auto& device : json["devices"]) {
                if (device.value("type", "") == "Computer") {
                    return device.value("id", "");
                }
            }
        }
    } else if (request.status() == 401 && refreshTokens()) {
        return getActiveComputerDeviceId();
    }
    return "";
}

SpotifyTrack SpotifyManager::getCurrentlyPlaying() {
    if (!isAuthenticated()) return {};

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";

    HttpReq request(url, &options);
    request.wait();

    auto processJson = [](const std::string& content) -> SpotifyTrack {
        auto json = nlohmann::json::parse(content);
        if (json.contains("item") && !json["item"].is_null()) {
            return {
                json["item"].value("name", "N/A"),
                (json["item"].contains("artists") && !json["item"]["artists"].empty() ? json["item"]["artists"][0].value("name", "N/A") : "N/A"),
                json["item"].value("uri", "")
            };
        }
        return {};
    };

    if (request.status() == 401 && refreshTokens()) {
        options.customHeaders[0] = "Authorization: Bearer " + getAccessToken();
        // ! FIX !: Crea un nuovo oggetto HttpReq invece di riassegnare.
        HttpReq retryReq(url, &options);
        retryReq.wait();
        if (retryReq.status() == 200) {
            return processJson(retryReq.getContent());
        }
    }
    else if (request.status() == 200) {
        return processJson(request.getContent());
    }

    return {};
}

void SpotifyManager::loadTokens() {
    if (!Utils::FileSystem::exists(mTokensPath)) return;
    std::ifstream file(mTokensPath);
    if(file.good()){
        try {
            nlohmann::json j; file >> j;
            mAccessToken = j.value("access_token", "");
            mRefreshToken = j.value("refresh_token", "");
        } catch(...) {}
    }
}

void SpotifyManager::saveTokens() {
    nlohmann::json j;
    j["access_token"] = mAccessToken;
    j["refresh_token"] = mRefreshToken;
    std::ofstream file(mTokensPath);
    file << j.dump(4);
}

void SpotifyManager::clearTokens() {
    mAccessToken = "";
    mRefreshToken = "";
    if (Utils::FileSystem::exists(mTokensPath))
        Utils::FileSystem::removeFile(mTokensPath);
}
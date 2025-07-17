#include "SpotifyManager.h"
#include "HttpReq.h"
#include "Log.h"
#include "Paths.h"
#include "Settings.h"
#include "Window.h"
#include "guis/GuiLoading.h"
#include "utils/FileSystemUtil.h"
#include <thread>
#include <json.hpp>
#include <fstream>
#include <chrono>

// --- Base64 utils (invariati) ---
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
static inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    unsigned char char_array_3[3], char_array_4[4];
    int i = 0, j = 0;
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                              ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                              ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                          ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                          ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
        while (i++ < 3) ret += '=';
    }
    return ret;
}

// --- Singleton setup ---
SpotifyManager* SpotifyManager::sInstance = nullptr;

SpotifyManager* SpotifyManager::getInstance(Window* window)
{
    if (sInstance == nullptr)
    {
        if (window == nullptr)
            throw std::runtime_error("SpotifyManager::getInstance(): window non fornita al primo avvio");
        sInstance = new SpotifyManager(window);
    }
    else if (window != nullptr)
    {
        sInstance->mWindow = window;
    }
    return sInstance;
}

SpotifyManager::SpotifyManager(Window* window)
  : mWindow(window)
{
    auto userPath = Paths::getUserEmulationStationPath();
    Utils::FileSystem::createDirectory(userPath + "/spotify");
    mTokensPath = userPath + "/spotify/spotify_tokens.json";
    loadTokens();
}

SpotifyManager::~SpotifyManager() {}

// --- exchangeCodeForTokens ---
void SpotifyManager::exchangeCodeForTokens(Window* window, const std::string& code)
{
    auto loader = new GuiLoading<int>(window, "AUTHENTICATING WITH SPOTIFY...",
        [this, code](IGuiLoadingHandler* loadingInterface) -> int {
            auto gui = static_cast<GuiLoading<int>*>(loadingInterface);

            // ** USA LE COSTANTI, NON Settings **
            const std::string clientId     = CLIENT_ID;
            const std::string clientSecret = CLIENT_SECRET;
            const std::string redirectUri  = "es-spotify://callback";

            HttpReqOptions opts;
            opts.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
            std::string auth = base64_encode(
                reinterpret_cast<const unsigned char*>((clientId + ":" + clientSecret).c_str()),
                clientId.size() + clientSecret.size() + 1
            );
            opts.customHeaders.push_back("Authorization: Basic " + auth);
            opts.dataToPost = "grant_type=authorization_code&code=" + code +
                              "&redirect_uri=" + HttpReq::urlEncode(redirectUri);

            HttpReq req("https://accounts.spotify.com/api/token", &opts);
            req.wait();

            if (req.status() == HttpReq::Status::REQ_SUCCESS) {
                try {
                    auto j = nlohmann::json::parse(req.getContent());
                    mAccessToken  = j.value("access_token", "");
                    mRefreshToken = j.value("refresh_token", "");
                    saveTokens();
                    gui->setText("LOGIN SUCCESSFUL!");
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                } catch (...) {
                    gui->setText("LOGIN FAILED");
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            } else {
                gui->setText("LOGIN FAILED:\n" + req.getErrorMsg());
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            return 0;
        });
    window->pushGui(loader);
}

std::string SpotifyManager::getAccessToken() const { return mAccessToken; }
void SpotifyManager::logout()   { clearTokens(); }
bool SpotifyManager::isAuthenticated() const { return !mAccessToken.empty(); }
void SpotifyManager::resumePlayback()       { startPlayback(""); }


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
                if (mWindow) mWindow->postToUiThread([w = mWindow] { w->displayNotificationMessage("Comando Player fallito: start Richiesto Account Premium"); });
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

// --- pausePlayback ---
void SpotifyManager::pausePlayback() {
    std::thread([this]() {
        try {
            if (!isAuthenticated()) return;

            // Trova il device Spotify
            std::string deviceId = getActiveComputerDeviceId();
            if (deviceId.empty()) {
                LOG(LogWarning) << "Spotify: nessun device trovato per la pausa.";
                return;
            }

            // Prepara la richiesta PUT /pause?device_id=...
            HttpReqOptions opts;
            opts.verb = "PUT";
            opts.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            opts.customHeaders.push_back("Content-Length: 0");

            std::string url = "https://api.spotify.com/v1/me/player/pause?device_id=" + deviceId;
            HttpReq req(url, &opts);
            req.wait();

            // Gestione del 403
            if (req.status() == 403) {
                if (mWindow) {
                    mWindow->postToUiThread([w = mWindow] {
                        w->displayNotificationMessage(
                            _("Comando Player fallito: Richiesto Account Premium (pause)"),
                            3000
                        );
                    });
                }
                return;
            }

            // Altri errori ignoro
            if (req.status() != HttpReq::Status::REQ_SUCCESS && req.status() != 204) {
                LOG(LogError) << "Spotify pause error: status=" << req.status()
                              << " msg=" << req.getErrorMsg();
            }
        }
        catch (const std::exception& e) {
            LOG(LogError) << "[Thread pausePlayback] Eccezione: " << e.what();
        }
    }).detach();
}

// --- getUserPlaylists & getPlaylistTracks (stesso pattern) ---
void SpotifyManager::getUserPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback)
{
    std::thread([this,callback]() {
        std::vector<SpotifyPlaylist> out;
        try {
            if (isAuthenticated()) {
                HttpReqOptions o; o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r("https://api.spotify.com/v1/me/playlists", &o); r.wait();
                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    for (auto& i : j["items"]) {
                        std::string imageUrl = (i["images"].empty() ? "" : i["images"][0].value("url", ""));
                        std::string name = i.value("name", "?");
                        
                        // -- LOG AGGIUNTO QUI --
                        LOG(LogInfo) << "SpotifyManager -> Playlist: " << name << " | Image URL: " << imageUrl;

                        out.push_back({
                            name,
                            i.value("id",""),
                            imageUrl
                        });
                    }
                }
            }
        } catch(...) {}
        mWindow->postToUiThread([callback,out]{ callback(out); });
    }).detach();
}

void SpotifyManager::getPlaylistTracks(const std::string& pid,
    const std::function<void(const std::vector<SpotifyTrack>&)>& callback)
{
    std::thread([this,pid,callback]() {
        std::vector<SpotifyTrack> out;
        try {
            if (isAuthenticated() && !pid.empty()) {
                std::string url = "https://api.spotify.com/v1/playlists/" + pid + "/tracks";
                HttpReqOptions o; o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                do {
                    HttpReq r(url, &o); r.wait();
                    if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                        auto j = nlohmann::json::parse(r.getContent());
                        for (auto& it : j["items"]) {
                            auto& tr = it["track"];
                            if (!tr.is_null()) {
                                std::string trackName = tr.value("name","?");
                                std::string imageUrl = (tr.contains("album") && !tr["album"]["images"].empty()) ? tr["album"]["images"][0].value("url", "") : "";

                                // -- LOG AGGIUNTO QUI --
                                LOG(LogInfo) << "SpotifyManager -> Track: " << trackName << " | Image URL: " << imageUrl;

                                out.push_back({
                                    trackName,
                                    tr["artists"][0].value("name","?"),
                                    tr.value("uri",""),
                                    imageUrl
                                });
                            }
                        }
                        url = j.value("next",""); 
                    } else break;
                } while (!url.empty());
            }
        } catch(...) {}
        mWindow->postToUiThread([callback,out]{ callback(out); });
    }).detach();
}

// --- getCurrentlyPlaying, refresh, deviceId, tokens ---
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

bool SpotifyManager::refreshTokens()
{
    if (mRefreshToken.empty()) return false;
    const std::string clientId     = CLIENT_ID;
    const std::string clientSecret = CLIENT_SECRET;
    HttpReqOptions o;
    std::string auth = base64_encode(
        reinterpret_cast<const unsigned char*>((clientId + ":" + clientSecret).c_str()),
        clientId.size() + clientSecret.size() + 1
    );
    o.customHeaders.push_back("Authorization: Basic " + auth);
    o.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    o.dataToPost = "grant_type=refresh_token&refresh_token=" + mRefreshToken;
    HttpReq r("https://accounts.spotify.com/api/token", &o); r.wait();
    if (r.status() == HttpReq::Status::REQ_SUCCESS) {
        auto j = nlohmann::json::parse(r.getContent());
        mAccessToken  = j.value("access_token","");
        mRefreshToken = j.value("refresh_token", mRefreshToken);
        saveTokens();
        return true;
    }
    logout();
    return false;
}

std::string SpotifyManager::getActiveComputerDeviceId()
{
    if (!isAuthenticated()) return "";
    HttpReqOptions o; o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    HttpReq r("https://api.spotify.com/v1/me/player/devices", &o); r.wait();
    if (r.status() == HttpReq::Status::REQ_SUCCESS) {
        auto j = nlohmann::json::parse(r.getContent());
        for (auto& d : j["devices"])
            if (d.value("type","")=="Computer" && d.value("is_active",false))
                return d.value("id","");
        for (auto& d : j["devices"])
            if (d.value("type","")=="Computer")
                return d.value("id","");
    } else if (r.status() == 401 && refreshTokens())
        return getActiveComputerDeviceId();
    return "";
}

void SpotifyManager::loadTokens()
{
    if (!Utils::FileSystem::exists(mTokensPath)) return;
    std::ifstream f(mTokensPath);
    try {
        nlohmann::json j; f >> j;
        mAccessToken  = j.value("access_token","");
        mRefreshToken = j.value("refresh_token","");
    } catch(...) {}
}

void SpotifyManager::saveTokens()
{
    nlohmann::json j;
    j["access_token"]  = mAccessToken;
    j["refresh_token"] = mRefreshToken;
    std::ofstream f(mTokensPath);
    f << j.dump(4);
}

void SpotifyManager::clearTokens()
{
    mAccessToken.clear();
    mRefreshToken.clear();
    if (Utils::FileSystem::exists(mTokensPath))
        Utils::FileSystem::removeFile(mTokensPath);
}

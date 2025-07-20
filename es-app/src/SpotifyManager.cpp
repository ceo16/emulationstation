#include "SpotifyManager.h"
#include "HttpReq.h"
#include "Log.h"
#include "Paths.h"
#include "Settings.h"
#include "SystemConf.h" 
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

std::string SpotifyManager::getDefaultMarket() {
    std::string lang = SystemConf::getInstance()->get("system.language"); // es. "it_IT"
    if (lang.length() >= 5 && lang[2] == '_') {
        return Utils::String::toUpper(lang.substr(3, 2)); // Estrae "IT"
    }
    return "US"; // Fallback
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

void SpotifyManager::getFeaturedPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&, const std::string&)>& callback, const std::string& market)
{
    std::thread([this, callback, market]() {
        std::vector<SpotifyPlaylist> out;
        std::string message = "In primo piano"; // Messaggio di default

        if (isAuthenticated()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://api.spotify.com/v1/browse/featured-playlists?country=IT&locale=it_IT&limit=20";

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    message = j.value("message", message); // Spotify fornisce un titolo per la sezione

                    if (j.contains("playlists") && j["playlists"].contains("items")) {
                        for (auto& p : j["playlists"]["items"]) {
                            if (p.is_null()) continue;
                            out.push_back({
                                p.value("name", "?"),
                                p.value("id", ""),
                                (!p["images"].empty()) ? p["images"][0].value("url", "") : ""
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "[SpotifyManager] Errore API in getFeaturedPlaylists: " << r.getErrorMsg();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "[SpotifyManager] Eccezione in getFeaturedPlaylists: " << e.what();
            }
        }
        
        mWindow->postToUiThread([callback, out, message]{ callback(out, message); });
    }).detach();
}

void SpotifyManager::getCategories(const std::function<void(const std::vector<SpotifyCategory>&)>& callback, const std::string& market)
{
    std::thread([this, callback, market]() {
        std::vector<SpotifyCategory> out;
        if (isAuthenticated()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://developer.spotify.com/documentation/web-api/reference/get-track8" + currentMarket + "&limit=50"; // Limite massimo di categorie

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    if (j.contains("categories") && j["categories"].contains("items")) {
                        for (auto& c : j["categories"]["items"]) {
                            if (c.is_null()) continue;
                            out.push_back({
                                c.value("name", "?"),
                                c.value("id", ""),
                                (!c["icons"].empty()) ? c["icons"][0].value("url", "") : ""
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "[SpotifyManager] Errore API in getCategories: " << r.getErrorMsg();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "[SpotifyManager] Eccezione in getCategories: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out]{ callback(out); });
    }).detach();
}

void SpotifyManager::getCategoryPlaylists(const std::string& categoryId, const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback, const std::string& market)
{
    std::thread([this, categoryId, callback, market]() {
        std::vector<SpotifyPlaylist> out;
        if (isAuthenticated() && !categoryId.empty()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://developer.spotify.com/documentation/web-api/reference/get-track9" + categoryId + "/playlists?country=" + currentMarket;

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    if (j.contains("playlists") && j["playlists"].contains("items")) {
                        for (auto& p : j["playlists"]["items"]) {
                            if (p.is_null()) continue;
                            out.push_back({
                                p.value("name", "?"),
                                p.value("id", ""),
                                (!p["images"].empty()) ? p["images"][0].value("url", "") : ""
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "[SpotifyManager] Errore API in getCategoryPlaylists: " << r.getErrorMsg();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "[SpotifyManager] Eccezione in getCategoryPlaylists: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out]{ callback(out); });
    }).detach();
}

// --- getUserPlaylists & getPlaylistTracks (stesso pattern) ---
void SpotifyManager::getUserPlaylists(const std::function<void(const std::vector<SpotifyPlaylist>&)>& callback)
{
    std::thread([this, callback]() {
        std::vector<SpotifyPlaylist> out;
        try {
            if (isAuthenticated()) {
                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r("https://api.spotify.com/v1/me/playlists", &o);
                r.wait();
                
                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    if (j.contains("items")) {
                        for (auto& i : j["items"]) {
                            std::string name = i.value("name", "?");
                            std::string id = i.value("id", ""); // Estraiamo l'ID qui

                            // ==========================================================
                            // ===                LOG DI DEBUG CRUCIALE               ===
                            // ==========================================================
                            // Questo log ci mostrerà l'ID di ogni playlist trovata.
                            // Se l'ID è vuoto qui, il problema è nella risposta dell'API.
                            LOG(LogInfo) << "[SpotifyManager] Playlist Trovata: '" << name << "', ID Estratto: '" << id << "'";

                            out.push_back({
                                name,
                                id, // Usiamo l'ID che abbiamo appena loggato
                                (i["images"].empty() ? "" : i["images"][0].value("url", ""))
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "[SpotifyManager] Errore API in getUserPlaylists: " << r.getErrorMsg();
                }
            }
        } catch(const std::exception& e) {
            LOG(LogError) << "[SpotifyManager] Eccezione in getUserPlaylists: " << e.what();
        }
        
        mWindow->postToUiThread([callback, out]{ callback(out); });
    }).detach();
}

void SpotifyManager::getPlaylistTracks(
    std::string playlistId,
    const std::function<void(const std::vector<SpotifyTrack>&)>& callback,
    const std::string& market)
{
    LOG(LogInfo) << "[SpotifyManager] Chiamata a getPlaylistTracks con ID: " << playlistId;

    std::thread([this, playlistId = std::move(playlistId), callback, market]() mutable {
        std::vector<SpotifyTrack> out;
        if (isAuthenticated() && !playlistId.empty()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://api.spotify.com/v1/playlists/" + playlistId + "/tracks?market=" + currentMarket;
                
                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

                do {
                    HttpReq r(url, &o);
                    r.wait();
                    if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                        auto j = nlohmann::json::parse(r.getContent());
                        for (auto& it : j["items"]) {
                            auto& tr = it["track"];
                            if (!tr.is_null()) {
                                out.push_back({
                                    tr.value("name", "?"),
                                    tr.contains("artists") && !tr["artists"].empty() ? tr["artists"][0].value("name", "?") : "?",
                                    tr.value("uri", ""),
                                    tr.contains("album") && !tr["album"]["images"].empty() ? tr["album"]["images"][0].value("url", "") : ""
                                });
                            }
                        }
                        url = j.value("next", "");
                    } else {
                        LOG(LogError) << "Errore in getPlaylistTracks: " << r.getErrorMsg();
                        break;
                    }
                } while (!url.empty());
            } catch (const std::exception& e) {
                LOG(LogError) << "Eccezione in getPlaylistTracks: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out] { callback(out); });
    }).detach();
}

void SpotifyManager::getAlbumTracks(const std::string& albumId, const std::function<void(const std::vector<SpotifyTrack>&)>& callback, const std::string& market) {
    LOG(LogInfo) << "[SpotifyManager] Chiamata a getAlbumTracks con ID: " << albumId;

    std::thread([this, albumId, callback, market]() {
        std::vector<SpotifyTrack> out;
        if (isAuthenticated() && !albumId.empty()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://api.spotify.com/v1/albums/" + albumId + "/tracks?market=" + currentMarket;

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                
                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    for (auto& tr : j["items"]) {
                        if (!tr.is_null()) {
                            // La struttura delle tracce di un album è leggermente diversa
                            out.push_back({
                                tr.value("name", "?"),
                                tr.contains("artists") && !tr["artists"].empty() ? tr["artists"][0].value("name", "?") : "?",
                                tr.value("uri", ""),
                                "" // L'album non contiene l'immagine per ogni traccia, andrebbe recuperata dall'album principale se necessario
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "Errore in getAlbumTracks: " << r.getErrorMsg();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "Eccezione in getAlbumTracks: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out] { callback(out); });
    }).detach();
}

void SpotifyManager::getUserLikedSongs(const std::function<void(const std::vector<SpotifyTrack>&)>& callback)
{
    std::thread([this, callback]() {
        std::vector<SpotifyTrack> out;
        try {
            if (isAuthenticated()) {
                std::string url = "https://api.spotify.com/v1/me/tracks"; // Endpoint per i brani piaciuti
                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                do {
                    HttpReq r(url, &o);
                    r.wait();
                    if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                        auto j = nlohmann::json::parse(r.getContent());
                        for (auto& it : j["items"]) {
                            auto& tr = it["track"];
                            if (!tr.is_null())
                                out.push_back({
                                    tr.value("name","?"),
                                    tr["artists"][0].value("name","?"),
                                    tr.value("uri",""),
                                    (tr.contains("album") && !tr["album"]["images"].empty()) ? tr["album"]["images"][0].value("url", "") : ""
                                });
                        }
                        url = j.value("next", ""); // Per la paginazione, se ci sono più di 50 canzoni
                    } else break;
                } while (!url.empty());
            }
        } catch (...) {}
        mWindow->postToUiThread([callback, out]{ callback(out); });
    }).detach();
}

void SpotifyManager::search(const std::string& query, const std::string& types, const std::function<void(const nlohmann::json&)>& callback, const std::string& market)
{
    std::thread([this, query, types, callback, market]() {
        nlohmann::json result;
        if (isAuthenticated() && !query.empty()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://api.spotify.com/v1/search?q=" + HttpReq::urlEncode(query) + "&type=" + types + "&market=" + currentMarket;

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r(url, &o);
                r.wait();
                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    result = nlohmann::json::parse(r.getContent());
                } else {
                    LOG(LogError) << "Errore nella ricerca Spotify. URL: " << url << " | Errore: " << r.getErrorMsg();
                }
            } catch(const std::exception& e) {
                LOG(LogError) << "Eccezione in search: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, result]{ callback(result); });
    }).detach();
}

void SpotifyManager::getArtistTopTracks(const std::string& artistId, const std::function<void(const std::vector<SpotifyTrack>&)>& callback, const std::string& market)
{
    LOG(LogInfo) << "[SpotifyManager] Chiamata a getArtistTopTracks per ID: " << artistId;
    
    std::thread([this, artistId, callback, market]() {
        std::vector<SpotifyTrack> out;
        if (isAuthenticated() && !artistId.empty()) {
            try {
                std::string currentMarket = market.empty() ? getDefaultMarket() : market;
                std::string url = "https://api.spotify.com/v1/artists/" + artistId + "/top-tracks?market=" + currentMarket;

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    if (j.contains("tracks")) {
                        for (auto& tr : j["tracks"]) {
                            if (!tr.is_null()) {
                                out.push_back({
                                    tr.value("name", "?"),
                                    tr.contains("artists") && !tr["artists"].empty() ? tr["artists"][0].value("name", "?") : "N/A",
                                    tr.value("uri", ""),
                                    (tr.contains("album") && !tr["album"]["images"].empty()) ? tr["album"]["images"][0].value("url", "") : ""
                                });
                            }
                        }
                    }
                } else {
                    LOG(LogError) << "[ASYNC] Errore nel recuperare le top tracks: " << r.getErrorMsg();
                }
            } catch(const std::exception& e) {
                LOG(LogError) << "[ASYNC] Eccezione: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out] { callback(out); });
    }).detach();
}

void SpotifyManager::getAvailableDevices(const std::function<void(const std::vector<SpotifyDevice>&)>& callback)
{
    std::thread([this, callback]() {
        std::vector<SpotifyDevice> out;
        if (isAuthenticated()) {
            try {
                std::string url = "https://api.spotify.com/v1/me/player/devices"; // GET /me/player/devices

                HttpReqOptions o;
                o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq r(url, &o);
                r.wait();

                if (r.status() == HttpReq::Status::REQ_SUCCESS) {
                    auto j = nlohmann::json::parse(r.getContent());
                    if (j.contains("devices")) {
                        for (auto& d : j["devices"]) {
                            if (d.is_null()) continue;
                            out.push_back({
                                d.value("name", "?"),
                                d.value("id", ""),
                                d.value("type", "Sconosciuto"),
                                d.value("is_active", false)
                            });
                        }
                    }
                } else {
                    LOG(LogError) << "[SpotifyManager] Errore API in getAvailableDevices: " << r.getErrorMsg();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "[SpotifyManager] Eccezione in getAvailableDevices: " << e.what();
            }
        }
        mWindow->postToUiThread([callback, out]{ callback(out); });
    }).detach();
}

void SpotifyManager::startPlaybackContextual(const std::string& context_uri, const std::string& track_uri_to_start)
{
    if (context_uri.empty() || !isAuthenticated()) {
        return;
    }

    std::thread([this, context_uri, track_uri_to_start]() {
        try {
            std::string deviceId = getActiveComputerDeviceId();
            if (deviceId.empty()) return;

            std::string url = "https://api.spotify.com/v1/me/player/play"; // PUT /me/player/play

            nlohmann::json body;
            body["context_uri"] = context_uri;

            // Specifichiamo da quale traccia iniziare
            if (!track_uri_to_start.empty()) {
                body["offset"] = { {"uri", track_uri_to_start} };
            }

            HttpReqOptions o;
            o.verb = "PUT";
            o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            o.customHeaders.push_back("Content-Type: application/json");
            o.dataToPost = body.dump();

            HttpReq r(url + "?device_id=" + deviceId, &o);
            r.wait();

            if (r.status() != 204 && r.status() != 202) {
                LOG(LogError) << "[SpotifyManager] Errore API in startPlaybackContextual: " << r.getErrorMsg();
            } else {
                LOG(LogInfo) << "[SpotifyManager] Riproduzione contestuale avviata per: " << context_uri;
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "[SpotifyManager] Eccezione in startPlaybackContextual: " << e.what();
        }
    }).detach();
}

void SpotifyManager::transferPlayback(const std::string& deviceId)
{
    if (deviceId.empty() || !isAuthenticated()) {
        return;
    }

    std::thread([this, deviceId]() {
        try {
            std::string url = "https://api.spotify.com/v1/me/player"; // PUT /me/player

            nlohmann::json body;
            body["device_ids"] = { deviceId };
            body["play"] = true; // Avvia la riproduzione subito dopo il trasferimento

            HttpReqOptions o;
            o.verb = "PUT";
            o.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            o.customHeaders.push_back("Content-Type: application/json");
            o.dataToPost = body.dump();

            HttpReq r(url, &o);
            r.wait();

            if (r.status() != 204) { // Spotify risponde 204 (No Content) in caso di successo
                LOG(LogError) << "[SpotifyManager] Errore API in transferPlayback: " << r.getErrorMsg();
            } else {
                LOG(LogInfo) << "[SpotifyManager] Riproduzione trasferita al dispositivo ID: " << deviceId;
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "[SpotifyManager] Eccezione in transferPlayback: " << e.what();
        }
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
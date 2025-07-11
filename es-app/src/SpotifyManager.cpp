#include "SpotifyManager.h"
#include "SystemConf.h" // <--- CORREZIONE 1: Aggiunto l'include mancante
#include "HttpReq.h"
#include "Log.h"
#include "Paths.h"
#include "Settings.h"
#include "Window.h"
#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"
#include <thread>
#include <json.hpp>
#include <fstream>

// --- Utility per la codifica Base64 ---
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
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
SpotifyManager* SpotifyManager::getInstance() {
    if (sInstance == nullptr) sInstance = new SpotifyManager();
    return sInstance;
}

SpotifyManager::SpotifyManager() {
    std::string userPath = Paths::getUserEmulationStationPath();
    Utils::FileSystem::createDirectory(userPath + "/spotify");
    mTokensPath = userPath + "/spotify/spotify_tokens.json";
    loadTokens();
}

SpotifyManager::~SpotifyManager() {}

void SpotifyManager::exchangeCodeForTokens(Window* window, const std::string& code)
{
    // <--- CORREZIONE 2: Usiamo GuiLoading<int> invece di <void> o <string>
    auto SincroAuth = new GuiLoading<int>(window, _("AUTHENTICATING WITH SPOTIFY..."),
        [this, code](IGuiLoadingHandler* loadingInterface) -> int // La lambda ora ritorna un int
        {
            auto loadingGui = (GuiLoading<int>*)loadingInterface;

            const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
            const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");
            const std::string SPOTIFY_REDIRECT_URI = "es-spotify://callback";

            if (clientId.empty() || clientSecret.empty()) {
                loadingGui->setText("Client ID/Secret non configurato!");
                std::this_thread::sleep_for(std::chrono::seconds(3));
                return 0; // Ritorna un valore fittizio
            }

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
                        loadingGui->setText(_("LOGIN SUCCESSFUL!"));
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    } else {
                        std::string error_desc = responseJson.value("error_description", "Unknown JSON error.");
                        loadingGui->setText(_("LOGIN FAILED") + ":\n" + error_desc);
                        std::this_thread::sleep_for(std::chrono::seconds(4));
                    }
                } catch (const std::exception& e) {
                    loadingGui->setText(_("LOGIN FAILED") + ":\n" + e.what());
                    std::this_thread::sleep_for(std::chrono::seconds(4));
                }
            } else {
                loadingGui->setText(_("LOGIN FAILED") + ":\n" + request.getErrorMsg());
                std::this_thread::sleep_for(std::chrono::seconds(4));
            }
            return 0; // Ritorna un valore fittizio
        });

    window->pushGui(SincroAuth);
}


bool SpotifyManager::refreshTokens() {
    LOG(LogInfo) << "Spotify: Access token scaduto. Tento il refresh...";

    const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
    const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");

    if (mRefreshToken.empty() || clientId.empty() || clientSecret.empty()) { return false; }

    HttpReqOptions options;
    std::string auth_str = clientId + ":" + clientSecret;
    std::string auth_b64 = base64_encode(reinterpret_cast<const unsigned char*>(auth_str.c_str()), auth_str.length());
    options.customHeaders.push_back("Authorization: Basic " + auth_b64);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    options.dataToPost = "grant_type=refresh_token&refresh_token=" + mRefreshToken;

    // AGGIUNGI QUESTA RIGA
    options.customHeaders.push_back("Content-Length: " + std::to_string(options.dataToPost.length()));

    HttpReq request("https://accounts.spotify.com/api/token", &options);
    request.wait();

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("access_token")) {
                mAccessToken = json.value("access_token", "");
                
                // Spotify potrebbe restituire un nuovo refresh_token, aggiorniamolo se presente
                if (json.contains("refresh_token")) {
                    mRefreshToken = json.value("refresh_token", "");
                }
                
                saveTokens();
                LOG(LogInfo) << "Spotify: Token rinfrescato con successo.";
                return true;
            }
        } catch (...) {
            LOG(LogError) << "Spotify: Errore nel parsing della risposta di refresh.";
        }
    }
    
    LOG(LogError) << "Spotify: Refresh del token fallito. Status: " << request.status();
    // Se il refresh fallisce, il refresh_token potrebbe essere stato revocato.
    // Facciamo il logout forzato per obbligare l'utente a ri-autenticarsi.
    logout();
    return false;
}

void SpotifyManager::logout() {
    clearTokens();
    LOG(LogInfo) << "Spotify Auth: Utente disconnesso.";
}

bool SpotifyManager::isAuthenticated() const {
    return !mAccessToken.empty();
}

std::string SpotifyManager::getAccessToken() const {
    return mAccessToken;
}

void SpotifyManager::loadTokens() {
    if (!Utils::FileSystem::exists(mTokensPath)) return;
    try {
        std::ifstream file(mTokensPath);
        nlohmann::json j;
        file >> j;
        mAccessToken = j.value("access_token", "");
        mRefreshToken = j.value("refresh_token", "");
        LOG(LogInfo) << "Spotify Auth: Token caricati da " << mTokensPath;
    } catch (...) {
        LOG(LogError) << "Spotify Auth: Errore nel caricamento dei token.";
        clearTokens();
    }
}

void SpotifyManager::saveTokens() {
    nlohmann::json j;
    j["access_token"] = mAccessToken;
    j["refresh_token"] = mRefreshToken;
    try {
        std::ofstream file(mTokensPath);
        file << j.dump(4);
        LOG(LogInfo) << "Spotify Auth: Token salvati in " << mTokensPath;
    } catch (...) {
        LOG(LogError) << "Spotify Auth: Errore nel salvataggio dei token.";
    }
}

void SpotifyManager::clearTokens() {
    mAccessToken = "";
    mRefreshToken = "";
    if (Utils::FileSystem::exists(mTokensPath))
        Utils::FileSystem::removeFile(mTokensPath);
}

void SpotifyManager::startPlayback(const std::string& track_uri)
{
    if (!isAuthenticated()) return;

    // 1. Otteniamo l'ID del dispositivo
    std::string deviceId = getActiveComputerDeviceId();
    if (deviceId.empty()) {
        LOG(LogError) << "Spotify: Impossibile avviare la riproduzione, nessun dispositivo computer attivo trovato.";
        return;
    }

    HttpReqOptions options;
    options.verb = "PUT"; 
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    options.customHeaders.push_back("Content-Type: application/json");

    nlohmann::json body;
    if (!track_uri.empty()) {
        body["uris"] = { track_uri };
    }
    options.dataToPost = body.dump();
    options.customHeaders.push_back("Content-Length: " + std::to_string(options.dataToPost.length()));

    // 2. Aggiungiamo l'ID del dispositivo all'URL
    std::string url = "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId;

    HttpReq request(url, &options);
    request.wait();

    if (request.status() != HttpReq::Status::REQ_SUCCESS && request.status() != 204) {
        LOG(LogError) << "Spotify: Errore nell'invio del comando di riproduzione. Status: " << request.status();
    }
}

void SpotifyManager::pausePlayback()
{
    if (!isAuthenticated()) return;
    
    // Anche la pausa è più affidabile se specifichiamo il dispositivo
    std::string deviceId = getActiveComputerDeviceId();
    if (deviceId.empty()) return;

    HttpReqOptions options;
    options.verb = "PUT";
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    options.customHeaders.push_back("Content-Length: 0");

    std::string url = "https://api.spotify.com/v1/me/player/pause?device_id=" + deviceId;
    
    HttpReq request(url, &options);
    request.wait();
}

std::vector<SpotifyPlaylist> SpotifyManager::getUserPlaylists()
{
    if (!isAuthenticated()) 
        return {};

    std::vector<SpotifyPlaylist> playlists; 

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    // URL CORRETTO E VERIFICATO
    std::string url = "https://api.spotify.com/v1/me/playlists";

    LOG(LogInfo) << "Spotify: Richiesta playlist all'URL: " << url;
    
    HttpReq request(url, &options);
    request.wait();

    // Se il token è scaduto, lo rinfreschiamo e riproviamo
    if (request.status() == 401)
    {
        if (this->refreshTokens())
        {
            LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta delle playlist.";
            HttpReqOptions newOptions;
            newOptions.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            
            HttpReq request_retry(url, &newOptions);
            request_retry.wait();

            if (request_retry.status() == HttpReq::Status::REQ_SUCCESS) {
                try {
                    auto json = nlohmann::json::parse(request_retry.getContent());
                    if (json.contains("items")) {
                        for (const auto& item : json["items"]) {
                            SpotifyPlaylist p;
                            p.name = item.value("name", "N/A");
                            p.id = item.value("id", "");
                            if (item.contains("images") && !item["images"].empty()) {
                                p.image_url = item["images"][0].value("url", "");
                            }
                            playlists.push_back(p);
                        }
                    }
                } catch (const std::exception& e) {
                    LOG(LogError) << "Spotify: Errore nel parsing delle playlist (retry): " << e.what();
                }
            }
            return playlists;
        }
    }

    // Processiamo il risultato della prima richiesta
    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("items")) {
                for (const auto& item : json["items"]) {
                    SpotifyPlaylist p;
                    p.name = item.value("name", "N/A");
                    p.id = item.value("id", "");
                    if (item.contains("images") && !item["images"].empty()) {
                        p.image_url = item["images"][0].value("url", "");
                    }
                    playlists.push_back(p);
                }
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing delle playlist: " << e.what();
        }
    } else {
        LOG(LogError) << "Spotify: La richiesta delle playlist è fallita con stato: " << request.status() << " - " << request.getErrorMsg();
    }
    
    return playlists;
}






std::string SpotifyManager::getActiveComputerDeviceId()
{
    if (!isAuthenticated()) return "";

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    HttpReq request("https://api.spotify.com/v1/me/player/devices", &options);
    request.wait();

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("devices")) {
                for (const auto& device : json["devices"]) {
                    // Cerchiamo un computer attivo, o il primo computer che troviamo
                    if (device.value("type", "") == "Computer") {
                        // Se è attivo, lo prendiamo subito
                        if (device.value("is_active", false)) {
                            return device.value("id", "");
                        }
                        // Altrimenti, continuiamo a cercare sperando di trovarne uno attivo
                    }
                }
                // Se non ne abbiamo trovato uno attivo, proviamo a prendere il primo computer della lista
                 for (const auto& device : json["devices"]) {
                    if (device.value("type", "") == "Computer") {
                         return device.value("id", "");
                    }
                 }
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing dei dispositivi: " << e.what();
        }
    }

    LOG(LogWarning) << "Spotify: Nessun dispositivo computer trovato o attivo.";
    return "";
}

SpotifyTrack SpotifyManager::getCurrentlyPlaying()
{
    if (!isAuthenticated()) return {};

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";
    
    // 1. Prima richiesta
    HttpReq request(url, &options);
    request.wait();

    // 2. Se il token è scaduto, rinfresca e riprova
    if (request.status() == 401)
    {
        if (this->refreshTokens())
        {
            LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta della traccia attuale.";
            HttpReqOptions newOptions;
            newOptions.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            
            // Creiamo un NUOVO oggetto per la seconda richiesta
            HttpReq request_retry(url, &newOptions);
            request_retry.wait();

            // Processiamo il risultato della SECONDA richiesta e usciamo
            SpotifyTrack info;
            if (request_retry.status() == 200) {
                try {
                    auto json = nlohmann::json::parse(request_retry.getContent());
                    if (json.contains("item") && !json["item"].is_null()) {
                        info.name = json["item"].value("name", "N/A");
                        info.uri = json["item"].value("uri", "N/A");
                        if (json["item"].contains("artists") && !json["item"]["artists"].empty()) {
                            info.artist = json["item"]["artists"][0].value("name", "N/A");
                        }
                    }
                } catch (...) { /* gestione errore */ }
            }
            return info;
        }
    }

    // 3. Altrimenti, processiamo il risultato della PRIMA richiesta
    SpotifyTrack info;
    if (request.status() == 200) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("item") && !json["item"].is_null()) {
                info.name = json["item"].value("name", "N/A");
                info.uri = json["item"].value("uri", "N/A");
                if (json["item"].contains("artists") && !json["item"]["artists"].empty()) {
                    info.artist = json["item"]["artists"][0].value("name", "N/A");
                }
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing della traccia attuale: " << e.what();
        }
    } else if (request.status() != 204) {
         LOG(LogWarning) << "Spotify: La richiesta della traccia attuale è fallita con stato: " << request.status();
    }
    
    return info;
}
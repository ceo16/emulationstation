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
#include <thread>
#include <json.hpp>
#include <fstream>
#include <chrono> // Necessario per std::chrono::seconds

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
    auto SincroAuth = new GuiLoading<int>(window, _("AUTHENTICATING WITH SPOTIFY..."),
        [this, code](IGuiLoadingHandler* loadingInterface) -> int
        {
            auto loadingGui = (GuiLoading<int>*)loadingInterface;

            const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
            const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");
            const std::string SPOTIFY_REDIRECT_URI = "es-spotify://callback";

            if (clientId.empty() || clientSecret.empty()) {
                loadingGui->setText("Client ID/Secret non configurato! Controlla emulationstation_settings.xml");
                std::this_thread::sleep_for(std::chrono::seconds(3));
                return 0;
            }

            HttpReqOptions options;
            options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
            std::string auth_str = clientId + ":" + clientSecret;
            std::string auth_b64 = base64_encode(reinterpret_cast<const unsigned char*>(auth_str.c_str()), auth_str.length());
            options.customHeaders.push_back("Authorization: Basic " + auth_b64);
            options.dataToPost = "grant_type=authorization_code&code=" + code + "&redirect_uri=" + HttpReq::urlEncode(SPOTIFY_REDIRECT_URI);

            // *** URL DI AUTENTICAZIONE CORRETTO ***
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
            return 0;
        });

    window->pushGui(SincroAuth);
}


bool SpotifyManager::refreshTokens() {
    LOG(LogInfo) << "Spotify: Access token scaduto. Tento il refresh...";

    const std::string clientId = SystemConf::getInstance()->get("spotify.client.id");
    const std::string clientSecret = SystemConf::getInstance()->get("spotify.client.secret");

    if (mRefreshToken.empty() || clientId.empty() || clientSecret.empty()) {
        LOG(LogError) << "Spotify: Impossibile rinfrescare i token. Credenziali mancanti o refresh token vuoto.";
        return false;
    }

    HttpReqOptions options;
    std::string auth_str = clientId + ":" + clientSecret;
    std::string auth_b64 = base64_encode(reinterpret_cast<const unsigned char*>(auth_str.c_str()), auth_str.length());
    options.customHeaders.push_back("Authorization: Basic " + auth_b64);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.dataToPost = "grant_type=refresh_token&refresh_token=" + mRefreshToken;
    options.customHeaders.push_back("Content-Length: " + std::to_string(options.dataToPost.length()));

    // *** URL DI REFRESH TOKEN CORRETTO ***
    HttpReq request("https://accounts.spotify.com/api/token", &options);
    request.wait();

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("access_token")) {
                mAccessToken = json.value("access_token", "");
                if (json.contains("refresh_token")) { // Spotify potrebbe restituire un nuovo refresh_token
                    mRefreshToken = json.value("refresh_token", "");
                }
                saveTokens();
                LOG(LogInfo) << "Spotify: Token rinfrescato con successo.";
                return true;
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing della risposta di refresh: " << e.what();
        }
    } else {
        LOG(LogError) << "Spotify: Refresh del token fallito. Status: " << request.status() << " - " << request.getErrorMsg()
                      << " Contenuto: " << request.getContent(); // Logga il contenuto per debug
    }

    logout(); // Se il refresh fallisce, forziamo il logout
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
    } catch (const std::exception& e) {
        LOG(LogError) << "Spotify Auth: Errore nel caricamento dei token da " << mTokensPath << ": " << e.what();
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
    } catch (const std::exception& e) {
        LOG(LogError) << "Spotify Auth: Errore nel salvataggio dei token in " << mTokensPath << ": " << e.what();
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

    std::string deviceId = getActiveComputerDeviceId();
    if (deviceId.empty()) {
        LOG(LogError) << "Spotify: Impossibile avviare la riproduzione, nessun dispositivo computer attivo o riproducibile trovato.";
        return;
    }

    HttpReqOptions options;
    options.verb = "PUT";
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    options.customHeaders.push_back("Content-Type: application/json");

    std::string bodyStr;
    if (!track_uri.empty()) {
        nlohmann::json body;
        body["uris"] = { track_uri };
        bodyStr = body.dump();
    } else {
        // Per riprendere la riproduzione corrente, body vuoto
        bodyStr = ""; 
    }
    options.dataToPost = bodyStr;
    options.customHeaders.push_back("Content-Length: " + std::to_string(bodyStr.length()));

    std::string url = "https://api.spotify.com/v1/me/player/play";
    if (!deviceId.empty()) {
        url += "?device_id=" + deviceId;
    }

    LOG(LogInfo) << "Spotify: Invio comando di riproduzione all'URL: " << url << " con body: " << (bodyStr.empty() ? "<vuoto>" : bodyStr);

    HttpReq request(url, &options);
    request.wait();

    if (request.status() != HttpReq::Status::REQ_SUCCESS && request.status() != 204) {
        LOG(LogError) << "Spotify: Errore nell'invio del comando di riproduzione. Status: " << request.status() << " - " << request.getErrorMsg();
        LOG(LogError) << "Spotify: Contenuto errore riproduzione: " << request.getContent();
    } else {
        LOG(LogInfo) << "Spotify: Comando di riproduzione inviato con successo (Status " << request.status() << ").";
    }
}


void SpotifyManager::pausePlayback()
{
    if (!isAuthenticated()) return;

    std::string deviceId = getActiveComputerDeviceId();
    if (deviceId.empty()) {
        LOG(LogWarning) << "Spotify: Impossibile mettere in pausa, nessun dispositivo computer attivo trovato.";
        return;
    }

    HttpReqOptions options;
    options.verb = "PUT";
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
    options.customHeaders.push_back("Content-Length: 0"); // PUT senza body

    // *** URL DI PAUSA CORRETTO CON device_id COME QUERY PARAMETER ***
    std::string url = "https://api.spotify.com/v1/me/player/pause";
    if (!deviceId.empty()) {
        url += "?device_id=" + deviceId;
    }

    LOG(LogInfo) << "Spotify: Invio comando di pausa all'URL: " << url;

    HttpReq request(url, &options);
    request.wait();

    if (request.status() != HttpReq::Status::REQ_SUCCESS && request.status() != 204) {
        LOG(LogError) << "Spotify: Errore nell'invio del comando di pausa. Status: " << request.status() << " - " << request.getErrorMsg();
        LOG(LogError) << "Spotify: Contenuto errore pausa: " << request.getContent();
    } else {
        LOG(LogInfo) << "Spotify: Comando di pausa inviato con successo (Status " << request.status() << ").";
    }
}

// Funzione resumePlayback, aggiunta anche in .h
void SpotifyManager::resumePlayback() {
    startPlayback(""); // Chiamiamo startPlayback senza URI per riprendere la riproduzione attuale
}

std::vector<SpotifyPlaylist> SpotifyManager::getUserPlaylists()
{
    if (!isAuthenticated())
        return {};

    std::vector<SpotifyPlaylist> playlists;

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    std::string url = "https://api.spotify.com/v1/me/playlists";

    LOG(LogInfo) << "Spotify: Richiesta playlist all'URL: " << url;

    HttpReq request(url, &options);
    request.wait();

    LOG(LogDebug) << "Spotify: Risposta JSON getUserPlaylists: " << request.getContent();

    if (request.status() == 401) // Token scaduto o non valido
    {
        if (this->refreshTokens()) // Tentiamo di rinfrescare
        {
            LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta delle playlist.";
            HttpReqOptions newOptions;
            newOptions.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

            HttpReq request_retry(url, &newOptions);
            request_retry.wait();

            LOG(LogDebug) << "Spotify: Risposta JSON getUserPlaylists (retry): " << request_retry.getContent();

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
            } else {
                LOG(LogError) << "Spotify: Richiesta playlist (retry) fallita con stato: " << request_retry.status() << " - " << request_retry.getErrorMsg();
            }
            return playlists;
        }
    }

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

std::vector<SpotifyTrack> SpotifyManager::getPlaylistTracks(const std::string& playlist_id)
{
    if (playlist_id.empty()) {
        LOG(LogError) << "ERRORE: getPlaylistTracks chiamata con ID playlist vuoto!";
        return {};
    }
    if (!isAuthenticated()) {
        LOG(LogWarning) << "getPlaylistTracks: Non autenticato con Spotify.";
        return {};
    }

    std::vector<SpotifyTrack> tracks;
    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    std::string url = "https://api.spotify.com/v1/playlists/" + playlist_id + "/tracks";

    do {
        LOG(LogInfo) << "Spotify: Richiesta tracce playlist all'URL: " << url;

        HttpReq request(url, &options);
        request.wait();

        LOG(LogDebug) << "Spotify: Risposta JSON getPlaylistTracks: " << request.getContent();

        if (request.status() == 401) { // Token scaduto o non valido
            if (this->refreshTokens()) { // Tentiamo di rinfrescare
                LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta delle tracce della playlist.";
                HttpReqOptions newOptions;
                newOptions.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
                HttpReq request_retry(url, &newOptions);
                request_retry.wait();

                LOG(LogDebug) << "Spotify: Risposta JSON getPlaylistTracks (retry): " << request_retry.getContent();

                if (request_retry.status() == HttpReq::Status::REQ_SUCCESS) {
                    try {
                        auto json = nlohmann::json::parse(request_retry.getContent());
                        if (json.contains("items")) {
                            for (const auto& item : json["items"]) {
                                if (item.contains("track") && !item["track"].is_null()) {
                                    SpotifyTrack t;
                                    t.name = item["track"].value("name", "N/A");
                                    t.uri = item["track"].value("uri", "");
                                    if (item["track"].contains("artists") && !item["track"]["artists"].empty()) {
                                        t.artist = item["track"]["artists"][0].value("name", "N/A");
                                    }
                                    tracks.push_back(t);
                                }
                            }
                        }
                        url.clear(); // Non continuiamo dopo il retry
                    } catch (const std::exception& e) {
                        LOG(LogError) << "getPlaylistTracks (retry) JSON parse error: " << e.what();
                    }
                } else {
                    LOG(LogError) << "Spotify: Richiesta tracce playlist (retry) fallita con stato: " << request_retry.status() << " - " << request_retry.getErrorMsg();
                }
                return tracks;
            }
        }

        if (request.status() == HttpReq::Status::REQ_SUCCESS) {
            try {
                auto json = nlohmann::json::parse(request.getContent());
                if (json.contains("items")) {
                    for (const auto& item : json["items"]) {
                        if (item.contains("track") && !item["track"].is_null()) {
                            SpotifyTrack t;
                            t.name = item["track"].value("name", "N/A");
                            t.uri = item["track"].value("uri", "");
                            if (item["track"].contains("artists") && !item["track"]["artists"].empty()) {
                                t.artist = item["track"]["artists"][0].value("name", "N/A");
                            }
                            tracks.push_back(t);
                        }
                    }
                }

                // Gestione paginazione: aggiorno URL alla pagina successiva
                if (json.contains("next") && !json["next"].is_null()) {
                    url = json["next"].get<std::string>();
                } else {
                    url.clear();
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "getPlaylistTracks JSON parse error: " << e.what();
                url.clear();
            }
        } else {
            LOG(LogError) << "Spotify: Richiesta tracce playlist fallita con stato: " << request.status() << " - " << request.getErrorMsg();
            url.clear();
        }
    } while (!url.empty());

    return tracks;
}

std::string SpotifyManager::getActiveComputerDeviceId()
{
    if (!isAuthenticated()) {
        LOG(LogWarning) << "Spotify: Non autenticato per ottenere i dispositivi.";
        return "";
    }

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    // *** URL GET DEVICES CORRETTO ***
    HttpReq request("https://api.spotify.com/v1/me/player/devices", &options);
    request.wait();

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto json = nlohmann::json::parse(request.getContent());
            if (json.contains("devices")) {
                std::string foundActiveComputerId = "";
                std::string foundAnyComputerId = "";

                for (const auto& device : json["devices"]) {
                    std::string deviceName = device.value("name", "N/A");
                    std::string deviceId = device.value("id", "N/A");
                    std::string deviceType = device.value("type", "N/A");
                    bool isActive = device.value("is_active", false);
                    bool isRestricted = device.value("is_restricted", false); // Aggiunto per debug
                    bool isPrivateSession = device.value("is_private_session", false); // Aggiunto per debug

                    LOG(LogInfo) << "Spotify: Dispositivo trovato: Nome='" << deviceName
                                 << "', ID='" << deviceId
                                 << "', Tipo='" << deviceType
                                 << "', Attivo=" << (isActive ? "SÌ" : "NO")
                                 << ", Limitato=" << (isRestricted ? "SÌ" : "NO")
                                 << ", Sessione privata=" << (isPrivateSession ? "SÌ" : "NO");

                    if (deviceType == "Computer") {
                        if (isActive && !isRestricted && !isPrivateSession) {
                            LOG(LogInfo) << "Spotify: Trovato dispositivo computer ATTIVO e USABILE con ID: " << deviceId;
                            return deviceId; // Restituisce subito il primo computer attivo e usabile
                        }
                        if (foundAnyComputerId.empty()) { // Conserva il primo ID computer trovato, come fallback
                            foundAnyComputerId = deviceId;
                        }
                    }
                }
                if (!foundActiveComputerId.empty()) {
                    return foundActiveComputerId; // Questo non dovrebbe essere raggiunto se il primo if funziona
                }
                if (!foundAnyComputerId.empty()) {
                    LOG(LogWarning) << "Spotify: Nessun dispositivo computer ATTIVO e USABILE trovato, ma è stato trovato un computer con ID: " << foundAnyComputerId << ". Potrebbe non essere riproducibile.";
                    return foundAnyComputerId; // Restituisce il primo computer trovato, anche se non attivo/usabile
                }
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing dei dispositivi: " << e.what();
        }
    } else if (request.status() == 401) { // Token scaduto
        if (refreshTokens()) {
            LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta dei dispositivi.";
            // Riprova la richiesta dopo il refresh
            return getActiveComputerDeviceId(); // Richiama la funzione per riprovare
        }
    } else {
        LOG(LogError) << "Spotify: Richiesta dispositivi fallita con stato: " << request.status() << " - " << request.getErrorMsg()
                      << " Contenuto: " << request.getContent();
    }

    LOG(LogWarning) << "Spotify: Nessun dispositivo computer trovato o utilizzabile.";
    return "";
}

SpotifyTrack SpotifyManager::getCurrentlyPlaying()
{
    if (!isAuthenticated()) return {};

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + getAccessToken());

    // *** URL GET CURRENTLY PLAYING CORRETTO ***
    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";

    LOG(LogInfo) << "Spotify: Richiesta traccia attualmente in riproduzione all'URL: " << url;

    HttpReq request(url, &options);
    request.wait();

    if (request.status() == 401) { // Token scaduto
        if (this->refreshTokens()) {
            LOG(LogInfo) << "Spotify: Token rinfrescato, ritento la richiesta della traccia attuale.";
            HttpReqOptions newOptions;
            newOptions.customHeaders.push_back("Authorization: Bearer " + getAccessToken());
            HttpReq request_retry(url, &newOptions);
            request_retry.wait();

            // Processiamo il risultato della SECONDA richiesta
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
                    LOG(LogInfo) << "Spotify: Traccia corrente (retry): " << info.name << " di " << info.artist;
                } catch (const std::exception& e) {
                    LOG(LogError) << "Spotify: Errore nel parsing della traccia attuale (retry): " << e.what();
                }
            } else if (request_retry.status() != 204) {
                 LOG(LogWarning) << "Spotify: La richiesta della traccia attuale (retry) è fallita con stato: " << request_retry.status() << " - " << request_retry.getErrorMsg();
            }
            return info;
        }
    }

    // Processiamo il risultato della PRIMA richiesta
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
            LOG(LogInfo) << "Spotify: Traccia corrente: " << info.name << " di " << info.artist;
        } catch (const std::exception& e) {
            LOG(LogError) << "Spotify: Errore nel parsing della traccia attuale: " << e.what();
        }
    } else if (request.status() != 204) { // 204 No Content significa che non c'è nulla in riproduzione
        LOG(LogWarning) << "Spotify: La richiesta della traccia attuale è fallita con stato: " << request.status() << " - " << request.getErrorMsg();
    }

    return info;
}


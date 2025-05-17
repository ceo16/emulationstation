#include "GameStore/Xbox/XboxAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "Settings.h" // Per ottenere il percorso di configurazione
#include "Paths.h"    // Per Paths::getEsConfigPath()
#include "Window.h"   // Se devi mostrare UI per il codice
#include "utils/StringUtil.h" // Per trim, ecc.
#include "utils/TimeUtil.h"   // Per la gestione del tempo di scadenza
#include "json.hpp"  // Per il parsing JSON
#include <string>
#include "utils/FileSystemUtil.h"

#include <fstream>      // Per std::ifstream, std::ofstream
#include <iomanip>      // Per std::put_time, std::get_time (per salvare/caricare expiry)

namespace nj = nlohmann;

// --- Definizione costanti statiche ---
const std::string XboxAuth::CLIENT_ID = "38cd2fa8-66fd-4760-afb2-405eb65d5b0c"; // Playnite's client_id
const std::string XboxAuth::REDIRECT_URI = "https://login.live.com/oauth20_desktop.srf";
const std::string XboxAuth::SCOPE = "XboxLive.signin XboxLive.offline_access";
const std::string XboxAuth::LIVE_AUTHORIZE_URL = "https://login.live.com/oauth20_authorize.srf";
const std::string XboxAuth::LIVE_TOKEN_URL = "https://login.live.com/oauth20_token.srf";
const std::string XboxAuth::XBOX_USER_AUTHENTICATE_URL = "https://user.auth.xboxlive.com/user/authenticate";
const std::string XboxAuth::XBOX_XSTS_AUTHORIZE_URL = "https://xsts.auth.xboxlive.com/xsts/authorize";

const std::string XboxAuth::LIVE_TOKENS_FILENAME_DEF = "xbox_live_tokens.json";
const std::string XboxAuth::XSTS_TOKENS_FILENAME_DEF = "xbox_xsts_tokens.json";
const std::string XboxAuth::USER_INFO_FILENAME_DEF = "xbox_user_info.json"; // File per XUID, UHS

// Helper per leggere/scrivere file (puoi usare quelli che hai già se sono globali)
static std::string readFileToStringLocal(const std::filesystem::path& p) {
    std::ifstream file(p, std::ios::binary); // Apri in binario per consistenza
    std::string content;
    if (file) {
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());
    } else {
        LOG(LogDebug) << "XboxAuth: Failed to open file for reading or file empty: " << p.string();
    }
    return content;
}

static bool writeStringToFileLocal(const std::filesystem::path& p, const std::string& content) {
    std::ofstream file(p, std::ios::binary); // Apri in binario
    if (file) {
        file.write(content.c_str(), content.length());
        return true;
    }
    LOG(LogError) << "XboxAuth: Failed to open file for writing: " << p.string();
    return false;
}


XboxAuth::XboxAuth(std::function<void(const std::string&)> setStateCallback)
    : mSetStateCallback(setStateCallback), 
      mLiveTokenExpiry(std::chrono::system_clock::from_time_t(0)), // Inizializza i time_point
      mXstsTokenExpiry(std::chrono::system_clock::from_time_t(0)),
      mHasTriedAutoLogin(false) {
    // Utilizza il metodo esistente Paths::getUserEmulationStationPath()
    mTokenStoragePath = ::Paths::getUserEmulationStationPath(); 
    
    if (!mTokenStoragePath.empty() && (mTokenStoragePath.back() == '/' || mTokenStoragePath.back() == '\\')) {
        // Se il percorso termina già con un separatore, aggiungi solo il nome della sottocartella
        mTokenStoragePath += "xbox_tokens";
    } else if (!mTokenStoragePath.empty()) { 
        // Altrimenti, aggiungi il separatore e poi il nome della sottocartella
        mTokenStoragePath += "/xbox_tokens"; 
    } else {
        LOG(LogError) << "XboxAuth: User EmulationStation path (from Paths::getUserEmulationStationPath()) is empty, cannot determine token storage path.";
        // mTokenStoragePath rimarrà vuoto; le operazioni di salvataggio/caricamento falliranno con un log.
    }
    
    if (!mTokenStoragePath.empty() && !Utils::FileSystem::exists(mTokenStoragePath)) {
        if (!Utils::FileSystem::createDirectory(mTokenStoragePath)) {
            LOG(LogError) << "XboxAuth: Failed to create token storage directory: " << mTokenStoragePath;
        }
    }
    LOG(LogInfo) << "XboxAuth initialized. Token storage path: " << mTokenStoragePath; // Cambiato LogDebug a LogInfo per visibilità
}


bool XboxAuth::isAuthenticated() const {
    if (mXstsToken.empty()) return false;
    return std::chrono::system_clock::now() < mXstsTokenExpiry;
}

std::string XboxAuth::getAuthorizationUrl(std::string& state_out) {
    // Per il flusso manuale, non usiamo lo state come per Epic, ma l'URL è fisso
    // Potresti voler generare uno state se il tuo HttpReq ne avesse bisogno per qualche motivo
    // state_out = generateRandomState(); // Se necessario
    std::string authUrl = LIVE_AUTHORIZE_URL +
                          "?client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                          "&response_type=code" +
                          "&approval_prompt=auto" + // O "force" se vuoi sempre il prompt
                          "&scope=" + HttpReq::urlEncode(SCOPE) +
                          "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI);
    LOG(LogDebug) << "XboxAuth: Generated Authorization URL: " << authUrl;
    return authUrl;
}

bool XboxAuth::exchangeAuthCodeForTokens(const std::string& authCode) {
    LOG(LogInfo) << "XboxAuth: Exchanging authorization code for Live tokens...";
    if (authCode.empty()) {
        LOG(LogError) << "XboxAuth: Authorization code is empty.";
        return false;
    }

    HttpReqOptions options;
    std::string postData = "grant_type=authorization_code"
                           "&code=" + HttpReq::urlEncode(authCode) +
                           "&scope=" + HttpReq::urlEncode(SCOPE) +
                           "&client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                           "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI);

    options.dataToPost = postData;
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    HttpReq request(LIVE_TOKEN_URL, &options);
    if (!request.wait()) {
        LOG(LogError) << "XboxAuth: Live token request failed (wait): " << request.getErrorMsg() << " Body: " << request.getContent();
        clearAllTokenData();
        return false;
    }

    if (request.status() == HttpReq::REQ_SUCCESS || request.status() == 200) {
        try {
            nj::json responseJson = nj::json::parse(request.getContent());
            if (responseJson.contains("error")) {
                LOG(LogError) << "XboxAuth: Live token API error: "
                              << responseJson.value("error", "") << " - "
                              << responseJson.value("error_description", "");
                clearAllTokenData();
                return false;
            }

            mLiveAccessToken = responseJson.value("access_token", "");
            mLiveRefreshToken = responseJson.value("refresh_token", "");
            int expiresIn = responseJson.value("expires_in", 0);
            mLiveTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn - 60); // Margine di sicurezza

            if (mLiveAccessToken.empty() || expiresIn <= 0) {
                LOG(LogError) << "XboxAuth: Essential Live token data missing from response.";
                clearAllTokenData();
                return false;
            }
            LOG(LogInfo) << "XboxAuth: Successfully obtained Live tokens.";
            saveTokenData(); // Salva i token Live
            return authenticateXSTS(); // Prosegui con l'autenticazione XSTS

        } catch (const nj::json::parse_error& e) {
            LOG(LogError) << "XboxAuth: JSON parse error in Live token response: " << e.what() << ". Response: " << request.getContent();
            clearAllTokenData();
            return false;
        }
    } else {
        LOG(LogError) << "XboxAuth: Live token request failed. Status: " << request.status()
                      << ", Body: " << request.getContent();
        clearAllTokenData();
        return false;
    }
}

bool XboxAuth::authenticateXSTS() {
    if (mLiveAccessToken.empty()) {
        LOG(LogError) << "XboxAuth: Cannot authenticate XSTS without a Live access token.";
        return false;
    }
    LOG(LogInfo) << "XboxAuth: Authenticating for XSTS token...";

    // Step 1: User Authentication (ottiene il primo token XSTS/User)
    std::string firstXstsToken;
    {
        HttpReqOptions optionsStep1;
        nj::json requestBodyStep1 = {
            {"RelyingParty", "http://auth.xboxlive.com"},
            {"TokenType", "JWT"},
            {"Properties", {
                {"AuthMethod", "RPS"},
                {"SiteName", "user.auth.xboxlive.com"},
                {"RpsTicket", "d=" + mLiveAccessToken} // Il "d=" è importante
            }}
        };
        optionsStep1.dataToPost = requestBodyStep1.dump();
        optionsStep1.customHeaders.push_back("Content-Type: application/json");
        optionsStep1.customHeaders.push_back("Accept: application/json");
        optionsStep1.customHeaders.push_back("x-xbl-contract-version: 1"); // O 0 secondo Playnite

        HttpReq requestStep1(XBOX_USER_AUTHENTICATE_URL, &optionsStep1);
        if (!requestStep1.wait()) {
            LOG(LogError) << "XboxAuth: XSTS Step 1 request failed (wait): " << requestStep1.getErrorMsg();
            return false;
        }

        if (requestStep1.status() != 200) {
            LOG(LogError) << "XboxAuth: XSTS Step 1 failed. Status: " << requestStep1.status() << " Body: " << requestStep1.getContent();
            return false;
        }

        try {
            nj::json responseJson = nj::json::parse(requestStep1.getContent());
            if (responseJson.contains("Token")) {
                firstXstsToken = responseJson["Token"].get<std::string>();
            } else {
                LOG(LogError) << "XboxAuth: XSTS Step 1 response missing 'Token'.";
                return false;
            }
        } catch (const nj::json::parse_error& e) {
            LOG(LogError) << "XboxAuth: JSON parse error in XSTS Step 1: " << e.what();
            return false;
        }
    }

    // Step 2: XSTS Authorization (ottiene il token XSTS finale)
    {
        HttpReqOptions optionsStep2;
        nj::json requestBodyStep2 = {
            {"RelyingParty", "http://xboxlive.com"}, // O "https://xsts.auth.xboxlive.com" o altri a seconda dell'audience
            {"TokenType", "JWT"},
            {"Properties", {
                {"SandboxId", "RETAIL"}, // Standard per la maggior parte degli utenti
                {"UserTokens", {firstXstsToken}}
            }}
        };
        optionsStep2.dataToPost = requestBodyStep2.dump();
        optionsStep2.customHeaders.push_back("Content-Type: application/json");
        optionsStep2.customHeaders.push_back("Accept: application/json");
        optionsStep2.customHeaders.push_back("x-xbl-contract-version: 1");

        HttpReq requestStep2(XBOX_XSTS_AUTHORIZE_URL, &optionsStep2);
        if (!requestStep2.wait()) {
            LOG(LogError) << "XboxAuth: XSTS Step 2 request failed (wait): " << requestStep2.getErrorMsg();
            return false;
        }

        if (requestStep2.status() != 200) {
            LOG(LogError) << "XboxAuth: XSTS Step 2 failed. Status: " << requestStep2.status() << " Body: " << requestStep2.getContent();
            // Qui potresti controllare errori specifici come 401 (Unauthorized) o altri codici errore Xbox.
            // Se il token è scaduto (XErr 2148916233), dovresti tentare un refresh del token Live.
            // Se è XErr 2148916238 (utente non ha account Xbox), allora non c'è nulla da fare.
            std::string content = requestStep2.getContent();
            if (content.find("2148916233") != std::string::npos || content.find("2148916236") != std::string::npos ) { // Token scaduto o utente deve accettare termini
                LOG(LogWarning) << "XboxAuth: XSTS token potentially expired or needs user action. Attempting Live token refresh.";
                return refreshTokens(); // Tenta un refresh completo
            }
            clearAllTokenData(); // Pulisci se l'errore XSTS è definitivo per questa sessione
            return false;
        }

        try {
            nj::json responseJson = nj::json::parse(requestStep2.getContent());
            mXstsToken = responseJson.value("Token", "");

            if (responseJson.contains("DisplayClaims") && responseJson["DisplayClaims"].is_object() &&
                responseJson["DisplayClaims"].contains("xui") && responseJson["DisplayClaims"]["xui"].is_array() &&
                !responseJson["DisplayClaims"]["xui"].empty()) {
                const auto& xuiClaims = responseJson["DisplayClaims"]["xui"][0];
                mUserXUID = xuiClaims.value("xid", ""); // Xbox User ID
                mUserHash = xuiClaims.value("uhs", ""); // User Hash
            }

            // Calcola expiry per XSTS (NotAfter - IssueInstant, o usa un valore fisso se non fornito)
            // Playnite sembra non fare affidamento su un `expires_in` esplicito per XSTS, ma rinfresca
            // quando una chiamata API fallisce o all'avvio. XSTS tokens sono generalmente di breve durata.
            // Per semplicità, diamo una scadenza (es. 1 ora) e ci affidiamo al refresh.
            mXstsTokenExpiry = std::chrono::system_clock::now() + std::chrono::hours(1); // Esempio: 1 ora

            if (mXstsToken.empty() || mUserXUID.empty() || mUserHash.empty()) {
                LOG(LogError) << "XboxAuth: Essential XSTS data missing from response.";
                clearAllTokenData();
                return false;
            }

            LOG(LogInfo) << "XboxAuth: Successfully obtained XSTS token. XUID: " << mUserXUID;
            saveTokenData(); // Salva tutti i token e info utente
            if (mSetStateCallback) mSetStateCallback("Authenticated");
            return true;

        } catch (const nj::json::parse_error& e) {
            LOG(LogError) << "XboxAuth: JSON parse error in XSTS Step 2: " << e.what();
            clearAllTokenData();
            return false;
        }
    }
}

bool XboxAuth::refreshTokens() {
    LOG(LogInfo) << "XboxAuth: Attempting to refresh Live tokens...";
    if (mLiveRefreshToken.empty()) {
        LOG(LogError) << "XboxAuth: No Live refresh token available. Cannot refresh.";
        return false; // Richiedi nuovo login
    }

    HttpReqOptions options;
    std::string postData = "grant_type=refresh_token"
                           "&refresh_token=" + HttpReq::urlEncode(mLiveRefreshToken) +
                           "&scope=" + HttpReq::urlEncode(SCOPE) +
                           "&client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                           "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI); // Anche se non è un redirect, alcune API lo vogliono

    options.dataToPost = postData;
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    HttpReq request(LIVE_TOKEN_URL, &options);
    if (!request.wait()) {
        LOG(LogError) << "XboxAuth: Live token refresh request failed (wait): " << request.getErrorMsg();
        clearAllTokenData(); // Se il refresh fallisce, i vecchi token sono probabilmente invalidi
        return false;
    }

    if (request.status() == HttpReq::REQ_SUCCESS || request.status() == 200) {
        try {
            nj::json responseJson = nj::json::parse(request.getContent());
             if (responseJson.contains("error")) {
                LOG(LogError) << "XboxAuth: Live token refresh API error: "
                              << responseJson.value("error", "") << " - "
                              << responseJson.value("error_description", "");
                clearAllTokenData(); // Il refresh token potrebbe essere stato revocato
                return false;
            }
            mLiveAccessToken = responseJson.value("access_token", "");
            // Alcune risposte di refresh potrebbero non restituire un nuovo refresh_token.
            // Se lo fanno, aggiornalo. Altrimenti, mantieni quello vecchio.
            if (responseJson.contains("refresh_token")) {
                mLiveRefreshToken = responseJson.value("refresh_token", mLiveRefreshToken);
            }
            int expiresIn = responseJson.value("expires_in", 0);
            mLiveTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn - 60);

            if (mLiveAccessToken.empty() || expiresIn <= 0) {
                LOG(LogError) << "XboxAuth: Essential data missing from Live token refresh response.";
                clearAllTokenData();
                return false;
            }

            LOG(LogInfo) << "XboxAuth: Live tokens successfully refreshed.";
            saveTokenData(); // Salva i nuovi token Live
            return authenticateXSTS(); // Ottieni un nuovo token XSTS con il nuovo token Live

        } catch (const nj::json::parse_error& e) {
            LOG(LogError) << "XboxAuth: JSON parse error in Live token refresh response: " << e.what();
            clearAllTokenData();
            return false;
        }
    } else {
        LOG(LogError) << "XboxAuth: Live token refresh request failed. Status: " << request.status() << " Body: " << request.getContent();
        clearAllTokenData();
        return false;
    }
}

void XboxAuth::loadTokenData() {
std::string liveTokenFilePath = mTokenStoragePath + "/" + LIVE_TOKENS_FILENAME_DEF; // Correzione: concatenazione manuale
    std::string liveTokenContent = readFileToStringLocal(liveTokenFilePath);
    if (!liveTokenContent.empty()) {
        try {
            nj::json j = nj::json::parse(liveTokenContent);
            mLiveAccessToken = j.value("live_access_token", "");
            mLiveRefreshToken = j.value("live_refresh_token", "");
            long long expiry_t = j.value("live_expiry_timestamp", 0LL);
            mLiveTokenExpiry = std::chrono::system_clock::from_time_t(static_cast<time_t>(expiry_t));
        } catch (const std::exception& e) {
            LOG(LogError) << "XboxAuth: Error parsing Live tokens file: " << e.what();
            clearAllTokenData(); // Pulisci se il file è corrotto
        }
    }

    std::string xstsTokenFilePath = mTokenStoragePath + "/" + XSTS_TOKENS_FILENAME_DEF; // Correzione: concatenazione manuale
    std::string xstsTokenContent = readFileToStringLocal(xstsTokenFilePath);
    if (!xstsTokenContent.empty()) {
        try {
            nj::json j = nj::json::parse(xstsTokenContent);
            mXstsToken = j.value("xsts_token", "");
            long long expiry_t = j.value("xsts_expiry_timestamp", 0LL);
            mXstsTokenExpiry = std::chrono::system_clock::from_time_t(static_cast<time_t>(expiry_t));
            mUserXUID = j.value("xuid", "");
            mUserHash = j.value("uhs", "");
        } catch (const std::exception& e) {
            LOG(LogError) << "XboxAuth: Error parsing XSTS tokens file: " << e.what();
            clearAllTokenData(); // Pulisci se il file è corrotto
        }
    }

    if (isAuthenticated()) {
        LOG(LogInfo) << "XboxAuth: Successfully loaded and validated XSTS tokens. XUID: " << mUserXUID;
        if (mSetStateCallback) mSetStateCallback("Authenticated");
    } else if (!mLiveRefreshToken.empty()) { // Se XSTS non è valido ma abbiamo un refresh token Live
        LOG(LogInfo) << "XboxAuth: XSTS token invalid or expired, attempting refresh...";
        if (!refreshTokens()) { // refreshTokens chiamerà authenticateXSTS e saveTokenData se ha successo
             if (mSetStateCallback) mSetStateCallback("Authentication Failed");
        }
    } else {
        LOG(LogInfo) << "XboxAuth: No valid tokens loaded.";
         if (mSetStateCallback) mSetStateCallback("Not Authenticated");
    }
}

void XboxAuth::saveTokenData() {
    nj::json live_tokens_json;
    if (!mLiveAccessToken.empty()) live_tokens_json["live_access_token"] = mLiveAccessToken;
    if (!mLiveRefreshToken.empty()) live_tokens_json["live_refresh_token"] = mLiveRefreshToken;
	live_tokens_json["live_expiry_timestamp"] = std::chrono::system_clock::to_time_t(mLiveTokenExpiry);
     writeStringToFileLocal(mTokenStoragePath + "/" + LIVE_TOKENS_FILENAME_DEF, live_tokens_json.dump(2)); // Correzione: concatenazione manuale


    nj::json xsts_tokens_json;
    if (!mXstsToken.empty()) xsts_tokens_json["xsts_token"] = mXstsToken;
    xsts_tokens_json["xsts_expiry_timestamp"] = std::chrono::system_clock::to_time_t(mXstsTokenExpiry);
    if (!mUserXUID.empty()) xsts_tokens_json["xuid"] = mUserXUID;
    if (!mUserHash.empty()) xsts_tokens_json["uhs"] = mUserHash;
    writeStringToFileLocal(mTokenStoragePath + "/" + XSTS_TOKENS_FILENAME_DEF, xsts_tokens_json.dump(2)); // Correzione: concatenazione manuale

    LOG(LogDebug) << "XboxAuth: Token data saved.";
}

void XboxAuth::clearAllTokenData() {
    LOG(LogInfo) << "XboxAuth: Clearing all token data.";
    mLiveAccessToken.clear();
    mLiveRefreshToken.clear();
    mLiveTokenExpiry = std::chrono::system_clock::from_time_t(0);
    mXstsToken.clear();
    mXstsTokenExpiry = std::chrono::system_clock::from_time_t(0);
    mUserXUID.clear();
    mUserHash.clear();

    Utils::FileSystem::removeFile(mTokenStoragePath + "/" + LIVE_TOKENS_FILENAME_DEF); // Correzione: concatenazione manuale
    Utils::FileSystem::removeFile(mTokenStoragePath + "/" + XSTS_TOKENS_FILENAME_DEF); // Correzione: concatenazione manuale
    // Utils::FileSystem::removeFile((mTokenStoragePath / USER_INFO_FILENAME_DEF).string()); // Se usi questo file separato

    if (mSetStateCallback) mSetStateCallback("Not Authenticated");
}

std::string XboxAuth::getLiveAccessToken() const { return mLiveAccessToken; }
std::string XboxAuth::getXstsToken() const { return mXstsToken; }
std::string XboxAuth::getXUID() const { return mUserXUID; }
std::string XboxAuth::getUserHash() const { return mUserHash; }
#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "Log.h"
#include "HttpReq.h"      // Per le richieste HTTP
#include "json.hpp"       // Per nlohmann/json
#include "utils/base64.h" // Per base64_encode
#include "utils/FileSystemUtil.h"
#include "Paths.h"        // Per Utils::FileSystem::getEsConfigPath() è in FileSystemUtil.h o Paths.h?

#include <fstream>
#include <iomanip>        // Per std::put_time
#include <vector>
#include <sstream>        // Per std::ostringstream
#include <random> // <<--- AGGIUNGI QUESTA RIGA

using json = nlohmann::json;

// Definizione dei nomi file statici (se dichiarati static nell'header)
const std::string EpicGamesAuth::ACCESS_TOKEN_FILENAME_DEF = "epic_access_token.txt";
const std::string EpicGamesAuth::REFRESH_TOKEN_FILENAME_DEF = "epic_refresh_token.txt";
const std::string EpicGamesAuth::ACCOUNT_ID_FILENAME_DEF = "epic_account_id.txt";
const std::string EpicGamesAuth::EXPIRY_FILENAME_DEF = "epic_expiry.txt";


// Funzioni helper per leggere/scrivere file (simili al tuo codice)
std::string readFileToStringLocalCpp(const std::filesystem::path& p) {
    std::ifstream file(p);
    std::string content;
    if (file) {
        content.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    } else {
        LOG(LogDebug) << "readFileToStringLocalCpp: Failed to open file or file empty: " << p.string();
    }
    return content;
}

bool writeStringToFileLocalCpp(const std::filesystem::path& p, const std::string& content) {
    std::ofstream file(p);
    if (file) {
        file << content;
        return true;
    }
    LOG(LogError) << "writeStringToFileLocalCpp: Failed to open file for writing: " << p.string();
    return false;
}

// Costruttori (dal tuo codice)
EpicGamesAuth::EpicGamesAuth(std::function<void(const std::string&)> setStateCallback)
    : mSetStateCallback(setStateCallback), mHasValidTokenInfo(false) {
    mTokenStoragePath = Utils::FileSystem::getEsConfigPath(); // Assumendo che questo restituisca il path corretto
     if (!std::filesystem::exists(mTokenStoragePath)) {
        // CORREZIONE: Utils::FileSystem::createDirectory si aspetta std::string
        if (!Utils::FileSystem::createDirectory(mTokenStoragePath.string())) {
             LOG(LogError) << "Failed to create token storage directory: " << mTokenStoragePath.string();
        }
    }
    LOG(LogDebug) << "EpicGamesAuth(callback) initialized. Token storage path: " << mTokenStoragePath.string();
    loadTokenData();
}

EpicGamesAuth::EpicGamesAuth() : mSetStateCallback(nullptr), mHasValidTokenInfo(false) {
    mTokenStoragePath = Utils::FileSystem::getEsConfigPath();
    if (!std::filesystem::exists(mTokenStoragePath)) {
        // CORREZIONE: Utils::FileSystem::createDirectory si aspetta std::string
        if (!Utils::FileSystem::createDirectory(mTokenStoragePath.string())) {
            LOG(LogError) << "Failed to create token storage directory: " << mTokenStoragePath.string();
        }
    }
    LOG(LogDebug) << "EpicGamesAuth() initialized. Token storage path: " << mTokenStoragePath.string();
    loadTokenData();
}

EpicGamesAuth::~EpicGamesAuth() {}

// Metodo per scambiare auth code con token (precedentemente getAccessToken con 2 argomenti nel tuo codice)
bool EpicGamesAuth::exchangeAuthCodeForToken(const std::string& authCode) {
    LOG(LogInfo) << "Exchanging authorization code for tokens...";
    HttpReqOptions options;
    // CORREZIONE: HttpReqOptions non ha 'method' o 'postParams'. Usa 'dataToPost'.
    
    std::string authString = EPIC_CLIENT_ID + ":" + EPIC_CLIENT_SECRET;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());
    
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    // Costruisci la stringa dataToPost
    std::ostringstream postDataStream;
    postDataStream << "grant_type=authorization_code"
                   << "&code=" << HttpReq::urlEncode(authCode) // Assicurati di fare l'urlencode del codice
                   << "&token_type=eg1";
    options.dataToPost = postDataStream.str();

    HttpReq request(TOKEN_ENDPOINT_URL, &options);
    if (!request.wait()) {
        LOG(LogError) << "HTTP request failed during authorization code exchange (wait): " << request.getErrorMsg() << " Body: " << request.getContent();
        clearAllTokenData();
        return false;
    }

    if (request.status() == HttpReq::REQ_SUCCESS || request.status() == 200) {
        try {
            return processTokenResponse(json::parse(request.getContent()), false);
        } catch (const json::parse_error& e) {
            LOG(LogError) << "JSON parse error in exchangeAuthCodeForToken: " << e.what() << ". Response: " << request.getContent();
            clearAllTokenData();
            return false;
        }
    } else {
        LOG(LogError) << "Authorization code exchange failed. Status: " << request.status()
                      << ", Body: " << request.getContent();
        clearAllTokenData();
        return false;
    }
}

// --- Implementazione di refreshAccessToken CORRETTA per il tuo HttpReqOptions ---
bool EpicGamesAuth::refreshAccessToken() {
    if (mRefreshToken.empty()) {
        LOG(LogWarning) << "No refresh token available. Cannot refresh access token.";
        return false;
    }

    LOG(LogInfo) << "Attempting to refresh access token using refresh_token...";
    HttpReqOptions options;
    // CORREZIONE: HttpReqOptions non ha 'method' o 'postParams'. Usa 'dataToPost'.

    std::string authString = EPIC_CLIENT_ID + ":" + EPIC_CLIENT_SECRET;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());

    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    
    // Costruisci la stringa dataToPost per il refresh
    std::ostringstream postDataStream;
    postDataStream << "grant_type=refresh_token"
                   << "&refresh_token=" << HttpReq::urlEncode(mRefreshToken) // Assicurati di fare l'urlencode
                   << "&token_type=eg1"; // Manteniamo per coerenza
    options.dataToPost = postDataStream.str();


    HttpReq request(TOKEN_ENDPOINT_URL, &options);
    if (!request.wait()) {
        LOG(LogError) << "HTTP request failed during token refresh (wait): " << request.getErrorMsg() << " Body: " << request.getContent();
        return false; 
    }

    std::string responseBody = request.getContent();
    if (request.status() == HttpReq::REQ_SUCCESS || request.status() == 200) {
        LOG(LogInfo) << "Token refresh HTTP request successful. Processing response...";
        try {
            return processTokenResponse(json::parse(responseBody), true);
        } catch (const json::parse_error& e) {
            LOG(LogError) << "JSON parse error in refreshAccessToken: " << e.what() << ". Response: " << responseBody;
            return false;
        }
    } else {
        LOG(LogError) << "Token refresh HTTP request failed. Status: " << request.status() << ", Body: " << responseBody;
        try {
            json errorJson = json::parse(responseBody);
            // Adatta il controllo dell'errore ai campi JSON effettivi restituiti da Epic in caso di fallimento del refresh
            std::string errorCode = errorJson.value("errorCode", ""); // Formato Epic
            std::string errorOAuth = errorJson.value("error", "");    // Formato OAuth standard
            
            if (errorCode == "errors.com.epicgames.common.oauth.invalid_grant" || errorOAuth == "invalid_grant") {
                LOG(LogWarning) << "Refresh token is invalid or expired (invalid_grant). Clearing all tokens.";
                clearAllTokenData();
            } else {
                 LOG(LogError) << "Token refresh API error. Code: " << errorCode << ", OAuthError: " << errorOAuth
                               << ", Desc: " << errorJson.value("errorMessage", errorJson.value("error_description", "N/A"));
            }
        } catch (const json::parse_error& e) {
            LOG(LogError) << "Failed to parse error JSON from failed refresh token response: " << e.what();
        }
        return false;
    }
}

// --- Metodo processTokenResponse (simile a prima, ma adattato per essere chiamato da entrambi) ---
bool EpicGamesAuth::processTokenResponse(const nlohmann::json& response, bool isRefreshing) {
    try {
        // Log dell'intera risposta JSON per debug (puoi rimuoverlo in produzione)
        LOG(LogDebug) << "Processing token response JSON: " << response.dump(2);

        if (response.contains("error") || response.contains("errorCode")) {
            std::string error = response.value("error", response.value("errorCode", "unknown_token_error"));
            std::string errorDesc = response.value("error_description", response.value("errorMessage", "No description"));
            LOG(LogError) << "OAuth/Epic Token Error in response: " << error << " - " << errorDesc;
            // Non cancellare i token qui automaticamente, il chiamante decide.
            // Ma segna che non abbiamo ottenuto dati validi.
            mHasValidTokenInfo = false; 
            return false;
        }

        std::string newAccessToken = response.value("access_token", "");
        std::string newAccountId = response.value("account_id", "");
        std::string newDisplayName = response.value("displayName", ""); // Potrebbe essere vuoto
        int expiresIn = response.value("expires_in", 0);

        // Il tuo codice originale non usava `newDisplayName` se non c'era, quindi lo rimuovo da qui per ora
        // Se vuoi mantenere il vecchio display name, la logica andrebbe qui.

        if (newAccessToken.empty() || newAccountId.empty() || expiresIn <= 0) {
            LOG(LogError) << "Essential token data (access_token, account_id, expires_in) missing or invalid in JSON response.";
            mHasValidTokenInfo = false;
            return false;
        }

        mAccessToken = newAccessToken;
        mAccountId = newAccountId;
        mDisplayName = newDisplayName; // Salva il display name se presente

        if (response.contains("refresh_token") && !response["refresh_token"].get<std::string>().empty()) {
            mRefreshToken = response["refresh_token"].get<std::string>();
        } else if (!isRefreshing && mRefreshToken.empty()) { // Solo se è lo scambio iniziale e non otteniamo un refresh token
            LOG(LogWarning) << "Initial token exchange did not provide a refresh_token. Future refreshes might not be possible if this is expected.";
        }
        // Se è un refresh e non c'è un nuovo refresh_token, quello vecchio in mRefreshToken rimane.

        mTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn);
        mTokenExpiry -= std::chrono::seconds(60); // Margine di sicurezza
        mHasValidTokenInfo = true;

        time_t expiry_tt_log = std::chrono::system_clock::to_time_t(mTokenExpiry + std::chrono::seconds(60));
        LOG(LogInfo) << "Tokens processed. Account ID: " << mAccountId 
                     << ". Access token valid until approx: " << std::put_time(std::localtime(&expiry_tt_log), "%Y-%m-%d %H:%M:%S %Z");

        saveTokenData();
        return true;

    } catch (const json::exception& e) { // json::exception è più generico di json::parse_error
        LOG(LogError) << "JSON exception in processTokenResponse: " << e.what();
        mHasValidTokenInfo = false;
        return false;
    }
}

// --- Salvataggio e Caricamento (CORRETTI per usare .string() con Utils::FileSystem) ---
void EpicGamesAuth::saveTokenData() {
    if (!mHasValidTokenInfo && mAccessToken.empty()) {
        LOG(LogDebug) << "saveTokenData: No valid token info. Clearing files.";
        // La chiamata a clearAllTokenData qui potrebbe essere problematica se è chiamata da clearAllTokenData.
        // Meglio solo non scrivere nulla se non c'è nulla da scrivere.
        // Ma il tuo codice originale lo faceva, quindi lo lascio per ora.
        // clearAllTokenData(); // Attenzione a ricorsione se clearAllTokenData chiama saveTokenData
        // Invece di chiamare clearAllTokenData, assicurati che i file vengano cancellati se i token sono vuoti.
        if (mAccessToken.empty()) Utils::FileSystem::removeFile((mTokenStoragePath / ACCESS_TOKEN_FILENAME_DEF).string());
        if (mRefreshToken.empty()) Utils::FileSystem::removeFile((mTokenStoragePath / REFRESH_TOKEN_FILENAME_DEF).string());
        // ... e così via
        return;
    }
     if (mTokenStoragePath.empty()) {
        LOG(LogError) << "Token storage path is not set. Cannot save tokens.";
        return;
    }
    LOG(LogInfo) << "Saving Epic token data to path: " << mTokenStoragePath.string();
    bool success = true;
    // CORREZIONE: Usa .string() per Utils::FileSystem
    success &= writeStringToFileLocalCpp(mTokenStoragePath / ACCESS_TOKEN_FILENAME_DEF, mAccessToken);
    success &= writeStringToFileLocalCpp(mTokenStoragePath / REFRESH_TOKEN_FILENAME_DEF, mRefreshToken);
    success &= writeStringToFileLocalCpp(mTokenStoragePath / ACCOUNT_ID_FILENAME_DEF, mAccountId);
    // Salva anche DisplayName se lo usi
    // success &= writeStringToFileLocalCpp(mTokenStoragePath / "epic_display_name.txt", mDisplayName);


    time_t expiryTimestamp = std::chrono::system_clock::to_time_t(mTokenExpiry);
    success &= writeStringToFileLocalCpp(mTokenStoragePath / EXPIRY_FILENAME_DEF, std::to_string(expiryTimestamp));

    if (success) LOG(LogInfo) << "Epic token data successfully saved.";
    else LOG(LogError) << "Failed to save one or more Epic token files.";
}

bool EpicGamesAuth::loadTokenData() {
    if (mTokenStoragePath.empty()) {
        LOG(LogError) << "Token storage path is not set. Cannot load tokens.";
        return false;
    }
    LOG(LogInfo) << "Loading Epic token data from path: " << mTokenStoragePath.string();
    // CORREZIONE: Usa .string() per Utils::FileSystem
    mAccessToken = readFileToStringLocalCpp(mTokenStoragePath / ACCESS_TOKEN_FILENAME_DEF);
    mRefreshToken = readFileToStringLocalCpp(mTokenStoragePath / REFRESH_TOKEN_FILENAME_DEF);
    mAccountId = readFileToStringLocalCpp(mTokenStoragePath / ACCOUNT_ID_FILENAME_DEF);
    // mDisplayName = readFileToStringLocalCpp(mTokenStoragePath / "epic_display_name.txt");
    std::string expiryStr = readFileToStringLocalCpp(mTokenStoragePath / EXPIRY_FILENAME_DEF);

    if (mAccessToken.empty() || mAccountId.empty() || expiryStr.empty()) {
        LOG(LogInfo) << "Essential token data not found or empty. Assuming no valid saved token.";
        // Non chiamare clearAllTokenData qui, perché questo metodo viene chiamato anche dal costruttore.
        // Resetta solo i membri e mHasValidTokenInfo.
        mAccessToken.clear(); mRefreshToken.clear(); mAccountId.clear(); mDisplayName.clear();
        mTokenExpiry = std::chrono::system_clock::from_time_t(0);
        mHasValidTokenInfo = false;
        return false;
    }

    try {
        time_t expiryTimestamp = std::stoll(expiryStr);
        mTokenExpiry = std::chrono::system_clock::from_time_t(expiryTimestamp);
        mHasValidTokenInfo = true;
        
        LOG(LogInfo) << "Tokens loaded. Access token was set to expire at: " << std::put_time(std::localtime(&expiryTimestamp), "%Y-%m-%d %H:%M:%S %Z");

        if (!isAuthenticated()) {
            LOG(LogWarning) << "Loaded access token is expired.";
            if (!mRefreshToken.empty()) {
                LOG(LogInfo) << "Attempting to refresh expired token immediately after load...";
                if (refreshAccessToken()) {
                    LOG(LogInfo) << "Successfully refreshed token after load."; // isAuthenticated ora dovrebbe essere true
                } else {
                    LOG(LogError) << "Failed to refresh expired token after load. User may need to log in again.";
                    // Se refreshAccessToken ha fallito a causa di invalid_grant, clearAllTokenData è già stato chiamato.
                    // Altrimenti, i token (scaduti ma non puliti) sono ancora in memoria.
                    // Considera di pulirli se il refresh fallisce per motivi diversi da invalid_grant.
                    // Per ora, lo stato rifletterà il fallimento del refresh.
                    mHasValidTokenInfo = false; // Aggiorna lo stato dopo il tentativo di refresh
                }
            } else {
                LOG(LogWarning) << "Loaded access token is expired, and no refresh token is available. Clearing tokens.";
                clearAllTokenData(); // Senza refresh token, non c'è speranza, pulisci.
            }
        }
        return mHasValidTokenInfo && isAuthenticated(); // Ritorna lo stato di autenticazione corrente

    } catch (const std::exception& e) {
        LOG(LogError) << "Error processing loaded token data (expiry conversion): " << e.what();
        clearAllTokenData(); // File corrotto o illeggibile
        return false;
    }
}

void EpicGamesAuth::clearAllTokenData() {
    LOG(LogInfo) << "Clearing all Epic token data from memory and disk.";
    mAccessToken.clear();
    mRefreshToken.clear();
    mAccountId.clear();
    mDisplayName.clear();
    mTokenExpiry = std::chrono::system_clock::from_time_t(0);
    mHasValidTokenInfo = false;

    if (!mTokenStoragePath.empty()) {
        // CORREZIONE: Usa .string() per Utils::FileSystem
        Utils::FileSystem::removeFile((mTokenStoragePath / ACCESS_TOKEN_FILENAME_DEF).string());
        Utils::FileSystem::removeFile((mTokenStoragePath / REFRESH_TOKEN_FILENAME_DEF).string());
        Utils::FileSystem::removeFile((mTokenStoragePath / ACCOUNT_ID_FILENAME_DEF).string());
        Utils::FileSystem::removeFile((mTokenStoragePath / EXPIRY_FILENAME_DEF).string());
        // Utils::FileSystem::removeFile((mTokenStoragePath / "epic_display_name.txt").string());
    } else {
        LOG(LogWarning) << "Token storage path not set, cannot delete token files.";
    }
}

// --- Getters ---
std::string EpicGamesAuth::getAccessToken() const { return mAccessToken; }
std::string EpicGamesAuth::getRefreshToken() const { return mRefreshToken; }
std::string EpicGamesAuth::getAccountId() const { return mAccountId; }
std::string EpicGamesAuth::getDisplayName() const { return mDisplayName; }
std::chrono::time_point<std::chrono::system_clock> EpicGamesAuth::getTokenExpiry() const { return mTokenExpiry; }

bool EpicGamesAuth::isAuthenticated() const {
    if (!mHasValidTokenInfo || mAccessToken.empty()) {
        return false;
    }
    return std::chrono::system_clock::now() < mTokenExpiry;
}

std::string EpicGamesAuth::getAuthorizationUrl(std::string& /*state_out_param_not_really_used*/) {
    LOG(LogInfo) << "Generating Epic Games authorization URL for manual code entry.";
    return AUTHORIZE_URL_MANUAL_CODE;
}

std::string EpicGamesAuth::generateRandomState() {
    // La tua implementazione esistente qui...
    // (copiala dal tuo file se è diversa da quella generica che avevo messo)
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int STATE_LENGTH = 32;
    std::string stateStr;
    stateStr.reserve(STATE_LENGTH);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, sizeof(alphanum) - 2);
    for (int i = 0; i < STATE_LENGTH; ++i) {
        stateStr += alphanum[distrib(gen)];
    }
    return stateStr;
}
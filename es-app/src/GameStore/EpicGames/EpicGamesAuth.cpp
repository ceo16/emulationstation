// Sostituisci completamente il tuo emulationstation-master/es-app/src/GameStore/EpicGames/EpicGamesAuth.cpp

#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/base64.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Paths.h"
#include "views/ViewController.h" // Aggiunto per ricaricare la UI

#include <fstream>
#include <iomanip>

using json = nlohmann::json;

// Definizione delle costanti statiche
const std::string EpicGamesAuth::EPIC_CLIENT_ID = "34a02cf8f4414e29b15921876da36f9a";
const std::string EpicGamesAuth::EPIC_CLIENT_SECRET = "daafbccc737745039dffe53d94fc76cf";
const std::string EpicGamesAuth::TOKEN_ENDPOINT_URL = "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token";


// Funzioni helper per leggere/scrivere file
std::string readFileToStringAuth(const std::filesystem::path& p) {
    std::ifstream file(p);
    std::string content;
    if (file) {
        file.seekg(0, std::ios::end);
        if (file.tellg() > 0) {
            content.reserve(file.tellg());
            file.seekg(0, std::ios::beg);
            content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }
    }
    return content;
}

bool writeStringToFileAuth(const std::filesystem::path& p, const std::string& content) {
    std::ofstream file(p, std::ios::binary);
    if (file) {
        file << content;
        return true;
    }
    return false;
}

EpicGamesAuth::EpicGamesAuth() : mHasValidTokenInfo(false) {
    mTokenStoragePath = std::filesystem::path(Paths::getEmulationStationPath()) / "epic_tokens";
     if (!Utils::FileSystem::exists(mTokenStoragePath.string())) {
        Utils::FileSystem::createDirectory(mTokenStoragePath.string());
    }
    loadTokenData();
}

EpicGamesAuth::~EpicGamesAuth() {}

// --- Implementazioni Funzioni per la UI ---

void EpicGamesAuth::logout() {
    clearAllTokenData();
}

std::string EpicGamesAuth::getUsername() const {
    if (!mDisplayName.empty()) return mDisplayName;
    if (!mAccountId.empty()) return mAccountId;
    return "N/A";
}

std::string EpicGamesAuth::getDisplayName() const {
    return getUsername();
}


std::string EpicGamesAuth::getInitialLoginUrl() {
    // Prova con l'URL di redirect usato da altri client di terze parti.
    const std::string redirectUrl = "https://www.epicgames.com/account/personal";
    
    return "https://www.epicgames.com/id/login?clientId=" + EPIC_CLIENT_ID +
           "&redirectUrl=" + HttpReq::urlEncode(redirectUrl) +
           "&responseType=code&prompt=login";
}

std::string EpicGamesAuth::getAuthorizationCodeUrl() {
    return "https://www.epicgames.com/id/api/redirect?clientId=34a02cf8f4414e29b15921876da36f9a&responseType=code";
}

bool EpicGamesAuth::processWebViewRedirect(const std::string& redirectUrl) {
    LOG(LogDebug) << "Processing WebView redirect URL: " << redirectUrl;
    
    // Il codice è un parametro nell'URL, cerchiamolo.
    const std::string codeNeedle = "code=";
    size_t codePos = redirectUrl.find(codeNeedle);

    if (codePos != std::string::npos) {
        // Trovato! Estraiamo il codice.
        std::string code = redirectUrl.substr(codePos + codeNeedle.length());
        
        // Rimuovi eventuali altri parametri che seguono (delimitati da '&')
        size_t endPos = code.find('&');
        if (endPos != std::string::npos) {
            code = code.substr(0, endPos);
        }

        LOG(LogInfo) << "Extracted authorization code from redirect URL: '" << code << "'";
        
        // Usa la tua funzione esistente per scambiare il codice con il token
        return exchangeAuthCodeForToken(code);
    }

    LOG(LogError) << "Could not find 'code=' parameter in redirect URL: " << redirectUrl;
    return false;
}

std::string EpicGamesAuth::getRefreshToken() const {
    return mRefreshToken;
}

// --- Implementazioni Funzioni Core (con correzioni) ---
// (Il resto del file .cpp rimane come nella versione precedente che ti ho dato, te lo rimetto per sicurezza)

bool EpicGamesAuth::exchangeAuthCodeForToken(const std::string& authCode) {
    LOG(LogInfo) << "Exchanging authorization code for tokens...";
    HttpReqOptions options;
    std::string authString = EPIC_CLIENT_ID + ":" + EPIC_CLIENT_SECRET;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.dataToPost = "grant_type=authorization_code&code=" + HttpReq::urlEncode(authCode) + "&token_type=eg1";
    HttpReq request(TOKEN_ENDPOINT_URL, &options);
    if (!request.wait()) { LOG(LogError) << "HTTP request failed (wait): " << request.getErrorMsg(); return false; }
    if (request.status() != 200) { LOG(LogError) << "Auth code exchange failed. Status: " << request.status() << ", Body: " << request.getContent(); return false; }
    try { return processTokenResponse(json::parse(request.getContent()), false); }
    catch (const json::exception& e) { LOG(LogError) << "JSON parse error: " << e.what(); return false; }
}

bool EpicGamesAuth::refreshAccessToken() {
    if (mRefreshToken.empty()) { LOG(LogWarning) << "No refresh token available."; return false; }
    LOG(LogInfo) << "Attempting to refresh access token...";
    HttpReqOptions options;
    std::string authString = EPIC_CLIENT_ID + ":" + EPIC_CLIENT_SECRET;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.dataToPost = "grant_type=refresh_token&refresh_token=" + HttpReq::urlEncode(mRefreshToken) + "&token_type=eg1";
    HttpReq request(TOKEN_ENDPOINT_URL, &options);
    if (!request.wait()) { LOG(LogError) << "HTTP refresh request failed (wait): " << request.getErrorMsg(); return false; }
    if (request.status() != 200) {
        LOG(LogError) << "Token refresh failed. Status: " << request.status() << ", Body: " << request.getContent();
        if (request.getContent().find("invalid_grant") != std::string::npos) { LOG(LogWarning) << "Refresh token is invalid or expired. Clearing all tokens."; clearAllTokenData(); }
        return false;
    }
    try { return processTokenResponse(json::parse(request.getContent()), true); }
    catch (const json::exception& e) { LOG(LogError) << "JSON parse error on refresh: " << e.what(); return false; }
}

bool EpicGamesAuth::processTokenResponse(const nlohmann::json& response, bool isRefreshing) {
    try {
        if (!response.is_object() || response.contains("errorCode")) { LOG(LogError) << "Token response contains an error: " << response.dump(2); return false; }
        mAccessToken = response.value("access_token", "");
        mAccountId = response.value("account_id", "");
        mDisplayName = response.value("displayName", "");
        int expiresIn = response.value("expires_in", 0);
        if (response.contains("refresh_token")) { mRefreshToken = response.value("refresh_token", ""); }
        if (mAccessToken.empty() || mAccountId.empty() || expiresIn <= 0) { LOG(LogError) << "Essential token data missing in response."; return false; }
        mTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn - 60);
        mHasValidTokenInfo = true;
        saveTokenData();
        return true;
    } catch (const json::exception& e) { LOG(LogError) << "JSON exception in processTokenResponse: " << e.what(); return false; }
}

void EpicGamesAuth::saveTokenData() {
    if (mTokenStoragePath.empty()) return;
    writeStringToFileAuth(mTokenStoragePath / "epic_access_token.txt", mAccessToken);
    writeStringToFileAuth(mTokenStoragePath / "epic_refresh_token.txt", mRefreshToken);
    writeStringToFileAuth(mTokenStoragePath / "epic_account_id.txt", mAccountId);
    writeStringToFileAuth(mTokenStoragePath / "epic_display_name.txt", mDisplayName);
    time_t expiryTimestamp = std::chrono::system_clock::to_time_t(mTokenExpiry);
    writeStringToFileAuth(mTokenStoragePath / "epic_expiry.txt", std::to_string(expiryTimestamp));
}

bool EpicGamesAuth::loadTokenData() {
    if (mTokenStoragePath.empty()) return false;
    mAccessToken = readFileToStringAuth(mTokenStoragePath / "epic_access_token.txt");
    mRefreshToken = readFileToStringAuth(mTokenStoragePath / "epic_refresh_token.txt");
    mAccountId = readFileToStringAuth(mTokenStoragePath / "epic_account_id.txt");
    mDisplayName = readFileToStringAuth(mTokenStoragePath / "epic_display_name.txt");
    std::string expiryStr = readFileToStringAuth(mTokenStoragePath / "epic_expiry.txt");
    if (mAccessToken.empty() || mAccountId.empty() || expiryStr.empty()) { mHasValidTokenInfo = false; return false; }
    try {
        time_t expiryTimestamp = std::stoll(expiryStr);
        mTokenExpiry = std::chrono::system_clock::from_time_t(expiryTimestamp);
        mHasValidTokenInfo = true;
        if (!isAuthenticated()) { LOG(LogInfo) << "Loaded Epic access token is expired. Attempting refresh..."; refreshAccessToken(); }
        return isAuthenticated();
    } catch (const std::exception&) { clearAllTokenData(); return false; }
}

void EpicGamesAuth::clearAllTokenData() {
    mAccessToken.clear(); mRefreshToken.clear(); mAccountId.clear(); mDisplayName.clear();
    mTokenExpiry = std::chrono::system_clock::from_time_t(0);
    mHasValidTokenInfo = false;
    if (!mTokenStoragePath.empty()) {
        Utils::FileSystem::removeFile((mTokenStoragePath / "epic_access_token.txt").string());
        Utils::FileSystem::removeFile((mTokenStoragePath / "epic_refresh_token.txt").string());
        Utils::FileSystem::removeFile((mTokenStoragePath / "epic_account_id.txt").string());
        Utils::FileSystem::removeFile((mTokenStoragePath / "epic_display_name.txt").string());
        Utils::FileSystem::removeFile((mTokenStoragePath / "epic_expiry.txt").string());
    }
}

std::string EpicGamesAuth::getAccessToken() const { return mAccessToken; }
std::string EpicGamesAuth::getAccountId() const { return mAccountId; }
bool EpicGamesAuth::isAuthenticated() const {
    if (!mHasValidTokenInfo || mAccessToken.empty()) return false;
    return std::chrono::system_clock::now() < mTokenExpiry;
}
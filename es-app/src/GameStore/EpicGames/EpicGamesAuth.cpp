#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "Log.h"
#include "utils/RandomString.h"
#include "utils/FileSystemUtil.h"
#include "Paths.h"
#include "HttpReq.h"
#include "json.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <random>
#include <cstring>
#include <stdexcept>
#include "Settings.h"
#include "utils/base64.h"


using json = nlohmann::json;

const std::string EpicGamesAuth::STATE_FILE_NAME = "epic_auth_state.txt";

// Constructor with setStateCallback
EpicGamesAuth::EpicGamesAuth(std::function<void(const std::string&)> setStateCallback)
    : mSetStateCallback(setStateCallback), mAuthState("") {
    LOG(LogDebug) << "EpicGamesAuth(callback) - Constructor - setStateCallback: NOT NULL  Instance: " << this << "  Address of setStateCallback: " << &setStateCallback;
    loadToken();
}

// Default Constructor
EpicGamesAuth::EpicGamesAuth() : mSetStateCallback(nullptr), mAuthState("") {
    LOG(LogDebug) << "EpicGamesAuth() - Default Constructor  Instance: " << this;
    loadToken();
}

EpicGamesAuth::~EpicGamesAuth() {
    LOG(LogDebug) << "EpicGamesAuth - Destructor. Instance: " << this;
    saveToken(std::string());
}

std::string EpicGamesAuth::getAuthorizationUrl(std::string& state) {
    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Entering. Instance: " << this;

    state = generateRandomState();

    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - mSetStateCallback: " << (mSetStateCallback ? "NOT NULL" : "NULL");
    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Current mAuthState: " << mAuthState;
    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Generated state: " << state;

    mAuthState = state;
    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Stored state in mAuthState: " << mAuthState << ". Instance: " << this;

    if (mSetStateCallback) {
        LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Calling mSetStateCallback BEFORE";
        mSetStateCallback(state);
        LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Called mSetStateCallback AFTER";
        LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl - Called mSetStateCallback with state: " << state;
    }

    std::string clientId = Settings::getInstance()->getString("EpicGames.ClientId");
    if (clientId.empty()) {
        clientId = "34a02cf8f4414e29b15921876da36f9a"; // IMPORTANT: REPLACE THIS!
    }
    std::string redirectUri = Settings::getInstance()->getString("EpicGames.RedirectUri");
    if (redirectUri.empty()) {
        redirectUri = "http://localhost:1234/epic_callback";
    }

    std::string authUrl = "https://www.epicgames.com/id/authorize?"
                          "client_id=" + clientId + "&"
                          "response_type=code&"
                          "redirect_uri=" + redirectUri + "&"
                          "scope=basic_profile%20offline_access&"
                          "state=" + state;

    LOG(LogDebug) << "Generated auth URL: " << authUrl;
    return authUrl;
}

bool EpicGamesAuth::getAccessToken(const std::string& authCode, std::string& accessToken) {
    LOG(LogDebug) << "EpicGamesAuth::getAccessToken - Entering. Using Playnite Credentials. Instance: " << this;

    // URL Endpoint Token (Fallback Playnite)
    std::string url = "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token";

    // Client ID e Secret (DI PLAYNITE)
    std::string clientId = "34a02cf8f4414e29b15921876da36f9a";
    std::string clientSecret = "daafbccc737745039dffe53d94fc76cf";

    // Codifica ID:Secret in Base64 per l'header Authorization Basic
    std::string authString = clientId + ":" + clientSecret;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());

    // POST Data (come Playnite)
    std::string postData = "grant_type=authorization_code&"
                           "code=" + authCode + "&"
                           "token_type=eg1";

    LOG(LogDebug) << "getAccessToken - Request URL: " << url;
    LOG(LogDebug) << "getAccessToken - Auth Header: Authorization: Basic " << authEncoded; // Usa credenziali Playnite
    // LOG(LogDebug) << "getAccessToken - Request Data: " << postData; // Meglio non loggare authCode

    // Prepara opzioni per HttpReq
    HttpReqOptions options;
    options.dataToPost = postData; // Imposta il corpo della richiesta POST
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    // Crea ed esegui la richiesta
    HttpReq httpreq(url, &options);

    // Attendi il completamento
    if (!httpreq.wait()) {
        LOG(LogError) << "HTTP request to get access token failed! Status: " << httpreq.status();
        LOG(LogError) << "Error: " << httpreq.getErrorMsg();
        LOG(LogError) << "Response Body: " << httpreq.getContent();
        saveToken(""); // Pulisce token vecchio se fallisce
        return false;
    }

    // Controlla lo stato HTTP (dovrebbe essere 200 OK)
    if (httpreq.status() != HttpReq::REQ_SUCCESS) {
        LOG(LogError) << "Failed to get access token. HTTP status: " << httpreq.status();
        LOG(LogError) << "Response: " << httpreq.getContent();
        saveToken(""); // Pulisce token vecchio se fallisce
        return false;
    }

    // Parsifica la risposta JSON per ottenere il token
    try {
        json response = json::parse(httpreq.getContent());
        if (response.contains("access_token")) {
            accessToken = response["access_token"];
            LOG(LogInfo) << "EpicGamesAuth: Successfully obtained new access token using Playnite credentials.";
            // TODO: Salva anche refresh_token, account_id, expires_in se presenti e necessari
            // std::string refreshToken = response.value("refresh_token", "");
            // int expiresIn = response.value("expires_in", 0);
            // std::string accountId = response.value("account_id", "");
            saveToken(accessToken); // Salva il nuovo token
            mAccessToken = accessToken; // Aggiorna anche il membro interno
            return true;
        } else {
            LOG(LogError) << "Access token not found in response JSON.";
            LOG(LogError) << "Response: " << httpreq.getContent();
            saveToken("");
            return false;
        }
    } catch (const json::parse_error& e) {
        LOG(LogError) << "JSON parse error while getting access token: " << e.what();
        LOG(LogError) << "Response: " << httpreq.getContent();
        saveToken("");
        return false;
    } catch (const std::exception& e) {
        LOG(LogError) << "Exception while parsing access token response: " << e.what();
        saveToken("");
        return false;
    }

    // Ritorno finale se qualcosa va storto prima
    saveToken("");
    return false;
} // Fine getAccessToken



std::string EpicGamesAuth::getAccessToken() const {
    return mAccessToken;
}

std::string EpicGamesAuth::generateRandomState() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const int STATE_LENGTH = 32;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

    std::string state;
    for (int i = 0; i < STATE_LENGTH; ++i) {
        state += alphanum[dis(gen)];
    }
    return state;
}



void EpicGamesAuth::saveToken(const std::string& accessToken) {
    std::string tokenFileName = "epic_access_token.txt";
    std::string tokenDirectory = Utils::FileSystem::getEsConfigPath();
    std::string tokenPath = Utils::FileSystem::combine(tokenDirectory, tokenFileName); // Keep this one
    if (accessToken.empty()) {
        //  Do NOT delete the file here!
        mAccessToken = "";
        LOG(LogDebug) << "Access token cleared from memory.";
        return;
    }
  

    try {
        if (!Utils::FileSystem::exists(tokenDirectory)) {
            Utils::FileSystem::createDirectory(tokenDirectory);
            LOG(LogDebug) << "Created directory: " << tokenDirectory;
        }

        std::ofstream file(tokenPath);
        if (file.is_open()) {
            file << accessToken;
            file.close();
            mAccessToken = accessToken;
            LOG(LogDebug) << "Access token saved to: " << tokenPath;
        } else {
            LOG(LogError) << "Unable to open file for saving token: " << tokenPath;
        }
    } catch (const std::exception& e) {
        LOG(LogError) << "Error saving token: " << e.what();
    }
}

void EpicGamesAuth::loadToken() {
    std::string tokenPath = Utils::FileSystem::getEsConfigPath() + "/epic_access_token.txt";
    try {
        std::ifstream file(tokenPath);
        if (file.is_open()) {
            std::getline(file, mAccessToken);
            file.close();
            LOG(LogDebug) << "Access token loaded from: " << tokenPath;
        } else {
            LOG(LogDebug) << "No access token file found at: " << tokenPath;
        }
    } catch (const std::exception& e) {
        LOG(LogError) << "Error loading token: " << e.what();
    }
}

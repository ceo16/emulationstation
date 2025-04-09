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
        clientId = "xyza78919vCLzoIKl2CjuMtYaR8kn7xM"; // IMPORTANT: REPLACE THIS!
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
    LOG(LogDebug) << "EpicGamesAuth::getAccessToken - Entering. Instance: " << this;

    std::string url = "https://api.epicgames.dev/epic/oauth/v2/token";
    std::string clientId = Settings::getInstance()->getString("EpicGames.ClientId");
    if (clientId.empty()) {
        clientId = "xyza78919vCLzoIKl2CjuMtYaR8kn7xM"; // IMPORTANT: REPLACE THIS!
    }
    std::string redirectUri = Settings::getInstance()->getString("EpicGames.RedirectUri");
    if (redirectUri.empty()) {
        redirectUri = "http://localhost:1234/epic_callback";
    }

    std::string clientSecret = Settings::getInstance()->getString("EpicGames.ClientSecret"); // Load from settings!
    if (clientSecret.empty()) {
        clientSecret = "GEBLQtVX7uuwDcqyKbBKu2jEKZjjrTuXfyZWb+v2JbY"; // IMPORTANT: REPLACE THIS!
    }
    std::string authEncoded = base64_encode(clientId + ":" + clientSecret); // Example

    std::string postData = "grant_type=authorization_code&"
                      "code=" + authCode + "&"
                      "redirect_uri=" + redirectUri + "&"
                      "client_id=" + clientId;

    LOG(LogDebug) << "getAccessToken - Request URL: " << url;
    LOG(LogDebug) << "getAccessToken - Request Data: " << postData;
    LOG(LogDebug) << "getAccessToken - Auth Header: Authorization: Basic " << authEncoded;  // Log the header

    HttpReqOptions options;
    options.dataToPost = postData;
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    HttpReq httpreq(url, &options);

    if (!httpreq.wait()) {
        LOG(LogError) << "HTTP request failed!";
        return false;
    }

    if (httpreq.status() != HttpReq::REQ_SUCCESS) {
        LOG(LogError) << "Failed to get access token. HTTP status: " << httpreq.status();
        LOG(LogError) << "Response: " << httpreq.getContent();
        return false;
    }

    try {
        json response = json::parse(httpreq.getContent());
        accessToken = response["access_token"];
        LOG(LogDebug) << "Got access token: " << accessToken;
        saveToken(accessToken);
        return true;
    } catch (const json::parse_error& e) {
        LOG(LogError) << "JSON parse error: " << e.what();
        return false;
    }
}

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
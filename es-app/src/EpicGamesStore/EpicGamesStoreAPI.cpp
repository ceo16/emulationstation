//  EpicGamesStoreAPI.cpp

#include "EpicGamesStoreAPI.h"
#include <iostream>
#include <curl/curl.h>
#include "json.hpp"
#include <fstream>
#include <sstream>  // For stringstream
#include <iomanip>  // For setprecision
#include <regex>
#include "EpicGamesParser.h" // Include EpicGamesParser.h
#include "ApiSystem.h" // Include ApiSystem.h
#include "SystemData.h"
#include "FileData.h"


using json = nlohmann::json;

//  --- Constants from EpicAccountClient.cs ---
const std::string EpicGamesStoreAPI::LOGIN_URL = "https://www.epicgames.com/id/login";
const std::string EpicGamesStoreAPI::AUTH_CODE_URL = "https://www.epicgames.com/id/api/redirect?clientId=34a02cf8f4414e29b15921876da36f9a&responseType=code";
const std::string EpicGamesStoreAPI::OAUTH_URL_MASK = "https://{0}/account/api/oauth/token";
const std::string EpicGamesStoreAPI::AUTH_ENCODED_STRING = "MzRhMDJjZjhmNDQxNGUyOWIxNTkyMTg3NmRhMzZmOWE6ZGFhZmJjY2M3Mzc3NDUwMzlkZmZlNTNkOTRmYzc2Y2Y=";

//  --- Helper Functions ---

// Helper function to URL encode a string
std::string EpicGamesStoreAPI::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }
    return escaped.str();
}

// Helper function to set headers
void EpicGamesStoreAPI::setHeaders(const std::list<std::string>& headers) {
    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, header_list);
}

//  --- Token Storage ---
static const std::string TOKEN_FILE = "epic_tokens.json";

bool EpicGamesStoreAPI::storeTokens(const std::string& accessToken, const std::string& refreshToken, const std::string& accountId, const std::string& tokenType) {
    try {
        json tokens;
        tokens["access_token"] = accessToken;
        tokens["refresh_token"] = refreshToken;
        tokens["account_id"] = accountId;
        tokens["token_type"] = tokenType;

        std::ofstream file(TOKEN_FILE);
        file << tokens.dump(4);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error storing tokens: " << e.what() << std::endl;
        return false;
    }
}

bool EpicGamesStoreAPI::loadTokens(std::string& accessToken, std::string& refreshToken, std::string& accountId, std::string& tokenType) {
    try {
        std::ifstream file(TOKEN_FILE);
        if (!file.is_open()) {
            return false;
        }
        json tokens;
        file >> tokens;

        accessToken = tokens.value("access_token", "");
        refreshToken = tokens.value("refresh_token", "");
        accountId = tokens.value("account_id", "");
        tokenType = tokens.value("token_type", "");
        return !accessToken.empty() && !refreshToken.empty() && !accountId.empty() && !tokenType.empty();
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading tokens: " << e.what() << std::endl;
        return false;
    }
}

// --- Authentication Flow ---

//  1.  Handle Web-based Login (EmulationStation's Responsibility)

//  EmulationStation needs to provide a way for the user to authenticate
//  through a web browser.  This part is outside the scope of this class.
//  EmulationStation should:
//      a)  Open a web view or browser window and navigate to LOGIN_URL.
//      b)  Monitor the URL for redirects to AUTH_CODE_URL.
//      c)  Extract the authorization code from the URL.
//      d)  Pass the authorization code to EpicGamesStoreAPI::authenticateUsingAuthCode().

// 2. Authenticate using Auth Code
bool EpicGamesStoreAPI::authenticateUsingAuthCode(const std::string& authorizationCode) {
    std::string oauthUrl;
    // In Playnite plugin, the domain is loaded from config, for simplicity, we use the default one here.
    oauthUrl = std::regex_replace(OAUTH_URL_MASK, std::regex("\\{0\\}"), "account-public-service-prod03.ol.epicgames.com");

    // Construct the request body
    std::string postData = "grant_type=authorization_code&code=" + authorizationCode + "&token_type=eg1";

    // Set headers
    setHeaders({
        "Authorization: basic " + AUTH_ENCODED_STRING,
        "Content-Type: application/x-www-form-urlencoded"
    });

    // Use performRequest() to send the request
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, postData.c_str());

    std::string response = performRequest(oauthUrl);

    // Reset post data
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, nullptr); // Reset headers

    if (response.empty()) {
        std::cerr << "Error: Authentication request failed" << std::endl;
        return false;
    }

    // Parse the JSON response
    try {
        json authResponse = json::parse(response);

        // Extract access_token, refresh_token, account_id, token_type
        std::string accessToken = authResponse.value("access_token", "");
        std::string refreshToken = authResponse.value("refresh_token", "");
        std::string accountId = authResponse.value("account_id", "");
        std::string tokenType = authResponse.value("token_type", "");

        if (accessToken.empty() || refreshToken.empty() || accountId.empty() || tokenType.empty()) {
            std::cerr << "Error: Could not retrieve tokens" << std::endl;
            return false;
        }

        // Store the tokens
        if (storeTokens(accessToken, refreshToken, accountId, tokenType)) {
            std::cout << "Authentication successful!" << std::endl;
            return true;
        }
        else {
            std::cerr << "Error: Failed to store tokens" << std::endl;
            return false;
        }

    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        return false;
    }
}

std::string EpicGamesStoreAPI::getAccessToken() {
    std::string accessToken, refreshToken, accountId, tokenType;

    // Load tokens
    if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) {
        std::cerr << "Error: No tokens found. Need to log in." << std::endl;
        return "";
    }

    // Check if the token is valid (this is a placeholder check)
    if (accessToken.empty()) { // Replace with proper expiration check
        std::cerr << "Error: Invalid access token." << std::endl;
        return "";
    }

    // (You'll need to implement a proper token expiration check here.
    //  Access tokens usually have an expiry time.)

    // If expired, refresh the token
    // if (isTokenExpired(accessToken)) {
    //     if (!refreshToken()) {
    //         return "";
    //     }
    //     if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) { // Reload tokens after refresh
    //        std::cerr << "Error loading tokens after refresh" << std::endl;
    //        return "";
    //     }
    // }

    return accessToken;
}

bool EpicGamesStoreAPI::refreshToken() {
    std::string accessToken, refreshToken, accountId, tokenType;
    if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) {
        std::cerr << "Error: No refresh token found." << std::endl;
        return false;
    }

    std::string oauthUrl;
    // In Playnite plugin, the domain is loaded from config, for simplicity, we use the default one here.
    oauthUrl = std::regex_replace(OAUTH_URL_MASK, std::regex("\\{0\\}"), "account-public-service-prod03.ol.epicgames.com");

    // Construct the request body
    std::string postData = "grant_type=refresh_token&refresh_token=" + refreshToken + "&token_type=eg1";

    // Set headers
    setHeaders({
        "Authorization: basic " + AUTH_ENCODED_STRING,
        "Content-Type: application/x-www-form-urlencoded"
    });

    // Use performRequest()
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, postData.c_str());
    std::string response = performRequest(oauthUrl);

    // Reset post data
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, nullptr); // Reset headers

    if (response.empty()) {
        std::cerr << "Error: Refresh token request failed" << std::endl;
        return false;
    }

    // Parse the JSON response
    try {
        json refreshResponse = json::parse(response);
        std::string newAccessToken = refreshResponse.value("access_token", "");
        std::string newRefreshToken = refreshResponse.value("refresh_token", "");
        std::string newAccountId = refreshResponse.value("account_id", "");
        std::string newTokenType = refreshResponse.value("token_type", "");

        if (newAccessToken.empty() || newRefreshToken.empty() || newAccountId.empty() || newTokenType.empty()) {
            std::cerr << "Error: Could not retrieve new tokens" << std::endl;
            return false;
        }

        // Store the new tokens
        if (storeTokens(newAccessToken, newRefreshToken, newAccountId, newTokenType)) {
            std::cout << "Token refresh successful!" << std::endl;
            return true;
        }
        else {
            std::cerr << "Error: Failed to store refreshed tokens" << std::endl;
            return false;
        }

    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        return false;
    }
}

//  --- Other Methods ---

EpicGamesStoreAPI::EpicGamesStoreAPI() : curlHandle(nullptr) {}

EpicGamesStoreAPI::~EpicGamesStoreAPI() {
    shutdown();
}

bool EpicGamesStoreAPI::initialize() {
    // Initialize libcurl
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        std::cerr << "Error initializing libcurl" << std::endl;
        return false;
    }
    curlHandle = curl_easy_init(); // Get a curl handle
    if (!curlHandle) {
        std::cerr << "Error getting curl handle" << std::endl;
        return false;
    }
    return true;
}

// Helper function to perform HTTP requests
size_t EpicGamesStoreAPI::performRequestCallback(char* buffer, size_t size, size_t nmemb, std::string* userdata) {
    size_t total_size = size * nmemb;
    userdata->append(buffer, total_size);
    return total_size;
}

std::string EpicGamesStoreAPI::performRequest(const std::string& url) {
    std::string response_string;
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());

    //  Set up the write callback (Correzione)
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
       EpicGamesStoreAPI::performRequestCallback);

    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curlHandle);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    return response_string;
}
std::string EpicGamesStoreAPI::getGamesList() {
    // ... (existing code)

    // Parse the JSON response and use the parseEpicGamesList function
    try {
        json games_data = json::parse(response);
        // Convert json data to string
        std::string gamesListString = games_data.dump();

        // Create the SystemMetadata object here
        SystemMetadata metadata;
        metadata.name = "epic_games"; // Or whatever name you want to give the system
        metadata.fullName = "Epic Games Store"; // And the full name

        // For now, let's create a dummy system
        SystemData* tempSystem = new SystemData(metadata, nullptr, nullptr, false, false, false, false);
        std::vector<FileData*> games = parseEpicGamesList(gamesListString, tempSystem);

        //  Convert the vector<FileData*> to a JSON string
        json json_array = json::array();
        for (const auto& game : games) {
            json game_json;
            game_json["title"] = game->getName();
            game_json["path"] = game->getPath().string();
            // Add other relevant data from FileData as needed
            json_array.push_back(game_json);
        }

        //  Return the json as a string
        return json_array.dump();
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        return ""; //  Return an empty JSON array on error
    }
}


void EpicGamesStoreAPI::shutdown() {
    if (curlHandle) {
        curl_easy_cleanup(curlHandle);
        curlHandle = nullptr;
    }
    curl_global_cleanup();
}

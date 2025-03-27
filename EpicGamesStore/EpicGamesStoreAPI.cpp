#include "EpicGamesStoreAPI.h"
#include "../../es-core/src/Log.h"
#include "utils/StringUtil.h"
#include "pugixml.hpp"
#include <sstream>
#include <iostream>
#include "json.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include "curl/curl.h"
#include "curl/curlver.h"// Include curl header
#include "../../es-core/src/Settings.h"

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
        LOG(LogError) << "Error storing tokens.";
        Log::flush();
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
        LOG(LogError) << "Error loading tokens.";
        Log::flush();
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
        LOG(LogError) << "Authentication request failed";
        Log::flush();
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
            LOG(LogError) << "Could not retrieve tokens";
            Log::flush();
            return false;
        }

        // Store the tokens
        if (storeTokens(accessToken, refreshToken, accountId, tokenType)) {
            LOG(LogInfo) << "Authentication successful!";
            Log::flush();
            return true;
        }
        else {
            LOG(LogError) << "Failed to store tokens";
            Log::flush();
            return false;
        }

    }
    catch (json::parse_error& e) {
        LOG(LogError) << "JSON Parse error";
        Log::flush();
        return false;
    }
}

std::string EpicGamesStoreAPI::getAccessToken() {
    std::string accessToken, refreshToken, accountId, tokenType;

    // Load tokens
    if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) {
        LOG(LogError) << "Error: No tokens found. Need to log in.";
        Log::flush();
        return "";
    }

    // Check if the token is valid (this is a placeholder check)
    if (accessToken.empty()) { // Replace with proper expiration check
        LOG(LogError) << "Error: Invalid access token.";
        Log::flush();
        return "";
    }

    // (You'll need to implement a proper token expiration check here.
    //  Access tokens usually have an expiry time.)

    // If expired, refresh the token
    // if (!refreshToken()) {
    //     return "";
    // }
    // if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) { // Reload tokens after refresh
    //    Log::error("Error loading tokens after refresh");
    //    return "";
    // }

    return accessToken;
}

bool EpicGamesStoreAPI::refreshToken() {
    std::string accessToken, refreshToken, accountId, tokenType;
    if (!loadTokens(accessToken, refreshToken, accountId, tokenType)) {
        LOG(LogError) << "Error: No refresh token found.";
        Log::flush();
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
        LOG(LogError) << "Refresh token request failed";
        Log::flush();
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
            LOG(LogError) << "Could not retrieve new tokens";
            Log::flush();
            return false;
        }

        // Store the new tokens
        if (storeTokens(newAccessToken, newRefreshToken, newAccountId, newTokenType)) {
            LOG(LogInfo) << "Token refresh successful!";
            Log::flush();
            return true;
        }
        else {
            LOG(LogError) << "Failed to store refreshed tokens";
            Log::flush();
            return false;
        }

    }
    catch (json::parse_error& e) {
        LOG(LogError) << "JSON Parse error";
        Log::flush();
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
        LOG(LogError) << "Error initializing libcurl";
        Log::flush();
        return false;
    }
    curlHandle = curl_easy_init(); // Get a curl handle
    if (!curlHandle) {
        LOG(LogError) << "Error getting curl handle";
        Log::flush();
        return false;
    }
    return true;
}

// Helper function to perform HTTP requests
std::string EpicGamesStoreAPI::performRequest(const std::string& url) {
    std::string response_string;
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());

    //  Set up the write callback (Correzione)
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
        [](char* buffer, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* output = static_cast<std::string*>(userdata);
            size_t total_size = size * nmemb;
            output->append(buffer, total_size);
            return total_size;
        });

    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curlHandle);
    if (res != CURLE_OK) {
        LOG(LogError) << curl_easy_strerror(res);
        Log::flush();
        return "";
    }
    return response_string;
}
std::string EpicGamesStoreAPI::getRefreshToken() {
    //  Add the code here to retrieve the refresh token
    //  For example, if you store it in a member variable:
    //  return this->refreshToken; 

    //  If you store it using Settings:
    return Settings::getInstance()->getString("EpicRefreshToken");
}
std::string EpicGamesStoreAPI::getGamesList() {
    //  (Implement this using the getAccessToken() method and
    //   the Epic Games API endpoint for retrieving the game list)
    std::string accessToken = getAccessToken();
    if (accessToken.empty()) {
        return ""; //  Return an empty JSON array on error
    }

    //  ... (API call to get games list, using accessToken)
    return "[{\"title\": \"Placeholder Game 1\", \"install_dir\": \"/path/1\"}, {\"title\": \"Placeholder Game 2\", \"install_dir\": \"/path/2\"}]";
}

void EpicGamesStoreAPI::shutdown() {
    if (curlHandle) {
        curl_easy_cleanup(curlHandle);
        curlHandle = nullptr;
    }
    curl_global_cleanup();
}

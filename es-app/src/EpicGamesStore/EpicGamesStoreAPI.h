#ifndef EMULATIONSTATION_EPICGAMESSTOREAPI_H
#define EMULATIONSTATION_EPICGAMESSTOREAPI_H

#include <string>
#include <curl/curl.h> // Include curl header
#include <list>

class EpicGamesStoreAPI {
public:
    EpicGamesStoreAPI();
    ~EpicGamesStoreAPI();

    // Initialize the API (e.g., setup libcurl)
    bool initialize();

    // Authentication
    // EmulationStation needs to call a web view to get the auth code
    bool authenticateUsingAuthCode(const std::string& authorizationCode);
    std::string getAccessToken();
    bool refreshToken();

    // Get a list of games (replace with actual API call)
    std::string getGamesList();

    // Shutdown the API (e.g., cleanup libcurl)
    void shutdown();

    // Helper function to URL encode strings
    std::string urlEncode(const std::string& value);

private:
    // libcurl handle (or other necessary data)
    CURL* curlHandle;
    std::string performRequest(const std::string& url);

    // Helper function to set HTTP headers
    void setHeaders(const std::list<std::string>& headers);

    // Token Storage
    bool storeTokens(const std::string& accessToken, const std::string& refreshToken, const std::string& accountId, const std::string& tokenType);
    bool loadTokens(std::string& accessToken, std::string& refreshToken, std::string& accountId, std::string& tokenType);

    // Constants (from EpicAccountClient.cs)
    static const std::string LOGIN_URL;
    static const std::string AUTH_CODE_URL;
    static const std::string OAUTH_URL_MASK;
    static const std::string AUTH_ENCODED_STRING;
};

#endif // EMULATIONSTATION_EPICGAMESSTOREAPI_H

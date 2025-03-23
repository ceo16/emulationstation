#ifndef EMULATIONSTATION_EPICGAMESSTOREAPI_H
#define EMULATIONSTATION_EPICGAMESSTOREAPI_H

#include <string>
#include <curl/curl.h> // Include curl header

class EpicGamesStoreAPI {
public:
    EpicGamesStoreAPI();
    ~EpicGamesStoreAPI();

    // Initialize the API (e.g., setup libcurl)
    bool initialize();

    // Get a list of games (replace with actual API call)
    std::string getGamesList();

    // Shutdown the API (e.g., cleanup libcurl)
    void shutdown();

private:
    // libcurl handle (or other necessary data)
    CURL* curlHandle;
    std::string performRequest(const std::string& url);
};

#endif // EMULATIONSTATION_EPICGAMESSTOREAPI_H

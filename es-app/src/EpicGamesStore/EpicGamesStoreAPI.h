#ifndef EMULATIONSTATION_EPICGAMESSTOREAPI_H
#define EMULATIONSTATION_EPICGAMESSTOREAPI_H

#include <string>

class EpicGamesStoreAPI {
public:
    EpicGamesStoreAPI();
    ~EpicGamesStoreAPI();

    // Initialize the API (e.g., setup libcurl)
    bool initialize();

    // Get a list of games (replace with actual API call)
    std::string getGamesList();
    std::string performRequest(const std::string& url);

    // Shutdown the API (e.g., cleanup libcurl)
    void shutdown();

private:
    // libcurl handle (or other necessary data)
    void* curlHandle;
};

#endif // EMULATIONSTATION_EPICGAMESSTOREAPI_H

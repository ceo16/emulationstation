#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H

#include <string>
#include <vector>

class EpicGamesStoreAPI {
public:
    EpicGamesStoreAPI();
    ~EpicGamesStoreAPI();

    bool initialize();
    void shutdown();


private:
    // Methods to interact with the Epic Games Store API
    // (e.g., authentication, fetching game lists, etc.)
};

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
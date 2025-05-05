#pragma once
#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H

#include <string>
#include <vector>
#include <map>
#include <future> // <<< AGGIUNTO: Per std::future
#include "GameStore/EpicGames/EpicGamesModels.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"

class EpicGamesStoreAPI {
public:
    EpicGamesStoreAPI(EpicGamesAuth* auth);
    ~EpicGamesStoreAPI();

    EpicGamesAuth* getAuth(); // Dichiarazione del getter

    // --- Metodi Sincroni Esistenti (Mantieni per ora) ---
    // Questi ora possono chiamare internamente la versione Async e attendere,
    // oppure mantenere la loro logica originale se preferisci.
    // L'implementazione sotto li fa chiamare la versione Async.
    std::vector<EpicGames::Asset> GetAllAssets();
    std::map<std::string, EpicGames::CatalogItem> GetCatalogItems(
         const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
         const std::string& country = "IT",
         const std::string& locale = "it-IT");


    // --- NUOVI Metodi Asincroni ---
    // Restituiscono un future che conterrà il risultato quando pronto.
    std::future<std::vector<EpicGames::Asset>> GetAllAssetsAsync();
    std::future<std::map<std::string, EpicGames::CatalogItem>> GetCatalogItemsAsync(
        const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
        const std::string& country = "IT",
        const std::string& locale = "it-IT");


private:
    EpicGamesAuth* mAuth;
    // std::string mAccessToken; // Non sembra usato nel .cpp fornito

    // --- Helper Privati Esistenti (rimangono sincroni) ---
    std::string getAssetsUrl();
    std::string getCatalogUrl(const std::string& ns);
    std::string makeApiRequest(const std::string& url, const std::string& token = "", const std::string& method = "GET", const std::string& body = "", const std::vector<std::string>& headers = {});

    // Aggiungiamo una funzione helper interna per la logica sincrona effettiva,
    // così non duplichiamo codice tra GetAllAssets e GetAllAssetsAsync.
    std::vector<EpicGames::Asset> performGetAllAssetsSync();
    std::map<std::string, EpicGames::CatalogItem> performGetCatalogItemsSync(
         const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
         const std::string& country,
         const std::string& locale);

};

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
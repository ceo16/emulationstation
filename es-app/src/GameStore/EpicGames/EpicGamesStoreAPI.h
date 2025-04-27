#pragma once
#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H

#include <string>
#include <vector>
#include <map>
#include <future> // Per std::future (chiamate asincrone)
#include "GameStore/EpicGames/EpicGamesModels.h" // <<< Include le struct definite sopra
#include "GameStore/EpicGames/EpicGamesAuth.h"   // <<< Assumiamo esista e gestisca l'autenticazione

class EpicGamesStoreAPI {
public:
    // Passa l'istanza di Auth (o crea un membro privato se API la gestisce internamente)
    EpicGamesStoreAPI(EpicGamesAuth* auth);
    ~EpicGamesStoreAPI();

    // --- METODI API PRINCIPALI (ASINCRONI) ---

    // Ottiene tutti gli asset (giochi posseduti) per mappare AppName -> CatalogItemId
    std::future<std::vector<EpicGames::Asset>> GetAllAssetsAsync();

    // Ottiene i dettagli del catalogo per una lista di giochi
    // itemsToFetch: vettore di pair(namespace, catalogItemId)
    std::future<std::map<std::string, EpicGames::CatalogItem>> GetCatalogItemsAsync(
        const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
        const std::string& country = "IT", // Parametri opzionali con default
        const std::string& locale = "it-IT");
	EpicGamesAuth* getAuth();


    // --- (Opzionale) Versioni Sincrone (per test o casi semplici) ---
    std::vector<EpicGames::Asset> GetAllAssets();
    std::map<std::string, EpicGames::CatalogItem> GetCatalogItems(
         const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
         const std::string& country = "IT",
         const std::string& locale = "it-IT");


private:
    EpicGamesAuth* mAuth; // Puntatore all'oggetto che gestisce l'autenticazione e il token
    std::string mAccessToken; // Potremmo voler memorizzare il token qui quando serve

    // Helper interni per costruire gli URL corretti (basati su Playnite/.ini se necessario)
    std::string getAssetsUrl();
    std::string getCatalogUrl(const std::string& ns);

    // Helper per eseguire richieste HTTP (potrebbe usare HttpReq)
    // Riceve: metodo, url, token (opzionale), body (opzionale), headers (opzionale)
    // Restituisce: stringa della risposta JSON
    std::string makeApiRequest(const std::string& url, const std::string& token = "", const std::string& method = "GET", const std::string& body = "", const std::vector<std::string>& headers = {});

};

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESSTOREAPI_H
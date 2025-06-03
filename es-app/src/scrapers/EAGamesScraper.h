// emulationstation-master/es-app/src/scrapers/EAGamesScraper.h
#pragma once

#include "scrapers/Scraper.h"
#include "GameStore/EAGames/EAGamesStore.h" // Per EAGamesStore::EAGameData
#include <string>
#include <set>
#include <functional>
#include <memory> // Per std::unique_ptr

// Forward declarations
// class EAGamesStore; // Già incluso sopra
class SystemData;
// class FileData; // Incluso da Scraper.h -> ScraperSearchParams

// Definiamo una classe concreta che eredita da ScraperRequest (non ScraperHttpRequest)
// per gestire meglio le chiamate API asincrone interne.
class EAGamesScraperRequest : public ScraperRequest
{
public:
    EAGamesScraperRequest(std::vector<ScraperSearchResult>& resultsWrite,
                          const ScraperSearchParams& params, // Passiamo i parametri per accedere a params.game
                          EAGamesStore* eaStore,
                          const std::string& gameIdToScrape); // L'ID specifico (OfferID o MasterTitleID) che lo scraper userà

    void update() override; // Qui gestirà la logica asincrona
	
	~EAGamesScraperRequest() override; // <--- AGGIUNGI QUESTA RIGA

private:
    ScraperSearchParams mSearchParams; // Riferimento ai parametri originali (contiene game, system, etc.)
    EAGamesStore* mEaStoreInstance;
    std::string mGameIdForApi; // L'ID (OfferID o MasterTitleID) da usare per la chiamata API
    bool mRequestLaunched;
};


class EAGamesScraper : public Scraper
{
public:
    EAGamesScraper();

    bool isSupportedPlatform(SystemData* system) override;
    const std::set<ScraperMediaSource>& getSupportedMedias() override;

protected:
    void generateRequests(const ScraperSearchParams& params,
                          std::queue<std::unique_ptr<ScraperRequest>>& requests,
                          std::vector<ScraperSearchResult>& results) override;

private:
    EAGamesStore* getEaStore(); // Helper per ottenere l'istanza di EAGamesStore
    std::set<ScraperMediaSource> mSupportedMedia;
};
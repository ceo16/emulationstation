// emulationstation-master/es-app/src/scrapers/IGDBScraper.h
#pragma once

#include "scrapers/Scraper.h"
#include "GameStore/IGDB/IGDBAPI.h"    // Per la classe IGDBAPI
#include "GameStore/IGDB/IGDBModels.h" // Per i modelli di dati IGDB
#include <string>
#include <set>
#include <functional>
#include <memory>
#include <queue>

class SystemData;

// Classe per la richiesta di scraping di un singolo gioco tramite IGDB
class IGDBScraperRequest : public ScraperRequest
{
public:
    IGDBScraperRequest(std::vector<ScraperSearchResult>& resultsWrite,
                       const ScraperSearchParams& params,
                       IGDB::IGDBAPI* igdbApi,
                       const std::string& scraperName); // <--- AGGIUNGI QUESTO ARGOMENTO

    void update() override; // Gestisce la logica asincrona della richiesta

    ~IGDBScraperRequest(); // <--- RIMUOVI 'override' qui

private:
    const ScraperSearchParams& mSearchParams; // Copia dei parametri (contiene game, system, etc.)
    IGDB::IGDBAPI* mIgdbApi;                 // Puntatore all'istanza dell'API di IGDB
    std::string mGameName;                    // Nome del gioco da cercare
    bool mSearchLaunched;                     // Flag per indicare se la ricerca è stata avviata
    std::string mScraperName;                 // <--- AGGIUNGI QUESTO MEMBRO
	

    // Stato per la gestione delle chiamate a due fasi (ricerca e dettagli)
    enum class RequestState {
        SEARCHING_GAME,
        FETCHING_DETAILS,
		FETCHING_LOGO,
        DONE
    } mState;

    std::vector<IGDB::GameMetadata> mSearchResults; // Risultati della ricerca iniziale
    size_t mCurrentSearchResultIndex;               // Indice del risultato corrente da processare
	ScraperSearchResult mTempResult;
};


// Classe principale dello scraper IGDB
class IGDBScraper : public Scraper
{
public:
    IGDBScraper();
    ~IGDBScraper(); // <--- RIMUOVI 'override' qui

    bool isSupportedPlatform(SystemData* system) override;
    const std::set<ScraperMediaSource>& getSupportedMedias() override;

protected:
    void generateRequests(const ScraperSearchParams& params,
                          std::queue<std::unique_ptr<ScraperRequest>>& requests,
                          std::vector<ScraperSearchResult>& results) override;

private:
    std::set<ScraperMediaSource> mSupportedMedia;

    // Credenziali API di IGDB (Client ID e Access Token)
    std::string mClientId;
    std::string mAccessToken;

    std::unique_ptr<IGDB::IGDBAPI> mIgdbApi; // Istanza dell'API di IGDB

    // Funzione per ottenere o refreshare il token di accesso (da implementare)
    bool ensureAccessToken();
};
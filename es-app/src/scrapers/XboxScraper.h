#pragma once
#ifndef ES_APP_SCRAPERS_XBOX_SCRAPER_H
#define ES_APP_SCRAPERS_XBOX_SCRAPER_H

#include "scrapers/Scraper.h"
#include "GameStore/Xbox/XboxStoreAPI.h" // << ASSICURATI CHE SIA QUI, FUORI DAL NAMESPACE

// Forward declarations per tipi usati solo come puntatori/riferimenti se Scraper.h non li definisce già completamente
// class GameStoreManager; // Probabilmente non necessario se getXboxApi() è privato e usa l'include nel .cpp
// class XboxAuth;       // Probabilmente non necessario

namespace Scrapers
{

// Forward declarations per tipi USATI nel namespace Scrapers, se non già inclusi da Scraper.h
// class XboxStoreAPI; // RIMOSSO DA QUI - XboxStoreAPI non è in Scrapers namespace

class XboxScraperHttpRequest : public ScraperHttpRequest
{
public:
    enum class RequestType {
        MEDIA_INFO 
    };

    XboxScraperHttpRequest(
        std::vector<ScraperSearchResult>& resultsWrite, 
        const std::string& url,                         
        const ScraperSearchParams& searchParams,        
        ::XboxStoreAPI* api, // << Usa ::XboxStoreAPI* o solo XboxStoreAPI* se globale
        RequestType type,                               
        std::string pfn,                           
        std::string productIdForMedia,              
        HttpReqOptions* options = nullptr 
    );

protected:
    bool process(HttpReq* request, std::vector<ScraperSearchResult>& results) override;

private:
    ScraperSearchParams mSearchParams; 
    ::XboxStoreAPI* mXboxApi; // << Usa ::XboxStoreAPI* o solo XboxStoreAPI*
    RequestType mRequestType;
    std::string mPfn;                
    std::string mProductIdForMedia;  

    bool processDisplayCatalogResponse(HttpReq* request, std::vector<ScraperSearchResult>& results);
};


class XboxScraper : public Scraper
{
public:
    XboxScraper(); 

    void generateRequests(
        const ScraperSearchParams& params,
        std::queue<std::unique_ptr<ScraperRequest>>& requests, 
        std::vector<ScraperSearchResult>& results) override;   

    bool isSupportedPlatform(SystemData* system) override;
    const std::set<ScraperMediaSource>& getSupportedMedias() override;

private:
    // Cambia il tipo restituito per corrispondere al namespace effettivo di XboxStoreAPI
    ::XboxStoreAPI* getXboxApi(); // << Usa ::XboxStoreAPI* o solo XboxStoreAPI*
    std::set<ScraperMediaSource> mSupportedMedia; 
};

} // namespace Scrapers
#endif // ES_APP_SCRAPERS_XBOX_SCRAPER_H
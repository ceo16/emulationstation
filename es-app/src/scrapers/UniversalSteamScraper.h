#pragma once
#include "scrapers/Scraper.h"
#include <set> // Necessario per std::set

// La classe UniversalSteamScraper eredita dalla classe base Scraper.
// Questo è il modo corretto per creare un nuovo scraper nel tuo progetto.
class UniversalSteamScraper : public Scraper
{
public:
    // La nostra classe deve implementare tutti i metodi virtuali puri della classe base.
    
    void generateRequests(const ScraperSearchParams& params,
                          std::queue<std::unique_ptr<ScraperRequest>>& requests,
                          std::vector<ScraperSearchResult>& results) override;

    bool isSupportedPlatform(SystemData* system) override;

    const std::set<ScraperMediaSource>& getSupportedMedias() override;
};
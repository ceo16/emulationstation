#pragma once
#ifndef ES_APP_SCRAPERS_EPIC_GAMES_H
#define ES_APP_SCRAPERS_EPIC_GAMES_H

#include "scrapers/Scraper.h"
#include <vector>
#include <string>
#include <memory> // Per std::unique_ptr se non già incluso

class EpicGamesScraper : public Scraper
{
public:
    void generateRequests(const ScraperSearchParams& params,
        std::queue<std::unique_ptr<ScraperRequest>>& requests,
        std::vector<ScraperSearchResult>& results) override;

    bool isSupportedPlatform(SystemData* system) override;
    const std::set<ScraperMediaSource>& getSupportedMedias() override;
};

#endif // ES_APP_SCRAPERS_EPIC_GAMES_H
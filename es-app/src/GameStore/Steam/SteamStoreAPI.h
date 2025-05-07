#pragma once
#ifndef ES_APP_GAMESTORE_STEAM_STORE_API_H
#define ES_APP_GAMESTORE_STEAM_STORE_API_H

#include <string>
#include <vector>
#include <map>
#include "SteamAuth.h"
#include "HttpReq.h"
#include "json.hpp" // nlohmann/json
#include "Log.h"

// Strutture dati per le risposte API (semplificate)
namespace Steam {
    struct OwnedGame {
        unsigned int appId;
        std::string name;
        unsigned int playtimeForever; // in minuti
        std::string imgIconUrl;
        std::string imgLogoUrl;
        // Aggiungi altri campi se necessario (es. last_played_time)
    };

    struct AppDetails { // Dati principali per lo scraper
        unsigned int appId;
        std::string type; // "game", "dlc", "mod", etc.
        std::string name;
        std::string detailedDescription;
        std::string aboutTheGame;
        std::string shortDescription;
        std::string headerImage; // URL
        std::vector<std::string> developers;
        std::vector<std::string> publishers;
        // std::vector<PriceOverview> priceOverview; // Potrebbe servire
        // std::vector<PackageGroup> packageGroups;
        // Platforms platforms; (windows, mac, linux)
        // Metacritic metacritic;
        struct Category { std::string id; std::string description; };
        std::vector<Category> categories; // Es. Single-player, Multi-player, Steam Achievements
        struct Genre { std::string id; std::string description; };
        std::vector<Genre> genres;       // Es. Action, Indie, RPG
        struct Screenshot { std::string id; std::string pathThumbnail; std::string pathFull; };
        std::vector<Screenshot> screenshots;
        struct ReleaseDate { bool comingSoon; std::string date; }; // date es. "14 Nov, 2019"
        ReleaseDate releaseDate;
        // SupportInfo supportInfo;
        std::string legalNotice;
        // Aggiungi altri campi come background_raw, movies
    };
} // namespace Steam


class SteamStoreAPI
{
public:
    SteamStoreAPI(SteamAuth* auth);

    // TODO: Implementare chiamate API Steam
    // https://partner.steamgames.com/doc/webapi
    // https://wiki.teamfortress.com/wiki/User:RJackson/StorefrontAPI#appdetails

    // Ottiene i giochi posseduti dall'utente autenticato
    std::vector<Steam::OwnedGame> GetOwnedGames(const std::string& steamId, const std::string& apiKey, bool includeAppInfo = true, bool includePlayedFreeGames = true, bool includeFreeSubs = false);

    // Ottiene i dettagli di una o più app (per lo scraper)
    // Restituisce una mappa <appId, AppDetails>
    std::map<unsigned int, Steam::AppDetails> GetAppDetails(const std::vector<unsigned int>& appIds, const std::string& countryCode = "US", const std::string& language = "english");


private:
    SteamAuth* mAuth; // Non posseduto, solo reference
    std::unique_ptr<HttpReq> createHttpRequest(const std::string& url);

    // Funzioni helper per il parsing JSON
    Steam::OwnedGame parseOwnedGame(const nlohmann::json& gameJson);
    Steam::AppDetails parseAppDetails(unsigned int appId, const nlohmann::json& appJsonData);
};

#endif // ES_APP_GAMESTORE_STEAM_STORE_API_H
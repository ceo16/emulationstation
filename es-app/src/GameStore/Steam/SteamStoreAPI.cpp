#include "GameStore/Steam/SteamStoreAPI.h" // Assicurati che il percorso sia corretto
#include "utils/StringUtil.h" // Per UrlEncode se necessario
#include "HttpReq.h"          // Includi HttpReq.h
#include "Log.h"              // Per il logging
#include "json.hpp"           // Per nlohmann/json

SteamStoreAPI::SteamStoreAPI(SteamAuth* auth) : mAuth(auth)
{
    if (!mAuth) {
        LOG(LogError) << "SteamStoreAPI: Auth object is null!";
    }
}

std::unique_ptr<HttpReq> SteamStoreAPI::createHttpRequest(const std::string& url)
{
    auto req = std::make_unique<HttpReq>(url);
    // Potresti voler impostare un User-Agent qui se il tuo HttpReq lo supporta
    // Esempio: req->SetUserAgent("EmulationStation/1.0 SteamIntegration");
    return req;
}

std::vector<Steam::OwnedGame> SteamStoreAPI::GetOwnedGames(const std::string& steamId, const std::string& apiKey, bool includeAppInfo, bool includePlayedFreeGames, bool includeFreeSubs)
{
    std::vector<Steam::OwnedGame> games;
    if (steamId.empty() || apiKey.empty()) {
        LOG(LogWarning) << "SteamStoreAPI::GetOwnedGames - SteamID o API Key mancanti.";
        return games;
    }

    std::string url = "https://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/?key=" + apiKey +
                      "&steamid=" + steamId + "&format=json";
    if (includeAppInfo) url += "&include_appinfo=1";
    if (includePlayedFreeGames) url += "&include_played_free_games=1";
    // include_free_sub è un parametro che esiste, ma potrebbe non essere sempre utile
    // if (includeFreeSubs) url += "&include_free_sub=1";

    LOG(LogDebug) << "SteamStoreAPI: Chiamata GetOwnedGames: " << url;
    auto req = createHttpRequest(url);
    req->wait(); // wait() è void
    HttpReq::Status status = req->status(); // Ottieni lo stato dopo wait()

    if (status == HttpReq::REQ_SUCCESS && !req->getContent().empty()) { // CORRETTO
        try {
            nlohmann::json responseJson = nlohmann::json::parse(req->getContent());
            if (responseJson.contains("response") && responseJson["response"].is_object() &&
                responseJson["response"].contains("games") && responseJson["response"]["games"].is_array()) {
                for (const auto& gameJson : responseJson["response"]["games"]) {
                    games.push_back(parseOwnedGame(gameJson));
                }
                LOG(LogInfo) << "SteamStoreAPI: Trovati " << games.size() << " giochi posseduti per SteamID " << steamId;
            } else if (responseJson.contains("response") && responseJson["response"].is_object() &&
                       responseJson["response"].value("game_count", -1) == 0) { // Controlla game_count se games è assente o vuoto
                 LOG(LogInfo) << "SteamStoreAPI: Nessun gioco trovato (game_count: 0) per SteamID " << steamId;
            }
            else {
                LOG(LogError) << "SteamStoreAPI::GetOwnedGames - Risposta JSON non valida o campo 'games' mancante/malformato.";
                LOG(LogDebug) << "Risposta ricevuta: " << req->getContent();
            }
        } catch (const nlohmann::json::exception& e) {
            LOG(LogError) << "SteamStoreAPI::GetOwnedGames - Errore parsing JSON: " << e.what();
            LOG(LogDebug) << "Risposta ricevuta: " << req->getContent();
        }
    } else {
        LOG(LogError) << "SteamStoreAPI::GetOwnedGames - Errore HTTP. Status: " << static_cast<int>(status) << " - " << req->getErrorMsg(); // CORRETTO
    }
    return games;
}

Steam::OwnedGame SteamStoreAPI::parseOwnedGame(const nlohmann::json& gameJson)
{
    Steam::OwnedGame game;
    game.appId = gameJson.value("appid", 0U); // Usa 0U per unsigned int
    game.name = gameJson.value("name", "N/A");
    game.playtimeForever = gameJson.value("playtime_forever", 0U);

    // Costruisci gli URL delle immagini se l'AppID e i fragment dell'URL sono presenti
    if (game.appId != 0) {
        if (gameJson.contains("img_icon_url") && gameJson["img_icon_url"].is_string()) {
            game.imgIconUrl = "http://media.steampowered.com/steamcommunity/public/images/apps/" +
                              std::to_string(game.appId) + "/" + gameJson.value("img_icon_url", "") + ".jpg";
        }
        // img_logo_url non è sempre un hash, a volte è un percorso completo.
        // Meglio affidarsi agli URL delle immagini ottenuti da GetAppDetails o costruiti per lo scraper.
    }
    return game;
}

std::map<unsigned int, Steam::AppDetails> SteamStoreAPI::GetAppDetails(const std::vector<unsigned int>& appIds, const std::string& countryCode, const std::string& language)
{
    std::map<unsigned int, Steam::AppDetails> allDetails;
    if (appIds.empty()) return allDetails;

    std::string appIdsStr;
    for (size_t i = 0; i < appIds.size(); ++i) {
        appIdsStr += std::to_string(appIds[i]);
        if (i < appIds.size() - 1) appIdsStr += ",";
    }

    std::string url = "https://store.steampowered.com/api/appdetails?appids=" + appIdsStr;
    if (!countryCode.empty()) url += "&cc=" + Utils::String::toUpper(countryCode); // Steam usa cc maiuscolo
    if (!language.empty()) url += "&l=" + language; // es. "english", "italian"

    LOG(LogDebug) << "SteamStoreAPI: Chiamata GetAppDetails: " << url;
    auto req = createHttpRequest(url);
    req->wait(); // wait() è void
    HttpReq::Status status = req->status(); // Ottieni lo stato dopo wait()

    if (status == HttpReq::REQ_SUCCESS && !req->getContent().empty()) { // CORRETTO
        try {
            nlohmann::json responseJson = nlohmann::json::parse(req->getContent());
            for (unsigned int appId : appIds) {
                std::string appIdKey = std::to_string(appId);
                if (responseJson.contains(appIdKey) &&
                    responseJson[appIdKey].is_object() &&
                    responseJson[appIdKey].value("success", false) && // Controlla il flag 'success'
                    responseJson[appIdKey].contains("data") &&
                    responseJson[appIdKey]["data"].is_object())
                {
                    allDetails[appId] = parseAppDetails(appId, responseJson[appIdKey]["data"]);
                } else {
                    LOG(LogWarning) << "SteamStoreAPI::GetAppDetails - Nessun dato o fallimento per AppID: " << appIdKey;
                    if (responseJson.contains(appIdKey) && responseJson[appIdKey].is_object()) {
                         LOG(LogDebug) << "  Success flag: " << responseJson[appIdKey].value("success", false)
                                       << ", Data presente: " << responseJson[appIdKey].contains("data");
                    } else if (!responseJson.contains(appIdKey)) {
                        LOG(LogDebug) << "  Chiave AppID " << appIdKey << " non trovata nella risposta JSON.";
                    }
                }
            }
            LOG(LogInfo) << "SteamStoreAPI: Ottenuti dettagli per " << allDetails.size() << " app su " << appIds.size() << " richieste.";

        } catch (const nlohmann::json::exception& e) {
            LOG(LogError) << "SteamStoreAPI::GetAppDetails - Errore parsing JSON: " << e.what();
            LOG(LogDebug) << "Risposta ricevuta: " << req->getContent();
        }
    } else {
        LOG(LogError) << "SteamStoreAPI::GetAppDetails - Errore HTTP. Status: " << static_cast<int>(status) << " - " << req->getErrorMsg(); // CORRETTO
    }
    return allDetails;
}

Steam::AppDetails SteamStoreAPI::parseAppDetails(unsigned int appId, const nlohmann::json& data)
{
    Steam::AppDetails details;
    details.appId = appId;
    details.type = data.value("type", "N/A");
    details.name = data.value("name", "N/A");
    details.detailedDescription = data.value("detailed_description", "");
    details.aboutTheGame = data.value("about_the_game", "");
    details.shortDescription = data.value("short_description", "");
    details.headerImage = data.value("header_image", "");
    details.legalNotice = data.value("legal_notice", ""); // Aggiunto se vuoi usarlo

    if (data.contains("developers") && data["developers"].is_array()) {
        for (const auto& dev : data["developers"]) {
            if (dev.is_string()) details.developers.push_back(dev.get<std::string>());
        }
    }
    if (data.contains("publishers") && data["publishers"].is_array()) {
        for (const auto& pub : data["publishers"]) {
            if (pub.is_string()) details.publishers.push_back(pub.get<std::string>());
        }
    }

    if (data.contains("genres") && data["genres"].is_array()) {
        for (const auto& genreJson : data["genres"]) {
            if (genreJson.is_object() && genreJson.contains("description") && genreJson["description"].is_string()) {
                details.genres.push_back({genreJson.value("id", ""), genreJson.value("description", "")});
            }
        }
    }
    if (data.contains("categories") && data["categories"].is_array()) {
    for (const auto& catJson : data["categories"]) {
        if (catJson.is_object() && catJson.contains("description") && catJson["description"].is_string()) {
            std::string categoryIdStr;
            if (catJson.contains("id")) { // Controlla se il campo "id" esiste
                if (catJson["id"].is_number()) {
                    categoryIdStr = std::to_string(catJson["id"].get<long long>()); // Leggi come numero, converti a stringa
                } else if (catJson["id"].is_string()) {
                    categoryIdStr = catJson["id"].get<std::string>(); // Se fosse una stringa, prendila
                } else {
                    LOG(LogWarning) << "SteamStoreAPI: Category ID di tipo inatteso per AppID " << appId;
                    categoryIdStr = ""; // o un valore di default
                }
            }
            // Assumendo che Steam::Category sia { std::string id; std::string description; }
            details.categories.push_back({categoryIdStr, catJson.value("description", "")});
        }
    }
}
    if (data.contains("screenshots") && data["screenshots"].is_array()) {
        for (const auto& ssJson : data["screenshots"]) {
            if (ssJson.is_object()) {
                details.screenshots.push_back({
                    // id in screenshot è un int, ma lo memorizziamo come stringa per coerenza con altri ID
                    ssJson.contains("id") ? std::to_string(ssJson.value("id", 0)) : "",
                    ssJson.value("path_thumbnail", ""),
                    ssJson.value("path_full", "")
                });
            }
        }
    }
    if (data.contains("release_date") && data["release_date"].is_object()) {
        details.releaseDate.comingSoon = data["release_date"].value("coming_soon", false);
        details.releaseDate.date = data["release_date"].value("date", "");
    }

    return details;
}
#include "GameStore/Steam/SteamStoreAPI.h"
#include "utils/StringUtil.h"
#include "HttpReq.h"
#include "Log.h"
#include "json.hpp"
#include "guis/GuiWebViewAuthLogin.h" // Per usare la WebView
#include "Settings.h" // Per ottenere lo SteamID dell'utente

SteamStoreAPI::SteamStoreAPI(SteamAuth* auth) : mAuth(auth)
{
    if (!mAuth) {
        LOG(LogError) << "SteamStoreAPI: Auth object is null!";
    }
}

std::unique_ptr<HttpReq> SteamStoreAPI::createHttpRequest(const std::string& url)
{
    auto req = std::make_unique<HttpReq>(url);
    // Potresti voler impostare un User-Agent qui se il tuo HttpReq lo support
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
    details.headerImage = data.value("header_image", ""); // Già presente
    details.legalNotice = data.value("legal_notice", "");   // Già presente

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
    if (data.contains("categories") && data["categories"].is_array()) { // Già presente nel tuo codice
        for (const auto& catJson : data["categories"]) {
            if (catJson.is_object() && catJson.contains("description") && catJson["description"].is_string()) {
                std::string categoryIdStr;
                if (catJson.contains("id")) { 
                    if (catJson["id"].is_number()) {
                        categoryIdStr = std::to_string(catJson["id"].get<long long>());
                    } else if (catJson["id"].is_string()) {
                        categoryIdStr = catJson["id"].get<std::string>();
                    }
                }
                details.categories.push_back({categoryIdStr, catJson.value("description", "")});
            }
        }
    }
    if (data.contains("screenshots") && data["screenshots"].is_array()) { // Già presente
        for (const auto& ssJson : data["screenshots"]) {
            if (ssJson.is_object()) {
                details.screenshots.push_back({
                    ssJson.contains("id") ? std::to_string(ssJson.value("id", 0)) : "",
                    ssJson.value("path_thumbnail", ""),
                    ssJson.value("path_full", "")
                });
            }
        }
    }
    if (data.contains("release_date") && data["release_date"].is_object()) { // Già presente
        details.releaseDate.comingSoon = data["release_date"].value("coming_soon", false);
        details.releaseDate.date = data["release_date"].value("date", "");
    }

    // --- INIZIO NUOVA LOGICA DI ESTRAZIONE PER MEDIA AGGIUNTIVI ---

    // Estrai Background URL (spesso chiamato "background_raw" nel JSON per l'immagine non elaborata)
    if (data.contains("background_raw") && data["background_raw"].is_string()) {
        details.background_raw_url = data.value("background_raw", "");
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Trovato background_raw_url: " << details.background_raw_url;
    } else if (data.contains("background") && data["background"].is_string()) { // Fallback se background_raw non c'è
        details.background_raw_url = data.value("background", "");
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Trovato background url (fallback): " << details.background_raw_url;
    }

    // Estrai e costruisci URL per Library Assets
    if (data.contains("library_assets") && data["library_assets"].is_object()) {
        const auto& assetsJson = data["library_assets"];
        std::string appId_str = std::to_string(appId); // Usa l'appId passato alla funzione
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Processando library_assets.";

        if (assetsJson.contains("library_capsule") && assetsJson["library_capsule"].is_string()) {
            std::string filename = assetsJson.value("library_capsule", "");
            if (!filename.empty())
                details.library_assets.capsule = "https://cdn.akamai.steamstatic.com/steam/apps/" + appId_str + "/" + filename;
        }
        if (assetsJson.contains("library_hero") && assetsJson["library_hero"].is_string()) {
             std::string filename = assetsJson.value("library_hero", "");
            if (!filename.empty())
                details.library_assets.hero = "https://cdn.akamai.steamstatic.com/steam/apps/" + appId_str + "/" + filename;
        }
        if (assetsJson.contains("library_logo") && assetsJson["library_logo"].is_string()) {
             std::string filename = assetsJson.value("library_logo", "");
            if (!filename.empty())
                details.library_assets.logo = "https://cdn.akamai.steamstatic.com/steam/apps/" + appId_str + "/" + filename;
        }
        // "library_header" o "header" (a volte chiamato "vertical_capsule" nel JSON, ma header è più comune in library_assets)
        if (assetsJson.contains("library_header") && assetsJson["library_header"].is_string()) {
             std::string filename = assetsJson.value("library_header", "");
            if (!filename.empty())
                details.library_assets.header = "https://cdn.akamai.steamstatic.com/steam/apps/" + appId_str + "/" + filename;
        } else if (assetsJson.contains("header") && assetsJson["header"].is_string()) { // Fallback
             std::string filename = assetsJson.value("header", "");
            if (!filename.empty())
                details.library_assets.header = "https://cdn.akamai.steamstatic.com/steam/apps/" + appId_str + "/" + filename;
        }
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Hero: " << details.library_assets.hero 
                      << ", Capsule: " << details.library_assets.capsule 
                      << ", Logo: " << details.library_assets.logo;
    } else {
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Nessun library_assets trovato.";
    }


    // Estrai Video/Movies
    if (data.contains("movies") && data["movies"].is_array()) {
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Processando movies.";
        for (const auto& movieJson : data["movies"]) {
            if (!movieJson.is_object()) continue;

            Steam::MovieInfo movieInfo;
            movieInfo.id = movieJson.value("id", 0u);
            movieInfo.name = movieJson.value("name", "");
            movieInfo.thumbnail_url = movieJson.value("thumbnail", ""); // URL completo dell'anteprima del video
            movieInfo.highlight = movieJson.value("highlight", false);

            if (movieJson.contains("mp4") && movieJson["mp4"].is_object()) {
                movieInfo.mp4_480_url = movieJson["mp4"].value("480", "");
                movieInfo.mp4_max_url = movieJson["mp4"].value("max", "");
            }
            if (movieJson.contains("webm") && movieJson["webm"].is_object()) {
                movieInfo.webm_480_url = movieJson["webm"].value("480", "");
                movieInfo.webm_max_url = movieJson["webm"].value("max", "");
            }
            
            if (!movieInfo.mp4_max_url.empty() || !movieInfo.mp4_480_url.empty()) { // Priorità a MP4
                details.movies.push_back(movieInfo);
                LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Aggiunto video MP4: " << movieInfo.name;
            } else if (!movieInfo.webm_max_url.empty() || !movieInfo.webm_480_url.empty()) { // Fallback a WebM
                details.movies.push_back(movieInfo);
                LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Aggiunto video WebM (fallback): " << movieInfo.name;
            }
        }
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Trovati " << details.movies.size() << " video(s).";
    } else {
        LOG(LogDebug) << "SteamStoreAPI: AppID " << appId << " - Nessun movies array trovato.";
    }
    // --- FINE NUOVA LOGICA DI ESTRAZIONE ---

    return details;
}

void SteamStoreAPI::getOwnedGamesViaScraping(Window* window, const std::string& steamId, std::function<void(bool success, const std::string& gameDataJson)> callback)
{
    LOG(LogInfo) << "[SteamStoreAPI] getOwnedGamesViaScraping - Inizio.";
    Log::flush(); // Forza il log immediatamente.

    if (!mAuth || !mAuth->isAuthenticated() || steamId.empty() || !window) {
        LOG(LogError) << "[SteamStoreAPI] getOwnedGamesViaScraping: Prerequisiti mancanti (autenticazione, SteamID o Window).";
        Log::flush();
        callback(false, "");
        return;
    }

    std::string gamesPageUrl = "https://steamcommunity.com/profiles/" + steamId + "/games/?tab=all";

    LOG(LogInfo) << "[SteamStoreAPI] Avvio scraping giochi Steam da: " << gamesPageUrl;
    Log::flush();

    // NUOVI LOG: Prima e dopo la creazione e il push della WebView.
    LOG(LogDebug) << "[SteamStoreAPI] About to create GuiWebViewAuthLogin for scraping.";
    Log::flush();
    
    // Controlla il puntatore a window di nuovo, per sicurezza, anche se è stato controllato all'inizio della funzione.
    if (!window) {
        LOG(LogError) << "[SteamStoreAPI] Window pointer is null before creating WebView. Aborting scraping.";
        Log::flush();
        callback(false, "Window is null.");
        return;
    }

   auto webViewScraper = new GuiWebViewAuthLogin(
    window,
    gamesPageUrl,
    "Steam",
    "",
    GuiWebViewAuthLogin::AuthMode::FETCH_STEAM_GAMES_JSON,
    false // <--- DIGLI DI ESSERE INVISIBILE
);

// --- INIZIO MODIFICA ---
// Recupera i cookie di sessione dal gestore di autenticazione.
// NOTA: Questo presume che la tua classe SteamAuth salvi e fornisca questi cookie.
// Potrebbe essere necessario aggiungere le funzioni getCookie() a SteamAuth.
std::string sessionCookie = mAuth->getCookie("sessionid"); 
std::string secureCookie = mAuth->getCookie("steamLoginSecure"); 

if (!sessionCookie.empty() && !secureCookie.empty())
{
    LOG(LogInfo) << "[SteamStoreAPI] Inserimento cookie di sessione nella WebView per lo scraping.";
    // Inserisce i cookie.
    // NOTA: La funzione addCookie è ipotetica, adatta il nome se la tua si chiama diversamente.
    webViewScraper->addCookie("sessionid", sessionCookie, "steamcommunity.com");
    webViewScraper->addCookie("steamLoginSecure", secureCookie, "steamcommunity.com");
}
else
{
    LOG(LogWarning) << "[SteamStoreAPI] Cookie di sessione non trovati in SteamAuth. Lo scraping potrebbe fallire e richiedere il login.";
}
// --- FINE MODIFICA ---

    LOG(LogDebug) << "[SteamStoreAPI] GuiWebViewAuthLogin created. About to push to window.";
    Log::flush();

    // <<< PUNTO CRITICO: La creazione della WebView e il push sullo stack della GUI.
    // Il crash potrebbe avvenire durante l'allocazione, l'inizializzazione o l'aggiunta alla GUI.
    window->pushGui(webViewScraper);
    
    LOG(LogDebug) << "[SteamStoreAPI] GuiWebViewAuthLogin pushed to window. Setting callback.";
    Log::flush();

    webViewScraper->setOnLoginFinishedCallback(
        [callback, window, webViewScraper](bool success, const std::string& jsonData)
    {
        // Questa callback viene eseguita sul thread UI dopo che la WebView ha completato la sua operazione.
        LOG(LogDebug) << "[SteamStoreAPI] Callback from WebView scraper received on UI thread. Success: " << (success ? "true" : "false");
        Log::flush();
        if (success && !jsonData.empty() && jsonData != "null") {
            LOG(LogInfo) << "[SteamStoreAPI] Scraping giochi Steam riuscito. JSON ricevuto (primi 200 char): " << jsonData.substr(0, std::min(jsonData.length(), (size_t)200));
            Log::flush();
            callback(true, jsonData); // Invia il JSON dei giochi al chiamante (SteamStore)
        } else {
            LOG(LogError) << "[SteamStoreAPI] Scraping giochi Steam fallito o dati mancanti. Messaggio: " << jsonData;
            Log::flush();
            callback(false, "");
        }
    });

    // La funzione non restituisce immediatamente, poiché la WebView opera in modo asincrono.
    // Il meccanismo promise/future in refreshSteamGamesListAsync gestisce l'attesa.
}
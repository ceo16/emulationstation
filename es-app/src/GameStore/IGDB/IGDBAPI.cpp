// emulationstation-master/es-app/src/GameStore/IGDB/IGDBAPI.cpp
#include "GameStore/IGDB/IGDBAPI.h"
#include "Log.h"
#include "json.hpp"
#include "HttpReq.h" 
#include "utils/StringUtil.h"
#include <thread> 
#include <chrono> 


namespace IGDB {

// URL dell'API di IGDB
const std::string IGDB_API_BASE_URL = "https://api.igdb.com/v4";
const std::string IGDB_API_AUTH_URL = "https://id.twitch.tv/oauth2/token";

IGDBAPI::IGDBAPI(const std::string& clientId, const std::string& accessToken)
    : mClientId(clientId), mAccessToken(accessToken) {
    LOG(LogInfo) << "IGDBAPI: Initialized with Client ID and Access Token."; //
}

// Funzione helper per eseguire richieste HTTP in un thread separato
template<typename ResultType, typename ParserFunc>
void IGDBAPI::executeRequestThreaded(const std::string& url,
                                     const std::string& method,
                                     const std::string& postBody,
                                     const std::vector<std::string>& customHeaders,
                                     const std::string& language,
                                     ParserFunc parser,
                                     std::function<void(ResultType result, bool success)> callback) {

    // Eseguiamo la richiesta nello stesso thread che ha chiamato questa funzione (il thread di ThreadedScraper).
    // Questo è sicuro perché non usiamo '.detach()'.

    HttpReqOptions options;
    options.customHeaders = customHeaders;

    if (!language.empty()) {
        std::string processedLanguage = language;
        size_t underscorePos = processedLanguage.find('_');
        if (underscorePos != std::string::npos) {
            processedLanguage = processedLanguage.substr(0, underscorePos);
        }
        options.customHeaders.push_back("Accept-Language: " + processedLanguage + ", en;q=0.8");
    } else {
        options.customHeaders.push_back("Accept-Language: en");
    }
    
    options.dataToPost = postBody;
    LOG(LogDebug) << "IGDBAPI: Full POST Data to be sent: " << options.dataToPost;

    // Crea la richiesta asincrona
    HttpReq request(url, &options);

    // Usa il metodo wait() per attendere in modo bloccante il completamento della richiesta.
    // Dato che siamo in un thread secondario di ThreadedScraper, non bloccherà l'interfaccia utente.
    request.wait();

    // Ora che la richiesta è completa, processa il risultato
    if (request.status() == HttpReq::REQ_SUCCESS) {
        std::string responseBody = request.getContent();
        LOG(LogDebug) << "IGDB API Response for " << url << ": " << responseBody.substr(0, 500);
        try {
            ResultType parsedResult = parser(responseBody);
            if (callback) callback(parsedResult, true);
        } catch (const std::exception& e) {
            LOG(LogError) << "IGDB API: Failed to parse response for " << url << ": " << e.what();
            if (callback) callback(ResultType{}, false);
        }
    } else {
        LOG(LogError) << "IGDB API: Request to " << url << " failed. Status: "
                      << static_cast<int>(request.status())
                      << " - Error: " << request.getErrorMsg();
        if (callback) callback(ResultType{}, false);
    }
}

void IGDBAPI::searchGames(const std::string& gameName,
                          std::function<void(std::vector<GameMetadata>, bool success)> callback,
                          const std::string& language) {
    if (mClientId.empty() || mAccessToken.empty()) { //
        LOG(LogError) << "IGDBAPI: Client ID or Access Token missing for searchGames."; //
        if (callback) callback({}, false); //
        return;
    }
    if (gameName.empty()) { //
        LOG(LogWarning) << "IGDBAPI: Game name is empty for searchGames."; //
        if (callback) callback({}, false); //
        return;
    }

    std::string url = IGDB_API_BASE_URL + "/games"; //
    std::vector<std::string> headers; //
    headers.push_back("Client-ID: " + mClientId); //
    headers.push_back("Authorization: Bearer " + mAccessToken); //
    headers.push_back("Accept: application/json"); //
    // Content-Type è aggiunto da HttpReq se dataToPost non è vuoto, ma è buona pratica specificarlo
    headers.push_back("Content-Type: text/plain"); // IGDB aspetta text/plain per query APOCALYPTO

    // AGGIUNTO artworks.image_id per la fanart
    std::string cleanGameName = Utils::String::replace(gameName, "\"", ""); 

std::string postBody = "search \"" + cleanGameName + "\"; " // <-- NUOVA RICERCA
                       "fields name, summary, cover.url, cover.image_id, first_release_date, genres.name, "
                       "involved_companies.company.name, involved_companies.developer, involved_companies.publisher, "
                       "artworks.image_id, game_modes.name; "
                       "limit 50;"; // Aumenta il limite per avere più chance

    LOG(LogInfo) << "IGDBAPI: Searching for game: " << gameName << " (Language: " << language << ")"; //
    LOG(LogDebug) << "IGDBAPI: POST Body: " << postBody; //

    executeRequestThreaded<std::vector<GameMetadata>>(url, "POST", postBody, headers, language, // Passa language
        [](const std::string& responseBody) -> std::vector<GameMetadata> {
            std::vector<GameMetadata> results; //
            if (responseBody.empty()) { //
                LOG(LogWarning) << "IGDBAPI (searchGames parser): Empty response body."; //
                return results; //
            }
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody); //
                if (jsonResponse.is_array()) { //
                    for (const auto& gameJson : jsonResponse) {
                        results.push_back(GameMetadata::fromJson(gameJson)); //
                    }
                } else if (jsonResponse.contains("errors") && jsonResponse["errors"].is_array()) { //
                     LOG(LogError) << "IGDBAPI (searchGames parser): API Errors: " << jsonResponse["errors"][0].dump(); //
                } else {
                     LOG(LogError) << "IGDBAPI (searchGames parser): Unexpected JSON response format."; //
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "IGDBAPI (searchGames parser): Exception parsing JSON: " << e.what() << " - Body: " << responseBody.substr(0, 500); //
            }
            return results; //
        },
        callback
    );
}

void IGDBAPI::getGameDetails(const std::string& igdbGameId,
                             std::function<void(GameMetadata, bool success)> callback,
                             const std::string& language) {
    if (mClientId.empty() || mAccessToken.empty()) { //
        LOG(LogError) << "IGDBAPI: Client ID or Access Token missing for getGameDetails."; //
        if (callback) callback({}, false); //
        return;
    }
    if (igdbGameId.empty()) { //
        LOG(LogWarning) << "IGDBAPI: IGDB Game ID is empty for getGameDetails."; //
        if (callback) callback({}, false); //
        return;
    }

    std::string url = IGDB_API_BASE_URL + "/games"; //
    std::vector<std::string> headers; //
    headers.push_back("Client-ID: " + mClientId); //
    headers.push_back("Authorization: Bearer " + mAccessToken); //
    headers.push_back("Accept: application/json"); //
    headers.push_back("Content-Type: text/plain"); // IGDB aspetta text/plain per query APOCALYPTO

    // AGGIUNTO artworks.image_id per la fanart
    std::string postBody = Utils::String::format( //
        "fields name, summary, storyline, first_release_date, genres.name, "
        "involved_companies.company.name, involved_companies.developer, involved_companies.publisher, "
        "cover.url, cover.image_id, screenshots.url, screenshots.image_id, videos.video_id, aggregated_rating, "
        "artworks.image_id, game_modes.name; " // Richiedi image_id per artworks
        "where id = %s; limit 1;", 
        igdbGameId.c_str()
    );

    LOG(LogInfo) << "IGDBAPI: Getting details for game ID: " << igdbGameId << " (Language: " << language << ")"; //
    LOG(LogDebug) << "IGDBAPI: POST Body: " << postBody; //

    executeRequestThreaded<GameMetadata>(url, "POST", postBody, headers, language, // Passa language
        [](const std::string& responseBody) -> GameMetadata {
            if (responseBody.empty()) { //
                LOG(LogWarning) << "IGDBAPI (getGameDetails parser): Empty response body."; //
                return GameMetadata{}; //
            }
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody); //
                if (jsonResponse.is_array() && !jsonResponse.empty()) { //
                    return GameMetadata::fromJson(jsonResponse[0]); //
                } else if (jsonResponse.contains("errors") && jsonResponse["errors"].is_array()) { //
                     LOG(LogError) << "IGDBAPI (getGameDetails parser): API Errors: " << jsonResponse["errors"][0].dump(); //
                } else {
                     LOG(LogError) << "IGDBAPI (getGameDetails parser): Unexpected JSON response format or no results."; //
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "IGDBAPI (getGameDetails parser): Exception parsing JSON: " << e.what() << " - Body: " << responseBody.substr(0, 500); //
            }
            return GameMetadata{}; //
        },
        callback
    );
}
void IGDBAPI::getGameLogo(const std::string& igdbGameId,
                          std::function<void(std::string, bool success)> callback,
                          const std::string& language)
{
    // Controllo delle credenziali
    if (mClientId.empty() || mAccessToken.empty()) {
        LOG(LogError) << "IGDBAPI: Client ID or Access Token missing for getGameLogo.";
        if (callback) callback("", false);
        return;
    }
    if (igdbGameId.empty()) {
        LOG(LogWarning) << "IGDBAPI: IGDB Game ID is empty for getGameLogo.";
        if (callback) callback("", false);
        return;
    }

    // L'endpoint per i loghi è /artworks
    std::string url = IGDB_API_BASE_URL + "/artworks";
    
    // Header standard
    std::vector<std::string> headers;
    headers.push_back("Client-ID: " + mClientId);
    headers.push_back("Authorization: Bearer " + mAccessToken);
    headers.push_back("Accept: application/json");
    headers.push_back("Content-Type: text/plain");

    // Query per ottenere i loghi (category = 3) per l'ID del gioco specifico
     std::string postBody = "fields url, image_id; "
                           "where game = " + igdbGameId + "; "
                           "limit 1;";

    LOG(LogInfo) << "IGDBAPI: Getting artwork (logo fallback) for game ID: " << igdbGameId;
    LOG(LogDebug) << "IGDBAPI: POST Body for getGameLogo (corrected): " << postBody;


    // Usa la tua funzione helper per eseguire la richiesta
    executeRequestThreaded<std::string>(url, "POST", postBody, headers, language,
        // Funzione di parsing del risultato
        [](const std::string& responseBody) -> std::string {
            if (responseBody.empty()) {
                LOG(LogWarning) << "IGDBAPI (getGameLogo parser): Empty response body.";
                return "";
            }
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                if (jsonResponse.is_array() && !jsonResponse.empty()) {
                    // Trovato almeno un logo
                    std::string imageId = jsonResponse[0].value("image_id", "");
                    if (!imageId.empty()) {
                        // Costruisci l'URL completo per il logo, usando un formato standard
                        return "https://images.igdb.com/igdb/image/upload/t_logo_med/" + imageId + ".png";
                    }
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "IGDBAPI (getGameLogo parser): Exception parsing JSON: " << e.what();
            }
            // Ritorna una stringa vuota se non trova nulla o in caso di errore
            return "";
        },
        // Callback finale per restituire il risultato
        callback
    );
}

} // namespace IGDB
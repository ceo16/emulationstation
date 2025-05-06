#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "GameStore/EpicGames/EpicGamesModels.h" // Contiene anche EpicGames namespace
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/StringUtil.h" // Assicurati sia incluso se usato
#include <future>           // Per std::future, std::async
#include <thread>           // Per std::this_thread::sleep_for, std::launch
#include <chrono>           // Per std::chrono::milliseconds
#include <vector>
#include <string>
#include <map>
#include <utility>          // Per std::pair
#include <stdexcept>        // Per std::runtime_error, std::exception

using json = nlohmann::json;

// --- Costruttore e Distruttore ---
EpicGamesStoreAPI::EpicGamesStoreAPI(EpicGamesAuth* auth) : mAuth(auth) {
    if (mAuth == nullptr) {
        LOG(LogError) << "EpicGamesStoreAPI: Constructor received null EpicGamesAuth pointer!";
    } else {
         LOG(LogDebug) << "EpicGamesStoreAPI: Constructor (auth object received)";
    }
}

EpicGamesStoreAPI::~EpicGamesStoreAPI() {
    LOG(LogDebug) << "EpicGamesStoreAPI: Destructor";
}

// --- Getter per Auth ---
EpicGamesAuth* EpicGamesStoreAPI::getAuth() {
     return mAuth;
}


// --- Implementazione Helper URL (Invariata dal tuo codice) ---
std::string EpicGamesStoreAPI::getAssetsUrl() {
    std::string hostname = "launcher-public-service-prod06.ol.epicgames.com";
    return "https://" + hostname + "/launcher/api/public/assets/Windows?label=Live";
}

std::string EpicGamesStoreAPI::getCatalogUrl(const std::string& ns) {
    std::string hostname = "catalog-public-service-prod06.ol.epicgames.com";
    return "https://" + hostname + "/catalog/api/shared/namespace/" + ns;
}

// --- Implementazione Helper makeApiRequest (Invariata dal tuo codice) ---
std::string EpicGamesStoreAPI::makeApiRequest(const std::string& url, const std::string& token, const std::string& method, const std::string& body, const std::vector<std::string>& headers) {
    LOG(LogDebug) << "makeApiRequest: Method=" << method << ", URL=" << url;
    HttpReqOptions options;
    if (!headers.empty()) { options.customHeaders = headers; }
    if (!token.empty()) { options.customHeaders.push_back("Authorization: bearer " + token); }
    if ((method == "POST" || method == "PUT") && !body.empty()) {
         options.dataToPost = body;
         bool contentTypeSet = false;
         for(const auto& h : options.customHeaders) { if (Utils::String::startsWith(Utils::String::toLower(h), "content-type:")) { contentTypeSet = true; break; } }
         if (!contentTypeSet) { options.customHeaders.push_back("Content-Type: application/json"); }
    }

    HttpReq* request = nullptr;
    try {
        request = new HttpReq(url, &options);
        if (!request->wait()) {
            std::string errorMsg = request->getErrorMsg();
            delete request; request = nullptr;
            throw std::runtime_error("HTTP request failed: " + errorMsg);
        }
        if (request->status() != HttpReq::REQ_SUCCESS) {
            std::string responseBodyOnError = request->getContent();
            int statusOnError = request->status();
            delete request; request = nullptr;
            throw std::runtime_error("HTTP request returned non-success status: " + std::to_string(statusOnError) + " Body: " + responseBodyOnError);
        }
        std::string responseBody = request->getContent();
        delete request; request = nullptr;
        return responseBody;
    } catch (...) {
        delete request;
        throw;
    }
}


// --- Helper Interno per Logica Sincrona GetAllAssets ---
std::vector<EpicGames::Asset> EpicGamesStoreAPI::performGetAllAssetsSync()
{
    LOG(LogDebug) << "EpicAPI SYNC_IMPL: Executing performGetAllAssetsSync()";
    if (!mAuth || mAuth->getAccessToken().empty()) { throw std::runtime_error("Not authenticated for GetAllAssets"); }

    std::string currentToken = mAuth->getAccessToken();
    std::string url = getAssetsUrl();
    LOG(LogDebug) << "performGetAllAssetsSync: Requesting URL: " << url;

    std::vector<EpicGames::Asset> assets;
    try {
        HttpReqOptions options;
        options.customHeaders.push_back("Authorization: bearer " + currentToken);
        HttpReq request(url, &options);
        if (!request.wait()) { throw std::runtime_error("HTTP request failed (wait): " + request.getErrorMsg()); }
        if (request.status() != HttpReq::REQ_SUCCESS) { throw std::runtime_error("HTTP request status not success: " + std::to_string(request.status()) + " Body: " + request.getContent()); }
        std::string responseBody = request.getContent();
        LOG(LogDebug) << "performGetAllAssetsSync: Response received (size: " << responseBody.length() << ")";

        if (!responseBody.empty()) {
            json parsedResponse = json::parse(responseBody);
            if (parsedResponse.is_array()) {
                assets.reserve(parsedResponse.size());
                for (const auto& assetJson : parsedResponse) {
                    try {
                        // La chiamata a ::fromJson ora verrà risolta perché definita inline in EpicGamesModels.h
                        assets.push_back(EpicGames::Asset::fromJson(assetJson));
                    } catch (const std::exception& e) { LOG(LogError) << "performGetAllAssetsSync: Error parsing individual asset JSON: " << e.what(); }
                    catch (...) { LOG(LogError) << "performGetAllAssetsSync: Unknown error parsing individual asset JSON."; }
                }
                LOG(LogInfo) << "performGetAllAssetsSync: Successfully parsed " << assets.size() << " assets.";
            } else { LOG(LogError) << "performGetAllAssetsSync: JSON response is not an array."; }
        } else { LOG(LogInfo) << "performGetAllAssetsSync: Response body is empty."; }

    } catch (const json::parse_error& e) { LOG(LogError) << "performGetAllAssetsSync: JSON parsing failed: " << e.what(); throw; }
      catch (const std::exception& e) { LOG(LogError) << "performGetAllAssetsSync: Exception: " << e.what(); throw; }
      catch (...) { LOG(LogError) << "performGetAllAssetsSync: Unknown exception."; throw; }
    return assets;
}

// --- Helper Interno per Logica Sincrona GetCatalogItems ---
std::map<std::string, EpicGames::CatalogItem> EpicGamesStoreAPI::performGetCatalogItemsSync(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicAPI: Executing performGetCatalogItemsSync for " << itemsToFetch.size() << " items.";

    // Controllo iniziale di autenticazione
    if (!mAuth || !mAuth->isAuthenticated()) {
        // Se non è autenticato MA ha un refresh token e l'access token è vuoto (o scaduto),
        // prova un refresh "preventivo". Questo gestisce il caso in cui i token sono stati caricati
        // da file ma l'access token era già scaduto.
        if (mAuth && !mAuth->getRefreshToken().empty()) {
            LOG(LogInfo) << "performGetCatalogItemsSync: Not authenticated but refresh token exists. Attempting preemptive refresh.";
            if (!mAuth->refreshAccessToken()) {
                LOG(LogError) << "performGetCatalogItemsSync: Preemptive token refresh failed.";
                // refreshAccessToken dovrebbe aver chiamato clearAllTokenData se il refresh token era invalido.
                throw std::runtime_error("Authentication failed (preemptive refresh). Please log in again.");
            }
            // Se il refresh ha successo, mAuth->isAuthenticated() dovrebbe ora essere true.
            if (!mAuth->isAuthenticated()) { // Ricontrolla
                 throw std::runtime_error("Authentication failed after preemptive refresh. Please log in again.");
            }
             LOG(LogInfo) << "performGetCatalogItemsSync: Preemptive refresh successful.";
        } else if (!mAuth || !mAuth->isAuthenticated()) { // Se ancora non autenticato
            LOG(LogError) << "performGetCatalogItemsSync: Not authenticated and no means to refresh for GetCatalogItems.";
            throw std::runtime_error("Not authenticated for GetCatalogItems. Please log in.");
        }
    }
    
    if (itemsToFetch.empty()) { return {}; }

    std::map<std::string, EpicGames::CatalogItem> results;
    // ... (Raggruppamento per namespace se usi la logica bulk, altrimenti ciclo diretto) ...

    // Esempio per una singola richiesta (da adattare se fai richieste bulk per namespace)
    // Questo ciclo dovrebbe essere il tuo ciclo sugli item o sui batch di item.
    for (const auto& itemPair : itemsToFetch) { // O per ogni batch
        const std::string& ns = itemPair.first; // Adatta se non iteri su itemPair
        const std::string& catalogId = itemPair.second; // Adatta come sopra
        
        // Costruisci l'URL per la richiesta corrente (itemUrl)
        // std::string itemUrl = ... ; (logica per costruire l'URL per 'catalogId' o batch)
        std::string baseUrl = getCatalogUrl(ns); // Assumendo che getCatalogUrl dipenda solo da ns
        std::string itemUrl = baseUrl + "/bulk/items?id=" + HttpReq::urlEncode(catalogId) + // O l'URL bulk corretto
                              "&country=" + HttpReq::urlEncode(country) +
                              "&locale=" + HttpReq::urlEncode(locale) +
                              "&includeMainGameDetails=true";


        bool requestSucceededForThisItem = false;
        int retryAttempt = 0;
        const int MAX_AUTH_RETRIES = 1; 

        while (!requestSucceededForThisItem && retryAttempt <= MAX_AUTH_RETRIES) {
            std::string currentAccessToken = mAuth->getAccessToken();
            if (currentAccessToken.empty()) {
                LOG(LogError) << "performGetCatalogItemsSync: No access token available for item " << catalogId << " before attempt " << retryAttempt + 1 << ". This should not happen if auth checks passed.";
                throw std::runtime_error("Critical: Access token disappeared. Please log in again.");
            }

            HttpReqOptions options;
            options.customHeaders.push_back("Authorization: bearer " + currentAccessToken);
            // options.timeoutMs = 15000; // Imposta un timeout ragionevole

            LOG(LogDebug) << "performGetCatalogItemsSync: Requesting URL (attempt " << retryAttempt + 1 << "): " << itemUrl;
            HttpReq request(itemUrl, &options);

            try {
                if (!request.wait()) {
                    LOG(LogError) << "performGetCatalogItemsSync: HTTP request failed (wait) for " << itemUrl << ": " << request.getErrorMsg();
                    // Questo è un errore di rete, non di autenticazione. Non ritentare con refresh qui.
                    throw std::runtime_error("HTTP request failed (wait condition) for " + catalogId);
                }

                int httpStatusCode = request.status();
                std::string responseBody = request.getContent();

                if (httpStatusCode == 200 || httpStatusCode == HttpReq::REQ_SUCCESS) {
                    LOG(LogInfo) << "performGetCatalogItemsSync: Successfully fetched data for " << itemUrl;
                    // --- Inizio Parsing Risposta (come nel tuo codice originale) ---
                    if (!responseBody.empty()) {
                        json parsedResponse = json::parse(responseBody); // Può lanciare json::parse_error
                        if (parsedResponse.is_object() && parsedResponse.contains(catalogId)) { // Adatta se la risposta è un array o struttura diversa
                            json itemJson = parsedResponse.at(catalogId);
                            results[catalogId] = EpicGames::CatalogItem::fromJson(itemJson); // Può lanciare eccezioni
                            LOG(LogInfo) << "performGetCatalogItemsSync: Successfully parsed item: " << catalogId << " ('" << results[catalogId].title << "')";
                        } else { 
                            LOG(LogError) << "performGetCatalogItemsSync: JSON response for " << catalogId << " is not an object or doesn't contain the key. Body: " << responseBody.substr(0, 500);
                        }
                    } else { 
                        LOG(LogWarning) << "performGetCatalogItemsSync: Response body for item " << catalogId << " is empty despite HTTP 200.";
                    }
                    // --- Fine Parsing Risposta ---
                    requestSucceededForThisItem = true; // Esce dal while per questo item/batch
                } else if (httpStatusCode == 401) {
                    LOG(LogWarning) << "performGetCatalogItemsSync: Received 401 (Unauthorized) for " << itemUrl << ". Attempt " << retryAttempt + 1;
                    if (retryAttempt < MAX_AUTH_RETRIES) {
                        LOG(LogInfo) << "Attempting token refresh...";
                        if (mAuth->refreshAccessToken()) {
                            LOG(LogInfo) << "Token refresh successful. Retrying request for " << itemUrl;
                            // Il token è stato aggiornato, il prossimo giro del while (retryAttempt++) lo userà.
                        } else {
                            LOG(LogError) << "Token refresh failed for " << itemUrl << ". Aborting operations.";
                            // mAuth->refreshAccessToken() dovrebbe aver chiamato clearAllTokenData se il refresh token era invalido.
                            throw std::runtime_error("Token refresh failed. Please log in again.");
                        }
                    } else {
                        LOG(LogError) << "Max auth retries reached for " << itemUrl << " after 401. Clearing tokens.";
                        if(mAuth) mAuth->clearAllTokenData(); // Assicura la pulizia
                        throw std::runtime_error("Authentication failed after token refresh attempts. Please log in again.");
                    }
                } else { // Altro errore HTTP
                    LOG(LogError) << "performGetCatalogItemsSync: API request for " << itemUrl << " failed with status " << httpStatusCode << ". Body: " << responseBody.substr(0, 500);
                    throw std::runtime_error("API request for " + catalogId + " failed with status: " + std::to_string(httpStatusCode));
                }
            } catch (const std::runtime_error& e_req) { // Eccezioni legate alla richiesta o al suo post-processing
                LOG(LogError) << "performGetCatalogItemsSync: Runtime error during request for " << itemUrl << ": " << e_req.what();
                if (std::string(e_req.what()).find("Please log in again.") != std::string::npos) {
                    throw; // Rilancia per fermare tutto se è un errore di autenticazione fatale
                }
                // Per altri errori runtime (es. HTTP wait failed, API error non 401), esci dal while per questo item/batch
                break; 
            } catch (const json::parse_error& e_json) {
                LOG(LogError) << "performGetCatalogItemsSync: JSON parsing error for " << itemUrl << ": " << e_json.what();
                break; // Esci dal while per questo item/batch
            } catch (const std::exception& e_std) {
                LOG(LogError) << "performGetCatalogItemsSync: Std exception for " << itemUrl << ": " << e_std.what();
                break; // Esci dal while per questo item/batch
            }
            retryAttempt++;
        } // Fine while loop (retry)

        if (!requestSucceededForThisItem) {
            LOG(LogError) << "performGetCatalogItemsSync: Failed to process item " << catalogId << " (NS: " << ns << ") after all attempts.";
            // Decidi se continuare con gli altri item o lanciare un'eccezione per interrompere tutto.
            // Se un'eccezione "Please log in again" è stata lanciata, l'esecuzione non arriverà qui.
        }
        
        // Mantieni il ritardo tra le richieste agli item/batch
        std::this_thread::sleep_for(std::chrono::milliseconds(250)); 

    } // Fine for loop (itemsToFetch)

    LOG(LogInfo) << "performGetCatalogItemsSync: Finished. Returning " << results.size() << " results.";
    return results;
}


// --- Implementazione Metodi API ASINCRONI (Chiama Helper Sincroni) ---

std::future<std::vector<EpicGames::Asset>> EpicGamesStoreAPI::GetAllAssetsAsync()
{
    LOG(LogDebug) << "EpicAPI ASYNC: Queuing GetAllAssets task...";
    return std::async(std::launch::async, [this]() {
        LOG(LogDebug) << "EpicAPI ASYNC_THREAD: Executing performGetAllAssetsSync()...";
        try {
            return this->performGetAllAssetsSync();
        } catch (...) {
             LOG(LogError) << "EpicAPI ASYNC_THREAD: Exception caught in performGetAllAssetsSync. Propagating...";
             throw;
        }
    });
}

std::future<std::map<std::string, EpicGames::CatalogItem>> EpicGamesStoreAPI::GetCatalogItemsAsync(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicAPI ASYNC: Queuing GetCatalogItems task for " << itemsToFetch.size() << " items...";
    return std::async(std::launch::async, [this, itemsToFetch, country, locale]() {
        LOG(LogDebug) << "EpicAPI ASYNC_THREAD: Executing performGetCatalogItemsSync(" << itemsToFetch.size() << " items)...";
         try {
             return this->performGetCatalogItemsSync(itemsToFetch, country, locale);
         } catch (...) {
              LOG(LogError) << "EpicAPI ASYNC_THREAD: Exception caught in performGetCatalogItemsSync. Propagating...";
             throw;
         }
    });
}


// --- Implementazione Metodi API SINCRONI (Wrapper) ---

std::vector<EpicGames::Asset> EpicGamesStoreAPI::GetAllAssets() {
    LOG(LogDebug) << "EpicAPI SYNC WRAPPER: Calling GetAllAssetsAsync().get()...";
    try {
        return GetAllAssetsAsync().get();
    } catch (const std::exception& e) {
        LOG(LogError) << "Exception caught in GetAllAssets (sync wrapper): " << e.what();
        return {};
    } catch (...) {
         LOG(LogError) << "Unknown exception caught in GetAllAssets (sync wrapper).";
         return {};
    }
}

std::map<std::string, EpicGames::CatalogItem> EpicGamesStoreAPI::GetCatalogItems(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicAPI SYNC WRAPPER: Calling GetCatalogItemsAsync().get()...";
    try {
        return GetCatalogItemsAsync(itemsToFetch, country, locale).get();
    } catch (const std::exception& e) {
        LOG(LogError) << "Exception caught in GetCatalogItems (sync wrapper): " << e.what();
        return {};
    } catch (...) {
         LOG(LogError) << "Unknown exception caught in GetCatalogItems (sync wrapper).";
         return {};
    }
}

// --- IL BLOCCO 'namespace EpicGames { ... }' con le definizioni di fromJson E' STATO RIMOSSO DA QUI ---
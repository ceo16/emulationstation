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
    LOG(LogDebug) << "EpicAPI SYNC_IMPL: Executing performGetCatalogItemsSync() for " << itemsToFetch.size() << " items.";
    if (!mAuth || mAuth->getAccessToken().empty()) { throw std::runtime_error("Not authenticated for GetCatalogItems"); }
    if (itemsToFetch.empty()) { return {}; }

    std::string currentToken = mAuth->getAccessToken();
    std::map<std::string, EpicGames::CatalogItem> results;
    int itemCount = 0; const int totalItems = itemsToFetch.size();

    for (const auto& itemPair : itemsToFetch) {
        itemCount++;
        const std::string& ns = itemPair.first;
        const std::string& catalogId = itemPair.second;
        LOG(LogInfo) << "performGetCatalogItemsSync: Fetching item " << itemCount << "/" << totalItems << " (ID: " << catalogId << ", NS: " << ns << ")";
        if (ns.empty() || catalogId.empty()) { LOG(LogWarning) << "performGetCatalogItemsSync: Skipping item with empty ns or id."; continue; }

        std::string baseUrl = getCatalogUrl(ns);
        std::string itemUrl = baseUrl + "/bulk/items?id=" + HttpReq::urlEncode(catalogId) +
                              "&country=" + HttpReq::urlEncode(country) +
                              "&locale=" + HttpReq::urlEncode(locale) +
                              "&includeMainGameDetails=true";
        LOG(LogDebug) << "performGetCatalogItemsSync: Requesting URL: " << itemUrl;

       try {
    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: bearer " + currentToken);
    HttpReq request(itemUrl, &options); // Rinominata per chiarezza

    if (!request.wait()) {
        // ERRORE DI TRASPORTO/TIMEOUT: request.wait() fallito.
        // In questo caso, request.status() NON è affidabile come status code HTTP dell'API.
        // Potrebbe essere 0 o un codice di errore interno della libreria HttpReq.
        LOG(LogError) << "performGetCatalogItemsSync: HTTP request failed (wait condition) for item "
                      << catalogId << " (NS: " << ns << ", URL: " << itemUrl << "): " << request.getErrorMsg();
        // NON controllare request.status() == 401 qui.
        // Se il fallimento di wait() è molto generico, lanciare un errore o continuare con il prossimo item.
        // L'originale lanciava un'eccezione per item, che poi veniva catturata esternamente al try-catch per item.
        // Per mantenere un comportamento simile ma più contenuto, potremmo solo loggare e continuare,
        // o lanciare un errore specifico che non sia confuso con un errore API.
        // Dato che l'originale aveva un try-catch per ogni item:
        LOG(LogWarning) << "Skipping item " << catalogId << " due to HTTP wait failure.";
        continue; // Passa al prossimo item nel ciclo for
    }

    // A QUESTO PUNTO, request.wait() È RIUSCITO.
    // Abbiamo ricevuto una risposta dal server e request.status() è uno status code HTTP valido.
    int httpStatusCode = request.status();

    if (httpStatusCode == 401) {
        // ERRORE 401 UNAUTHORIZED DALL'API
        LOG(LogWarning) << "performGetCatalogItemsSync: Epic API returned 401 Unauthorized for item " << catalogId
                        << " (NS: " << ns << ", URL: " << itemUrl
                        << "). Clearing token. Response: " << request.getContent();
        if (mAuth != nullptr) {
            mAuth->clearAllTokenData();
            // È FONDAMENTALE FERMARE L'OPERAZIONE QUI.
            // Se il token è invalido, le successive richieste per altri item falliranno.
            // Lanciare un'eccezione che interrompa performGetCatalogItemsSync è la scelta migliore.
            throw std::runtime_error("API token invalid (401 Unauthorized). Please log in again to continue.");
        } else {
            LOG(LogError) << "performGetCatalogItemsSync: mAuth is null, cannot clear tokens despite 401.";
            throw std::runtime_error("API returned 401 Unauthorized and Auth object is missing.");
        }
    } else if (httpStatusCode != HttpReq::REQ_SUCCESS) { // Assumendo REQ_SUCCESS sia 200 o simile
        // ALTRO ERRORE HTTP DALL'API (es. 400, 403, 404, 500, ecc.)
        LOG(LogError) << "performGetCatalogItemsSync: API request failed for item " << catalogId
                      << " (NS: " << ns << ", URL: " << itemUrl
                      << "). Status: " << httpStatusCode << ". Response: " << request.getContent();
        // In base alla logica originale, si continuava con il prossimo item.
        LOG(LogWarning) << "Skipping item " << catalogId << " due to API error status " << httpStatusCode;
        continue; // Passa al prossimo item
    }

    // Se siamo qui, httpStatusCode è di successo (es. 200)
    std::string responseBody = request.getContent();

    if (!responseBody.empty()) {
        // ... (parsing JSON come prima) ...
        // results[catalogId] = EpicGames::CatalogItem::fromJson(itemJson);
    } else {
        LOG(LogWarning) << "performGetCatalogItemsSync: Response body for item " << catalogId << " is empty even with success status.";
        // Potresti voler saltare o gestire questo caso specificamente
    }

} catch (const std::runtime_error& e) {
    // Questo catturerà l'eccezione lanciata dal blocco 401.
    LOG(LogError) << "performGetCatalogItemsSync: Runtime error processing item " << catalogId
                  << " (NS: " << ns << "): " << e.what();
    // Se l'eccezione indica un token invalido, RILANCIA per fermare l'intera funzione performGetCatalogItemsSync.
    if (std::string(e.what()).find("API token invalid") != std::string::npos ||
        std::string(e.what()).find("401 Unauthorized") != std::string::npos) {
        throw; // Rilancia l'eccezione.
    }
    // Per altri errori runtime catturati qui (non dal nostro 401), potremmo continuare con il prossimo item
    // se questo è il comportamento desiderato per la resilienza a livello di singolo item.
    // L'originale continuava implicitamente.
} catch (const json::parse_error& e) {
    LOG(LogError) << "performGetCatalogItemsSync: JSON parsing failed for " << catalogId << ": " << e.what();
    // Continua con il prossimo item
} catch (const std::exception& e) {
    LOG(LogError) << "performGetCatalogItemsSync: Generic exception processing item " << catalogId << ": " << e.what();
    // Continua con il prossimo item
} catch (...) {
    LOG(LogError) << "performGetCatalogItemsSync: Unknown exception processing item " << catalogId;
    // Continua con il prossimo item
}


        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
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
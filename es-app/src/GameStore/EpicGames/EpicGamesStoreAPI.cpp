#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"   // Necessario per usare mAuth
#include "GameStore/EpicGames/EpicGamesModels.h" // Contiene le struct e safe_get
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/StringUtil.h"                    // Per toLower, ecc.
#include <future>                                // Per std::future, std::async
#include <vector>
#include <string>
#include <map>
#include <thread>                                // Per std::this_thread
#include <chrono>                                // Per std::chrono::milliseconds
#include <stdexcept>                             // Per std::exception (opzionale)

using json = nlohmann::json;


// --- Costruttore e Distruttore ---
EpicGamesStoreAPI::EpicGamesStoreAPI(EpicGamesAuth* auth) : mAuth(auth) {
    if (mAuth == nullptr) {
        LOG(LogError) << "EpicGamesStoreAPI: Constructor received null EpicGamesAuth pointer!";
        // Potresti voler lanciare un'eccezione o gestire questo caso in modo più robusto
    } else {
         LOG(LogDebug) << "EpicGamesStoreAPI: Constructor (auth object received)";
         // Qui potremmo anche caricare il token iniziale da mAuth se necessario
         // mAccessToken = mAuth->getAccessToken();
    }
}

EpicGamesStoreAPI::~EpicGamesStoreAPI() {
    LOG(LogDebug) << "EpicGamesStoreAPI: Destructor";
}

// --- Getter per Auth (necessario per l'esterno) ---
// NOTA: Questa definizione era nel .h, che va bene per getter semplici,
// ma per coerenza con le altre la mettiamo qui.
// Se preferisci inline nel .h, assicurati che ci sia la parola chiave 'inline'
// o che sia dentro la definizione della classe nel .h
/* // Definizione alternativa inline nel .h:
class EpicGamesStoreAPI {
public:
    inline EpicGamesAuth* getAuth() { return mAuth; } // <-- Definizione inline
    // ...
}; */
// Definizione standard nel .cpp:
EpicGamesAuth* EpicGamesStoreAPI::getAuth() {
     return mAuth;
}


// --- Implementazione Helper URL ---
std::string EpicGamesStoreAPI::getAssetsUrl() {
    std::string hostname = "launcher-public-service-prod06.ol.epicgames.com";
    // TODO: Leggere da config .ini se possibile
    // LOG(LogDebug) << "Using fallback hostname for Assets URL: " << hostname;
    return "https://" + hostname + "/launcher/api/public/assets/Windows?label=Live";
}

std::string EpicGamesStoreAPI::getCatalogUrl(const std::string& ns) {
    std::string hostname = "catalog-public-service-prod06.ol.epicgames.com";
    // TODO: Leggere da config .ini se possibile
    // LOG(LogDebug) << "Using fallback hostname for Catalog URL: " << hostname;
    return "https://" + hostname + "/catalog/api/shared/namespace/" + ns;
}

// --- Implementazione Helper makeApiRequest (SE NECESSARIO) ---
// Sembra che HttpReq sia usato direttamente, quindi questa funzione potrebbe
// non essere strettamente necessaria, ma la lasciamo come placeholder se serve in futuro.
std::string EpicGamesStoreAPI::makeApiRequest(const std::string& url, const std::string& token, const std::string& method, const std::string& body, const std::vector<std::string>& headers) {
    LOG(LogDebug) << "makeApiRequest: Method=" << method << ", URL=" << url;

    HttpReqOptions options; // Crea le opzioni

    // Imposta gli header personalizzati (inclusa l'autenticazione)
    // Copia gli header passati come argomento
    if (!headers.empty()) {
        options.customHeaders = headers;
    }
    // Aggiungi l'header di autorizzazione se c'è un token
    if (!token.empty()) {
        options.customHeaders.push_back("Authorization: bearer " + token);
    }

    // Imposta i dati POST/PUT se il body non è vuoto e il metodo lo richiede
    if ((method == "POST" || method == "PUT") && !body.empty()) {
         options.dataToPost = body; // Imposta i dati POST nelle opzioni
         // Esempio opzionale: Aggiungi Content-Type se necessario e non già presente
         // bool contentTypeSet = false;
         // for(const auto& h : options.customHeaders) { if (Utils::String::startsWith(Utils::String::toLower(h), "content-type:")) { contentTypeSet = true; break; } }
         // if (!contentTypeSet) { options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded"); } // O application/json ecc.
    }

    // Variabile per il puntatore alla richiesta
    HttpReq* request = nullptr;

    try {
        // Usa SEMPRE il costruttore a 2 argomenti: HttpReq(url, options*)
        request = new HttpReq(url, &options); // <<< CHIAMATA CORRETTA AL COSTRUTTORE

        // Aspetta il completamento della richiesta
        if (!request->wait()) {
            // Gestione errore: la richiesta non è riuscita a completarsi
            LOG(LogError) << "makeApiRequest: HTTP request failed! Status: "
                          << request->status() << ", Error: " << request->getErrorMsg();
            std::string errorMsg = request->getErrorMsg(); // Salva messaggio prima di delete
            delete request; // Pulisci memoria
            request = nullptr; // Resetta puntatore (buona norma)
            throw std::runtime_error("HTTP request failed: " + errorMsg); // Lancia eccezione
        }

        // Controlla lo status HTTP dopo il completamento (wait() è andato a buon fine)
        if (request->status() != HttpReq::REQ_SUCCESS) {
            // Gestione errore: status HTTP non è 2xx (es. 404, 403, 500)
            LOG(LogError) << "makeApiRequest: Bad HTTP status: " << request->status() << ". Error: " << request->getErrorMsg();
            std::string responseBodyOnError = request->getContent(); // Ottieni corpo per log
            LOG(LogError) << "makeApiRequest: Response body: " << responseBodyOnError;
            int statusOnError = request->status(); // Salva status prima di delete
            delete request; // Pulisci memoria
            request = nullptr;
            throw std::runtime_error("HTTP request returned non-success status: " + std::to_string(statusOnError)); // Lancia eccezione
        }

        // --- Successo! ---
        std::string responseBody = request->getContent(); // Ottieni il corpo della risposta
        LOG(LogDebug) << "makeApiRequest: Response received (size: " << responseBody.length() << ")";
        delete request; // Pulisci memoria
        request = nullptr;
        return responseBody; // Restituisci il corpo della risposta

    } catch (...) {
        // Blocco catch generico per assicurarsi che 'request' venga cancellato
        // se un'eccezione viene lanciata PRIMA del delete nei blocchi try/if sopra
        // (anche se in questo caso è ridondante perché abbiamo delete prima di throw)
        // Ma è una buona pratica difensiva.
        delete request; // Cancella se non è già stato fatto
        throw;          // Rilancia l'eccezione originale
    }
}

// --- Implementazione Parsing per Struct (DENTRO IL NAMESPACE EPICGAMES) ---
// Questo blocco definisce come convertire il JSON nelle struct definite in EpicGamesModels.h
namespace EpicGames {

    // Helper template per ottenere valori JSON in modo sicuro
    template<typename T>
    T safe_get(const nlohmann::json& j, const std::string& key, const T& default_value) {
        if (j.contains(key) && !j.at(key).is_null()) {
            try {
                return j.at(key).get<T>();
            } catch (const nlohmann::json::exception& e) {
                LOG(LogWarning) << "JSON safe_get failed for key '" << key << "': " << e.what();
                return default_value;
            }
        }
        return default_value;
    }
    // Specializzazione per const char* (per evitare ambiguità con bool in alcuni casi)
    template<>
    std::string safe_get<std::string>(const nlohmann::json& j, const std::string& key, const std::string& default_value) {
         if (j.contains(key)) {
            const auto& val = j.at(key);
            if (val.is_string()) {
                return val.get<std::string>();
            } else if (!val.is_null()) {
                 // Prova a convertire in stringa se non è null ma non è stringa (es. numero)
                 try { return val.dump(); } catch (...) { return default_value; }
            }
         }
         return default_value;
    }


    // Implementazione per Asset
    Asset Asset::fromJson(const nlohmann::json& j) {
        Asset asset;
        asset.appName       = safe_get<std::string>(j, "appName", "");
        asset.labelName     = safe_get<std::string>(j, "labelName", "");
        asset.buildVersion  = safe_get<std::string>(j, "buildVersion", "");
        asset.catalogItemId = safe_get<std::string>(j, "catalogItemId", "");
        asset.ns            = safe_get<std::string>(j, "namespace", "");
        asset.assetId       = safe_get<std::string>(j, "assetId", "");
        if (asset.appName.empty()) LOG(LogWarning) << "Asset JSON missing 'appName'";
        //if (asset.catalogItemId.empty()) LOG(LogWarning) << "Asset JSON missing 'catalogItemId' for appName: " << asset.appName;
        //if (asset.ns.empty()) LOG(LogWarning) << "Asset JSON missing 'namespace' for appName: " << asset.appName;
        return asset;
    }

    // Implementazione per Image
    Image Image::fromJson(const nlohmann::json& j) {
        Image img;
        img.url    = safe_get<std::string>(j, "url", "");
        img.type   = safe_get<std::string>(j, "type", "");
        img.width  = safe_get<int>(j, "width", 0);
        img.height = safe_get<int>(j, "height", 0);
        return img;
    }

    // Implementazione per Category
    Category Category::fromJson(const nlohmann::json& j) {
        Category cat;
        cat.path = safe_get<std::string>(j, "path", "");
        return cat;
    }

    // Implementazione per ReleaseInfo
    ReleaseInfo ReleaseInfo::fromJson(const nlohmann::json& j) {
        ReleaseInfo info;
        info.appId     = safe_get<std::string>(j, "appId", "");
        if (j.contains("platform") && j.at("platform").is_array()) {
             try { info.platform = j.at("platform").get<std::vector<std::string>>(); }
             catch (...) { info.platform = {}; LOG(LogWarning) << "Failed to parse 'platform' array."; }
        } else { info.platform = {}; }
        info.dateAdded = safe_get<std::string>(j, "dateAdded", "");
        return info;
    }

    // Implementazione per CustomAttribute
    CustomAttribute CustomAttribute::fromJson(const std::string& k, const nlohmann::json& j_val) {
         CustomAttribute attr;
         attr.key = k;
         attr.type = safe_get<std::string>(j_val, "type", "UNKNOWN");
         if (j_val.contains("value")) {
             const auto& valueField = j_val["value"];
             if (valueField.is_string()) {
                 attr.value = valueField.get<std::string>();
             } else if (valueField.is_boolean()) {
                 attr.value = valueField.get<bool>() ? "true" : "false";
             } else if (valueField.is_number()) {
                 // Usa dump per mantenere la precisione originale come stringa
                 attr.value = valueField.dump();
             } else if (valueField.is_null()){
                 attr.value = ""; // Tratta null come stringa vuota
             } else {
                 // Per altri tipi (array, object), fai il dump JSON
                 attr.value = valueField.dump();
             }
         } else {
             attr.value = ""; // Campo 'value' mancante
         }
         return attr;
    }

     // Implementazione per CatalogItem
     CatalogItem CatalogItem::fromJson(const nlohmann::json& j) {
         CatalogItem item;
         item.id                 = safe_get<std::string>(j, "id", "");
         item.title              = safe_get<std::string>(j, "title", "");
         item.description        = safe_get<std::string>(j, "description", "");
         item.ns                 = safe_get<std::string>(j, "namespace", ""); // Namespace è importante
         item.status             = safe_get<std::string>(j, "status", "");
         item.creationDate       = safe_get<std::string>(j, "creationDate", "");
         item.lastModifiedDate   = safe_get<std::string>(j, "lastModifiedDate", "");
         item.entitlementName    = safe_get<std::string>(j, "entitlementName", "");
         item.entitlementType    = safe_get<std::string>(j, "entitlementType", "");
         item.itemType           = safe_get<std::string>(j, "itemType", "");
         item.developer          = safe_get<std::string>(j, "developer", "");
         item.developerId        = safe_get<std::string>(j, "developerId", "");
         item.endOfSupport       = safe_get<bool>(j, "endOfSupport", false);

         if (j.contains("keyImages") && j.at("keyImages").is_array()) {
             for (const auto& imgJson : j.at("keyImages")) {
                 try { item.keyImages.push_back(Image::fromJson(imgJson)); }
                 catch(const std::exception& e) { LOG(LogWarning) << "Failed to parse an image in keyImages for item " << item.id << ": " << e.what(); }
                 catch(...) { LOG(LogWarning) << "Failed to parse an image in keyImages for item " << item.id << " (unknown exception)"; }
             }
         }

         if (j.contains("categories") && j.at("categories").is_array()) {
             for (const auto& catJson : j.at("categories")) {
                  try { item.categories.push_back(Category::fromJson(catJson)); }
                  catch(const std::exception& e) { LOG(LogWarning) << "Failed to parse a category for item " << item.id << ": " << e.what(); }
                  catch(...) { LOG(LogWarning) << "Failed to parse a category for item " << item.id << " (unknown exception)"; }
             }
         }

         if (j.contains("releaseInfo") && j.at("releaseInfo").is_array()) {
             for (const auto& relJson : j.at("releaseInfo")) {
                  try { item.releaseInfo.push_back(ReleaseInfo::fromJson(relJson)); }
                  catch(const std::exception& e) { LOG(LogWarning) << "Failed to parse a releaseInfo for item " << item.id << ": " << e.what(); }
                  catch(...) { LOG(LogWarning) << "Failed to parse a releaseInfo for item " << item.id << " (unknown exception)"; }
             }
         }

         item.publisher = ""; item.releaseDate = ""; // Inizializza
         if (j.contains("customAttributes") && j.at("customAttributes").is_object()) {
             for (auto const& [key, val] : j.at("customAttributes").items()) {
                  try {
                      CustomAttribute attr = CustomAttribute::fromJson(key, val);
                      item.customAttributes.push_back(attr);
                      // Usa Utils::String::toLower per confronto case-insensitive
                      std::string lowerKey = Utils::String::toLower(key);
                      if (item.publisher.empty() && (lowerKey == "publishername" || lowerKey == "publisher")) {
                           item.publisher = attr.value;
                      }
                      if (item.releaseDate.empty() && (lowerKey == "releasedate" || lowerKey == "pcreleasedate")) {
                          item.releaseDate = attr.value;
                      }
                  }
                  catch(const std::exception& e) { LOG(LogWarning) << "Failed to parse custom attribute '" << key << "' for item " << item.id << ": " << e.what(); }
                  catch(...) { LOG(LogWarning) << "Failed to parse custom attribute '" << key << "' for item " << item.id << " (unknown exception)"; }
             }
         }

         // Fallback per developer se non trovato direttamente
         if (item.developer.empty()) {
             for (const auto& attr : item.customAttributes) {
                  std::string lowerKey = Utils::String::toLower(attr.key);
                  if (lowerKey == "developername" || lowerKey == "developer") {
                       item.developer = attr.value;
                       break;
                  }
             }
         }

         // Fallback per data rilascio da releaseInfo se non trovata negli attributi custom
         if (item.releaseDate.empty() && !item.releaseInfo.empty()) {
             std::string firstDate = ""; // Memorizza la prima data trovata come fallback estremo
             for(const auto& ri : item.releaseInfo) {
                 bool isWindows = false;
                 // Controlla se tra le piattaforme c'è "Windows"
                 for(const auto& p : ri.platform) { if (p == "Windows") { isWindows = true; break; } }

                 // Salva la prima data incontrata
                 if (firstDate.empty() && !ri.dateAdded.empty()) { firstDate = ri.dateAdded; }

                 // Se è per Windows e ha una data, usala e interrompi
                 if(isWindows && !ri.dateAdded.empty()) {
                     item.releaseDate = ri.dateAdded;
                     break;
                 }
             }
             // Se non abbiamo trovato una data specifica per Windows, usa la prima data trovata
             if(item.releaseDate.empty()) { item.releaseDate = firstDate; }
         }
         return item;
     }

} // namespace EpicGames


// --- Implementazione Funzioni API ASINCRONE ---

std::future<std::vector<EpicGames::Asset>> EpicGamesStoreAPI::GetAllAssetsAsync() {
    LOG(LogDebug) << "EpicGamesStoreAPI::GetAllAssetsAsync called";
    // Cattura mAuth per valore nel lambda per sicurezza (o usa un puntatore locale)
    EpicGamesAuth* authPtr = mAuth;
    return std::async(std::launch::async, [authPtr, this]() { // Cattura this e authPtr
        std::vector<EpicGames::Asset> assets;
        if (!authPtr) { LOG(LogError) << "GetAllAssetsAsync: Auth object is null."; return assets; }

        std::string currentToken = authPtr->getAccessToken();
        if (currentToken.empty()) {
            LOG(LogWarning) << "GetAllAssetsAsync: Not authenticated (token is empty). Cannot fetch assets.";
            return assets; // Restituisci vettore vuoto se non autenticato
        }
        LOG(LogDebug) << "GetAllAssetsAsync: Using access token (length: " << currentToken.length() << ")";

        std::string url = getAssetsUrl(); // Non serve this-> qui perché siamo dentro un metodo
        LOG(LogDebug) << "GetAllAssetsAsync: Requesting URL: " << url;

        HttpReqOptions options;
        options.customHeaders.push_back("Authorization: bearer " + currentToken);

        HttpReq request(url, &options); // Usa GET di default

        if (!request.wait()) {
             LOG(LogError) << "GetAllAssetsAsync: HTTP request failed! Status: "
                           << request.status() << ", Error: " << request.getErrorMsg();
             return assets; // Restituisci vettore vuoto in caso di fallimento HTTP
        }

        if (request.status() != HttpReq::REQ_SUCCESS) {
             LOG(LogError) << "GetAllAssetsAsync: Bad HTTP status: " << request.status() << ". Error: " << request.getErrorMsg();
              LOG(LogError) << "GetAllAssetsAsync: Response Body: " << request.getContent(); // Logga corpo errore
             return assets; // Restituisci vettore vuoto se lo status non è 200 OK
        }

        std::string responseBody = request.getContent();
        LOG(LogDebug) << "GetAllAssetsAsync: Response received (size: " << responseBody.length() << ")";

        try {
            if (!responseBody.empty()) {
                json parsedResponse = json::parse(responseBody);
                if (parsedResponse.is_array()) {
                    LOG(LogInfo) << "GetAllAssetsAsync: Parsing " << parsedResponse.size() << " assets...";
                    assets.reserve(parsedResponse.size()); // Pre-alloca memoria
                    for (const auto& assetJson : parsedResponse) {
                        try {
                             assets.push_back(EpicGames::Asset::fromJson(assetJson));
                        } catch (const std::exception& e) {
                             LOG(LogError) << "GetAllAssetsAsync: Error parsing individual asset JSON: " << e.what();
                             // Continua con gli altri asset anche se uno fallisce? Decidi tu.
                        } catch (...) {
                             LOG(LogError) << "GetAllAssetsAsync: Unknown error parsing individual asset JSON.";
                        }
                    }
                     LOG(LogInfo) << "GetAllAssetsAsync: Successfully parsed " << assets.size() << " assets.";
                } else {
                     LOG(LogError) << "GetAllAssetsAsync: JSON response is not an array as expected.";
                }
            } else {
                 LOG(LogInfo) << "GetAllAssetsAsync: Response body is empty, parsed 0 assets.";
            }
        } catch (const json::parse_error& e) {
            LOG(LogError) << "GetAllAssetsAsync: JSON parsing failed for the entire response: " << e.what();
             // Non restituire assets parziali se il JSON globale è invalido
             assets.clear();
        } catch (const std::exception& e) {
             LOG(LogError) << "GetAllAssetsAsync: Exception during JSON processing: " << e.what();
             assets.clear();
        } catch (...) {
             LOG(LogError) << "GetAllAssetsAsync: Unknown exception during JSON processing.";
             assets.clear();
        }
        return assets; // Restituisci il vettore di asset (potrebbe essere vuoto)
    });
}


std::future<std::map<std::string, EpicGames::CatalogItem>> EpicGamesStoreAPI::GetCatalogItemsAsync(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicGamesStoreAPI::GetCatalogItemsAsync called for " << itemsToFetch.size() << " items.";
    EpicGamesAuth* authPtr = mAuth; // Cattura puntatore per lambda

    // Cattura per valore le variabili semplici, this e authPtr per accedere ai membri/metodi
    return std::async(std::launch::async, [authPtr, itemsToFetch, country, locale, this]() {
        std::map<std::string, EpicGames::CatalogItem> results;
        if (!authPtr) { LOG(LogError) << "GetCatalogItemsAsync: Auth object is null."; return results; }

        std::string currentToken = authPtr->getAccessToken();
        if (currentToken.empty()) {
            LOG(LogWarning) << "GetCatalogItemsAsync: Not authenticated (token is empty). Cannot fetch catalog items.";
            return results;
        }
        LOG(LogDebug) << "GetCatalogItemsAsync: Using access token (length: " << currentToken.length() << ")";

        int itemCount = 0;
        const int totalItems = itemsToFetch.size();

        // Ciclo per recuperare i dettagli di ogni item (una richiesta per item)
        for (const auto& itemPair : itemsToFetch) {
            itemCount++;
            const std::string& ns = itemPair.first;
            const std::string& catalogId = itemPair.second;
            // Log più informativo
            LOG(LogInfo) << "GetCatalogItemsAsync: Fetching item " << itemCount << "/" << totalItems
                         << " (CatalogID: " << catalogId << ", Namespace: " << ns << ")";

            if (ns.empty() || catalogId.empty()) {
                LOG(LogWarning) << "GetCatalogItemsAsync: Skipping item with empty namespace or catalogId.";
                continue;
            }

            // Costruisci URL per la chiamata bulk (ma con un solo ID)
            // Usa this-> perché siamo dentro una lambda catturando this
            std::string baseUrl = this->getCatalogUrl(ns);
            std::string itemUrl = baseUrl + "/bulk/items?id=" + HttpReq::urlEncode(catalogId) +
                                  "&country=" + HttpReq::urlEncode(country) +
                                  "&locale=" + HttpReq::urlEncode(locale) +
                                  "&includeMainGameDetails=true"; // Include dettagli gioco base se è DLC
            LOG(LogDebug) << "GetCatalogItemsAsync: Requesting URL: " << itemUrl;

            HttpReqOptions options;
            options.customHeaders.push_back("Authorization: bearer " + currentToken);
            HttpReq request(itemUrl, &options); // GET di default

            if (!request.wait()) {
                LOG(LogError) << "GetCatalogItemsAsync: HTTP request failed for item " << catalogId << "! Status: "
                              << request.status() << ", Error: " << request.getErrorMsg();
                continue; // Prova con il prossimo item
            }

            if (request.status() != HttpReq::REQ_SUCCESS) {
                 LOG(LogError) << "GetCatalogItemsAsync: Bad HTTP status for item " << catalogId << ": " << request.status() << ". Error: " << request.getErrorMsg();
                 LOG(LogError) << "GetCatalogItemsAsync: Response Body: " << request.getContent(); // Logga corpo errore
                 continue; // Prova con il prossimo item
            }

            std::string responseBody = request.getContent();
            // LOG(LogDebug) << "GetCatalogItemsAsync: Response for " << catalogId << " (size: " << responseBody.length() << ")";
            // LOG(LogDebug) << "GetCatalogItemsAsync: Response Body for " << catalogId << ": " << responseBody; // Troppo verboso

            try {
                if (!responseBody.empty()) {
                    json parsedResponse = json::parse(responseBody);
                    // La risposta contiene direttamente l'oggetto JSON per l'ID richiesto
                    if (parsedResponse.is_object() && parsedResponse.contains(catalogId)) {
                        json itemJson = parsedResponse.at(catalogId); // Estrai l'oggetto JSON dell'item
                        try {
                            // Parse dell'item e aggiunta alla mappa dei risultati
                            results[catalogId] = EpicGames::CatalogItem::fromJson(itemJson);
                            LOG(LogInfo) << "GetCatalogItemsAsync: Successfully parsed item: " << catalogId << " ('" << results[catalogId].title << "')";
                        } catch (const std::exception& e) {
                             LOG(LogError) << "GetCatalogItemsAsync: Error parsing CatalogItem JSON for " << catalogId << ": " << e.what();
                        } catch (...) {
                             LOG(LogError) << "GetCatalogItemsAsync: Unknown error parsing CatalogItem JSON for " << catalogId;
                        }
                    } else {
                         LOG(LogError) << "GetCatalogItemsAsync: JSON response for " << catalogId << " is not an object or does not contain the key '" << catalogId << "'.";
                    }
                } else {
                     LOG(LogWarning) << "GetCatalogItemsAsync: Response body for item " << catalogId << " is empty.";
                }
            } catch (const json::parse_error& e) {
                LOG(LogError) << "GetCatalogItemsAsync: JSON parsing failed for item " << catalogId << ": " << e.what();
            } catch (const std::exception& e) {
                 LOG(LogError) << "GetCatalogItemsAsync: Exception processing response for item " << catalogId << ": " << e.what();
            } catch (...) {
                 LOG(LogError) << "GetCatalogItemsAsync: Unknown exception processing response for item " << catalogId;
            }

            // Pausa tra le richieste per non sovraccaricare l'API
            std::this_thread::sleep_for(std::chrono::milliseconds(250)); // 250ms di pausa

        } // Fine loop for sugli itemsToFetch

        LOG(LogInfo) << "GetCatalogItemsAsync: Finished processing all items. Returning " << results.size() << " successfully parsed results.";
        return results; // Restituisci la mappa dei risultati
    });
}


// --- Implementazione Funzioni API SINCRONE ---
// Queste semplicemente chiamano la versione asincrona e aspettano il risultato

std::vector<EpicGames::Asset> EpicGamesStoreAPI::GetAllAssets() {
    LOG(LogDebug) << "EpicGamesStoreAPI::GetAllAssets (sync wrapper) called";
    try {
        // Chiama la versione async e ottieni il risultato bloccando l'attesa
        return GetAllAssetsAsync().get();
    } catch (const std::exception& e) {
        // Logga eventuali eccezioni che sono state lanciate dal task asincrono
        LOG(LogError) << "Exception caught in GetAllAssets (sync wrapper): " << e.what();
        return {}; // Restituisci un vettore vuoto in caso di errore
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
    LOG(LogDebug) << "EpicGamesStoreAPI::GetCatalogItems (sync wrapper) called for " << itemsToFetch.size() << " items.";
    try {
        // Chiama la versione async e ottieni il risultato bloccando l'attesa
        return GetCatalogItemsAsync(itemsToFetch, country, locale).get();
    } catch (const std::exception& e) {
        // Logga eventuali eccezioni
        LOG(LogError) << "Exception caught in GetCatalogItems (sync wrapper): " << e.what();
        return {}; // Restituisci una mappa vuota in caso di errore
    } catch (...) {
         LOG(LogError) << "Unknown exception caught in GetCatalogItems (sync wrapper).";
         return {};
    }
}
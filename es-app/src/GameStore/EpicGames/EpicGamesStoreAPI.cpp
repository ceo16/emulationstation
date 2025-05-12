#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "GameStore/EpicGames/EpicGamesModels.h" // Contiene EpicGames namespace e le struct aggiornate
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/StringUtil.h"
#include "Settings.h"          // <<< AGGIUNTO INCLUDE PER SETTINGS
#include <future>              // Per std::future, std::async
#include <thread>              // Per std::this_thread::sleep_for
#include <chrono>              // Per std::chrono::milliseconds
#include <vector>
#include <string>
#include <map>
#include <utility>             // Per std::pair
#include <stdexcept>           // Per std::runtime_error, std::exception

using json = nlohmann::json;

namespace EpicGames // Mettiamo la funzione helper nello stesso namespace dei modelli
{
    // Dichiarazione forward della funzione helper se la definizione è dopo il suo primo uso,
    // o semplicemente definiscila prima di performGetCatalogItemsSync.
    // Per semplicità, la definisco prima.

    static CatalogItem parseCompleteCatalogItemFromJson(const nlohmann::json& j_item, const std::string& catalog_id_for_log) {
        CatalogItem item; // Usa EpicGames::CatalogItem
        if (!j_item.is_object()) {
            LOG(LogError) << "EpicGamesParser: JSON fornito per l'item " << catalog_id_for_log << " non è un oggetto.";
            item.id = catalog_id_for_log; 
            item.title = "Invalid JSON Data";
            return item;
        }

        // Popola i campi base usando la tua funzione safe_get (assicurati che sia nel namespace EpicGames o accessibile)
        item.id                = safe_get<std::string>(j_item, "id", catalog_id_for_log);
        item.title             = safe_get<std::string>(j_item, "title", "N/A");
        item.description       = safe_get<std::string>(j_item, "description", "");
        item.ns                = safe_get<std::string>(j_item, "namespace", "");
        item.status            = safe_get<std::string>(j_item, "status", "");
        item.creationDate      = safe_get<std::string>(j_item, "creationDate", "");
        item.lastModifiedDate  = safe_get<std::string>(j_item, "lastModifiedDate", "");
        item.entitlementName   = safe_get<std::string>(j_item, "entitlementName", "");
        item.entitlementType   = safe_get<std::string>(j_item, "entitlementType", "");
        item.itemType          = safe_get<std::string>(j_item, "itemType", "");
        item.developer         = safe_get<std::string>(j_item, "developerDisplayName", "");
        item.developerId       = safe_get<std::string>(j_item, "developerId", "");
        item.endOfSupport      = safe_get<bool>(j_item, "endOfSupport", false);

        if (j_item.contains("keyImages") && j_item.at("keyImages").is_array()) {
            for (const auto& imgJson : j_item.at("keyImages")) {
                try { item.keyImages.push_back(Image::fromJson(imgJson)); } // Image::fromJson è OK
                catch (const std::exception& e) { LOG(LogWarning) << "EpicGamesParser: Failed to parse an image for item " << item.id << ": " << e.what(); }
            }
        }

        if (j_item.contains("categories") && j_item.at("categories").is_array()) {
            for (const auto& catJson : j_item.at("categories")) {
                try { item.categories.push_back(Category::fromJson(catJson)); } // Category::fromJson è OK
                catch (const std::exception& e) { LOG(LogWarning) << "EpicGamesParser: Failed to parse a category for item " << item.id << ": " << e.what(); }
            }
        }
        
        if (j_item.contains("releaseInfo") && j_item.at("releaseInfo").is_array()) {
            for (const auto& relJson : j_item.at("releaseInfo")) {
                try { item.releaseInfo.push_back(ReleaseInfo::fromJson(relJson)); } // ReleaseInfo::fromJson è OK
                catch (const std::exception& e) { LOG(LogWarning) << "EpicGamesParser: Failed to parse a releaseInfo for item " << item.id << ": " << e.what(); }
            }
        }

        item.publisher = ""; 
        item.releaseDate = ""; 
        if (j_item.contains("customAttributes") && j_item.at("customAttributes").is_object()) {
            for (auto const& [key, val] : j_item.at("customAttributes").items()) {
                try {
                    CustomAttribute attr = CustomAttribute::fromJson(key, val); // CustomAttribute::fromJson è OK
                    item.customAttributes.push_back(attr);
                    std::string lowerKey = Utils::String::toLower(key);
                    if (item.publisher.empty() && (lowerKey == "publishername" || lowerKey == "publisher")) { item.publisher = attr.value; }
                    if (item.releaseDate.empty() && (lowerKey == "releasedate" || lowerKey == "pcreleasedate")) { item.releaseDate = attr.value; }
                    if (item.developer.empty() && (lowerKey == "developername" || lowerKey == "developer")) { item.developer = attr.value; }
                } catch (const std::exception& e) { LOG(LogWarning) << "EpicGamesParser: Failed to parse custom attribute '" << key << "': " << e.what(); }
            }
        }

        if (item.releaseDate.empty() && !item.releaseInfo.empty()) {
            std::string firstDateOverall = "";
            for(const auto& ri : item.releaseInfo) {
                if (firstDateOverall.empty() && !ri.dateAdded.empty()) { firstDateOverall = ri.dateAdded; }
                bool isWindows = false;
                for(const auto& p : ri.platform) { if (p == "Windows") { isWindows = true; break; } }
                if(isWindows && !ri.dateAdded.empty()) { item.releaseDate = ri.dateAdded; break; }
            }
            if(item.releaseDate.empty()) { item.releaseDate = firstDateOverall; }
        }

        // --- LOGICA DI SELEZIONE MEDIA DA keyImages ---
        // Tipi di immagine comuni in keyImages e a cosa potrebbero mappare:
        // "DieselStoreFrontWide", "OfferImageWide", "TakeoverWide", "HeroTall" (o simili) -> FanArt (sfondo largo)
        // "DieselStoreFrontTall", "OfferImageTall", "VaultClosedTall", "DieselGameBoxTall" -> BoxArt (verticale)
        // "Thumbnail" -> Thumbnail
        // "Screenshot", "ProductScreenshot" -> Screenshots
        // "Logo", "Icon", "Branding" -> Logo/Marquee (più difficile da standardizzare)

        int bestFanartWidth = 0;
        int bestBoxartHeight = 0;
        int bestBannerWidth = 0;
        int bestThumbWidth = 0;
        // int bestLogoSize = 0; // Se dimensioni disponibili per logo

        for (const auto& img : item.keyImages) {
            if (img.url.empty()) continue;

            // Screenshots
            if (img.type == "Screenshot" || img.type == "ProductScreenshot") {
                item.screenshotUrls.push_back(img.url);
            }
            // Thumbnail
            else if (img.type == "Thumbnail") {
                if (item.thumbnailUrl.empty() || img.width > bestThumbWidth) {
                    item.thumbnailUrl = img.url;
                    bestThumbWidth = img.width;
                }
            }
            // Boxart (Verticale/Tall)
            else if (img.type == "DieselStoreFrontTall" || img.type == "OfferImageTall" || 
                     img.type == "VaultClosedTall" || img.type == "DieselGameBoxTall" || img.type == " calitate") { // "calitate" ? forse un typo per quality o un tipo specifico?
                if (item.boxartUrl.empty() || img.height > bestBoxartHeight) { // Prendi la più alta
                    item.boxartUrl = img.url;
                    bestBoxartHeight = img.height;
                }
            }
            // Fanart/Hero/Banner (Orizzontale/Wide)
            else if (img.type == "OfferImageWide" || img.type == "DieselStoreFrontWide" || 
                     img.type == "TakeoverWide" || img.type == "HeroTall" || /* HeroTall è spesso wide */
                     img.type == "Featured" || img.type == "Background") { 
                // Per Fanart, di solito vogliamo la più larga
                if (item.fanartUrl.empty() || img.width > bestFanartWidth) {
                    item.fanartUrl = img.url;
                    bestFanartWidth = img.width;
                }
                // Per Banner, potremmo volere una OfferImageWide specifica se diversa dalla fanart
                if (img.type == "OfferImageWide" && (item.bannerUrl.empty() || img.width > bestBannerWidth) ) {
                     // Se fanart è già OfferImageWide, bannerUrl potrebbe essere ridondante o uguale
                     // o potresti scegliere una seconda OfferImageWide se ce ne sono più.
                     // Per semplicità, se OfferImageWide è la migliore per fanart, bannerUrl potrebbe essere uguale.
                    item.bannerUrl = img.url;
                    bestBannerWidth = img.width;
                }
            }
            // Logo (ipotetico, Epic non ha un tipo "Logo" standard come Steam in library_assets)
            else if (img.type == "Logo" || img.type == "Icon" || img.type == "Branding") {
                if (item.logoUrl.empty()) { // Prendi il primo che trovi
                    item.logoUrl = img.url;
                }
            }
        }
        
        // Fallback per Thumbnail se non specificamente trovata
        if (item.thumbnailUrl.empty() && !item.bannerUrl.empty()) item.thumbnailUrl = item.bannerUrl;
        else if (item.thumbnailUrl.empty() && !item.fanartUrl.empty()) item.thumbnailUrl = item.fanartUrl; // Fallback a fanart
        else if (item.thumbnailUrl.empty() && !item.keyImages.empty()) { // Fallback estremo alla prima immagine disponibile
            for(const auto& img : item.keyImages) { if(img.type == "OfferImageWide"){ item.thumbnailUrl = img.url; break;}}
            if(item.thumbnailUrl.empty()) item.thumbnailUrl = item.keyImages[0].url;
        }
        
        // Fallback per Banner se non specificamente trovato (può essere uguale a fanart)
        if (item.bannerUrl.empty() && !item.fanartUrl.empty()) item.bannerUrl = item.fanartUrl;


        // --- LOGICA PER ESTRARRE VIDEO URL ---
        // Questo dipende da come Epic fornisce i video nel JSON di `itemJson`.
        // Cerca un array "videos", "trailers", o simili.
        // Esempio IPOTETICO:
        if (j_item.contains("videos") && j_item.at("videos").is_array() && !j_item.at("videos").empty()) {
            for (const auto& videoNode : j_item.at("videos")) {
                if (videoNode.is_object() && videoNode.contains("url") && videoNode.at("url").is_string()) {
                    // Potrebbe esserci un array di "sources" o "files" con diverse qualità/formati
                    // Scegli il primo o il migliore (es. mp4)
                    item.videoUrl = videoNode.at("url").get<std::string>();
                    // Se ci sono tipi di video, potresti voler filtrare per "trailer" o "gameplay"
                    // if (videoNode.value("type", "") == "Trailer") ...
                    if (!item.videoUrl.empty()) break; // Prendi il primo video valido
                } else if (videoNode.is_object() && videoNode.contains("sources") && videoNode.at("sources").is_array()){
                    for(const auto& sourceNode : videoNode.at("sources")){
                        if(sourceNode.is_object() && sourceNode.contains("src") && sourceNode.at("src").is_string()){
                            std::string srcUrl = sourceNode.at("src").get<std::string>();
                            if (Utils::String::toLower(srcUrl).find(".mp4") != std::string::npos) { // Preferisci MP4
                                item.videoUrl = srcUrl;
                                break;
                            }
                        }
                    }
                    if (!item.videoUrl.empty()) break;
                }
            }
            LOG(LogDebug) << "EpicGamesParser: Trovato videoUrl da array 'videos': " << item.videoUrl;
        }
        // Altro esempio se i video fossero dentro customAttributes (improbabile per URL diretti)
        // for (const auto& attr : item.customAttributes) {
        //    if (Utils::String::toLower(attr.key) == "videourl" || Utils::String::toLower(attr.key) == "trailerurl") {
        //        item.videoUrl = attr.value;
        //        if (!item.videoUrl.empty()) break;
        //    }
        // }

        if (item.videoUrl.empty()) {
            LOG(LogDebug) << "EpicGamesParser: Nessun videoUrl trovato per item " << item.id;
        }
        
        LOG(LogInfo) << "EpicGamesParser: Parsed item " << item.id << " ('" << item.title << "'). "
                     << "Boxart: " << (!item.boxartUrl.empty()) << ", Fanart: " << (!item.fanartUrl.empty())
                     << ", Video: " << (!item.videoUrl.empty()) << ", Thumb: " << (!item.thumbnailUrl.empty());

        return item;
    }

} // namespace EpicGames


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

// --- Implementazione Helper URL ---
std::string EpicGamesStoreAPI::getAssetsUrl() {
    std::string hostname = "launcher-public-service-prod06.ol.epicgames.com";
    return "https://" + hostname + "/launcher/api/public/assets/Windows?label=Live";
}

std::string EpicGamesStoreAPI::getCatalogUrl(const std::string& ns) {
    // L'endpoint del catalogo per item individuali o bulk è spesso diverso da quello degli asset generali.
    // Potrebbe essere qualcosa tipo:
    // std::string hostname = "catalog-public-service-prod06.ol.epicgames.com"; // Già così
    // return "https://" + hostname + "/catalog/api/shared/namespace/" + ns + "/items"; // o /bulk/items
    // Il tuo codice usa /catalog/api/shared/namespace/NS, a cui poi aggiunge /bulk/items?id=...
    // Questo va bene, assicurati solo che l'URL finale sia corretto per l'API che stai usando.
    std::string hostname = "catalog-public-service-prod06.ol.epicgames.com";
    return "https://" + hostname + "/catalog/api/shared/namespace/" + ns;
}

// --- Implementazione Helper makeApiRequest ---
// Questa funzione sembra robusta per fare richieste generiche.
std::string EpicGamesStoreAPI::makeApiRequest(const std::string& url, const std::string& token, const std::string& method, const std::string& body, const std::vector<std::string>& headers) {
    LOG(LogDebug) << "makeApiRequest: Method=" << method << ", URL=" << url;
    HttpReqOptions options;
    if (!token.empty()) { 
        options.customHeaders.push_back("Authorization: bearer " + token); 
    }
    // Aggiungi gli header passati, sovrascrivendo Authorization se già presente (improbabile)
    for(const auto& h : headers) { options.customHeaders.push_back(h); }

    if ((method == "POST" || method == "PUT") && !body.empty()) {
        options.dataToPost = body;
        bool contentTypeSet = false;
        for(const auto& h_check : options.customHeaders) { if (Utils::String::startsWith(Utils::String::toLower(h_check), "content-type:")) { contentTypeSet = true; break; } }
        if (!contentTypeSet) { options.customHeaders.push_back("Content-Type: application/json"); }
    }

    std::unique_ptr<HttpReq> request = std::make_unique<HttpReq>(url, &options); // Usa std::unique_ptr
    
    if (!request->wait()) {
        throw std::runtime_error("HTTP request failed (wait): " + request->getErrorMsg());
    }
    if (request->status() != HttpReq::REQ_SUCCESS) {
        throw std::runtime_error("HTTP request status not success: " + std::to_string(request->status()) + " Body: " + request->getContent().substr(0, 500));
    }
    return request->getContent();
}

// --- Helper Interno per Logica Sincrona GetAllAssets ---
// Questa funzione sembra recuperare una lista di giochi posseduti/asset,
// ma NON i dettagli completi di ciascuno (come keyImages o video).
// È corretto che restituisca std::vector<EpicGames::Asset>.
std::vector<EpicGames::Asset> EpicGamesStoreAPI::performGetAllAssetsSync()
{
    LOG(LogDebug) << "EpicAPI SYNC_IMPL: Executing performGetAllAssetsSync()";
    if (!mAuth || mAuth->getAccessToken().empty()) { throw std::runtime_error("Not authenticated for GetAllAssets"); }

    std::string currentToken = mAuth->getAccessToken();
    std::string url = getAssetsUrl();
    LOG(LogDebug) << "performGetAllAssetsSync: Requesting URL: " << url;

    std::vector<EpicGames::Asset> assets;
    try {
        // Usiamo il makeApiRequest helper per consistenza, anche se qui non c'è body o method specifico se è un GET
        std::string responseBody = makeApiRequest(url, currentToken); // GET è il default per HttpReq se method non specificato

        LOG(LogDebug) << "performGetAllAssetsSync: Response received (size: " << responseBody.length() << ")";
        if (!responseBody.empty()) {
            json parsedResponse = json::parse(responseBody);
            if (parsedResponse.is_array()) {
                assets.reserve(parsedResponse.size());
                for (const auto& assetJson : parsedResponse) {
                    try {
                        assets.push_back(EpicGames::Asset::fromJson(assetJson));
                    } catch (const std::exception& e) { LOG(LogError) << "performGetAllAssetsSync: Error parsing individual asset JSON: " << e.what(); }
                }
                LOG(LogInfo) << "performGetAllAssetsSync: Successfully parsed " << assets.size() << " assets.";
            } else { LOG(LogError) << "performGetAllAssetsSync: JSON response is not an array."; }
        } else { LOG(LogInfo) << "performGetAllAssetsSync: Response body is empty."; }

    } catch (const std::exception& e) { 
        LOG(LogError) << "performGetAllAssetsSync: Exception: " << e.what(); 
        throw; // Rilancia per essere gestita dal chiamante (es. GetAllAssetsAsync)
    }
    return assets;
}


// --- Helper Interno per Logica Sincrona GetCatalogItems (MODIFICATO) ---
std::map<std::string, EpicGames::CatalogItem> EpicGamesStoreAPI::performGetCatalogItemsSync(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch, // pair di <namespace, catalogItemId>
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicAPI: Executing performGetCatalogItemsSync for " << itemsToFetch.size() << " items.";

    if (!mAuth || !mAuth->isAuthenticated()) {
        if (mAuth && !mAuth->getRefreshToken().empty()) {
            LOG(LogInfo) << "performGetCatalogItemsSync: Not authenticated but refresh token exists. Attempting preemptive refresh.";
            if (!mAuth->refreshAccessToken()) {
                LOG(LogError) << "performGetCatalogItemsSync: Preemptive token refresh failed.";
                throw std::runtime_error("Authentication failed (preemptive refresh). Please log in again.");
            }
            if (!mAuth->isAuthenticated()) { 
                throw std::runtime_error("Authentication failed after preemptive refresh. Please log in again.");
            }
            LOG(LogInfo) << "performGetCatalogItemsSync: Preemptive refresh successful.";
        } else if (!mAuth || !mAuth->isAuthenticated()) { 
            LOG(LogError) << "performGetCatalogItemsSync: Not authenticated and no means to refresh for GetCatalogItems.";
            throw std::runtime_error("Not authenticated for GetCatalogItems. Please log in.");
        }
    }
    
    if (itemsToFetch.empty()) { return {}; }

    std::map<std::string, EpicGames::CatalogItem> results;
    
    // L'API /bulk/items può accettare più ID, ma per semplicità e per gestire meglio i ritardi,
    // potremmo fare una chiamata per ogni item o per piccoli batch.
    // Il tuo codice attuale itera su itemsToFetch e fa una chiamata per item. Va bene per ora.
    for (const auto& itemPair : itemsToFetch) {
        const std::string& ns = itemPair.first;
        const std::string& catalogId = itemPair.second;
        
        // L'URL per /bulk/items di solito prende un array di ID nel corpo di una POST,
        // o una lista di ID come parametro query se è un GET.
        // Il tuo URL attuale: baseUrl + "/bulk/items?id=" + HttpReq::urlEncode(catalogId)
        // Questo implica che l'API supporta un singolo ID nel parametro query 'id'.
        // Se questo è corretto, va bene. Se /bulk/items si aspetta un array di ID per una vera
        // richiesta bulk, dovresti raggruppare gli itemsToFetch per namespace e fare meno richieste.
        // Per ora, manteniamo la logica 1 chiamata per item ID.
        std::string baseUrl = getCatalogUrl(ns); 
        std::string itemUrl = baseUrl + "/items/" + HttpReq::urlEncode(catalogId) + // Spesso l'endpoint per singolo item è /items/ID
                              "?country=" + HttpReq::urlEncode(country) +
                              "&locale=" + HttpReq::urlEncode(locale) +
                              "&includeMainGameDetails=true"; // Questo parametro è ipotetico, verifica se l'API lo supporta
        // L'URL originale "/bulk/items?id=" potrebbe essere per un'API diversa o un alias.
        // Un endpoint comune per un singolo item è /items/{catalogItemId}
        // Verifica l'URL corretto per ottenere i dettagli di un singolo CatalogItem.
        // Esempio: https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/items/CATALOG_ITEM_ID?namespace=NAMESPACE&country=US&locale=en-US
        // O, se usi il product slug (nome-gioco-in-url):
        // https://store-content-ipv4.ak.epicgames.com/api/en-US/content/products/SLUG
        // Per ora, assumo che il tuo URL con /bulk/items?id= singolo sia corretto e restituisca
        // un JSON del tipo { "catalogId": { ... dettagli ... } }

        bool requestSucceededForThisItem = false;
        int retryAttempt = 0;
        const int MAX_AUTH_RETRIES = 1; 

        while (!requestSucceededForThisItem && retryAttempt <= MAX_AUTH_RETRIES) {
            std::string currentAccessToken = mAuth->getAccessToken(); // Necessario per API catalogo? Alcuni endpoint sono pubblici.
                                                                    // Se non necessario, puoi passare un token vuoto a makeApiRequest.
            if (currentAccessToken.empty() && /* l'endpoint richiede auth */ false) { // Aggiungi condizione se l'endpoint è protetto
                 LOG(LogError) << "performGetCatalogItemsSync: No access token for " << catalogId;
                 throw std::runtime_error("Access token disappeared. Please log in again.");
            }
            
            LOG(LogDebug) << "performGetCatalogItemsSync: Requesting URL (attempt " << retryAttempt + 1 << "): " << itemUrl;
            
            try {
                // Se l'endpoint è pubblico, token può essere vuoto.
                // Il tuo makeApiRequest aggiunge l'header Auth solo se token non è vuoto.
                std::string responseBody = makeApiRequest(itemUrl, currentAccessToken /* o "" se pubblico */); 

                LOG(LogInfo) << "performGetCatalogItemsSync: Successfully fetched data for " << itemUrl;
                if (!responseBody.empty()) {
                    json parsedResponse = json::parse(responseBody); 
                    
                    // La struttura della risposta dall'API /catalog/api/shared/namespace/{namespace}/items/{itemId}
                    // o simili, di solito è direttamente l'oggetto dell'item, non inscatolato in una chiave ID.
                    // Se il tuo URL /bulk/items?id=CATALOG_ID restituisce { "CATALOG_ID": {details} }, allora:
                    // if (parsedResponse.is_object() && parsedResponse.contains(catalogId)) {
                    //    json itemJson = parsedResponse.at(catalogId);
                    //    results[catalogId] = EpicGames::parseCompleteCatalogItemFromJson(itemJson, catalogId);
                    // } else { ... errore o parsing diretto ... }
                    // Assumendo che parsedResponse SIA l'oggetto dell'item:
                    results[catalogId] = EpicGames::parseCompleteCatalogItemFromJson(parsedResponse, catalogId);
                    
                    LOG(LogInfo) << "performGetCatalogItemsSync: Successfully parsed item: " << catalogId << " ('" << results[catalogId].title << "')";
                } else { 
                    LOG(LogWarning) << "performGetCatalogItemsSync: Response body for item " << catalogId << " is empty despite HTTP 200.";
                }
                requestSucceededForThisItem = true; 
            
            // Rimozione della gestione 401 e retry specifici qui, dato che makeApiRequest 
            // dovrebbe lanciare eccezione in caso di errore HTTP non 200.
            // La logica di refresh token dovrebbe essere gestita a un livello più alto o prima della chiamata.
            // O makeApiRequest dovrebbe restituire lo status code per permettere qui il retry su 401.
            // Per ora, assumiamo che makeApiRequest lanci eccezione per errori HTTP.

            } catch (const std::runtime_error& e_req) { 
                LOG(LogError) << "performGetCatalogItemsSync: Runtime error during request for " << itemUrl << ": " << e_req.what();
                // Controlla se è un errore di autenticazione per tentare il refresh
                std::string errorMsg = e_req.what();
                if (errorMsg.find("status: 401") != std::string::npos && retryAttempt < MAX_AUTH_RETRIES) {
                    LOG(LogWarning) << "Received 401. Attempting token refresh...";
                    if (mAuth && mAuth->refreshAccessToken()) {
                        LOG(LogInfo) << "Token refresh successful. Retrying request for " << itemUrl;
                        // Non rompere il while, retryAttempt++ lo farà riprovare
                    } else {
                        LOG(LogError) << "Token refresh failed for " << itemUrl << ". Aborting.";
                        if(mAuth) mAuth->clearAllTokenData();
                        throw std::runtime_error("Authentication failed after token refresh attempt. Please log in again.");
                    }
                } else {
                    // Altro errore runtime, non ritentare all'infinito per questo item
                    LOG(LogError) << "Non-authentication runtime error or max retries reached for " << catalogId << ". Skipping item.";
                    break; // Esce dal while di retry per questo item
                }
            } catch (const json::parse_error& e_json) {
        //        LOG(LogError) << "performGetCatalogItemsSync: JSON parsing error for " << itemUrl << ": " << e_json.what() << ". Response: " << request.getContent().substr(0, 500);
                break; 
            } catch (const std::exception& e_std) {
                LOG(LogError) << "performGetCatalogItemsSync: Std exception for " << itemUrl << ": " << e_std.what();
                break; 
            }
            retryAttempt++;
        } // Fine while loop (retry)

        if (!requestSucceededForThisItem) {
            LOG(LogError) << "performGetCatalogItemsSync: Failed to process item " << catalogId << " (NS: " << ns << ") after all attempts.";
        }
        
     int scraperDelayMs = Settings::getInstance()->getInt("ScraperEpicDelay"); // Legge l'impostazione. Ritorna 0 se non trovata o se il valore è 0.
if (scraperDelayMs <= 0) { // Se l'impostazione non è stata trovata (e getInt ha restituito 0),
                           // o se è stata trovata ma è 0 o negativa (valori che non attiverebbero il delay),
                           // allora impostiamo il nostro default di 750.
    scraperDelayMs = 750;
}
    } // Fine for loop (itemsToFetch)

    LOG(LogInfo) << "performGetCatalogItemsSync: Finished. Returning " << results.size() << " results.";
    return results;
}


// --- Implementazione Metodi API ASINCRONI (Chiama Helper Sincroni) ---
// (Queste rimangono come nel tuo codice, chiamando le versioni perform...Sync)
std::future<std::vector<EpicGames::Asset>> EpicGamesStoreAPI::GetAllAssetsAsync()
{
    LOG(LogDebug) << "EpicAPI ASYNC: Queuing GetAllAssets task...";
    return std::async(std::launch::async, [this]() {
        LOG(LogDebug) << "EpicAPI ASYNC_THREAD: Executing performGetAllAssetsSync()...";
        try {
            return this->performGetAllAssetsSync();
        } catch (...) {
            LOG(LogError) << "EpicAPI ASYNC_THREAD: Exception caught in performGetAllAssetsSync. Propagating...";
            throw; // Rilancia per essere gestita da .get() o dal chiamante
        }
    });
}

std::future<std::map<std::string, EpicGames::CatalogItem>> EpicGamesStoreAPI::GetCatalogItemsAsync(
    const std::vector<std::pair<std::string, std::string>>& itemsToFetch,
    const std::string& country,
    const std::string& locale)
{
    LOG(LogDebug) << "EpicAPI ASYNC: Queuing GetCatalogItems task for " << itemsToFetch.size() << " items...";
    return std::async(std::launch::async, [this, itemsToFetch, country, locale]() { // Cattura per valore se necessario
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
// (Queste rimangono come nel tuo codice)
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
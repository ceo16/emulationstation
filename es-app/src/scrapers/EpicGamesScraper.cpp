#include "scrapers/EpicGamesScraper.h"
#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "GameStore/EpicGames/EpicGamesModels.h"
#include "FileData.h"
#include "MetaData.h" // Incluso per MetaDataId
#include "SystemData.h"
#include "Log.h"
#include "Settings.h"
#include "utils/TimeUtil.h"
#include "HttpReq.h"       // Serve ancora per HttpReqOptions? Forse no.
#include "scrapers/Scraper.h" // Incluso per ScraperSearchResult, ScraperSearchItem, ecc.
#include "AsyncHandle.h"  // Includi la classe base di ScraperRequest

#include <memory>
#include <filesystem>
#include <mutex>
#include <string> // Per std::string() cast
#include <set>    // Per std::set
#include <map>    // Per std::map

// --- Definizione della Richiesta Personalizzata ---
// !!! EREDITA DA ScraperRequest (che eredita da AsyncHandle) !!!
class EpicGamesScraperRequest : public ScraperRequest
{
private:
    std::string mNamespace;
    std::string mCatalogId;
    bool mRequestLaunched = false; // Flag per eseguire la logica solo una volta

public:
    // Il costruttore ora chiama solo ScraperRequest(resultsWrite)
    EpicGamesScraperRequest(const std::string& ns, const std::string& catalogId, std::vector<ScraperSearchResult>& resultsWrite)
        : ScraperRequest(resultsWrite), // Chiama il costruttore base corretto
          mNamespace(ns), mCatalogId(catalogId)
    {
        LOG(LogDebug) << "EpicGamesScraperRequest (derived from ScraperRequest) created for ns=" << ns << ", id=" << catalogId;
        // Lo stato mStatus (da AsyncHandle) viene inizializzato a ASYNC_IN_PROGRESS dal costruttore di AsyncHandle
    }

    // Implementiamo update() come richiesto da ScraperRequest (che eredita da AsyncHandle)
    void update() override
    {
        // Se lo stato è già completato o in errore, non fare nulla (update viene chiamato ripetutamente)
        if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR)
            return;

        // Se non abbiamo ancora lanciato la nostra logica API, falla ora
        if (!mRequestLaunched)
        {
            mRequestLaunched = true; // Segna come lanciata per le chiamate successive a update()
            // mStatus è già ASYNC_IN_PROGRESS

            LOG(LogError) << "!!! EPIC SCRAPER: update() function EXECUTING LOGIC for id=" << mCatalogId;

            try {
                 LOG(LogError) << "!!! EPIC SCRAPER: update() Instantiating EpicGamesAuth...";
                 EpicGamesAuth auth;
                 LOG(LogError) << "!!! EPIC SCRAPER: update() EpicGamesAuth instantiated.";

                 if (auth.getAccessToken().empty()) {
                     LOG(LogWarning) << "EpicGamesScraperRequest: update() Cannot proceed, Epic authentication failed or token invalid.";
                      setError("Authentication Failed"); // Usa setError da AsyncHandle
                      return; // Lo stato è ora ASYNC_ERROR, le prossime chiamate a update() usciranno subito
                 }
                  LOG(LogError) << "!!! EPIC SCRAPER: update() Authentication check passed.";

                 EpicGamesStoreAPI api(&auth);
                 std::vector<std::pair<std::string, std::string>> itemsToFetch = {{mNamespace, mCatalogId}};
                 std::string country = Settings::getInstance()->getString("EpicCountryCode");
                 std::string locale = Settings::getInstance()->getString("EpicLocaleCode");
                 if (country.empty()) country = "IT";
                 if (locale.empty()) locale = "it-IT";

                 LOG(LogError) << "!!! EPIC SCRAPER: update() Calling GetCatalogItems...";
                 std::map<std::string, EpicGames::CatalogItem> resultsMap = api.GetCatalogItems(itemsToFetch, country, locale);
                 LOG(LogError) << "!!! EPIC SCRAPER: update() GetCatalogItems returned.";

                 if (resultsMap.count(mCatalogId))
                 {
                     const EpicGames::CatalogItem& item = resultsMap.at(mCatalogId);
                     LOG(LogError) << "!!! EPIC SCRAPER: update() Found item: " << item.title;

                     ScraperSearchResult searchResult;
                     searchResult.scraper = "epicgames";
                     searchResult.mdl.set(MetaDataId::Name, item.title);
                     searchResult.mdl.set(MetaDataId::Desc, item.description);
                     searchResult.mdl.set(MetaDataId::Developer, item.developer);
                     searchResult.mdl.set(MetaDataId::Publisher, item.publisher);
                    if (!item.releaseDate.empty())
{
    std::string isoDateStr = item.releaseDate; // Es: "2019-09-10T00:00:00.000Z"
    std::string esDateStr;

    // Tentativo di conversione: Rimuovi '-', ':' e tutto da '.' o 'Z' in poi
    try {
        // Rimuovi trattini dalla parte data
        isoDateStr.erase(std::remove(isoDateStr.begin(), isoDateStr.end(), '-'), isoDateStr.end());
        // Rimuovi due punti dalla parte ora
        isoDateStr.erase(std::remove(isoDateStr.begin(), isoDateStr.end(), ':'), isoDateStr.end());

        // Trova la 'T' che separa data e ora
        size_t tPos = isoDateStr.find('T');
        if (tPos != std::string::npos && tPos + 7 <= isoDateStr.length()) // Assicurati che ci siano abbastanza caratteri per YYYYMMDDTHHMMSS
        {
             // Prendi YYYYMMDDTHHMMSS (15 caratteri)
             esDateStr = isoDateStr.substr(0, tPos + 7); // Prende fino a SS

             // Verifica che il risultato sia nel formato corretto (15 cifre + 'T')
             bool formatOk = (esDateStr.length() == 15 && esDateStr[8] == 'T');
             for(size_t i = 0; formatOk && i < esDateStr.length(); ++i) {
                 if (i != 8 && !std::isdigit(esDateStr[i])) {
                     formatOk = false;
                 }
             }

             if (!formatOk) {
                  LOG(LogWarning) << "EpicGamesScraper: Conversione data fallita (formato non valido dopo manipolazione) per: " << item.releaseDate;
                  esDateStr = ""; // Reset se la conversione non è valida
             } else {
                  LOG(LogDebug) << "EpicGamesScraper: Data convertita da '" << item.releaseDate << "' a '" << esDateStr << "'";
             }
        } else {
             LOG(LogWarning) << "EpicGamesScraper: Formato data ISO non riconosciuto o troppo corto: " << item.releaseDate;
        }

    } catch (const std::exception& e) {
        LOG(LogError) << "EpicGamesScraper: Errore durante la conversione della data '" << item.releaseDate << "': " << e.what();
        esDateStr = ""; // Reset in caso di errore
    }

    // Imposta il metadato SOLO se la conversione è andata a buon fine
    if (!esDateStr.empty()) {
        searchResult.mdl.set(MetaDataId::ReleaseDate, esDateStr);
    }
}
                     std::string genres;
                     for(const auto& cat : item.categories) {
                         if (!cat.path.empty()) {
                             if (!genres.empty()) genres += "; ";
                             size_t lastSlash = cat.path.find_last_of('/');
                             genres += (lastSlash == std::string::npos) ? cat.path : cat.path.substr(lastSlash + 1);
                         }
                     }
                     if (!genres.empty()) searchResult.mdl.set(MetaDataId::Genre, genres);

                     std::string imageUrl_str, thumbUrl_str, marqueeUrl_str;
                     for (const auto& img : item.keyImages) {
                         if (imageUrl_str.empty() && (img.type == "OfferImageTall" || img.type == "VaultClosed" || img.type == "DieselGameBoxTall")) { imageUrl_str = img.url; }
                         else if (thumbUrl_str.empty() && (img.type == "OfferImageWide" || img.type == "DieselStoreFrontWide" || img.type == "DieselGameBoxWide")) { thumbUrl_str = img.url; }
                         else if (marqueeUrl_str.empty() && (img.type == "Thumbnail" || img.type == "ProductLogo" || img.type == "DieselGameBoxLogo")) { marqueeUrl_str = img.url; }
                     }

                     // Popola la mappa 'urls' (Usa i nomi/chiavi corrette che hai trovato!)
                     if (!imageUrl_str.empty()) {
                         ScraperSearchItem itemUrl; itemUrl.url = imageUrl_str; itemUrl.format = ".jpg";
                         searchResult.urls[MetaDataId::Image] = itemUrl; // O Box2d? Sostituisci con i tuoi nomi corretti
                     }
                     if (!thumbUrl_str.empty()) {
                         ScraperSearchItem itemThumb; itemThumb.url = thumbUrl_str; itemThumb.format = ".jpg";
                         searchResult.urls[MetaDataId::Thumbnail] = itemThumb; // O Screenshot? Sostituisci!
                     }
                     if (!marqueeUrl_str.empty()) {
                         ScraperSearchItem itemMarquee; itemMarquee.url = marqueeUrl_str; itemMarquee.format = ".png";
                          searchResult.urls[MetaDataId::Marquee] = itemMarquee; // O Wheel? Sostituisci!
                     }

                     // Aggiungi il risultato al vettore mResults ereditato
                     // Usiamo il lock della classe base ScraperRequest (che è AsyncHandle, quindi non ha mResultsLock)
                     // L'accesso a mResults è sicuro perché ogni oggetto Request ha il suo? Sembra di sì.
                     // std::lock_guard<std::mutex> lock(mResultsLock); // Rimosso perché non esiste in AsyncHandle
                     mResults.push_back(searchResult);

                     LOG(LogError) << "!!! EPIC SCRAPER: update() Successfully processed result.";
                     setStatus(ASYNC_DONE); // Imposta stato a Completato

                 } else {
                      LOG(LogWarning) << "EpicGamesScraperRequest: update() Item not found in API response for " << mCatalogId;
                      setStatus(ASYNC_DONE); // Consideriamo completato anche se non trovato
                      LOG(LogError) << "!!! EPIC SCRAPER: update() Item not found in API.";
                 }

            } catch (const std::exception& e) {
                 LOG(LogError) << "!!! EPIC SCRAPER: update() Exception caught: " << e.what();
                 setError(e.what()); // Imposta errore
            } catch (...) {
                 LOG(LogError) << "!!! EPIC SCRAPER: update() Unknown exception caught.";
                 setError("Unknown Error"); // Imposta errore generico
            }
        } // Fine if (!mRequestLaunched)

        // Se arriviamo qui, o abbiamo appena eseguito la logica e impostato lo stato a DONE/ERROR,
        // o la logica era già stata eseguita in una chiamata precedente a update().
        // Non facciamo altro, il sistema chiamerà di nuovo update() finché lo stato non è DONE/ERROR.
         LOG(LogDebug) << "EpicGamesScraperRequest::update() finished iteration for id=" << mCatalogId << " with status " << mStatus; // Leggiamo mStatus da AsyncHandle
    }

    // NON implementiamo più process() perché non è richiesto da ScraperRequest / AsyncHandle

}; // Fine classe EpicGamesScraperRequest


// --- Implementazione della classe EpicGamesScraper (INVARIATA) ---
// ... (isSupportedPlatform, getSupportedMedias, generateRequests come nell'ultima versione OK) ...
bool EpicGamesScraper::isSupportedPlatform(SystemData* system) { return system != nullptr && system->getName() == "epicgamestore"; }
const std::set<Scraper::ScraperMediaSource>& EpicGamesScraper::getSupportedMedias() { static std::set<Scraper::ScraperMediaSource> supportedMedia = { Scraper::ScraperMediaSource::Screenshot, Scraper::ScraperMediaSource::Box2d, Scraper::ScraperMediaSource::Marquee }; return supportedMedia; }
void EpicGamesScraper::generateRequests(const ScraperSearchParams& params, std::queue<std::unique_ptr<ScraperRequest>>& requests, std::vector<ScraperSearchResult>& results)
{
    std::string gamePathStr; try { gamePathStr = std::string(params.game->getPath()); } catch (...) { gamePathStr = "[[PATH CONVERSION FAILED]]"; }
    LOG(LogDebug) << "EpicGamesScraper::generateRequests for: " << gamePathStr;
    std::string ns = params.game->getMetadata(MetaDataId::EpicNamespace);
    std::string catalogId = params.game->getMetadata(MetaDataId::EpicCatalogId);
    if (ns.empty() || catalogId.empty()) { LOG(LogWarning) << "EpicGamesScraper: Missing 'epicns' or 'epiccstid' metadata for game '" << params.game->getName() << "'. Check gamelist.xml. Skipping."; return; }
    LOG(LogInfo) << "EpicGamesScraper: Found ns=" << ns << ", catalogId=" << catalogId << " for game " << params.game->getName();
    requests.push(std::unique_ptr<ScraperRequest>(new EpicGamesScraperRequest(ns, catalogId, results)));
    LOG(LogDebug) << "EpicGamesScraper: Queued request for " << catalogId;
}
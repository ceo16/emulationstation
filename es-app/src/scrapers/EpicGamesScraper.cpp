#include "scrapers/EpicGamesScraper.h" // Include il tuo header

// Include necessari per lo scraper e le funzionalità Epic/ES
#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "GameStore/EpicGames/EpicGamesModels.h"
#include "FileData.h"
#include "MetaData.h" // Per MetaDataId
#include "SystemData.h"
#include "Log.h"
#include "Settings.h"
#include "utils/TimeUtil.h" // Per la conversione delle date
#include "utils/FileSystemUtil.h" // Per l'estensione dei file immagine
#include "scrapers/Scraper.h" // Necessario per ScraperSearchResult, ScraperRequest, etc.
#include "AsyncHandle.h" // Necessario perché ScraperRequest ne eredita

#include <memory>     // Per std::unique_ptr
#include <filesystem> // Potrebbe servire per gestire path
#include <mutex>      // Se necessario (ma ScraperRequest è gestito singolarmente)
#include <string>     // Per std::string() cast
#include <set>        // Per std::set in getSupportedMedias
#include <map>        // Per std::map nei risultati API
#include <vector>     // Per std::vector
#include <queue>      // Per std::queue in generateRequests
#include <algorithm>  // Per std::remove in conversione data
#include <cctype>     // Per std::isdigit in conversione data

//-----------------------------------------------------------------------------------
// Definizione della classe interna per gestire la singola richiesta allo scraper
//-----------------------------------------------------------------------------------
class EpicGamesScraperRequest : public ScraperRequest // Eredita da ScraperRequest
{
private:
    // Dati essenziali da preservare e usare per la richiesta API
    std::string mNamespace;
    std::string mCatalogId;
    std::string mEpicId;
    std::string mVirtual; // Conservato come stringa ("true" o "false")
    std::string mLaunchCmd;
    bool mRequestLaunched; // Flag per eseguire la logica API solo una volta

public:
    // Costruttore: Riceve tutti i dati da preservare e la lista dei risultati
    EpicGamesScraperRequest(
        const std::string& ns,
        const std::string& catalogId,
        const std::string& epicId,
        const std::string& isVirtual,
        const std::string& launchCmd,
        std::vector<ScraperSearchResult>& resultsWrite)
        : ScraperRequest(resultsWrite), // Chiama il costruttore della classe base
          mNamespace(ns),
          mCatalogId(catalogId),
          mEpicId(epicId),
          mVirtual(isVirtual),
          mLaunchCmd(launchCmd),
          mRequestLaunched(false)
    {
        LOG(LogDebug) << "EpicGamesScraperRequest created for ns=" << ns << ", cId=" << catalogId << ", eId=" << epicId;
        // Lo stato mStatus viene inizializzato a ASYNC_IN_PROGRESS dal costruttore base di AsyncHandle
    }

    // Metodo principale chiamato ripetutamente da EmulationStation finché lo stato non è DONE o ERROR
    void update() override
    {
        // Se la richiesta è già terminata (successo o errore), non fare nulla
        if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) {
            return;
        }

        // Esegui la logica di chiamata API solo la prima volta
        if (!mRequestLaunched)
        {
            mRequestLaunched = true; // Segna che abbiamo iniziato
            LOG(LogDebug) << "EpicGamesScraperRequest::update() executing API call for catalogId=" << mCatalogId;

            try {
                // 1. Autenticazione (necessaria prima di chiamare l'API)
                // Istanziamo EpicGamesAuth qui dentro; gestirà il token internamente
                EpicGamesAuth auth;
                if (auth.getAccessToken().empty()) {
                    LOG(LogWarning) << "EpicGamesScraperRequest: Epic Authentication Token is empty. Cannot proceed with scraping.";
                    setError("Epic Authentication Failed"); // Imposta lo stato di errore
                    return; // Esce da questa chiamata a update()
                }
                LOG(LogDebug) << "EpicGamesScraperRequest: Authentication check passed.";

                // 2. Preparazione e Chiamata API
                EpicGamesStoreAPI api(&auth); // Crea l'oggetto API passando l'auth
                std::vector<std::pair<std::string, std::string>> itemsToFetch = {{mNamespace, mCatalogId}};

                // Ottieni le preferenze utente per paese e lingua (con fallback)
                std::string country = Settings::getInstance()->getString("EpicCountryCode");
                std::string locale = Settings::getInstance()->getString("EpicLocaleCode");
                if (country.empty()) country = "IT"; // Fallback a Italia
                if (locale.empty()) locale = "it-IT"; // Fallback a Italiano

                LOG(LogDebug) << "EpicGamesScraperRequest: Calling GetCatalogItems with ns=" << mNamespace << ", id=" << mCatalogId << ", country=" << country << ", locale=" << locale;
                std::map<std::string, EpicGames::CatalogItem> resultsMap = api.GetCatalogItems(itemsToFetch, country, locale);
                LOG(LogDebug) << "EpicGamesScraperRequest: GetCatalogItems returned " << resultsMap.size() << " results.";

                // 3. Processa il Risultato API
                if (resultsMap.count(mCatalogId)) // Controlla se l'ID richiesto è presente nella risposta
                {
                    const EpicGames::CatalogItem& item = resultsMap.at(mCatalogId); // Ottieni i dettagli del gioco
                    LOG(LogInfo) << "EpicGamesScraper: Found API data for '" << item.title << "' (ID: " << mCatalogId << ")";

                    ScraperSearchResult searchResult; // Crea l'oggetto che conterrà i risultati dello scraping
                    searchResult.scraper = "epicgames"; // Identifica questo scraper

                    // ---->> FASE 1: PRESERVA I METADATI ESSENZIALI <<----
                    // Copia i valori che avevamo salvato nel costruttore dentro al risultato
                    searchResult.mdl.set(MetaDataId::Virtual, mVirtual);
                    searchResult.mdl.set(MetaDataId::EpicId, mEpicId);
                    searchResult.mdl.set(MetaDataId::EpicNamespace, mNamespace);
                    searchResult.mdl.set(MetaDataId::EpicCatalogId, mCatalogId);
                    searchResult.mdl.set(MetaDataId::LaunchCommand, mLaunchCmd); // !!! Fondamentale !!!
                    LOG(LogDebug) << "  Preserved: virtual=" << mVirtual << ", epicId=" << mEpicId << ", ns=" << mNamespace << ", cId=" << mCatalogId << ", launch=...";

                    // ---->> FASE 2: AGGIUNGI/SOVRASCRIVI CON I DATI DALL'API <<----
                    if (!item.title.empty())       searchResult.mdl.set(MetaDataId::Name, item.title);
                    if (!item.description.empty()) searchResult.mdl.set(MetaDataId::Desc, item.description);
                    if (!item.developer.empty())   searchResult.mdl.set(MetaDataId::Developer, item.developer);
                    if (!item.publisher.empty())   searchResult.mdl.set(MetaDataId::Publisher, item.publisher);

                    // Gestione Data di Rilascio (con conversione ISO -> ES)
                    if (!item.releaseDate.empty()) {
                        time_t release_t = Utils::Time::iso8601ToTime(item.releaseDate); // Funzione per convertire da ISO 8601
                        if (release_t != Utils::Time::NOT_A_DATE_TIME) { // Controlla se la conversione è valida
                            std::string esDate = Utils::Time::timeToMetaDataString(release_t); // Converti in formato YYYYMMDDTHHMMSS
                            if (!esDate.empty()) {
                                searchResult.mdl.set(MetaDataId::ReleaseDate, esDate);
                                LOG(LogDebug) << "  Set ReleaseDate from API: " << esDate;
                            } else {
                                LOG(LogWarning) << "  Failed to convert timestamp to metadata string for ReleaseDate: " << item.releaseDate;
                            }
                        } else {
                            LOG(LogWarning) << "  Failed to parse ISO8601 ReleaseDate: " << item.releaseDate;
                        }
                    }

                    // Gestione Genere (concatena i path delle categorie)
                    std::string genres;
                    for (const auto& cat : item.categories) {
                        if (!cat.path.empty()) {
                            if (!genres.empty()) genres += "; "; // Separatore
                            size_t lastSlash = cat.path.find_last_of('/');
                            genres += (lastSlash == std::string::npos) ? cat.path : cat.path.substr(lastSlash + 1); // Prendi l'ultima parte del path
                        }
                    }
                    if (!genres.empty()) {
                         searchResult.mdl.set(MetaDataId::Genre, genres);
                         LOG(LogDebug) << "  Set Genre from API: " << genres;
                    }

                    // Gestione Immagini (popola la mappa 'urls')
                    std::string imageUrl_str, thumbUrl_str, marqueeUrl_str;
                    // Trova gli URL migliori tra le keyImages fornite dall'API
                    for (const auto& img : item.keyImages) {
                        // Priorità per immagini specifiche (adatta i tipi se necessario)
                        if (img.type == "OfferImageTall" || img.type == "DieselGameBoxTall") imageUrl_str = img.url;
                        else if (img.type == "OfferImageWide" || img.type == "DieselStoreFrontWide") thumbUrl_str = img.url;
                        else if (img.type == "ProductLogo" || img.type == "DieselGameBoxLogo") marqueeUrl_str = img.url;

                        // Fallback generici se i tipi preferiti non ci sono
                        if (imageUrl_str.empty() && (img.type == "VaultClosed" || img.type == "Thumbnail")) imageUrl_str = img.url;
                        if (thumbUrl_str.empty() && img.type == "Thumbnail") thumbUrl_str = img.url;
                    }
                     if (imageUrl_str.empty() && !item.keyImages.empty()) imageUrl_str = item.keyImages[0].url; // Fallback estremo alla prima immagine
                     if (thumbUrl_str.empty() && !imageUrl_str.empty()) thumbUrl_str = imageUrl_str; // Usa immagine principale se manca thumb

                    // Aggiungi gli URL trovati alla mappa 'urls' del risultato
      if (!imageUrl_str.empty()) {
    ScraperSearchItem itemUrl;
    itemUrl.url = imageUrl_str;
    itemUrl.format = ".jpg"; // <<< IMPOSTA ESTENSIONE FISSA (o determina in altro modo)
    searchResult.urls[MetaDataId::Image] = itemUrl;
    LOG(LogDebug) << "  Found Image URL: " << imageUrl_str;
}
if (!thumbUrl_str.empty()) {
    ScraperSearchItem itemThumb;
    itemThumb.url = thumbUrl_str;
    itemThumb.format = ".jpg"; // <<< IMPOSTA ESTENSIONE FISSA (o determina in altro modo)
    searchResult.urls[MetaDataId::Thumbnail] = itemThumb;
    LOG(LogDebug) << "  Found Thumbnail URL: " << thumbUrl_str;
}
if (!marqueeUrl_str.empty()) {
    ScraperSearchItem itemMarquee;
    itemMarquee.url = marqueeUrl_str;
    itemMarquee.format = ".png"; // <<< USA .png PER LOGHI/MARQUEE se appropriato
    searchResult.urls[MetaDataId::Marquee] = itemMarquee;
    LOG(LogDebug) << "  Found Marquee URL: " << marqueeUrl_str;
}
                    // Aggiungi qui la logica per altri tipi di media se necessario (Video, FanArt, etc.)

                    // 4. Aggiungi il risultato completo alla lista dei risultati
                    // L'accesso a mResults è gestito dalla classe base ScraperRequest
                    mResults.push_back(searchResult);
                    setStatus(ASYNC_DONE); // Imposta lo stato a completato (successo)
                    LOG(LogInfo) << "EpicGamesScraperRequest: Successfully processed result for " << mCatalogId;

                } else {
                    // L'ID richiesto non è stato trovato nella risposta API
                    LOG(LogWarning) << "EpicGamesScraperRequest: Item not found in API response for catalogId=" << mCatalogId;
                    setStatus(ASYNC_DONE); // Segna come completato comunque, per non bloccare la coda
                }

            } catch (const std::exception& e) {
                // Errore durante chiamata API o processamento
                LOG(LogError) << "EpicGamesScraperRequest: Exception during API call or processing for " << mCatalogId << ": " << e.what();
                setError(std::string("API Error: ") + e.what()); // Imposta stato di errore con messaggio
            } catch (...) {
                // Errore sconosciuto
                LOG(LogError) << "EpicGamesScraperRequest: Unknown exception during API call or processing for " << mCatalogId;
                setError("Unknown API Error"); // Imposta stato di errore generico
            }
        } // Fine if (!mRequestLaunched)

        // Log per debug per vedere lo stato ad ogni chiamata di update
        LOG(LogDebug) << "EpicGamesScraperRequest::update() finished iteration for id=" << mCatalogId << " with status " << mStatus;

    } // Fine update()

}; // Fine definizione classe EpicGamesScraperRequest


//-----------------------------------------------------------------------------------
// Implementazione dei metodi della classe EpicGamesScraper
//-----------------------------------------------------------------------------------

// Funzione chiamata per generare le richieste di scraping per un gioco
void EpicGamesScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests, // Coda dove inserire le richieste
    std::vector<ScraperSearchResult>& results)           // Vettore (condiviso?) dove la richiesta scriverà i risultati
{
    std::string gamePathStr; try { gamePathStr = std::string(params.game->getPath()); } catch (...) { gamePathStr = "[[PATH CONVERSION FAILED]]"; }
    LOG(LogDebug) << "EpicGamesScraper::generateRequests for game path: " << gamePathStr;

    // Leggi TUTTI i metadati essenziali dal FileData passato nei parametri
    std::string ns = params.game->getMetadata(MetaDataId::EpicNamespace);
    std::string catalogId = params.game->getMetadata(MetaDataId::EpicCatalogId);
    std::string epicId = params.game->getMetadata(MetaDataId::EpicId);
    std::string isVirtual = params.game->getMetadata(MetaDataId::Virtual);
    std::string launchCmd = params.game->getMetadata(MetaDataId::LaunchCommand);

    // Controlla che tutti i metadati necessari siano presenti, altrimenti salta lo scraping
    if (ns.empty() || catalogId.empty() || epicId.empty() || isVirtual.empty() || launchCmd.empty()) {
        LOG(LogWarning) << "EpicGamesScraper: Skipping game '" << params.game->getName()
                        << "' - Missing one or more essential metadata tags (epicns, epiccstid, epicid, virtual, launch)."
                        << " Check gamelist or initial fetch/creation process.";
        return; // Non possiamo fare scraping senza questi dati
    }

    LOG(LogInfo) << "EpicGamesScraper: Found required metadata for game '" << params.game->getName() << "': ns=" << ns << ", cId=" << catalogId << ", eId=" << epicId << ", virtual=" << isVirtual;

    // Crea una nuova istanza della nostra classe di richiesta personalizzata, passando tutti i dati necessari
    auto req = std::unique_ptr<ScraperRequest>(new EpicGamesScraperRequest(ns, catalogId, epicId, isVirtual, launchCmd, results));

    // Aggiungi la richiesta alla coda
    requests.push(std::move(req)); // Usa std::move per trasferire la proprietà del puntatore unico
    LOG(LogDebug) << "EpicGamesScraper: Queued request for catalogId=" << catalogId;
}

// Indica se questo scraper supporta la piattaforma del sistema dato
bool EpicGamesScraper::isSupportedPlatform(SystemData* system) {
    // Abilita lo scraper solo per il sistema "epicgamestore"
    return system != nullptr && system->getName() == "epicgamestore";
}

// Indica quali tipi di media questo scraper è in grado di fornire
const std::set<Scraper::ScraperMediaSource>& EpicGamesScraper::getSupportedMedias() {
    // Dichiara i tipi di media che puoi ottenere dall'API Epic e che mapperai
    // ai tipi standard di EmulationStation (potresti dover adattare i mapping)
    static std::set<Scraper::ScraperMediaSource> supportedMedia = {
        Scraper::ScraperMediaSource::Screenshot, // Es. OfferImageWide o DieselStoreFrontWide
        Scraper::ScraperMediaSource::Box2d,      // Es. OfferImageTall o DieselGameBoxTall
        Scraper::ScraperMediaSource::Marquee     // Es. ProductLogo o DieselGameBoxLogo
        // Aggiungi altri tipi se li gestisci (es. ScraperMediaSource::Video, FanArt, etc.)
    };
    return supportedMedia;
}
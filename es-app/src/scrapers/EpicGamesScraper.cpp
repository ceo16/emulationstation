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
#include "utils/StringUtil.h" // Necessario per Utils::String::toLower (se non già transitive)

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
    }

    // Metodo principale chiamato ripetutamente da EmulationStation finché lo stato non è DONE o ERROR
    void update() override
    {
        if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) {
            return;
        }

        if (!mRequestLaunched)
        {
            mRequestLaunched = true;
            LOG(LogDebug) << "EpicGamesScraperRequest::update() executing API call for catalogId=" << mCatalogId;

            try {
                EpicGamesAuth auth;
                if (auth.getAccessToken().empty()) {
                    LOG(LogWarning) << "EpicGamesScraperRequest: Epic Authentication Token is empty. Cannot proceed with scraping.";
                    setError("Epic Authentication Failed");
                    return;
                }
                LOG(LogDebug) << "EpicGamesScraperRequest: Authentication check passed.";

                EpicGamesStoreAPI api(&auth);
                std::vector<std::pair<std::string, std::string>> itemsToFetch = {{mNamespace, mCatalogId}};

                std::string country = Settings::getInstance()->getString("EpicCountryCode");
                std::string locale = Settings::getInstance()->getString("EpicLocaleCode");
                if (country.empty()) country = "IT"; 
                if (locale.empty()) locale = "it-IT";

                LOG(LogDebug) << "EpicGamesScraperRequest: Calling GetCatalogItems with ns=" << mNamespace << ", id=" << mCatalogId << ", country=" << country << ", locale=" << locale;
                std::map<std::string, EpicGames::CatalogItem> resultsMap = api.GetCatalogItems(itemsToFetch, country, locale);
                LOG(LogDebug) << "EpicGamesScraperRequest: GetCatalogItems returned " << resultsMap.size() << " results.";

                if (resultsMap.count(mCatalogId))
                {
                    const EpicGames::CatalogItem& item = resultsMap.at(mCatalogId);
                    LOG(LogInfo) << "EpicGamesScraper: Found API data for '" << item.title << "' (ID: " << mCatalogId << ")";

                    // <<< INIZIO BLOCCO LOG CUSTOM ATTRIBUTES >>>
                    LOG(LogDebug) << "--- Custom Attributes for: " << item.title << " ---";
                    for (const auto& attr : item.customAttributes) {
                        LOG(LogDebug) << "  Key: " << attr.key << " | Value: " << attr.value << " | Type: " << attr.type;
                        std::string lowerKey = Utils::String::toLower(attr.key);
                        if (lowerKey.find("video") != std::string::npos || lowerKey.find("trailer") != std::string::npos) {
                            LOG(LogInfo) << "POTENTIAL VIDEO/TRAILER customAttribute for " << item.title << " - Key: " << attr.key << ", Value: " << attr.value;
                        }
                    }
                    LOG(LogDebug) << "--- End Custom Attributes for: " << item.title << " ---";
                    // <<< FINE BLOCCO LOG CUSTOM ATTRIBUTES >>>

                    ScraperSearchResult searchResult;
                    searchResult.scraper = "epicgames";

                    searchResult.mdl.set(MetaDataId::Virtual, mVirtual);
                    searchResult.mdl.set(MetaDataId::EpicId, mEpicId);
                    searchResult.mdl.set(MetaDataId::EpicNamespace, mNamespace);
                    searchResult.mdl.set(MetaDataId::EpicCatalogId, mCatalogId);
                    searchResult.mdl.set(MetaDataId::LaunchCommand, mLaunchCmd);
                    LOG(LogDebug) << "  Preserved: virtual=" << mVirtual << ", epicId=" << mEpicId << ", ns=" << mNamespace << ", cId=" << mCatalogId << ", launch=...";

                    if (!item.title.empty())       searchResult.mdl.set(MetaDataId::Name, item.title);
                    if (!item.description.empty()) searchResult.mdl.set(MetaDataId::Desc, item.description);
                    if (!item.developer.empty())   searchResult.mdl.set(MetaDataId::Developer, item.developer);
                    if (!item.publisher.empty())   searchResult.mdl.set(MetaDataId::Publisher, item.publisher);

                    if (!item.releaseDate.empty()) {
                        time_t release_t = Utils::Time::iso8601ToTime(item.releaseDate);
                        if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                            std::string esDate = Utils::Time::timeToMetaDataString(release_t);
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

                    std::string genres;
                    for (const auto& cat : item.categories) {
                        if (!cat.path.empty()) {
                            if (!genres.empty()) genres += "; ";
                            size_t lastSlash = cat.path.find_last_of('/');
                            genres += (lastSlash == std::string::npos) ? cat.path : cat.path.substr(lastSlash + 1);
                        }
                    }
                    if (!genres.empty()) {
                        searchResult.mdl.set(MetaDataId::Genre, genres);
                        LOG(LogDebug) << "  Set Genre from API: " << genres;
                    }

                    // Gestione Immagini e Media
                    std::string imageUrl_str, thumbUrl_str, marqueeUrl_str;
                    std::string fanartUrl_str, videoUrl_str;
                    int previously_selected_fanart_width = 0; 

                    for (const auto& img : item.keyImages) {
                        LOG(LogDebug) << "Processing keyImage - Type: " << img.type << ", URL: " << img.url << ", Width: " << img.width << ", Height: " << img.height;

                        // Boxart (Image) - Priorità a immagini verticali
                        if (img.type == "OfferImageTall" || img.type == "DieselGameBoxTall") {
                            if (imageUrl_str.empty() || img.height > 1000) { // Prendi la prima o una più alta se già ne hai una
                                imageUrl_str = img.url;
                            }
                        }
                        // Thumbnail/Wide Banner - Priorità a immagini orizzontali larghe
                        // (Può anche essere un candidato per fanart)
                        else if (img.type == "OfferImageWide" || img.type == "DieselStoreFrontWide") {
                            if (thumbUrl_str.empty() || img.width > 1000) { // Prendi la prima o una più larga
                                thumbUrl_str = img.url;
                            }
                        }
                        // Marquee/Logo
                          else if (img.type == "ProductLogo" || img.type == "DieselGameBoxLogo") {
                            // Aggiungiamo controlli su dimensioni e aspect ratio per marquee più specifici
                            if (img.width > 0 && img.height > 0) {
                                float aspect_ratio = (float)img.width / img.height;
                                // Definiamo criteri più restrittivi per un logo:
                                // - Non troppo grande (es. nessuna dimensione oltre 1000px)
                                // - Aspect ratio non estremamente largo o alto (es. tra 0.3 e 4.0)
                                // Questi valori sono esempi, potresti doverli aggiustare.
                                if (img.width <= 1000 && img.height <= 600 && aspect_ratio >= 0.2f && aspect_ratio <= 5.0f) {
                                    marqueeUrl_str = img.url;
                                    LOG(LogInfo) << ">>> MARQUEE SELEZIONATO (Type: " << img.type << ", AR: " << aspect_ratio << ", W: " << img.width << " H: " << img.height << "): " << img.url;
                                } else {
                                    LOG(LogDebug) << "Scartato potenziale marquee (Type: " << img.type << ") per dimensioni/AR non ideali per un logo: " << img.url << " (W:" << img.width << " H:" << img.height << ")";
                                }
                            } else {
                                // Se le dimensioni non sono valide, ma il tipo è corretto, prendilo comunque (comportamento precedente)
                                // o scartalo. Per ora, se le dimensioni sono 0, non lo prendiamo.
                                LOG(LogDebug) << "Scartato potenziale marquee (Type: " << img.type << ") per dimensioni invalide (0): " << img.url;
                            }
                        }

                        // Fanart Selection Logic
                        // Cerchiamo tipi specifici o immagini landscape di alta qualità
                        if (img.type == "DieselGameBox" || // Ottimo candidato se landscape (es. FGSS04_KeyArt_OfferImageLandscape)
                            img.type == "DieselStoreFrontWide" ||
                            img.type == "OfferImageWide" ||
                            img.type == "TakeoverWide" ||        // Tipo comune per sfondi in alcune API
                            (img.type == "Screenshot" && img.width >= 1920) // Screenshot di alta qualità e molto larghi
                           ) {
                            if (img.width >= 1280 && img.height > 0 && ((float)img.width / img.height > 1.45) ) { // Assicura che sia landscape (ratio > ~1.5)
                                if (img.width > previously_selected_fanart_width) { 
                                    fanartUrl_str = img.url;
                                    previously_selected_fanart_width = img.width;
                                    LOG(LogInfo) << ">>> FANART Candidate (Type: " << img.type << ", Width: " << img.width << "): " << img.url;
                                }
                            }
                        }
                        
                        // Video Selection Logic (Placeholder - necessita dei tipi corretti)
                        // SOSTITUISCI "PlaceholderVideoType" con il tipo reale identificato dai log!
                        else if (img.type == "PlaceholderVideoType") { 
                            if (videoUrl_str.empty()) { // Prendi il primo video trovato
                                 videoUrl_str = img.url;
                                 LOG(LogInfo) << ">>> VIDEO Candidate (Type: " << img.type << "): " << img.url;
                            }
                        }
                    }

                    // Fallback per immagini principali se non trovate specificamente
                    if (imageUrl_str.empty() && !item.keyImages.empty()) {
                        for (const auto& img : item.keyImages) { // Cerca la prima "Thumbnail" o "VaultClosed" come fallback per image
                            if (img.type == "VaultClosed" || img.type == "Thumbnail") { imageUrl_str = img.url; break; }
                        }
                        if (imageUrl_str.empty()) imageUrl_str = item.keyImages[0].url; // Fallback estremo
                    }
                    if (thumbUrl_str.empty() && !imageUrl_str.empty() && !item.keyImages.empty()) { // Se thumb è vuota, prova a usare imageUrl o la prima wide
                         if (!imageUrl_str.empty() && previously_selected_fanart_width > 0 && fanartUrl_str == imageUrl_str) { // Evita di usare la stessa immagine della fanart se possibile
                             for (const auto& img : item.keyImages) { if (img.type == "Thumbnail") {thumbUrl_str = img.url; break;}}
                         }
                         if (thumbUrl_str.empty()) thumbUrl_str = imageUrl_str; // Fallback finale
                    }
                     // Non c'è un fallback esplicito per fanartUrl_str su thumbUrl_str qui per evitare di usare immagini verticali come fanart.
                     // La logica di selezione sopra dovrebbe essere abbastanza buona. Se fanartUrl_str è vuota, non verrà scaricata.


                    if (!imageUrl_str.empty()) {
                        ScraperSearchItem itemUrl; itemUrl.url = imageUrl_str; itemUrl.format = ".jpg";
                        searchResult.urls[MetaDataId::Image] = itemUrl;
                        LOG(LogDebug) << "  Set Image URL: " << imageUrl_str;
                    }
                    if (!thumbUrl_str.empty()) {
                        ScraperSearchItem itemThumb; itemThumb.url = thumbUrl_str; itemThumb.format = ".jpg";
                        searchResult.urls[MetaDataId::Thumbnail] = itemThumb;
                        LOG(LogDebug) << "  Set Thumbnail URL: " << thumbUrl_str;
                    }
                    if (!marqueeUrl_str.empty()) {
                        ScraperSearchItem itemMarquee; itemMarquee.url = marqueeUrl_str; itemMarquee.format = ".png";
                        searchResult.urls[MetaDataId::Marquee] = itemMarquee;
                        LOG(LogDebug) << "  Set Marquee URL: " << marqueeUrl_str;
                    }
                    if (!fanartUrl_str.empty()) {
                        ScraperSearchItem itemFanart; itemFanart.url = fanartUrl_str; itemFanart.format = ".jpg";
                        searchResult.urls[MetaDataId::FanArt] = itemFanart;
                        LOG(LogDebug) << "  Set FanArt URL: " << fanartUrl_str;
                    }
                    if (!videoUrl_str.empty()) {
                        ScraperSearchItem itemVideo; itemVideo.url = videoUrl_str; itemVideo.format = ".mp4"; 
                        searchResult.urls[MetaDataId::Video] = itemVideo;
                        LOG(LogDebug) << "  Set Video URL: " << videoUrl_str;
                    }

                    mResults.push_back(searchResult);
                    setStatus(ASYNC_DONE);
                    LOG(LogInfo) << "EpicGamesScraperRequest: Successfully processed result for " << mCatalogId;

                } else {
                    LOG(LogWarning) << "EpicGamesScraperRequest: Item not found in API response for catalogId=" << mCatalogId;
                    setStatus(ASYNC_DONE); 
                }

            } catch (const std::exception& e) {
                LOG(LogError) << "EpicGamesScraperRequest: Exception during API call or processing for " << mCatalogId << ": " << e.what();
                setError(std::string("API Error: ") + e.what());
            } catch (...) {
                LOG(LogError) << "EpicGamesScraperRequest: Unknown exception during API call or processing for " << mCatalogId;
                setError("Unknown API Error");
            }
        } 
        LOG(LogDebug) << "EpicGamesScraperRequest::update() finished iteration for id=" << mCatalogId << " with status " << mStatus;
    } 
}; 


//-----------------------------------------------------------------------------------
// Implementazione dei metodi della classe EpicGamesScraper
//-----------------------------------------------------------------------------------

void EpicGamesScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests, 
    std::vector<ScraperSearchResult>& results)        
{
    std::string gamePathStr; try { gamePathStr = std::string(params.game->getPath()); } catch (...) { gamePathStr = "[[PATH CONVERSION FAILED]]"; }
    LOG(LogDebug) << "EpicGamesScraper::generateRequests for game path: " << gamePathStr;

    std::string ns = params.game->getMetadata(MetaDataId::EpicNamespace);
    std::string catalogId = params.game->getMetadata(MetaDataId::EpicCatalogId);
    std::string epicId = params.game->getMetadata(MetaDataId::EpicId);
    std::string isVirtual = params.game->getMetadata(MetaDataId::Virtual);
    std::string launchCmd = params.game->getMetadata(MetaDataId::LaunchCommand);

    if (ns.empty() || catalogId.empty() || epicId.empty() || isVirtual.empty() || launchCmd.empty()) {
        LOG(LogWarning) << "EpicGamesScraper: Skipping game '" << params.game->getName()
                         << "' - Missing one or more essential metadata tags (epicns, epiccstid, epicid, virtual, launch)."
                         << " Check gamelist or initial fetch/creation process.";
        return; 
    }

    LOG(LogInfo) << "EpicGamesScraper: Found required metadata for game '" << params.game->getName() << "': ns=" << ns << ", cId=" << catalogId << ", eId=" << epicId << ", virtual=" << isVirtual;
    
    auto req = std::unique_ptr<ScraperRequest>(new EpicGamesScraperRequest(ns, catalogId, epicId, isVirtual, launchCmd, results));
    requests.push(std::move(req)); 
    LOG(LogDebug) << "EpicGamesScraper: Queued request for catalogId=" << catalogId;
}

bool EpicGamesScraper::isSupportedPlatform(SystemData* system) {
    return system != nullptr && system->getName() == "epicgamestore";
}

const std::set<Scraper::ScraperMediaSource>& EpicGamesScraper::getSupportedMedias() {
    static std::set<Scraper::ScraperMediaSource> supportedMedia = {
        Scraper::ScraperMediaSource::Screenshot, 
        Scraper::ScraperMediaSource::Box2d,      
        Scraper::ScraperMediaSource::Marquee,    
        Scraper::ScraperMediaSource::FanArt,     // AGGIUNTO
        Scraper::ScraperMediaSource::Video       // AGGIUNTO
    };
    return supportedMedia;
}
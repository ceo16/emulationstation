#include "scrapers/SteamScraper.h"
#include "GameStore/Steam/SteamStoreAPI.h" // Per Steam::AppDetails e la definizione delle struct
#include "FileData.h"
#include "MetaData.h"     // Contiene MetaDataId
#include "SystemData.h"
#include "Log.h"
#include "Settings.h"
#include "utils/TimeUtil.h"
#include "utils/StringUtil.h"     // Per conversioni e manipolazioni stringhe
#include "utils/FileSystemUtil.h" // Per l'estensione dei file
#include "AsyncHandle.h"          // ScraperRequest ne eredita

#include <algorithm> // Per std::all_of, std::transform
#include <iomanip>   // Per std::get_time, std::put_time
#include <sstream>   // Per std::stringstream nella conversione date


// Classe interna per la richiesta dello scraper Steam
class SteamScraperRequest : public ScraperRequest
{
public:
    SteamScraperRequest(
        unsigned int appId,
        const std::string& originalName, // Nome originale dal FileData
        const std::string& virtualStatus, // "true" o "false"
        const std::string& launchCmd,     // Comando di lancio
        std::vector<ScraperSearchResult>& resultsWrite)
        : ScraperRequest(resultsWrite),
          mAppId(appId),
          mOriginalName(originalName),
          mVirtual(virtualStatus),
          mLaunchCmd(launchCmd),
          mRequestLaunched(false)
    {
        LOG(LogDebug) << "SteamScraperRequest created for AppID: " << appId;
    }

    // Converte la data di Steam (es. "14 Nov, 2014" o "Nov 14, 2014") in formato YYYYMMDDTHHMMSS
    std::string convertSteamDateToESFormat(const std::string& steamDateStr) {
        if (steamDateStr.empty() || Utils::String::toLower(steamDateStr) == "coming soon" || Utils::String::toLower(steamDateStr) == "tba") {
            return "";
        }

        std::tm t{};
        std::istringstream ss(steamDateStr);
        std::string format;

        // Prova diversi formati comuni. Steam a volte usa "1 Apr, 2020", altre "Apr 1, 2020"
        const char* formats[] = { "%d %b, %Y", "%b %d, %Y", "%B %d, %Y", "%d %B, %Y" };
        bool parsed = false;
        for (const char* fmt : formats) {
            ss.clear();
            ss.str(steamDateStr);
            ss >> std::get_time(&t, fmt);
            if (!ss.fail()) {
                parsed = true;
                break;
            }
        }

        // Tentativo per date come "Q1 2024", "Spring 2023" (molto approssimativo)
        if (!parsed) {
            // TODO: Potresti voler gestire formati di data più vaghi qui,
            // per ora li ignoriamo se non corrispondono ai formati più precisi.
            // Esempio: Se contiene solo l'anno "2023", imposta Gen 1
            if (steamDateStr.length() == 4 && std::all_of(steamDateStr.begin(), steamDateStr.end(), ::isdigit)) {
                ss.clear();
                ss.str(steamDateStr + "-01-01"); // Aggiungi mese e giorno fittizi
                ss >> std::get_time(&t, "%Y-%m-%d"); // Questo dovrebbe funzionare per solo anno
                if(!ss.fail()) parsed = true;
            }
        }
        
        if (!parsed) {
            LOG(LogWarning) << "SteamScraperRequest: Impossibile parsare la data Steam: '" << steamDateStr << "' con i formati noti.";
            return "";
        }

        std::time_t time = mktime(&t);
        if (time == -1) {
            LOG(LogWarning) << "SteamScraperRequest: mktime fallito per la data Steam: '" << steamDateStr << "'";
            return "";
        }
        return Utils::Time::timeToMetaDataString(time);
    }


    void update() override {
        if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) {
            return;
        }

        if (!mRequestLaunched) {
            mRequestLaunched = true;
            LOG(LogDebug) << "SteamScraperRequest::update() executing API call for AppID=" << mAppId;

            try {
                // L'API di Steam per i dettagli dell'app (storefront API) non richiede autenticazione utente.
                SteamStoreAPI api(nullptr); // Passa nullptr per auth

                std::string country = Settings::getInstance()->getString("ScraperSteamCountry");
if (country.empty()) {
    country = "US"; // Valore predefinito se l'impostazione non esiste o è vuota
}

// std::string language = Settings::getInstance()->getString("Language", "english"); // Riga 102 originale
std::string language_code = Settings::getInstance()->getString("Language"); // "Language" solitamente contiene codici come "en_US", "it_IT"
std::string language; // Lingua per l'API Steam (es. "english", "italian")

if (language_code == "it_IT") language = "italian";
else if (language_code == "en_US") language = "english"; // Assumendo che tu abbia en_US
else if (language_code == "en_GB") language = "english"; // Altra variante inglese
else if (language_code == "es_ES") language = "spanish";
else if (language_code == "fr_FR") language = "french";
else if (language_code == "de_DE") language = "german";
// Aggiungi altre mappature necessarie qui
else {
    language = "english"; // Fallback predefinito se il codice lingua non è mappato
    LOG(LogWarning) << "SteamScraper: Codice lingua '" << language_code << "' non mappato a una lingua API Steam, usando 'english' come fallback.";
}
if (language_code.empty()){ // Se l'impostazione "Language" è completamente vuota
     language = "english"; // Fallback finale
     LOG(LogWarning) << "SteamScraper: Impostazione 'Language' vuota, usando 'english' come fallback per API Steam.";
}


                std::map<unsigned int, Steam::AppDetails> resultsMap = api.GetAppDetails({mAppId}, country, language);

                if (resultsMap.count(mAppId)) {
                    const Steam::AppDetails& details = resultsMap.at(mAppId);
                    LOG(LogInfo) << "SteamScraper: Trovati dati API per '" << details.name << "' (AppID: " << mAppId << ")";

                    ScraperSearchResult searchResult;
                    searchResult.scraper = "steam"; // Identificativo dello scraper

                    // 1. Preserva i metadati essenziali passati
                    searchResult.mdl.set(MetaDataId::SteamAppId, std::to_string(mAppId));
                    searchResult.mdl.set(MetaDataId::Virtual, mVirtual);
                    searchResult.mdl.set(MetaDataId::LaunchCommand, mLaunchCmd);

                    if (!details.name.empty() && details.name != "N/A") {
                        searchResult.mdl.set(MetaDataId::Name, details.name);
                    } else if (!mOriginalName.empty()){
                        searchResult.mdl.set(MetaDataId::Name, mOriginalName);
                    } else {
                         searchResult.mdl.set(MetaDataId::Name, "Steam Game " + std::to_string(mAppId));
                    }

                    // 2. Aggiungi/Sovrascrivi con i dati dall'API
                    std::string desc_text;
                    if (!details.shortDescription.empty()) {
                        desc_text = details.shortDescription;
                    } else if (!details.aboutTheGame.empty()) {
                        desc_text = details.aboutTheGame;
                    } else if (!details.detailedDescription.empty()) {
                         desc_text = details.detailedDescription;
                    }
                    // TODO: Implementare una funzione robusta per rimuovere i tag HTML da desc_text.
                    // Esempio: desc_text = Utils::String::stripHtmlTags(desc_text);
                    // Per ora, la descrizione potrebbe contenere HTML.
                    searchResult.mdl.set(MetaDataId::Desc, desc_text);


                    if (!details.developers.empty()) searchResult.mdl.set(MetaDataId::Developer, Utils::String::vectorToCommaString(details.developers));
                    if (!details.publishers.empty()) searchResult.mdl.set(MetaDataId::Publisher, Utils::String::vectorToCommaString(details.publishers));

                    if (!details.releaseDate.date.empty() && !details.releaseDate.comingSoon) {
                        std::string esDate = convertSteamDateToESFormat(details.releaseDate.date);
                        if (!esDate.empty()) searchResult.mdl.set(MetaDataId::ReleaseDate, esDate);
                    }

                    std::string genresStr;
                    for (const auto& genre : details.genres) {
                        if (!genresStr.empty()) genresStr += "; "; // Usa punto e virgola come separatore standard
                        genresStr += genre.description;
                    }
                    if (!genresStr.empty()) searchResult.mdl.set(MetaDataId::Genre, genresStr);

                    // Mappatura Immagini:
                    // Header Image (di solito 460x215, wide banner) per Thumbnail
                    if (!details.headerImage.empty()) {
                        ScraperSearchItem itemThumb;
                        itemThumb.url = details.headerImage;
                        itemThumb.format = Utils::String::toLower(Utils::FileSystem::getExtension(details.headerImage));
                        if (itemThumb.format.empty() || itemThumb.format == ".") itemThumb.format = ".jpg"; // Default se estensione non trovata
                        searchResult.urls[MetaDataId::Thumbnail] = itemThumb; // CORRETTO: Usa MetaDataId::Thumbnail
                        LOG(LogDebug) << "SteamScraper: Impostata Thumbnail URL: " << itemThumb.url;
                    }

                    // Box Art Verticale (library_600x900.jpg) per Immagine Principale
                    // Questi URL sono costruiti, Steam non li garantisce per ogni gioco.
                    // Sarebbe ideale un controllo HEAD HTTP per verificarne l'esistenza.
                    std::string boxArtUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + std::to_string(mAppId) + "/library_600x900.jpg";
                    ScraperSearchItem itemImage;
                    itemImage.url = boxArtUrl;
                    itemImage.format = ".jpg"; // Estensione nota
                    searchResult.urls[MetaDataId::Image] = itemImage; // CORRETTO: Usa MetaDataId::Image
                    LOG(LogDebug) << "SteamScraper: Impostata Image URL (BoxArt): " << itemImage.url;

                    // Logo (logo.png) per Marquee
                    std::string logoUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + std::to_string(mAppId) + "/logo.png";
                    ScraperSearchItem itemMarquee;
                    itemMarquee.url = logoUrl;
                    itemMarquee.format = ".png"; // Estensione nota
                    searchResult.urls[MetaDataId::Marquee] = itemMarquee; // CORRETTO: Usa MetaDataId::Marquee
                    LOG(LogDebug) << "SteamScraper: Impostata Marquee URL (Logo): " << itemMarquee.url;

                    // Gestione Screenshot (opzionale, se si vuole usare il primo come fallback per l'immagine principale)
                    if (details.screenshots.empty() == false && searchResult.urls.find(MetaDataId::Image) == searchResult.urls.end()) {
                        // Se non abbiamo trovato una BoxArt, usiamo il primo screenshot come immagine principale
                         for(const auto& ss : details.screenshots) { // details.screenshots è std::vector<Steam::Screenshot>
                            if (!ss.pathFull.empty()) { // pathFull è l'URL completo dello screenshot
                                ScraperSearchItem itemSSAsImage;
                                itemSSAsImage.url = ss.pathFull;
                                itemSSAsImage.format = Utils::String::toLower(Utils::FileSystem::getExtension(ss.pathFull));
                                if (itemSSAsImage.format.empty() || itemSSAsImage.format == ".") itemSSAsImage.format = ".jpg";
                                searchResult.urls[MetaDataId::Image] = itemSSAsImage;
                                LOG(LogDebug) << "SteamScraper: Impostata Image URL (Screenshot Fallback): " << itemSSAsImage.url;
                                break; 
                            }
                         }
                    }

                    mResults.push_back(searchResult);
                    setStatus(ASYNC_DONE);
                    LOG(LogInfo) << "SteamScraperRequest: Risultato processato con successo per AppID " << mAppId;

                } else {
                    LOG(LogWarning) << "SteamScraperRequest: Nessun dato trovato nella risposta API per AppID=" << mAppId;
                    setStatus(ASYNC_DONE); 
                }

            } catch (const std::exception& e) {
                LOG(LogError) << "SteamScraperRequest: Eccezione durante chiamata API o processamento per AppID=" << mAppId << ": " << e.what();
                setError(std::string("API Error: ") + e.what());
            } catch (...) {
                LOG(LogError) << "SteamScraperRequest: Eccezione sconosciuta durante API call o processamento per AppID=" << mAppId;
                setError("Unknown API Error");
            }
        } 
    } 

private:
    unsigned int mAppId;
    std::string mOriginalName;
    std::string mVirtual;
    std::string mLaunchCmd;
    bool mRequestLaunched;
};


// Implementazione dei metodi di SteamScraper (il resto della classe)
void SteamScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results)
{
    if (!params.game) {
        LOG(LogError) << "SteamScraper: params.game è nullo.";
        return;
    }

    std::string gamePathStr = params.game->getPath(); 
    LOG(LogDebug) << "SteamScraper::generateRequests for game: " << params.game->getName() << " (" << gamePathStr << ")";

    std::string steamAppIdStr = params.game->getMetadata(MetaDataId::SteamAppId); // Assicurati che SteamAppId sia nell'enum MetaDataId
    std::string virtualStatus = params.game->getMetadata(MetaDataId::Virtual); 
    std::string launchCmd = params.game->getMetadata(MetaDataId::LaunchCommand); 
    std::string originalName = params.game->getMetadata(MetaDataId::Name);


    if (steamAppIdStr.empty()) {
        LOG(LogWarning) << "SteamScraper: SteamAppId mancante per il gioco '" << params.game->getName() << "'. Impossibile fare scraping.";
        return;
    }
     if (virtualStatus.empty() || launchCmd.empty()) {
        LOG(LogWarning) << "SteamScraper: Metadati Virtual o LaunchCommand mancanti per '" << params.game->getName() << "'.";
    }


    unsigned int appId = 0;
    try {
        appId = static_cast<unsigned int>(std::stoul(steamAppIdStr));
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamScraper: SteamAppId non valido '" << steamAppIdStr << "' per il gioco '" << params.game->getName() << "'. Errore: " << e.what();
        return;
    }

    if (appId == 0) {
         LOG(LogWarning) << "SteamScraper: AppID Steam è zero per il gioco '" << params.game->getName() << "'. Salto scraping.";
        return;
    }

    LOG(LogInfo) << "SteamScraper: Trovato SteamAppId=" << appId << " per il gioco '" << originalName << "'. Accodamento richiesta scraper.";

    // Crea la richiesta specifica per lo scraper Steam
    auto req = std::make_unique<SteamScraperRequest>(appId, originalName, virtualStatus, launchCmd, results);
    requests.push(std::move(req)); // Aggiungi alla coda delle richieste
}

bool SteamScraper::isSupportedPlatform(SystemData* system) {
    // Abilita lo scraper solo per il sistema "steam" (o come lo hai chiamato in es_systems.cfg)
    return system != nullptr && system->getName() == "steam"; // Assicurati che "steam" sia il nome corretto
}

// Indica quali tipi di media questo scraper è in grado di fornire.
// Questi sono usati dalla UI dello scraper per mostrare le opzioni.
// Le URL effettive verranno mappate a MetaDataId::Image, MetaDataId::Thumbnail, ecc.
const std::set<Scraper::ScraperMediaSource>& SteamScraper::getSupportedMedias() {
    static const std::set<Scraper::ScraperMediaSource> supportedMedia = {
        Scraper::ScraperMediaSource::Screenshot,   // Corrisponde a MetaDataId::Screenshot (e potenzialmente MetaDataId::Image se il tema lo usa)
        Scraper::ScraperMediaSource::Box2d,        // Corrisponde a MetaDataId::Box2d
        Scraper::ScraperMediaSource::Marquee,      // Corrisponde a MetaDataId::Marquee
        Scraper::ScraperMediaSource::FanArt,                                         // Thumbnail in ES è spesso derivato da Image se non specificato.
                                                 // L'importante è che i tipi qui corrispondano a ciò che
                                                 // il downloader si aspetta quando gli chiedi di scaricare
                                                 // ScraperMediaSource::Screenshot, ecc.
                                                 // Il mapping esatto tra ScraperMediaSource e MetaDataId
                                                 // avviene quando il ScraperRequest popola ScraperSearchResult.urls
                                                 // e poi quando GuiGameScraper (o simili) applica questi URL
                                                 // ai FileData.

        // Aggiungiamo anche un tipo generico per l'immagine principale/thumbnail se non è uno screenshot
       // ScraperMediaSource::FanArt potrebbe essere usato per l'header_image se non è prettamente uno screenshot.
        // Oppure, se "Image" è inteso come una copertina generica, potremmo usare ScraperMediaSource::Cover.
        // Per ora, manteniamo quelli che abbiamo mappato chiaramente:
        // Screenshot -> MetaDataId::Screenshot
        // Box2d -> MetaDataId::Box2d
        // Marquee -> MetaDataId::Marquee
        //
        // Se l'header_image viene usato come Thumbnail:
        // Non c'è un ScraperMediaSource::Thumbnail diretto. Il sistema ES di solito
        // considera l'immagine principale (spesso "Image" o "Screenshot") e ne crea una thumbnail
        // o usa un MetaDataId::Thumbnail separato se fornito.
        // Per ora, se forniamo Screenshot, Box2d, Marquee, questo dovrebbe essere sufficiente
        // per popolare i campi corrispondenti. Il tema poi decide cosa mostrare.
    };
    return supportedMedia;
}
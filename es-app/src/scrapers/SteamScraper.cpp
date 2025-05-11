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

    std::string convertSteamDateToESFormat(const std::string& steamDateStr) {
        if (steamDateStr.empty() || Utils::String::toLower(steamDateStr) == "coming soon" || Utils::String::toLower(steamDateStr) == "tba") {
            return "";
        }
        std::tm t{};
        std::istringstream ss(steamDateStr);
        const char* formats[] = { "%d %b, %Y", "%b %d, %Y", "%B %d, %Y", "%d %B, %Y" };
        bool parsed = false;
        for (const char* fmt : formats) {
            ss.clear(); ss.str(steamDateStr);
            ss >> std::get_time(&t, fmt);
            if (!ss.fail()) { parsed = true; break; }
        }
        if (!parsed) {
            if (steamDateStr.length() == 4 && std::all_of(steamDateStr.begin(), steamDateStr.end(), ::isdigit)) {
                ss.clear(); ss.str(steamDateStr + "-01-01");
                ss >> std::get_time(&t, "%Y-%m-%d");
                if(!ss.fail()) parsed = true;
            }
        }
        if (!parsed) {
            LOG(LogWarning) << "SteamScraperRequest: Impossibile parsare la data Steam: '" << steamDateStr << "'";
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
                SteamStoreAPI api(nullptr); 
                std::string country = Settings::getInstance()->getString("ScraperSteamCountry");
                if (country.empty()) country = "US";
                
                std::string language_code = Settings::getInstance()->getString("Language");
                std::string language = "english"; // Default
                if (language_code == "it_IT") language = "italian";
                else if (language_code == "en_US" || language_code == "en_GB") language = "english";
                else if (language_code == "es_ES") language = "spanish";
                else if (language_code == "fr_FR") language = "french";
                else if (language_code == "de_DE") language = "german";
                else if (!language_code.empty()) {
                    LOG(LogWarning) << "SteamScraper: Codice lingua '" << language_code << "' non mappato, usando 'english'.";
                }
                if (language_code.empty()){
                     LOG(LogWarning) << "SteamScraper: Impostazione 'Language' vuota, usando 'english'.";
                }

                std::map<unsigned int, Steam::AppDetails> resultsMap = api.GetAppDetails({mAppId}, country, language);

                if (resultsMap.count(mAppId)) {
                    const Steam::AppDetails& details = resultsMap.at(mAppId);
                    LOG(LogInfo) << "SteamScraper: Trovati dati API per '" << details.name << "' (AppID: " << mAppId << ")";

                    ScraperSearchResult searchResult;
                    searchResult.scraper = "steam";

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

                    std::string desc_text;
                    if (!details.shortDescription.empty()) desc_text = details.shortDescription;
                    else if (!details.aboutTheGame.empty()) desc_text = details.aboutTheGame;
                    else if (!details.detailedDescription.empty()) desc_text = details.detailedDescription;
                    searchResult.mdl.set(MetaDataId::Desc, desc_text); // TODO: strip HTML

                    if (!details.developers.empty()) searchResult.mdl.set(MetaDataId::Developer, Utils::String::vectorToCommaString(details.developers));
                    if (!details.publishers.empty()) searchResult.mdl.set(MetaDataId::Publisher, Utils::String::vectorToCommaString(details.publishers));

                    if (!details.releaseDate.date.empty() && !details.releaseDate.comingSoon) {
                        std::string esDate = convertSteamDateToESFormat(details.releaseDate.date);
                        if (!esDate.empty()) searchResult.mdl.set(MetaDataId::ReleaseDate, esDate);
                    }

                    std::string genresStr;
                    for (const auto& genre : details.genres) {
                        if (!genresStr.empty()) genresStr += "; "; 
                        genresStr += genre.description;
                    }
                    if (!genresStr.empty()) searchResult.mdl.set(MetaDataId::Genre, genresStr);

                    // --- INIZIO MAPPATURA MEDIA AVANZATA ---
                    std::string appIdStr = std::to_string(mAppId);

                    // VIDEO (MetaDataId::Video)
                    if (!details.movies.empty()) {
                        std::string videoUrlToUse;
                        const auto& firstMovie = details.movies[0]; 
                        if (!firstMovie.mp4_max_url.empty()) videoUrlToUse = firstMovie.mp4_max_url;
                        else if (!firstMovie.mp4_480_url.empty()) videoUrlToUse = firstMovie.mp4_480_url;
                        else if (!firstMovie.webm_max_url.empty()) videoUrlToUse = firstMovie.webm_max_url;
                        else if (!firstMovie.webm_480_url.empty()) videoUrlToUse = firstMovie.webm_480_url;

                        if (!videoUrlToUse.empty()) {
                            ScraperSearchItem itemVideo;
                            itemVideo.url = videoUrlToUse;
                            if (videoUrlToUse.find(".mp4") != std::string::npos) itemVideo.format = ".mp4";
                            else if (videoUrlToUse.find(".webm") != std::string::npos) itemVideo.format = ".webm";
                            else itemVideo.format = ".mp4"; 
                            searchResult.urls[MetaDataId::Video] = itemVideo;
                            LOG(LogDebug) << "SteamScraper: Impostata Video URL: " << itemVideo.url;
                        }
                    }

                    // FANART (MetaDataId::FanArt)
                    std::string fanartUrlString;
                    std::string fanartFormat = ".jpg"; 
                    if (!details.library_assets.hero.empty()) {
                        fanartUrlString = details.library_assets.hero;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fanartUrlString)); // Pulisci URL per getExtension
                        if (!ext.empty() && ext != ".") fanartFormat = ext;
                        LOG(LogDebug) << "SteamScraper: Trovato FanArt URL (library_hero): " << fanartUrlString;
                    } else if (!details.background_raw_url.empty()) {
                        fanartUrlString = details.background_raw_url;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fanartUrlString));
                        if (!ext.empty() && ext != ".") fanartFormat = ext; else fanartFormat = ".jpg";
                        LOG(LogDebug) << "SteamScraper: Trovato FanArt URL (background_raw_url fallback): " << fanartUrlString;
                    } else if (!details.headerImage.empty()) { 
                        fanartUrlString = details.headerImage;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fanartUrlString));
                        if (!ext.empty() && ext != ".") fanartFormat = ext; else fanartFormat = ".jpg";
                        LOG(LogDebug) << "SteamScraper: Trovato FanArt URL (header_image fallback): " << fanartUrlString;
                    }
                    if (!fanartUrlString.empty()) {
                        ScraperSearchItem itemFanArt;
                        itemFanArt.url = fanartUrlString;
                        itemFanArt.format = fanartFormat;
                        searchResult.urls[MetaDataId::FanArt] = itemFanArt;
                        LOG(LogDebug) << "SteamScraper: Impostata FanArt URL: " << itemFanArt.url;
                    }

                    // IMMAGINE/BOXART (MetaDataId::Image)
                    std::string imageUrlString;
                    std::string imageFormat = ".jpg";
                    bool imageSet = false;

                    // Priorità 1: library_600x900.jpg (costruito, ma non verificato se esiste)
                    // Potresti voler fare una richiesta HEAD qui per verificarlo o dare priorità a library_capsule.
                    // Per ora, lo aggiungiamo come tentativo, e se fallisce, gli altri fallback possono subentrare.
                    std::string potentialBoxArtUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/library_600x900.jpg";
                    imageUrlString = potentialBoxArtUrl; // Tentativo iniziale
                    LOG(LogDebug) << "SteamScraper: Tentativo Image URL (library_600x900): " << imageUrlString;

                    // Priorità 2: library_capsule
                    if (!details.library_assets.capsule.empty()) {
                        imageUrlString = details.library_assets.capsule;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(imageUrlString));
                        if (!ext.empty() && ext != ".") imageFormat = ext; else imageFormat = ".jpg";
                        LOG(LogDebug) << "SteamScraper: Sovrascritto/Impostato Image URL (library_capsule): " << imageUrlString;
                        imageSet = true;
                    }
                    
                    // Se library_capsule non ha impostato l'immagine, e stiamo ancora usando potentialBoxArtUrl,
                    // allora il formato è .jpg per library_600x900.
                    if (!imageSet && imageUrlString == potentialBoxArtUrl) {
                         imageFormat = ".jpg"; // Per library_600x900
                         imageSet = true; // Consideriamo questo come un'impostazione (anche se da verificare)
                    }


                    // Fallback a header_image (se gli altri mancano o se library_600x900 non dovesse esistere)
                    // Questo fallback ha senso se le opzioni sopra non hanno prodotto un'immagine o se falliscono il download.
                    // Lo scraper generale potrebbe già fare un fallback da solo se il download fallisce.
                    // Per ora, ci affidiamo alla priorità data. Se library_600x900 non esiste, il download fallirà e 
                    // il sistema potrebbe non avere un'immagine. Il fallback a screenshot è già presente dopo.
                                        
                    if (imageSet && !imageUrlString.empty()) { // imageUrlString deve essere stato impostato da library_600x900 o library_capsule
                        ScraperSearchItem itemImage;
                        itemImage.url = imageUrlString;
                        itemImage.format = imageFormat;
                        searchResult.urls[MetaDataId::Image] = itemImage;
                        LOG(LogDebug) << "SteamScraper: Impostata Image URL (Primaria): " << itemImage.url;
                    }


                    // MARQUEE/LOGO (MetaDataId::Marquee)
                    std::string marqueeUrlString;
                    std::string marqueeFormat = ".png"; 
                    if (!details.library_assets.logo.empty()) {
                        marqueeUrlString = details.library_assets.logo;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(marqueeUrlString));
                        if (!ext.empty() && ext != ".") marqueeFormat = ext;
                        LOG(LogDebug) << "SteamScraper: Trovato Marquee URL (library_logo): " << marqueeUrlString;
                    } else { 
                        marqueeUrlString = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/logo.png";
                        LOG(LogDebug) << "SteamScraper: Tentativo Marquee URL (logo.png costruito): " << marqueeUrlString;
                    }
                    if (!marqueeUrlString.empty()) {
                        ScraperSearchItem itemMarquee;
                        itemMarquee.url = marqueeUrlString;
                        itemMarquee.format = marqueeFormat;
                        searchResult.urls[MetaDataId::Marquee] = itemMarquee;
                        LOG(LogDebug) << "SteamScraper: Impostata Marquee URL: " << itemMarquee.url;
                    }

                    // THUMBNAIL (MetaDataId::Thumbnail)
                    std::string thumbUrlString;
                    std::string thumbFormat = ".jpg";
                    if (!details.library_assets.header.empty()) {
                        thumbUrlString = details.library_assets.header;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(thumbUrlString));
                        if (!ext.empty() && ext != ".") thumbFormat = ext;
                        LOG(LogDebug) << "SteamScraper: Trovato Thumbnail URL (library_header): " << thumbUrlString;
                    } else if (!details.headerImage.empty()) { 
                        thumbUrlString = details.headerImage;
                        std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(thumbUrlString));
                        if (!ext.empty() && ext != ".") thumbFormat = ext; else thumbFormat = ".jpg";
                        LOG(LogDebug) << "SteamScraper: Trovato Thumbnail URL (header_image fallback): " << thumbUrlString;
                    }
                    if (!thumbUrlString.empty()) {
                        ScraperSearchItem itemThumb;
                        itemThumb.url = thumbUrlString;
                        itemThumb.format = thumbFormat;
                        searchResult.urls[MetaDataId::Thumbnail] = itemThumb;
                        LOG(LogDebug) << "SteamScraper: Impostata Thumbnail URL: " << itemThumb.url;
                    }
                    
                    // GESTIONE SCREENSHOT
                    // Usiamo il primo screenshot per TitleShot
                    if (!details.screenshots.empty() && !details.screenshots[0].pathFull.empty()) {
                        ScraperSearchItem itemTitleShot;
                        itemTitleShot.url = details.screenshots[0].pathFull;
                        itemTitleShot.format = Utils::String::toLower(Utils::FileSystem::getExtension(itemTitleShot.url));
                        if (itemTitleShot.format.empty() || itemTitleShot.format == ".") itemTitleShot.format = ".jpg";
                        searchResult.urls[MetaDataId::TitleShot] = itemTitleShot; // Assicurati che MetaDataId::TitleShot esista
                        LOG(LogDebug) << "SteamScraper: Impostata TitleShot URL (primo screenshot): " << itemTitleShot.url;
                    }
                    // Fallback a screenshot per Image se Image è ancora vuoto (dopo library_600x900 e library_capsule)
                    if (details.screenshots.empty() == false && searchResult.urls.find(MetaDataId::Image) == searchResult.urls.end()) {
                        for(const auto& ss : details.screenshots) { 
                            if (!ss.pathFull.empty()) { 
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
                    // --- FINE MAPPATURA MEDIA AVANZATA ---

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

    std::string steamAppIdStr = params.game->getMetadata(MetaDataId::SteamAppId);
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
    
    auto req = std::make_unique<SteamScraperRequest>(appId, originalName, virtualStatus, launchCmd, results);
    requests.push(std::move(req)); 
}

bool SteamScraper::isSupportedPlatform(SystemData* system) {
    return system != nullptr && system->getName() == "steam"; 
}

const std::set<Scraper::ScraperMediaSource>& SteamScraper::getSupportedMedias() {
    static const std::set<Scraper::ScraperMediaSource> supportedMedia = {
        Scraper::ScraperMediaSource::Screenshot, // Può essere usato per MetaDataId::TitleShot
        Scraper::ScraperMediaSource::Box2d,        // Questo è usato per MetaDataId::Image (boxart principale)
        Scraper::ScraperMediaSource::Marquee,      // Per MetaDataId::Marquee (logo)
        Scraper::ScraperMediaSource::FanArt,       // Per MetaDataId::FanArt (sfondo)
        Scraper::ScraperMediaSource::Video         // AGGIUNTO: Per MetaDataId::Video
    };
    return supportedMedia;
}
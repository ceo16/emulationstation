// emulationstation-master/es-app/src/scrapers/IGDBScraper.cpp
#include "scrapers/IGDBScraper.h"
#include "Log.h"
#include "MetaData.h" 
#include "SystemData.h"
#include "FileData.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "LocaleES.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/FileSystemUtil.h" // Necessario per getExtension

#include <thread>
#include <chrono>

// Credenziali Twitch per IGDB (DA SOSTITUIRE CON I TUOI VALORI REALI)
#define IGDB_CLIENT_ID "spnxgx5wpq50lx5bulmfy1qyk5ahuu" //
#define IGDB_CLIENT_SECRET "rvseeb3t24ueceznht5dlmneg0x0th" //

// --- Implementazione di IGDBScraperRequest ---
IGDBScraperRequest::IGDBScraperRequest(
    std::vector<ScraperSearchResult>& resultsWrite,
    const ScraperSearchParams& params,
    IGDB::IGDBAPI* igdbApi,
    const std::string& scraperName)
    : ScraperRequest(resultsWrite),
      mSearchParams(params),
      mIgdbApi(igdbApi),
     // mGameName(params.game->getName()), //
      mSearchLaunched(false),
      mScraperName(scraperName), //
      mState(RequestState::SEARCHING_GAME), //
      mCurrentSearchResultIndex(0)
{
 std::string originalName = params.game->getName();
    std::string sanitizedName = originalName;

    // --- Inizio Blocco di Sanitizzazione Aggressiva ---

    // 1. Sostituisci i separatori comuni con spazi
    sanitizedName = Utils::String::replace(sanitizedName, ":", " ");
    sanitizedName = Utils::String::replace(sanitizedName, "-", " ");
    sanitizedName = Utils::String::replace(sanitizedName, "_", " ");

    // 2. Rimuovi i simboli speciali sostituendoli con uno spazio
    //    (per evitare che parole come "Speed™Hot" diventino "SpeedHot")
    std::string tm_char_utf8 = "\xE2\x84\xA2"; // ™
    sanitizedName = Utils::String::replace(sanitizedName, tm_char_utf8, " ");

    std::string reg_char_utf8 = "\xC2\xAE"; // ®
    sanitizedName = Utils::String::replace(sanitizedName, reg_char_utf8, " ");

    std::string copy_char_utf8 = "\xC2\xA9"; // ©
    sanitizedName = Utils::String::replace(sanitizedName, copy_char_utf8, " ");

    // 3. Rimuovi eventuali parentesi o altri caratteri non desiderati
    sanitizedName = Utils::String::replace(sanitizedName, "(", " ");
    sanitizedName = Utils::String::replace(sanitizedName, ")", " ");
    sanitizedName = Utils::String::replace(sanitizedName, "[", " ");
    sanitizedName = Utils::String::replace(sanitizedName, "]", " ");

    // 4. Comprimi gli spazi multipli in uno singolo
    while (sanitizedName.find("  ") != std::string::npos) {
        sanitizedName = Utils::String::replace(sanitizedName, "  ", " ");
    }

    // 5. Rimuovi spazi all'inizio e alla fine
    sanitizedName = Utils::String::trim(sanitizedName);

    // 6. Infine, fai l'escape delle virgolette doppie per la sintassi della query
    mGameName = Utils::String::replace(sanitizedName, "\"", "\\\"");

    // --- Fine Blocco di Sanitizzazione ---

    LOG(LogInfo) << "IGDBScraperRequest: Original Game Name: '" << originalName 
                 << "', Aggressively Sanitized Search Name: '" << mGameName << "'";
}

IGDBScraperRequest::~IGDBScraperRequest() { //
    // Eventuali pulizie della richiesta
}

void IGDBScraperRequest::update()
{
    if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) { //
        return; 
    }

    if (mState == RequestState::SEARCHING_GAME) { //
        if (!mSearchLaunched) { 
            mSearchLaunched = true; 
            LOG(LogInfo) << "IGDBScraperRequest: Searching for game: " << mGameName; //

            mIgdbApi->searchGames(mGameName, //
                [this](std::vector<IGDB::GameMetadata> results, bool success) {
                    if (success && !results.empty()) {
                        mSearchResults = results; //
                        mCurrentSearchResultIndex = 0; //
                        LOG(LogInfo) << "IGDBScraperRequest: Found " << results.size() << " results for " << mGameName << ". Pronto per recuperare i dettagli."; //
                        mState = RequestState::FETCHING_DETAILS; //
                        mSearchLaunched = false; 
                    } else {
                        LOG(LogWarning) << "IGDBScraperRequest: No results found for " << mGameName << " or search failed."; //
                        setError(_("Nessun risultato trovato o ricerca fallita.")); //
                        setStatus(ASYNC_ERROR); //
                        mSearchLaunched = false; 
                    }
                },
                Settings::getInstance()->getString("Language")); //
        }
    } else if (mState == RequestState::FETCHING_DETAILS) { //
        if (!mSearchLaunched) { 
            if (mCurrentSearchResultIndex < mSearchResults.size()) { //
                mSearchLaunched = true; 
                std::string igdbGameId = mSearchResults[mCurrentSearchResultIndex].id; //

                if (igdbGameId.empty()) { //
                    LOG(LogError) << "IGDBScraperRequest: L'ID IGDB è vuoto per l'indice del risultato " << mCurrentSearchResultIndex << ". Salto questo risultato."; //
                    mSearchLaunched = false; 
                    mCurrentSearchResultIndex++; //
                    return; 
                }

                LOG(LogInfo) << "IGDBScraperRequest: Recupero dettagli per IGDB ID: " << igdbGameId << " (Risultato " << (mCurrentSearchResultIndex + 1) << "/" << mSearchResults.size() << ")"; //

                mIgdbApi->getGameDetails(igdbGameId, //
                    [this, igdbGameId](IGDB::GameMetadata gameDetails, bool success) {
                        bool processedThisDetailSuccessfully = false;
                        if (success && !gameDetails.name.empty()) { //
                            ScraperSearchResult sr(mScraperName); //

                            sr.mdl.set(MetaDataId::Name, gameDetails.name); //
                            sr.mdl.set(MetaDataId::Desc, gameDetails.summary); //

                            if (!gameDetails.storyline.empty() && gameDetails.storyline != gameDetails.summary) { //
                                if (gameDetails.summary.empty()) { //
                                    sr.mdl.set(MetaDataId::Desc, gameDetails.storyline); //
                                } else {
                                    sr.mdl.set(MetaDataId::Desc, gameDetails.summary + "\n\nStoryline: " + gameDetails.storyline); //
                                }
                            }

                            sr.mdl.set(MetaDataId::Developer, Utils::String::vectorToCommaString(gameDetails.developers)); //
                            sr.mdl.set(MetaDataId::Publisher, Utils::String::vectorToCommaString(gameDetails.publishers)); //
                            sr.mdl.set(MetaDataId::Genre, Utils::String::vectorToCommaString(gameDetails.genres)); //

                            if (!gameDetails.gameModes.empty()) {
                                std::string players_string;
                                bool has_single = false;
                                bool has_multi = false;

                                for (const auto& mode : gameDetails.gameModes) {
                                    // IGDB usa l'inglese per i nomi delle modalità
                                    if (mode == "Single player") {
                                        has_single = true;
                                    }
                                    if (mode == "Multiplayer" || mode == "Co-operative" || mode == "Split screen") {
                                        has_multi = true;
                                    }
                                }

                                if (has_single && has_multi) {
                                    players_string = "1-2+"; // Gioco sia single che multi
                                } else if (has_single) {
                                    players_string = "1";
                                } else if (has_multi) {
                                    players_string = "2+"; // Solo multi/coop
                                }
                                
                                if (!players_string.empty()) {
                                    sr.mdl.set(MetaDataId::Players, players_string);
                                    LOG(LogDebug) << "IGDBScraperRequest: Aggiunto Players: " << players_string;
                                }
                            }

                            if (!gameDetails.releaseDate.empty()) { //
                                time_t release_t = (time_t)std::stoll(gameDetails.releaseDate); //
                                if (release_t != Utils::Time::NOT_A_DATE_TIME) { //
									sr.mdl.set(MetaDataId::ScraperId, gameDetails.id); //
                                    sr.mdl.set(MetaDataId::ReleaseDate, Utils::Time::timeToMetaDataString(release_t)); //
                                }
                            }

                            // Cover
                            if (!gameDetails.coverImageId.empty()) {
                                ScraperSearchItem itemCover;
                               itemCover.url = "https://images.igdb.com/igdb/image/upload/t_original/" + gameDetails.coverImageId + ".jpg";
                                itemCover.format = ".jpg"; 
                                sr.urls[MetaDataId::Image] = itemCover;
                                LOG(LogDebug) << "IGDBScraperRequest: Aggiunto Cover URL: " << itemCover.url;
                            }

                            // Screenshot (usato per TitleShot o MD_SCREENSHOT_URL)
                            if (!gameDetails.screenshotImageId.empty()) {
                                ScraperSearchItem itemScreenshot;
                                itemScreenshot.url = "https://images.igdb.com/igdb/image/upload/t_screenshot_huge/" + gameDetails.screenshotImageId + ".jpg";
                                itemScreenshot.format = ".jpg";
                                sr.urls[MetaDataId::TitleShot] = itemScreenshot; // O usa MetaDataId::MD_SCREENSHOT_URL se preferisci
                                LOG(LogDebug) << "IGDBScraperRequest: Aggiunto Screenshot/TitleShot URL: " << itemScreenshot.url;
                            }
                            
                            // Fanart
                            if (!gameDetails.fanartImageId.empty()) {
                                ScraperSearchItem itemFanArt;
                                itemFanArt.url = "https://images.igdb.com/igdb/image/upload/t_1080p/" + gameDetails.fanartImageId + ".jpg"; // o t_original, t_screenshot_huge
                                itemFanArt.format = ".jpg";
                                sr.urls[MetaDataId::FanArt] = itemFanArt;
                                LOG(LogDebug) << "IGDBScraperRequest: Aggiunto FanArt URL: " << itemFanArt.url;
                            }

                            // Video (salva solo il link, non tenta il download)
                            if (!gameDetails.videoUrl.empty()) { 
                                sr.mdl.set(MetaDataId::Video, gameDetails.videoUrl); 
                                LOG(LogDebug) << "IGDBScraperRequest: Salvato Video URL (link YouTube) [" << gameDetails.videoUrl << "] come metadato testuale (MetaDataId::Video).";
                            }

                            if (!gameDetails.aggregatedRating.empty()) { //
                                try {
                                    float rating = std::stof(gameDetails.aggregatedRating); //
                                    sr.mdl.set(MetaDataId::Rating, std::to_string(rating / 100.0f)); //
                                } catch (const std::exception& e) {
                                    LOG(LogWarning) << "IGDBScraperRequest: Fallimento nel parsing del voto aggregato: " << e.what(); //
                                }
                            }
                            sr.mdl.set(MetaDataId::StoreProvider, mScraperName); //
                            mResults.push_back(sr); //
                            LOG(LogInfo) << "IGDBScraperRequest: Dettagli elaborati con successo per " << gameDetails.name; //
                            processedThisDetailSuccessfully = true;
                        }
                        
                        mSearchLaunched = false; 

                        if (processedThisDetailSuccessfully) {
                            setStatus(ASYNC_DONE);
                        } else {
                            LOG(LogWarning) << "IGDBScraperRequest: Recupero dettagli fallito per IGDB ID " << igdbGameId << ". Tentativo prossimo risultato se disponibile."; //
                            mCurrentSearchResultIndex++; //
                            if (mCurrentSearchResultIndex >= mSearchResults.size()) {
                                if (mResults.empty()) { 
                                    setError(_("Nessun dettaglio trovato o fallito per tutti i risultati.")); //
                                    setStatus(ASYNC_ERROR); //
                                } else {
                                    setStatus(ASYNC_DONE);
                                }
                            }
                        }
                    },
                    Settings::getInstance()->getString("Language")); //
            } else { 
                mSearchLaunched = false; 
                LOG(LogWarning) << "IGDBScraperRequest: Nessun altro risultato di ricerca da elaborare per i dettagli."; //
                if (mResults.empty()) { 
                    setError(_("Nessun risultato di ricerca da elaborare per i dettagli.")); //
                    setStatus(ASYNC_ERROR); //
                } else {
                    setStatus(ASYNC_DONE);
                }
            }
        }
    }
}

// --- Implementazione di IGDBScraper ---
IGDBScraper::IGDBScraper() : mClientId(IGDB_CLIENT_ID), mAccessToken("") { //
    mSupportedMedia.insert(ScraperMediaSource::Box2d); // Immagine principale/cover //
    mSupportedMedia.insert(ScraperMediaSource::FanArt); //
    mSupportedMedia.insert(ScraperMediaSource::Screenshot); // Per TitleShot o screenshot generici
    mSupportedMedia.insert(ScraperMediaSource::Video);      // Per il link video
}

IGDBScraper::~IGDBScraper() { //
}

bool IGDBScraper::ensureAccessToken() { //
    if (!mAccessToken.empty()) { //
        LOG(LogDebug) << "IGDBScraper: Access Token già presente. Saltato l'ottenimento di un nuovo token."; //
        return true;
    }

    LOG(LogInfo) << "IGDBScraper: Access Token non presente o scaduto. Tentativo di ottenerne uno nuovo..."; //

    HttpReqOptions options; //
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded"); //
   // options.httpMethod = "POST"; //
    options.dataToPost = "client_id=" + mClientId + "&client_secret=" + IGDB_CLIENT_SECRET + "&grant_type=client_credentials"; //

    const std::string twitchAuthUrl = "https://id.twitch.tv/oauth2/token"; //
    LOG(LogDebug) << "IGDBAPI: Full POST Data to be sent for a new game: " << options.dataToPost;
    HttpReq request(twitchAuthUrl, &options); //

    while (request.status() == HttpReq::REQ_IN_PROGRESS) { //
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); //
    }

    if (request.status() == HttpReq::REQ_SUCCESS) { //
        try {
            auto jsonResponse = nlohmann::json::parse(request.getContent()); //
            mAccessToken = jsonResponse.value("access_token", ""); //

            if (!mAccessToken.empty()) { //
                LOG(LogInfo) << "IGDBScraper: Access Token ottenuto con successo."; //
                return true;
            } else {
                LOG(LogError) << "IGDBScraper: Fallimento ottenimento Access Token: token non trovato nella risposta JSON."; //
                LOG(LogError) << "Risposta JSON ricevuta: " << request.getContent(); //
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "IGDBScraper: Fallimento parsing risposta JSON Access Token: " << e.what(); //
            LOG(LogError) << "Corpo risposta: " << request.getContent().substr(0, 500); //
        }
    } else {
        LOG(LogError) << "IGDBScraper: Fallimento richiesta Access Token HTTP: " << request.getErrorMsg(); //
        LOG(LogError) << "Stato HTTP: " << static_cast<int>(request.status()); //
        LOG(LogError) << "URL della richiesta: " << twitchAuthUrl; //
        LOG(LogError) << "Corpo POST inviato: " << options.dataToPost; //
    }

    return false; //
}

bool IGDBScraper::isSupportedPlatform(SystemData* system) { 
    return true; 
}

const std::set<Scraper::ScraperMediaSource>& IGDBScraper::getSupportedMedias() { //
    return mSupportedMedia; //
}

void IGDBScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results)
{
    if (!params.game) { //
        LOG(LogError) << "IGDBScraper::generateRequests - params.game is null."; //
        return;
    }

    if (!ensureAccessToken()) { //
        LOG(LogError) << "IGDBScraper: Cannot generate requests, Access Token not available or failed to refresh."; //
        return;
    }

    if (!mIgdbApi) { //
        mIgdbApi = std::make_unique<IGDB::IGDBAPI>(mClientId, mAccessToken); //
    }

    LOG(LogInfo) << "IGDBScraper: Generating request for game: " << params.game->getName(); //
    requests.push(std::make_unique<IGDBScraperRequest>(results, params, mIgdbApi.get(), "IGDB")); //
}
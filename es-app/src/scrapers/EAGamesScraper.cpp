// emulationstation-master/es-app/src/scrapers/EAGamesScraper.cpp
#include "scrapers/EAGamesScraper.h"
#include "Log.h"
#include "MetaData.h"
#include "SystemData.h"
#include "FileData.h"     // Per FileData e FileType::GAME
#include "Settings.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/EAGames/EAGamesStore.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "LocaleES.h" // <<< AGGIUNTO PER _()

// --- Implementazione di EAGamesScraperRequest ---
EAGamesScraperRequest::EAGamesScraperRequest(
    std::vector<ScraperSearchResult>& resultsWrite,
    const ScraperSearchParams& params,
    EAGamesStore* eaStore,
    const std::string& gameIdToScrape)
    : ScraperRequest(resultsWrite),
      mSearchParams(params),
      mEaStoreInstance(eaStore),
      mGameIdForApi(gameIdToScrape),
      mRequestLaunched(false)
{
    LOG(LogDebug) << "EAGamesScraperRequest created for EA ID: " << mGameIdForApi << " (Game: " << params.game->getName() << ")";
}

void EAGamesScraperRequest::update()
{
    if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) {
        return;
    }

    if (!mRequestLaunched)
    {
        mRequestLaunched = true;
        LOG(LogDebug) << "EAGamesScraperRequest::update() executing API call for EA ID=" << mGameIdForApi;

        if (!mEaStoreInstance) {
            LOG(LogError) << "EAGamesScraperRequest::update - EAGamesStore instance is null.";
            setError(_("Istanza EA Store non trovata.")); // CORRETTO
            return;
        }
        if (!mSearchParams.game) {
             LOG(LogError) << "EAGamesScraperRequest::update - mSearchParams.game is null.";
             setError(_("Dati del gioco non validi per lo scraping.")); // CORRETTO
             return;
        }

        EAGamesStore::MetadataFetchedCallbackStore apiCallback =
            [this](const EAGamesStore::EAGameData& eaData, bool success) {
            if (success) {
                ScraperSearchResult sr(mEaStoreInstance->getStoreName());

                if (!eaData.name.empty()) sr.mdl.set(MetaDataId::Name, eaData.name);
                if (!eaData.description.empty()) sr.mdl.set(MetaDataId::Desc, eaData.description);
                if (!eaData.developer.empty()) sr.mdl.set(MetaDataId::Developer, eaData.developer);
                if (!eaData.publisher.empty()) sr.mdl.set(MetaDataId::Publisher, eaData.publisher);
                if (!eaData.genre.empty()) sr.mdl.set(MetaDataId::Genre, eaData.genre);

                if (!eaData.releaseDate.empty()) {
                    time_t release_t = Utils::Time::iso8601ToTime(eaData.releaseDate);
                    if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                        std::string esDate = Utils::Time::timeToMetaDataString(release_t);
                        if (!esDate.empty()) {
                            sr.mdl.set(MetaDataId::ReleaseDate, esDate);
                        } else {
                            LOG(LogWarning) << "EAGamesScraper: Failed to convert releaseDate to ES format: " << eaData.releaseDate;
                        }
                    } else {
                        LOG(LogWarning) << "EAGamesScraper: Failed to parse ISO8601 releaseDate: " << eaData.releaseDate;
                    }
                }

                // Preserva gli ID esistenti dal FileData originale
                sr.mdl.set(MetaDataId::EaOfferId, mSearchParams.game->getMetadata().get(MetaDataId::EaOfferId));
                sr.mdl.set(MetaDataId::EaMasterTitleId, mSearchParams.game->getMetadata().get(MetaDataId::EaMasterTitleId));

                if (!eaData.imageUrl.empty()) {
                    ScraperSearchItem itemUrl;
                    itemUrl.url = eaData.imageUrl;
                    itemUrl.format = Utils::String::toLower(Utils::FileSystem::getExtension(eaData.imageUrl));
                    if (itemUrl.format.empty() || itemUrl.format == ".") itemUrl.format = ".jpg";
                    sr.urls[MetaDataId::Image] = itemUrl;
                    LOG(LogDebug) << "  Set Image URL: " << eaData.imageUrl;
                }
                if (!eaData.backgroundUrl.empty()) {
                    ScraperSearchItem itemFanart;
                    itemFanart.url = eaData.backgroundUrl;
                    itemFanart.format = Utils::String::toLower(Utils::FileSystem::getExtension(eaData.backgroundUrl));
                     if (itemFanart.format.empty() || itemFanart.format == ".") itemFanart.format = ".jpg";
                    sr.urls[MetaDataId::FanArt] = itemFanart;
                    LOG(LogDebug) << "  Set FanArt URL: " << eaData.backgroundUrl;
                }

                mResults.push_back(sr);
                setStatus(ASYNC_DONE);
                LOG(LogInfo) << "EAGamesScraperRequest: Successfully processed result for " << mGameIdForApi;

            } else {
                LOG(LogError) << "EAGamesScraperRequest: Failed to get metadata for EA ID: " << mGameIdForApi;
                setError(_("Recupero metadati EA fallito.")); // CORRETTO
            }
        };
        mEaStoreInstance->GetGameMetadata(mSearchParams.game, apiCallback);
    }
}

EAGamesScraper::EAGamesScraper() {
    mSupportedMedia.insert(ScraperMediaSource::Box2d);
    mSupportedMedia.insert(ScraperMediaSource::FanArt);
}

EAGamesStore* EAGamesScraper::getEaStore() {
    // Usa getInstance(Window*) se GameStoreManager è stato modificato così,
    // altrimenti usa get() se Window è preso globalmente o non necessario.
    // Per ora, assumiamo che get() funzioni o che il GameStoreManager sia già inizializzato altrove.
    GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);
    if (!gsm) {
         LOG(LogError) << "EAGamesScraper::getEaStore - GameStoreManager instance is null.";
         return nullptr;
    }
    GameStore* store = gsm->getStore("EA Games");
    if (store) {
        return dynamic_cast<EAGamesStore*>(store);
    }
    LOG(LogError) << "EAGamesScraper::getEaStore - EAGamesStore not found in GameStoreManager.";
    return nullptr;
}

bool EAGamesScraper::isSupportedPlatform(SystemData* system) {
    if (!system) return false;
    std::string systemName = system->getName();
    return systemName == "eagames" || systemName == "eapc" || systemName == "origin" || systemName == "originpc";
}

const std::set<Scraper::ScraperMediaSource>& EAGamesScraper::getSupportedMedias() {
    return mSupportedMedia;
}

void EAGamesScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results)
{
    if (!params.game) {
        LOG(LogError) << "EAGamesScraper::generateRequests - params.game is null.";
        return;
    }

    std::string offerId = params.game->getMetadata().get(MetaDataId::EaOfferId);
    std::string masterTitleId = params.game->getMetadata().get(MetaDataId::EaMasterTitleId);
    std::string gameIdToUse;

    if (!offerId.empty()) {
        gameIdToUse = offerId;
    } else if (!masterTitleId.empty()) {
        gameIdToUse = masterTitleId;
    } else {
        LOG(LogWarning) << "EAGamesScraper: No EA OfferID or MasterTitleID found for game: " << params.game->getName() << ". Cannot scrape.";
        return;
    }

    LOG(LogInfo) << "EAGamesScraper: Generating request for EA game: " << params.game->getName() << " (Using ID: " << gameIdToUse << ")";

    EAGamesStore* eaStore = getEaStore();
    if (!eaStore) {
        LOG(LogError) << "EAGamesScraper: EAGamesStore instance not available.";
        return;
    }
    requests.push(std::make_unique<EAGamesScraperRequest>(results, params, eaStore, gameIdToUse));
}
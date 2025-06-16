#include "scrapers/Scraper.h"

#include "FileData.h"
#include "ArcadeDBJSONScraper.h"
#include "GamesDBJSONScraper.h"
#include "ScreenScraper.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <FreeImage.h>
#include <fstream>
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include <thread>
#include <SDL_timer.h>
#include "HfsDBScraper.h"
#include "utils/Uri.h"
#include "scrapers/EpicGamesScraper.h"
#include "scrapers/SteamScraper.h"
#include "scrapers/XboxScraper.h"
#include "scrapers/EAGamesScraper.h"
#include "scrapers/IGDBScraper.h" // Assicurati che questo sia incluso se usi IGDBScraper
#include "scrapers/UniversalSteamScraper.h"
#include "Paths.h"

#define OVERQUOTA_RETRY_DELAY 15000
#define OVERQUOTA_RETRY_COUNT 5

// Definizione del vettore statico degli scraper
std::vector<std::pair<std::string, Scraper*>> Scraper::scrapers
{
#ifdef SCREENSCRAPER_DEV_LOGIN
	{ "ScreenScraper", new ScreenScraperScraper() },
#endif

#ifdef GAMESDB_APIKEY
	{ "TheGamesDB", new TheGamesDBScraper() },
#endif

#ifdef HFS_DEV_LOGIN
	{ "HfsDB", new HfsDBScraper() },
#endif

	{ "ArcadeDB", new ArcadeDBScraper() },
	{ "UNIVERSALSTEAM", new UniversalSteamScraper() }, 
	{ "IGDB", new IGDBScraper() }, // Aggiunta dell'IGDBScraper alla lista
	{ "EPIC GAMES STORE", new EpicGamesScraper() },
	{ "STEAM", new SteamScraper() },
	{ "XBOX", new Scrapers::XboxScraper() }
	// Potresti dover aggiungere EAGamesScraper qui se non è già presente e se lo usi
	// { "EAGAMES", new EAGamesScraper() },
};


std::string Scraper::getScraperName(Scraper* scraper)
{
	for (auto engine : scrapers)
		if (scraper == engine.second)
			return engine.first;

	return std::string();
}

int Scraper::getScraperIndex(const std::string& name)
{
	for (int i = 0 ; i < (int)scrapers.size() ; i++) // Cast a int per evitare warning size_t vs int
		if (name == scrapers[i].first)
			return i;

	return -1;
}

std::string Scraper::getScraperNameFromIndex(int index)
{
	if (index >= 0 && index < (int)scrapers.size()) // Cast a int
		return scrapers[index].first;

	return std::string();
}

Scraper* Scraper::getScraper(const std::string name)
{
	auto scraper_name = name; // Rinominato per evitare shadowing con Scraper::scrapers
	if(scraper_name.empty())
		scraper_name = Settings::getInstance()->getString("Scraper"); //

	for (auto scrap : Scraper::scrapers)
		if (scrap.first == scraper_name)
			return scrap.second;

	return nullptr;
}

bool Scraper::isValidConfiguredScraper()
{
	return getScraper() != nullptr;
}

bool Scraper::hasAnyMedia(FileData* file)
{
	if (isMediaSupported(ScraperMediaSource::Screenshot) || isMediaSupported(ScraperMediaSource::Box2d) || isMediaSupported(ScraperMediaSource::Box3d) || isMediaSupported(ScraperMediaSource::Mix) || isMediaSupported(ScraperMediaSource::TitleShot) || isMediaSupported(ScraperMediaSource::FanArt)) //
		if (!Settings::getInstance()->getString("ScrapperImageSrc").empty() && !file->getMetadata(MetaDataId::Image).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Image))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Box2d) || isMediaSupported(ScraperMediaSource::Box3d)) //
		if (!Settings::getInstance()->getString("ScrapperThumbSrc").empty() && !file->getMetadata(MetaDataId::Thumbnail).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Thumbnail))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Wheel) || isMediaSupported(ScraperMediaSource::Marquee)) //
		if (!Settings::getInstance()->getString("ScrapperLogoSrc").empty() && !file->getMetadata(MetaDataId::Marquee).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Marquee))) // Netplay typo, deve essere ScrapperLogoSrc; //
			return true;

	if (isMediaSupported(ScraperMediaSource::Manual)) //
		if (Settings::getInstance()->getBool("ScrapeManual") && !file->getMetadata(MetaDataId::Manual).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Manual))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Map)) //
		if (Settings::getInstance()->getBool("ScrapeMap") && !file->getMetadata(MetaDataId::Map).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Map))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::FanArt)) //
		if (Settings::getInstance()->getBool("ScrapeFanart") && !file->getMetadata(MetaDataId::FanArt).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::FanArt))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Video)) //
		if (Settings::getInstance()->getBool("ScrapeVideos") && !file->getMetadata(MetaDataId::Video).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Video))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::BoxBack)) //
		if (Settings::getInstance()->getBool("ScrapeBoxBack") && !file->getMetadata(MetaDataId::BoxBack).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::BoxBack))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::TitleShot)) //
		if (Settings::getInstance()->getBool("ScrapeTitleShot") && !file->getMetadata(MetaDataId::TitleShot).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::TitleShot))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Cartridge)) //
		if (Settings::getInstance()->getBool("ScrapeCartridge") && !file->getMetadata(MetaDataId::Cartridge).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Cartridge))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Bezel_16_9)) //
		if (Settings::getInstance()->getBool("ScrapeBezel") && !file->getMetadata(MetaDataId::Bezel).empty() && Utils::FileSystem::exists(file->getMetadata(MetaDataId::Bezel))) //
			return true;

	return false;
}

bool Scraper::hasMissingMedia(FileData* file)
{
	if (isMediaSupported(ScraperMediaSource::Screenshot) || isMediaSupported(ScraperMediaSource::Box2d) || isMediaSupported(ScraperMediaSource::Box3d) || isMediaSupported(ScraperMediaSource::Mix) || isMediaSupported(ScraperMediaSource::TitleShot) || isMediaSupported(ScraperMediaSource::FanArt)) //
		if (!Settings::getInstance()->getString("ScrapperImageSrc").empty() && (file->getMetadata(MetaDataId::Image).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Image)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Box2d) || isMediaSupported(ScraperMediaSource::Box3d)) //
		if (!Settings::getInstance()->getString("ScrapperThumbSrc").empty() && (file->getMetadata(MetaDataId::Thumbnail).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Thumbnail)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Wheel) || isMediaSupported(ScraperMediaSource::Marquee)) //
		if (!Settings::getInstance()->getString("ScrapperLogoSrc").empty() && (file->getMetadata(MetaDataId::Marquee).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Marquee)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Manual)) //
		if (Settings::getInstance()->getBool("ScrapeManual") && (file->getMetadata(MetaDataId::Manual).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Manual)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Map)) //
		if (Settings::getInstance()->getBool("ScrapeMap") && (file->getMetadata(MetaDataId::Map).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Map)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::FanArt)) //
		if (Settings::getInstance()->getBool("ScrapeFanart") && (file->getMetadata(MetaDataId::FanArt).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::FanArt)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Video)) //
		if (Settings::getInstance()->getBool("ScrapeVideos") && (file->getMetadata(MetaDataId::Video).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Video)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::BoxBack)) //
		if (Settings::getInstance()->getBool("ScrapeBoxBack") && (file->getMetadata(MetaDataId::BoxBack).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::BoxBack)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::TitleShot)) //
		if (Settings::getInstance()->getBool("ScrapeTitleShot") && (file->getMetadata(MetaDataId::TitleShot).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::TitleShot)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Cartridge)) //
		if (Settings::getInstance()->getBool("ScrapeCartridge") && (file->getMetadata(MetaDataId::Cartridge).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Cartridge)))) //
			return true;

	if (isMediaSupported(ScraperMediaSource::Bezel_16_9)) //
		if (Settings::getInstance()->getBool("ScrapeBezel") && (file->getMetadata(MetaDataId::Bezel).empty() || !Utils::FileSystem::exists(file->getMetadata(MetaDataId::Bezel)))) //
			return true;


	return false;
}

bool Scraper::isMediaSupported(const Scraper::ScraperMediaSource& md)
{
	auto mdds = getSupportedMedias(); //
	return mdds.find(md) != mdds.cend(); //
}

std::unique_ptr<ScraperSearchHandle> Scraper::search(const ScraperSearchParams& params)
{
	std::unique_ptr<ScraperSearchHandle> handle(new ScraperSearchHandle()); //
	generateRequests(params, handle->mRequestQueue, handle->mResults); //
	return handle;
}

std::vector<std::string> Scraper::getScraperList()
{
	std::vector<std::string> list;
	for(auto& it : Scraper::scrapers) //
		list.push_back(it.first);

	return list;
}

// ScraperSearchHandle
ScraperSearchHandle::ScraperSearchHandle()
{
	setStatus(ASYNC_IN_PROGRESS); //
}

void ScraperSearchHandle::update()
{
	if(mStatus == ASYNC_DONE) //
		return;

	if(!mRequestQueue.empty()) //
	{
		auto& req = *(mRequestQueue.front()); //
		AsyncHandleStatus status = req.status(); //

		if(status == ASYNC_ERROR) //
		{
			setError(req.getErrorCode(), Utils::String::removeHtmlTags(req.getStatusString())); //
			while(!mRequestQueue.empty()) //
				mRequestQueue.pop(); //

			return;
		}

		if (status == ASYNC_DONE) //
		{
			if (mResults.size() > 0) //
			{
				while (!mRequestQueue.empty()) //
					mRequestQueue.pop(); //
			}
			else
				mRequestQueue.pop(); //
		}
	}

	if(mRequestQueue.empty() && mStatus != ASYNC_ERROR) //
	{
		setStatus(ASYNC_DONE); //
		return;
	}
}

// ScraperHttpRequest
ScraperHttpRequest::ScraperHttpRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url, HttpReqOptions* options)
	: ScraperRequest(resultsWrite)
{
	setStatus(ASYNC_IN_PROGRESS); //

	if (options != nullptr) //
		mOptions = *options; //

	mRequest = new HttpReq(url, &mOptions); //
	mRetryCount = 0; //
	mOverQuotaPendingTime = 0; //
	mOverQuotaRetryDelay = OVERQUOTA_RETRY_DELAY; //
	mOverQuotaRetryCount = OVERQUOTA_RETRY_COUNT; //
}

ScraperHttpRequest::~ScraperHttpRequest()
{
	delete mRequest; //
}

void ScraperHttpRequest::update()
{
	if (mOverQuotaPendingTime > 0) //
	{
		int lastTime = SDL_GetTicks(); //
		if (lastTime - mOverQuotaPendingTime > mOverQuotaRetryDelay) //
		{
			mOverQuotaPendingTime = 0; //

			LOG(LogDebug) << "REQ_429_TOOMANYREQUESTS : Retrying"; //

			std::string url = mRequest->getUrl(); //
			delete mRequest; //
			mRequest = new HttpReq(url, &mOptions); //
		}
		return;
	}

	HttpReq::Status status = mRequest->status(); //

	if (status == HttpReq::REQ_IN_PROGRESS) //
		return;

	if(status == HttpReq::REQ_SUCCESS) //
	{
		setStatus(ASYNC_DONE); //
		process(mRequest, mResults); //
		return;
	}

	if (status == HttpReq::REQ_429_TOOMANYREQUESTS) //
	{
		mRetryCount++; //
		if (mRetryCount >= mOverQuotaRetryCount) //
		{
			setStatus(ASYNC_DONE); //
			return;
		}

		auto retryDelay = mRequest->getResponseHeader("Retry-After"); //
		if (!retryDelay.empty()) //
		{
			mOverQuotaRetryCount = 1; //
			mOverQuotaRetryDelay = Utils::String::toInteger(retryDelay) * 1000; //

			if (!retryOn249() && mOverQuotaRetryDelay > 5000) //
			{
				setStatus(ASYNC_DONE); //
				return;
			}
		}

		setStatus(ASYNC_IN_PROGRESS); //
		mOverQuotaPendingTime = SDL_GetTicks(); //
		LOG(LogDebug) << "REQ_429_TOOMANYREQUESTS : Retrying in " << mOverQuotaRetryDelay << " seconds"; //
		return;
	}

	if (status == HttpReq::REQ_404_NOTFOUND || status == HttpReq::REQ_IO_ERROR) //
	{
		setStatus(ASYNC_DONE); //
		return;
	}

	if (status != HttpReq::REQ_SUCCESS) //
	{
		setError(status, Utils::String::removeHtmlTags(mRequest->getErrorMsg())); //
		return;
	}

	LOG(LogError) << "ScraperHttpRequest network error (status: " << status << ") - " << mRequest->getErrorMsg(); //
	setError(Utils::String::removeHtmlTags(mRequest->getErrorMsg())); //
}

// Implementazione di ScraperSearchResult::resolveMetaDataAssets
std::unique_ptr<MDResolveHandle> ScraperSearchResult::resolveMetaDataAssets(const ScraperSearchParams& search)
{
	return std::unique_ptr<MDResolveHandle>(new MDResolveHandle(*this, search)); //
}


// metadata resolving stuff
MDResolveHandle::MDResolveHandle(const ScraperSearchResult& result, const ScraperSearchParams& search) : mResult(result)
{
	  LOG(LogInfo) << "[MDResolveHandle CONSTRUCTOR] Inizio per gioco: '" << search.game->getName()
                 << "', Scraper: '" << result.scraper << "', PFN: '" << result.pfn
                 << "'. Num URLs da processare: " << result.urls.size(); //
	mPercent = -1;

	bool overWriteMedias = Settings::getInstance()->getBool("ScrapeOverWrite") && search.overWriteMedias; //

	for (auto& url_map_entry : result.urls) // Rinominato url a url_map_entry per chiarezza
	{
		if (url_map_entry.second.url.empty())
			continue;

		if (!overWriteMedias && Utils::FileSystem::exists(search.game->getMetadata(url_map_entry.first))) //
		{
			mResult.mdl.set(url_map_entry.first, search.game->getMetadata(url_map_entry.first)); //
			if (mResult.urls.find(url_map_entry.first) != mResult.urls.cend())
				mResult.urls[url_map_entry.first].url = ""; //

			continue;
		}

		bool resize = true;

		std::string suffix = "image";
		switch (url_map_entry.first) // Usato url_map_entry
		{
		case MetaDataId::Video: suffix = "video";  resize = false; break; //
		case MetaDataId::FanArt: suffix = "fanart"; resize = false; break; //
		case MetaDataId::BoxBack: suffix = "boxback"; resize = false; break; //
		case MetaDataId::BoxArt: suffix = "box"; resize = false; break; //
		case MetaDataId::Image: suffix = "cover"; break;
		case MetaDataId::Thumbnail: suffix = "thumb"; break;
		case MetaDataId::Marquee: suffix = "marquee"; resize = false; break;
		case MetaDataId::Wheel: suffix = "wheel"; resize = false; break; //
		case MetaDataId::TitleShot: suffix = "titleshot"; break; //
		case MetaDataId::Manual: suffix = "manual"; resize = false;  break; //
		case MetaDataId::Magazine: suffix = "magazine"; resize = false;  break; //
		case MetaDataId::Map: suffix = "map"; resize = false; break; //
		case MetaDataId::Cartridge: suffix = "cartridge"; break; //
		case MetaDataId::Bezel: suffix = "bezel"; resize = false; break; //
        case MetaDataId::MD_SCREENSHOT_URL: suffix = "screenshot"; break;
        case MetaDataId::MD_VIDEO_URL: suffix = "video_url"; resize = false; break;
		}

		auto ext = url_map_entry.second.format; //
		if (ext.empty())
			ext = Utils::FileSystem::getExtension(url_map_entry.second.url); //

		std::string resourcePath = Scraper::getSaveAsPath(search.game, url_map_entry.first, ext); //

		if (!overWriteMedias && Utils::FileSystem::exists(resourcePath)) //
		{
			mResult.mdl.set(url_map_entry.first, resourcePath); //
			if (mResult.urls.find(url_map_entry.first) != mResult.urls.cend())
				mResult.urls[url_map_entry.first].url = ""; //

			continue;
		}

		mFuncs.push_back(new ResolvePair( //
			[this, url_map_entry, resourcePath, resize] // Cattura url_map_entry
			{
				return downloadImageAsync(url_map_entry.second.url, resourcePath, resize); //
			},
			[this, url_map_entry](ImageDownloadHandle* resolve_result) // Cattura url_map_entry
			{
				auto finalFile = resolve_result->getImageFileName(); //

				if (Utils::FileSystem::getFileSize(finalFile) > 0) //
					mResult.mdl.set(url_map_entry.first, finalFile); //

				if (mResult.urls.find(url_map_entry.first) != mResult.urls.cend())
					mResult.urls[url_map_entry.first].url = ""; //
			},
			suffix, result.mdl.getName()));
	}

	auto it = mFuncs.cbegin();
	if (it == mFuncs.cend())
		setStatus(ASYNC_DONE); //
	else
	{
		mSource = (*it)->source; //
		mCurrentItem = (*it)->name; //
		(*it)->Run(); //
	}
}

void MDResolveHandle::update()
{
	if(mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) //
		return;

	auto it = mFuncs.cbegin();
	if (it == mFuncs.cend())
	{
		setStatus(ASYNC_DONE); //
		return;
	}

	ResolvePair* pPair = (*it); //

	if (pPair->handle->status() == ASYNC_IN_PROGRESS) //
		mPercent = pPair->handle->getPercent(); //

	if (pPair->handle->status() == ASYNC_ERROR) //
	{
		setError(pPair->handle->getErrorCode(), pPair->handle->getStatusString()); //
		for (auto fc : mFuncs)
			delete fc;
        mFuncs.clear();
		return;
	}
	else if (pPair->handle->status() == ASYNC_DONE) //
	{
		pPair->onFinished(pPair->handle.get()); //
		mFuncs.erase(it); //
		delete pPair;

		auto next = mFuncs.cbegin();
		if (next != mFuncs.cend())
		{
			mSource = (*next)->source; //
			mCurrentItem = (*next)->name; //
			(*next)->Run(); //
		}
	}

	if(mFuncs.empty())
		setStatus(ASYNC_DONE); //
}

std::unique_ptr<ImageDownloadHandle> MDResolveHandle::downloadImageAsync(const std::string& url, const std::string& saveAs, bool resize)
{
	LOG(LogInfo) << "[MDResolveHandle downloadImageAsync] Preparazione download effettivo: URL: [" << url << "] -> SaveAs: [" << saveAs << "]"; //

	return std::unique_ptr<ImageDownloadHandle>(new ImageDownloadHandle(url, saveAs,
		resize ? Settings::getInstance()->getInt("ScraperResizeWidth") : 0, //
		resize ? Settings::getInstance()->getInt("ScraperResizeHeight") : 0)); //
}

ImageDownloadHandle::ImageDownloadHandle(const std::string& url, const std::string& path, int maxWidth, int maxHeight) :
    mUrl(url),
    mRequest(nullptr),
    mSavePath(path),
    mMaxWidth(maxWidth),
    mMaxHeight(maxHeight),
    mRetryCount(0),
    mOverQuotaPendingTime(0),
    mOverQuotaRetryDelay(OVERQUOTA_RETRY_DELAY),
    mOverQuotaRetryCount(OVERQUOTA_RETRY_COUNT)
{
    mOptions.outputFilename = mSavePath; //
    //mOptions.httpMethod = "GET"; //
    mOptions.dataToPost = ""; //

    if (mUrl.find("screenscraper") != std::string::npos && mUrl.find("/medias/") != std::string::npos) //
    {
        auto splits = Utils::String::split(mUrl, '/', true); //
        if (splits.size() > 1)
            mOptions.customHeaders.push_back("Referer: https://" + splits[1] + "/gameinfos.php?gameid=" + splits[splits.size() - 2] + "&action=onglet&zone=gameinfosmedias"); //
    }
}


ImageDownloadHandle::~ImageDownloadHandle()
{
	delete mRequest; //
}

int ImageDownloadHandle::getPercent()
{
	if (mRequest && mRequest->status() == HttpReq::REQ_IN_PROGRESS) //
		return mRequest->getPercent(); //

	return -1;
}

void ImageDownloadHandle::update()
{
	if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR) //
		return;

	if (mOverQuotaPendingTime > 0) //
	{
		int lastTime = SDL_GetTicks(); //
		if (lastTime - mOverQuotaPendingTime > mOverQuotaRetryDelay) //
		{
			mOverQuotaPendingTime = 0; //
			LOG(LogDebug) << "ImageDownloadHandle: REQ_429_TOOMANYREQUESTS : Ritento URL: " << mUrl; //

            if (mRequest) { //
                delete mRequest; //
                mRequest = nullptr;
            }
		}
        else
        {
		    return; 
        }
	}

	if(!mRequest) //
	{
        std::string effectiveUrl = mUrl;
        if (mUrl.find("screenscraper") != std::string::npos && (mSavePath.find(".jpg") != std::string::npos || mSavePath.find(".png") != std::string::npos) && mUrl.find("media=map") == std::string::npos) //
        {
            Utils::Uri uri(mUrl); //
            if (mMaxWidth > 0) //
            {
                uri.arguments.set("maxwidth", std::to_string(mMaxWidth)); //
                uri.arguments.set("maxheight", std::to_string(mMaxWidth)); //
            }
            else if (mMaxHeight > 0) //
            {
                uri.arguments.set("maxwidth", std::to_string(mMaxHeight)); //
                uri.arguments.set("maxheight", std::to_string(mMaxHeight)); //
            }
            effectiveUrl = uri.toString(); //
        }

		mRequest = new HttpReq(effectiveUrl, &mOptions); //
        LOG(LogDebug) << "ImageDownloadHandle - HttpReq avviato per URL: \"" << effectiveUrl << "\". Salvataggio in: \"" << mOptions.outputFilename << "\"";
	}

	HttpReq::Status status = mRequest->status(); //

	if (status == HttpReq::REQ_IN_PROGRESS) //
		return;

	if (status == HttpReq::REQ_SUCCESS) //
	{
        LOG(LogInfo) << "ImageDownloadHandle - HttpReq SUCCESSO per URL: \"" << mRequest->getUrl() << "\". File dovrebbe essere salvato in: \"" << mSavePath << "\" da HttpReq."; //

        if (!Utils::FileSystem::exists(mSavePath) || Utils::FileSystem::getFileSize(mSavePath) == 0) { //
            LOG(LogError) << "ImageDownloadHandle - ERRORE: File non trovato o vuoto dopo download HttpReq: " << mSavePath;
            setError(HttpReq::REQ_IO_ERROR, "File not saved correctly by HttpReq"); //
            return;
        }

		LOG(LogInfo) << "ImageDownloadHandle - FILE SALVATO DA HTTPREQ: \"" << mRequest->getUrl() << "\" a \"" << mSavePath << "\""; //

		std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(mSavePath)); //
		std::string contentType = mRequest->getResponseHeader("Content-Type"); //
		if (!contentType.empty()) //
		{
			std::string trueExtension;
			if (Utils::String::startsWith(contentType, "image/")) //
			{
				trueExtension = "." + contentType.substr(6); //
				if (trueExtension == ".jpeg") trueExtension = ".jpg"; //
				else if (trueExtension == ".svg+xml") trueExtension = ".svg"; //
			}
			else if (Utils::String::startsWith(contentType, "video/")) //
			{
				trueExtension = "." + contentType.substr(6); //
				if (trueExtension == ".quicktime") trueExtension = ".mov"; //
			}

			if (!trueExtension.empty() && trueExtension != ext) //
			{
				auto newFileName = Utils::FileSystem::changeExtension(mSavePath, trueExtension); //
				if (Utils::FileSystem::renameFile(mSavePath, newFileName)) //
				{
					mSavePath = newFileName; //
					LOG(LogInfo) << "ImageDownloadHandle - Rinominato file a: " << mSavePath << " basato su Content-Type."; //
				} else {
                    LOG(LogWarning) << "ImageDownloadHandle - Impossibile rinominare file da Content-Type: " << mSavePath << " a " << newFileName; //
                }
			}
		}

		if (mSavePath.find("-fanart") == std::string::npos && mSavePath.find("-bezel") == std::string::npos && mSavePath.find("-map") == std::string::npos && (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".gif")) //
		{
			try {
                if (mMaxWidth > 0 || mMaxHeight > 0) { //
                    if (resizeImage(mSavePath, mMaxWidth, mMaxHeight)) { //
                        LOG(LogInfo) << "ImageDownloadHandle - Ridimensionata immagine a: " << mSavePath; //
                    } else {
                        LOG(LogError) << "ImageDownloadHandle - Fallimento ridimensionamento immagine (funzione resizeImage ha restituito false): " << mSavePath; //
                    }
                }
            }
			catch(const std::exception& e) { //
                LOG(LogError) << "ImageDownloadHandle - ERRORE nel ridimensionamento immagine: " << e.what(); //
            }
            catch(...) { //
                LOG(LogError) << "ImageDownloadHandle - ERRORE SCONOSCIUTO nel ridimensionamento immagine."; //
            }
		}

		setStatus(ASYNC_DONE); //
        LOG(LogInfo) << "ImageDownloadHandle - Download e processo completati con status ASYNC_DONE."; //

	}
  
    else if (status == HttpReq::REQ_429_TOOMANYREQUESTS) //
    {
        mRetryCount++; //
        if (mRetryCount >= mOverQuotaRetryCount) //
        {
            LOG(LogError) << "ImageDownloadHandle - REQ_429_TOOMANYREQUESTS: Troppi tentativi di riprova per URL: \"" << mRequest->getUrl() << "\". Ignoro errore."; //
            setStatus(ASYNC_DONE);
            return;
        }

        auto retryDelayHeader = mRequest->getResponseHeader("Retry-After"); //
        if (!retryDelayHeader.empty()) //
        {
            mOverQuotaRetryDelay = Utils::String::toInteger(retryDelayHeader) * 1000; //
        } else {
            mOverQuotaRetryDelay = 5000 * mRetryCount; //
        }
        if (mOverQuotaRetryDelay < 1000) mOverQuotaRetryDelay = 1000; //
        if (mOverQuotaRetryDelay > 60000) mOverQuotaRetryDelay = 60000; //

        mOverQuotaPendingTime = SDL_GetTicks(); //
        LOG(LogWarning) << "ImageDownloadHandle - REQ_429_TOOMANYREQUESTS: Ritento tra " << mOverQuotaRetryDelay / 1000 << " secondi per URL: \"" << mRequest->getUrl() << "\""; //
        setStatus(ASYNC_IN_PROGRESS);

        if(mRequest) { //
            delete mRequest; //
            mRequest = nullptr; //
        }
    }
    else
    {
        LOG(LogError) << "ImageDownloadHandle - Download FALLITO per URL: \"" << (mRequest ? mRequest->getUrl() : mUrl) << "\". Stato: " << static_cast<int>(status) << ", Errore: " << (mRequest ? mRequest->getErrorMsg() : "N/A"); //
        setError(status, Utils::String::removeHtmlTags(mRequest ? mRequest->getErrorMsg() : "Unknown HTTP error before request object creation")); //
    }
}

bool resizeImage(const std::string& path, int maxWidth, int maxHeight)
{
	if(maxWidth == 0 && maxHeight == 0) //
		return true;

    if (!Utils::FileSystem::exists(path) || Utils::FileSystem::getFileSize(path) == 0) { //
        LOG(LogError) << "Error - cannot resize image, file does not exist or is empty: \"" << path << "\"!"; //
        return false;
    }

	FREE_IMAGE_FORMAT format = FIF_UNKNOWN; //
	FIBITMAP* image = NULL; //

#if WIN32
	format = FreeImage_GetFileTypeU(Utils::String::convertToWideString(path).c_str(), 0); //
	if(format == FIF_UNKNOWN) //
		format = FreeImage_GetFIFFromFilenameU(Utils::String::convertToWideString(path).c_str()); //
#else
	format = FreeImage_GetFileType(path.c_str(), 0); //
	if(format == FIF_UNKNOWN) //
		format = FreeImage_GetFIFFromFilename(path.c_str()); //
#endif
	if(format == FIF_UNKNOWN) //
	{
		LOG(LogError) << "Error - could not detect filetype for image \"" << path << "\"!"; //
		return false;
	}

	if(FreeImage_FIFSupportsReading(format)) //
	{
#if WIN32
		image = FreeImage_LoadU(format, Utils::String::convertToWideString(path).c_str()); //
#else
		image = FreeImage_Load(format, path.c_str()); //
#endif
	}else{
		LOG(LogError) << "Error - file format reading not supported for image \"" << path << "\"!"; //
		return false;
	}

    if (!image) {
        LOG(LogError) << "Error - FreeImage_Load failed for image \"" << path << "\"!"; //
        return false;
    }

	float width = (float)FreeImage_GetWidth(image); //
	float height = (float)FreeImage_GetHeight(image); //

	if (width == 0 || height == 0) //
	{
		FreeImage_Unload(image); //
        LOG(LogWarning) << "Warning - image has zero width or height: \"" << path << "\"!";
		return true;
	}

    float targetWidth = static_cast<float>(maxWidth);
    float targetHeight = static_cast<float>(maxHeight);

    if (targetWidth == 0 && targetHeight == 0) {
         FreeImage_Unload(image); return true; //
    } else if (targetWidth == 0) {
        targetWidth = (targetHeight / height) * width;
    } else if (targetHeight == 0) {
        targetHeight = (targetWidth / width) * height;
    } else {
        float ratioX = targetWidth / width;
        float ratioY = targetHeight / height;
        float ratio = std::min(ratioX, ratioY);
        targetWidth = width * ratio;
        targetHeight = height * ratio;
    }

	if (width <= targetWidth && height <= targetHeight) //
	{
		FreeImage_Unload(image); //
		return true;
	}

	FIBITMAP* imageRescaled = FreeImage_Rescale(image, static_cast<int>(targetWidth), static_cast<int>(targetHeight), FILTER_BILINEAR); //
	FreeImage_Unload(image); //

	if(imageRescaled == NULL) //
	{
		LOG(LogError) << "Could not resize image! (not enough memory? invalid bitdepth?) Path: " << path; //
		return false;
	}

	bool saved = false; //

	try
	{
#if WIN32
		saved = (FreeImage_SaveU(format, imageRescaled, Utils::String::convertToWideString(path).c_str()) != 0); //
#else
		saved = (FreeImage_Save(format, imageRescaled, path.c_str()) != 0); //
#endif
	}
	catch(...) {
        LOG(LogError) << "Exception during FreeImage_Save for: " << path; //
    }

	FreeImage_Unload(imageRescaled); //

	if(!saved) //
		LOG(LogError) << "Failed to save resized image! Path: " << path; //

	return saved; //
}

std::string Scraper::getSaveAsPath(FileData* game, const MetaDataId metadataId, const std::string& givenExtension)
{
    if (!game || !game->getSourceFileData() || !game->getSourceFileData()->getSystem()) { //
        LOG(LogError) << "Scraper::getSaveAsPath - Gioco, SourceFileData o SystemData nullo per il gioco passato."; //
        return "";
    }

    std::string suffix = "image"; //
    std::string mediaTypeFolder = "images"; //

    switch (metadataId)
    {
        case MetaDataId::Image:           suffix = "cover";       mediaTypeFolder = "images"; break; //
        case MetaDataId::Thumbnail:       suffix = "thumb";       mediaTypeFolder = "images"; break; //
        case MetaDataId::Marquee:         suffix = "marquee";     mediaTypeFolder = "images"; break; //
        case MetaDataId::Video:           suffix = "video";       mediaTypeFolder = "videos"; break; //
        case MetaDataId::FanArt:          suffix = "fanart";      mediaTypeFolder = "images"; break; //
        case MetaDataId::BoxBack:         suffix = "boxback";     mediaTypeFolder = "images"; break; //
        case MetaDataId::BoxArt:          suffix = "boxart";      mediaTypeFolder = "images"; break;
        case MetaDataId::Wheel:           suffix = "wheel";       mediaTypeFolder = "images"; break; //
        case MetaDataId::TitleShot:       suffix = "titleshot";   mediaTypeFolder = "images"; break; //
        case MetaDataId::Manual:          suffix = "manual";      mediaTypeFolder = "manuals"; break; //
        case MetaDataId::Magazine:        suffix = "magazine";    mediaTypeFolder = "magazines"; break; //
        case MetaDataId::Map:             suffix = "map";         mediaTypeFolder = "maps";   break; //
        case MetaDataId::Cartridge:       suffix = "cartridge";   mediaTypeFolder = "images"; break; //
        case MetaDataId::Bezel:           suffix = "bezel";       mediaTypeFolder = "bezels"; break; //
        case MetaDataId::MD_SCREENSHOT_URL: suffix = "screenshot"; mediaTypeFolder = "images"; break; //
        case MetaDataId::MD_VIDEO_URL:    suffix = "video_url";   mediaTypeFolder = "videos"; break;
        default:
            LOG(LogWarning) << "Scraper::getSaveAsPath - MetaDataId (" << static_cast<int>(metadataId)
                            << ") non gestito specificamente nello switch, usando defaults '" << mediaTypeFolder << "'/'other'."; //
            suffix = "other";
            break;
    }

    SystemData* system = game->getSourceFileData()->getSystem(); //
    std::string baseName; //
    std::string storeProvider = game->getMetadata().get(MetaDataId::StoreProvider); //
    if (storeProvider.empty()) storeProvider = "UnknownProvider";

    LOG(LogDebug) << "getSaveAsPath START: Game: '" << game->getName() << "', Path: '" << game->getPath() << "', StoreProvider: '" << storeProvider << "'"; //

    std::string candidateId;

    if (storeProvider == "IGDB") candidateId = game->getMetadata().get(MetaDataId::ScraperId); //
    else if (storeProvider == "EAGAMESSTORE") { //
        candidateId = game->getMetadata().get(MetaDataId::EaOfferId); //
        if (candidateId.empty()) candidateId = game->getMetadata().get(MetaDataId::EaMasterTitleId); //
    }
    else if (storeProvider == "STEAM") candidateId = game->getMetadata().get(MetaDataId::SteamAppId); //
    else if (storeProvider == "XBOX") { //
        candidateId = game->getMetadata().get(MetaDataId::XboxProductId); //
    }
    else if (storeProvider == "EPIC") candidateId = game->getMetadata().get(MetaDataId::EpicId); //

    if (!candidateId.empty()) {
        baseName = candidateId; //
        LOG(LogInfo) << "getSaveAsPath - Usando " << storeProvider << " ID: '" << baseName << "' come baseName."; //
    } else {
        baseName = Utils::FileSystem::getFileName(game->getPath()); //
        LOG(LogInfo) << "getSaveAsPath - Nessun ID specifico trovato per StoreProvider '" << storeProvider
                     << "'. Fallback a nome file completo dal percorso del gioco: '" << baseName << "'"; //
    }

    if (baseName.empty()) {
        Utils::Time::DateTime dateTimeNow = Utils::Time::DateTime::now(); //
        baseName = "mediafile_" + std::to_string(dateTimeNow.getTime()); //
        LOG(LogError) << "getSaveAsPath - baseName era VUOTO dopo tutti i tentativi. Usato timestamp: '" << baseName << "'"; //
    }

       baseName = Utils::String::replace(baseName, " ", "_");
    baseName = Utils::String::removeParenthesis(baseName);
    const std::string invalidChars = ":*?\"<>|/\\";
    for (char c : invalidChars) {
        baseName = Utils::String::replace(baseName, std::string(1, c), "");
    }
    if (baseName.length() > 100) { // Limit length
        baseName = baseName.substr(0, 100);
        LOG(LogDebug) << "getSaveAsPath - Troncato baseName a 100 caratteri: '" << baseName << "'";
    }
     if (baseName.empty()) { // Ensure baseName is not empty after sanitization
        baseName = "sanitized_empty_" + std::to_string(Utils::Time::DateTime::now().getTime());
        LOG(LogError) << "getSaveAsPath - baseName era VUOTO dopo sanitizzazione. Usato fallback: '" << baseName << "'";
    }
    LOG(LogInfo) << "getSaveAsPath FINALE: baseName sanitizzato per il file: '" << baseName << "'";

    // MODIFICA INIZIA QUI:
    // Rimuoviamo l'aggiunta della sottocartella "media".
    // Il percorso radice per i media specifici del tipo (images, videos
    // sarà direttamente dentro la cartella del sistema (getStartPath()).
    std::string systemSpecificMediaRoot = system->getStartPath(); //
    LOG(LogInfo) << "getSaveAsPath - Percorso radice per i media specifici del tipo (es. images, videos): " << systemSpecificMediaRoot;
    // MODIFICA FINISCE QUI

    std::string finalSaveDir = Utils::FileSystem::combine(systemSpecificMediaRoot, mediaTypeFolder);
    Utils::FileSystem::createDirectory(finalSaveDir); // Ensure directory exists

    std::string extension = givenExtension;
    if (extension.empty() || extension[0] != '.') {
        extension = "." + (extension.empty() ? "dat" : extension);
    }
    size_t queryPosInExt = extension.find('?');
    if (queryPosInExt != std::string::npos) extension = extension.substr(0, queryPosInExt);
    if (extension == ".") extension = ".dat";

    std::string finalPath = Utils::FileSystem::combine(finalSaveDir, baseName + "-" + suffix + extension);

    LOG(LogInfo) << "Scraper::getSaveAsPath - Percorso generato per MetaDataId " << static_cast<int>(metadataId)
                 << " (" << suffix << "): " << finalPath;

    return finalPath;
}
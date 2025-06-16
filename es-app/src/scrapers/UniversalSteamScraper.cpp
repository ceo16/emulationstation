#include "scrapers/UniversalSteamScraper.h"
#include "Log.h"
#include "HttpReq.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"
#include "utils/TimeUtil.h" // Per la conversione della data
#include "json.hpp"
#include <iomanip>   // Per std::get_time
#include <sstream>   // Per std::stringstream
#include <regex>     // Per la ricerca dei numeri nei titoli

using json = nlohmann::json;

namespace {

const std::set<Scraper::ScraperMediaSource> supportedMedias = {
    Scraper::ScraperMediaSource::Box2d,
    Scraper::ScraperMediaSource::Screenshot,
    Scraper::ScraperMediaSource::Marquee,
    Scraper::ScraperMediaSource::FanArt,
    Scraper::ScraperMediaSource::Video,
    Scraper::ScraperMediaSource::TitleShot
};

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
    if (!parsed && steamDateStr.length() == 4 && std::all_of(steamDateStr.begin(), steamDateStr.end(), ::isdigit)) {
        ss.clear(); ss.str(steamDateStr + "-01-01");
        ss >> std::get_time(&t, "%Y-%m-%d");
        if(!ss.fail()) parsed = true;
    }
    if (!parsed) return "";
    std::time_t time = mktime(&t);
    if (time == -1) return "";
    return Utils::Time::timeToMetaDataString(time);
}

class UniversalSteamDetailsRequest : public ScraperHttpRequest
{
public:
    UniversalSteamDetailsRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url)
        : ScraperHttpRequest(resultsWrite, url, nullptr) {}

protected:
    bool process(HttpReq* req, std::vector<ScraperSearchResult>& results) override
    {
        json data;
        try { data = json::parse(req->getContent()); }
        catch (const json::exception&) { return false; }

        if (!data.is_object() || data.empty()) return false;
        
        auto it = data.begin();
        const std::string appIdStr = it.key();
        json& gameDataContainer = it.value();

        if (!gameDataContainer.value("success", false)) return true;

        json& gameData = gameDataContainer["data"];
        if (!gameData.is_object()) return false;

        ScraperSearchResult result("UniversalSteam");
        result.mdl.set("name", gameData.value("name", ""));
        result.mdl.set("desc", gameData.value("short_description", ""));

        if (gameData.contains("developers") && !gameData["developers"].empty())
            result.mdl.set("developer", gameData["developers"][0].get<std::string>());
        
        if (gameData.contains("publishers") && !gameData["publishers"].empty())
            result.mdl.set("publisher", gameData["publishers"][0].get<std::string>());
        
        if (gameData.contains("release_date") && gameData["release_date"].is_object() && gameData["release_date"].contains("date")) {
            std::string esDate = convertSteamDateToESFormat(gameData["release_date"].value("date", ""));
            if (!esDate.empty()) result.mdl.set("releasedate", esDate);
        }

        if (gameData.contains("genres") && gameData["genres"].is_array()) {
            std::string genreStr;
            for (const auto& genre : gameData["genres"]) {
                if (genre.is_object() && genre.contains("description")) {
                    if (!genreStr.empty()) genreStr += ", ";
                    genreStr += genre["description"].get<std::string>();
                }
            }
            result.mdl.set("genre", genreStr);
        }
        
        if (gameData.contains("movies") && gameData["movies"].is_array() && !gameData["movies"].empty()) {
            const auto& firstMovie = gameData["movies"][0];
            std::string videoUrl;
            if (firstMovie.contains("mp4") && firstMovie["mp4"].contains("max")) videoUrl = firstMovie["mp4"].value("max", "");
            else if (firstMovie.contains("mp4") && firstMovie["mp4"].contains("480")) videoUrl = firstMovie["mp4"].value("480", "");
            if (!videoUrl.empty()) result.urls[MetaDataId::Video] = ScraperSearchItem(videoUrl, ".mp4");
        }

        if (gameData.contains("background_raw")) {
            result.urls[MetaDataId::FanArt] = ScraperSearchItem(gameData.value("background_raw", ""), ".jpg");
        }

        std::string imageUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/library_600x900.jpg";
        result.urls[MetaDataId::Image] = ScraperSearchItem(imageUrl, ".jpg");

        std::string thumbUrl;
        if (gameData.contains("library_assets") && gameData["library_assets"].is_object() && gameData["library_assets"].contains("header")) thumbUrl = gameData["library_assets"].value("header", "");
        else if (gameData.contains("header_image")) thumbUrl = gameData.value("header_image", "");
        if (!thumbUrl.empty()) result.urls[MetaDataId::Thumbnail] = ScraperSearchItem(thumbUrl, ".jpg");

        std::string marqueeUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/logo.png";
        if (!marqueeUrl.empty()) result.urls[MetaDataId::Marquee] = ScraperSearchItem(marqueeUrl, ".png");

        if (gameData.contains("screenshots") && gameData["screenshots"].is_array() && !gameData["screenshots"].empty()) {
            const auto& firstScreenshot = gameData["screenshots"][0];
            if (firstScreenshot.contains("path_full")) {
                 result.urls[MetaDataId::TitleShot] = ScraperSearchItem(firstScreenshot.value("path_full", ""), ".jpg");
            }
        }

        results.push_back(result);
        return true;
    }
};

std::string findSteamAppId(const std::string& gameName)
{
    if (gameName.empty()) return "";

    std::string lang = "italian";
    std::string searchUrl = "https://store.steampowered.com/search/suggest?term=" + HttpReq::urlEncode(gameName) + "&f=games&cc=IT&l=" + lang;
    
    // CORREZIONE: Aggiungiamo un User-Agent per far sembrare la richiesta legittima.
    HttpReqOptions options;
    options.customHeaders.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

    HttpReq req(searchUrl, &options);
    req.wait();
    
    // Non logghiamo più l'intero HTML per non intasare i log, ma lo lasciamo in caso di debug futuro
    // LOG(LogInfo) << "[Debug] Risposta da Steam per \"" << gameName << "\":\n" << req.getContent();

    if (req.status() == HttpReq::REQ_SUCCESS) 
    {
        std::string content = req.getContent();
        const std::string searchString = "data-ds-appid=\"";
        size_t startPos = content.find(searchString);

        if (startPos != std::string::npos) 
        {
            startPos += searchString.length();
            size_t endPos = content.find("\"", startPos);
            if (endPos != std::string::npos) 
            {
                return content.substr(startPos, endPos - startPos);
            }
        }
    } else {
        LOG(LogError) << "Richiesta di ricerca a Steam fallita con status: " << req.status();
    }
    
    return "";
}

// Funzione helper per verificare se una stringa è un probabile ID e non un nome
bool isLikelyAnId(const std::string& s)
{
    // Se la stringa è lunga 32 caratteri e contiene solo cifre esadecimali (tipico di Epic)
    if (s.length() == 32 && std::all_of(s.begin(), s.end(), ::isxdigit)) {
        return true;
    }
    // Aggiungi altri controlli se necessario (es. per Xbox)
    return false;
}

} // fine namespace anonimo

bool UniversalSteamScraper::isSupportedPlatform(SystemData* system) { return true; }
const std::set<Scraper::ScraperMediaSource>& UniversalSteamScraper::getSupportedMedias() { return supportedMedias; }

void UniversalSteamScraper::generateRequests(const ScraperSearchParams& params,
                                       std::queue<std::unique_ptr<ScraperRequest>>& requests,
                                       std::vector<ScraperSearchResult>& results)
{
    // LOGICA DI RICERCA MIGLIORATA
    std::string searchName;

    // 1. Prova a usare il nome dai metadati, perché è il più affidabile
    std::string metadataName = params.game->getMetadata().get("name");
    if (!metadataName.empty() && !isLikelyAnId(metadataName)) {
        searchName = metadataName;
        LOG(LogInfo) << "Avvio scraping [UniversalSteam] usando il nome dai metadati: \"" << searchName << "\"";
    } else {
        // 2. Altrimenti, usa il "clean name"
        std::string cleanName = params.game->getCleanName();
        if (!cleanName.empty() && !isLikelyAnId(cleanName)) {
            searchName = cleanName;
            LOG(LogInfo) << "Avvio scraping [UniversalSteam] usando il clean name: \"" << searchName << "\"";
        } else {
             // 3. Se anche il clean name è un ID, non possiamo fare nulla
            LOG(LogWarning) << "Scraping fallito per il gioco, nome non valido trovato: \"" << (metadataName.empty() ? cleanName : metadataName) << "\"";
            return;
        }
    }
    
    std::string appId = findSteamAppId(searchName);
    
    if (!appId.empty()) {
        LOG(LogInfo) << "AppID trovato: " << appId << ". Richiesta dettagli...";
        std::string lang = "italian"; 
        std::string url = "https://store.steampowered.com/api/appdetails?appids=" + appId + "&l=" + lang;
        requests.push(std::unique_ptr<ScraperRequest>(new UniversalSteamDetailsRequest(results, url)));
    } else {
        LOG(LogWarning) << "Scraping fallito per il gioco \"" << searchName << "\", nessun AppID trovato.";
    }
}
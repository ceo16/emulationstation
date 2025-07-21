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
{
    // 1. Estrai il nome del developer
    std::string developer_name = gameData["developers"][0].get<std::string>();
    
    // 2. Controlla che non sia vuoto e non sia "N/A" prima di salvarlo
    if (!developer_name.empty() && developer_name != "N/A")
        result.mdl.set("developer", developer_name);
}

// Gestione Publisher con controllo
if (gameData.contains("publishers") && !gameData["publishers"].empty())
{
    // 1. Estrai il nome del publisher
    std::string publisher_name = gameData["publishers"][0].get<std::string>();
    
    // 2. Controlla che non sia vuoto e non sia "N/A" prima di salvarlo
    if (!publisher_name.empty() && publisher_name != "N/A")
        result.mdl.set("publisher", publisher_name);
}
        
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

std::string imageUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/library_600x900_2x.jpg";

// Opzione B: Usa sempre la copertina standard (600x900). Qualità inferiore ma presente per quasi tutti i giochi.
// std::string imageUrl = "https://cdn.akamai.steamstatic.com/steam/apps/" + appIdStr + "/library_600x900.jpg";
// =========================================================================================

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

struct SteamSearchResult {
    std::string name;
    std::string appId;
};

// Aggiungi questa funzione helper per i numeri romani
std::string toRoman(int number) {
    if (number < 1 || number > 3999) return std::to_string(number); // Non gestibile
    const std::vector<std::pair<int, std::string>> roman_map = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"},
        {90, "XC"}, {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"},
        {5, "V"}, {4, "IV"}, {1, "I"}
    };
    std::string result;
    for (const auto& pair : roman_map) {
        while (number >= pair.first) {
            result += pair.second;
            number -= pair.first;
        }
    }
    return result;
}

std::string normalizeName(const std::string& name) {
    std::string lowerName = Utils::String::toLower(name);
    // Aggiungi questa riga per rimuovere i simboli speciali
    lowerName = std::regex_replace(lowerName, std::regex("[®™©]"), "");
    lowerName = std::regex_replace(lowerName, std::regex("[']"), "");
    lowerName = std::regex_replace(lowerName, std::regex("[:!-]"), " ");
    lowerName = std::regex_replace(lowerName, std::regex("\\s+"), " "); // Rimuovi spazi doppi
    return Utils::String::trim(lowerName);
}

std::string findBestMatch(const std::string& originalName, const std::vector<SteamSearchResult>& searchResults)
{
    // Funzione interna per cercare nella lista
    auto findInList = [&](const std::string& nameToFind) -> std::string {
        std::string normalizedNameToFind = normalizeName(nameToFind);
        for (const auto& result : searchResults) {
            if (normalizeName(result.name) == normalizedNameToFind) {
                LOG(LogInfo) << "Match trovato per \"" << nameToFind << "\" -> [" << result.name << ", AppID: " << result.appId << "]";
                return result.appId;
            }
        }
        return "";
    };

    std::string appId;
    std::string currentSearchName = originalName;

    // Tentativo 1: Corrispondenza diretta
    appId = findInList(currentSearchName);
    if (!appId.empty()) return appId;

    // Tentativo 2: Sostituzione con numeri romani
    std::string romanName;
    try {
        std::regex num_regex("\\d+");
        auto matches_begin = std::sregex_iterator(currentSearchName.begin(), currentSearchName.end(), num_regex);
        auto matches_end = std::sregex_iterator();
        auto last_match_end = currentSearchName.cbegin();

        for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
            const std::smatch& match = *i;
            romanName.append(last_match_end, match.prefix().second);
            romanName.append(toRoman(std::stoi(match.str())));
            last_match_end = match.suffix().first;
        }
        romanName.append(last_match_end, currentSearchName.cend());

    } catch (const std::exception& e) {
        LOG(LogError) << "Errore durante la conversione in numeri romani: " << e.what();
        romanName = currentSearchName; // In caso di errore, non fare nulla
    }

    // Qui c'era la duplicazione. Questo è il blocco singolo e corretto.
    if (romanName != currentSearchName) {
        appId = findInList(romanName);
        if (!appId.empty()) return appId;
    }
    
    // Tentativo 3: Aggiunta di "The"
    appId = findInList("The " + currentSearchName);
    if (!appId.empty()) return appId;

    // Tentativo 4: Sostituzione di " & " con " and " (e viceversa)
    if (currentSearchName.find(" & ") != std::string::npos) {
         appId = findInList(std::regex_replace(currentSearchName, std::regex(" & "), " and "));
         if (!appId.empty()) return appId;
    } else if (currentSearchName.find(" and ") != std::string::npos) {
         appId = findInList(std::regex_replace(currentSearchName, std::regex(" and "), " & "));
         if (!appId.empty()) return appId;
    }
    
    // Tentativo 5: Ricerca senza sottotitolo (es. "Gioco: Sottotitolo" -> "Gioco")
    size_t colonPos = currentSearchName.find(':');
    if (colonPos != std::string::npos) {
        appId = findInList(Utils::String::trim(currentSearchName.substr(0, colonPos)));
        if (!appId.empty()) return appId;
    }
    
    // Nessuna corrispondenza trovata dopo tutti i tentativi
    return "";
}

std::string findSteamAppId(const std::string& gameName)
{
    if (gameName.empty()) return "";

    // 1. Esegui la richiesta HTTP una sola volta
    std::string searchUrl = "https://store.steampowered.com/search/suggest?term=" + HttpReq::urlEncode(gameName) + "&f=games&cc=IT&l=italian";
    HttpReqOptions options;
    options.customHeaders.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    HttpReq req(searchUrl, &options);
    req.wait();

    if (req.status() != HttpReq::REQ_SUCCESS) {
        LOG(LogError) << "Richiesta di ricerca a Steam fallita con status: " << req.status();
        return "";
    }

    // 2. Analizza l'HTML per estrarre TUTTI i risultati
    std::vector<SteamSearchResult> results;
    std::string content = req.getContent();
	LOG(LogDebug) << "CONTENUTO HTML RICEVUTO DA STEAM per '" << gameName << "':\n" << content;
    std::regex search_regex("<a.*?data-ds-appid=\"(\\d+)\".*?<div class=\"match_name\">(.*?)</div>");
    auto matches_begin = std::sregex_iterator(content.begin(), content.end(), search_regex);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        if (match.size() == 3) {
            results.push_back({ Utils::String::trim(match[2].str()), match[1].str() });
        }
    }

    if (results.empty()) {
        LOG(LogWarning) << "Nessun risultato trovato nella pagina di ricerca di Steam per \"" << gameName << "\"";
        return "";
    }

    // 3. Chiama la nuova funzione di matching per trovare la migliore corrispondenza
    return findBestMatch(gameName, results);
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

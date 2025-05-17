#include "GameStore/Xbox/XboxModels.h"
#include "Log.h" 
#include "utils/TimeUtil.h" 
#include <iomanip>      
#include <sstream>      

namespace nj = nlohmann;

namespace Xbox
{
    std::string safe_get_string(const nj::json& j, const std::string& key) {
        if (j.contains(key) && j[key].is_string()) {
            return j[key].get<std::string>();
        }
        return "";
    }

    int safe_get_int(const nj::json& j, const std::string& key, int default_val = 0) {
        if (j.contains(key) && j[key].is_number_integer()) {
            return j[key].get<int>();
        }
        return default_val;
    }

    std::vector<std::string> safe_get_string_vector(const nj::json& j, const std::string& key) {
        std::vector<std::string> vec;
        if (j.contains(key) && j[key].is_array()) {
            for (const auto& item : j[key]) {
                if (item.is_string()) {
                    vec.push_back(item.get<std::string>());
                }
            }
        }
        return vec;
    }

    TitleDetail TitleDetail::fromJson(const nj::json& j_detail_object) {
        TitleDetail parsed_detail; 
        
        if (!j_detail_object.is_object()) {
            LOG(LogWarning) << "TitleDetail::fromJson: JSON fornito (j_detail_object) non è un oggetto.";
            return parsed_detail;
        }

        LOG(LogDebug) << "TitleDetail::fromJson - JSON grezzo ricevuto per j_detail_object: " << j_detail_object.dump(2);

        // Estrazione dello Store ID alfanumerico dall'array "availabilities"
        if (j_detail_object.contains("availabilities") && j_detail_object["availabilities"].is_array()) {
            auto& availabilities_array = j_detail_object["availabilities"];
            if (!availabilities_array.empty()) {
                const auto& first_availability = availabilities_array[0]; 
                if (first_availability.is_object()) {
                    parsed_detail.productId = safe_get_string(first_availability, "ProductId");
                    if (!parsed_detail.productId.empty()) {
                        LOG(LogInfo) << "TitleDetail::fromJson: Estratto ProductId (Store ID) da availabilities[0].ProductId: [" << parsed_detail.productId << "]";
                    } else {
                        LOG(LogWarning) << "TitleDetail::fromJson: 'ProductId' non trovato o vuoto nel primo oggetto di 'availabilities'.";
                    }
                } else {
                    LOG(LogWarning) << "TitleDetail::fromJson: Il primo elemento di 'availabilities' non è un oggetto.";
                }
            } else {
                LOG(LogWarning) << "TitleDetail::fromJson: L'array 'availabilities' è vuoto.";
            }
        } else {
            LOG(LogWarning) << "TitleDetail::fromJson: Chiave 'availabilities' non trovata o non è un array nel JSON detail. Tentativo di cercare ProductId direttamente.";
            // Fallback se 'availabilities' non esiste: cerca ProductId direttamente nell'oggetto detail
            // Questo potrebbe non essere necessario se 'availabilities' è sempre presente quando c'è un ProductId
            parsed_detail.productId = safe_get_string(j_detail_object, "ProductId");
             if (parsed_detail.productId.empty()) {
                parsed_detail.productId = safe_get_string(j_detail_object, "productId");
             }
             if(!parsed_detail.productId.empty()){
                 LOG(LogInfo) << "TitleDetail::fromJson: Estratto ProductId direttamente da j_detail_object: [" << parsed_detail.productId << "]";
             }
        }
        
        if (parsed_detail.productId.empty()) {
             LOG(LogWarning) << "TitleDetail::fromJson: ProductId (Store ID alfanumerico) finale NON TROVATO.";
        }

        // Per gli altri campi: description, publisherName, developerName, releaseDate, minAge
        // Questi potrebbero NON essere presenti nell'oggetto detail da titlehub,
        // quindi safe_get_string li lascerà vuoti se non trovati.
        // Saranno primariamente recuperati da displaycatalog.mp.microsoft.com.
        parsed_detail.description = safe_get_string(j_detail_object, "description");
        parsed_detail.publisherName = safe_get_string(j_detail_object, "publisherName");
        parsed_detail.developerName = safe_get_string(j_detail_object, "developerName");
        parsed_detail.minAge = safe_get_int(j_detail_object, "minAge");

        if (j_detail_object.contains("releaseDate")) {
             if (j_detail_object["releaseDate"].is_string()) {
                parsed_detail.releaseDate = j_detail_object["releaseDate"].get<std::string>();
            } else if (j_detail_object["releaseDate"].is_object() && j_detail_object["releaseDate"].contains("$date")) { 
                 long long ms = 0;
                 if (j_detail_object["releaseDate"]["$date"].is_number()) {
                    ms = j_detail_object["releaseDate"]["$date"].get<long long>();
                 } else if (j_detail_object["releaseDate"]["$date"].is_string()) { 
                    try { ms = std::stoll(j_detail_object["releaseDate"]["$date"].get<std::string>()); } catch (...) { ms = 0; }
                 }
                if (ms > 0) {
                    time_t tt = ms / 1000;
                    std::tm GmtTime;
                    #ifdef _WIN32
                        gmtime_s(&GmtTime, &tt);
                    #else
                        gmtime_r(&tt, &GmtTime);
                    #endif
                    std::stringstream ss;
                    ss << std::put_time(&GmtTime, "%Y-%m-%dT%H:%M:%SZ");
                    parsed_detail.releaseDate = ss.str();
                } else {
                    parsed_detail.releaseDate = ""; // Assicura che sia vuoto se $date non è valido
                }
            } else {
                 parsed_detail.releaseDate = ""; // Assicura che sia vuoto se il formato non è riconosciuto
            }
        } else {
            parsed_detail.releaseDate = ""; // Assicura che sia vuoto se non c'è releaseDate
        }

        if (parsed_detail.description.empty() && parsed_detail.publisherName.empty() && parsed_detail.developerName.empty() && !parsed_detail.productId.empty()) {
            LOG(LogDebug) << "TitleDetail::fromJson: Description, Publisher, Developer non trovati in detail da titlehub per ProductID " << parsed_detail.productId << ". Saranno cercati via displaycatalog.";
        }
        
        return parsed_detail;
    }

    TitleLastPlayedHistory TitleLastPlayedHistory::fromJson(const nj::json& j) {
        TitleLastPlayedHistory history;
        if (!j.is_object()) return history;
        history.lastTimePlayed = safe_get_string(j, "lastTimePlayed"); 
        return history;
    }

    OnlineTitleInfo OnlineTitleInfo::fromJson(const nj::json& j_title_node) { // Parametro rinominato
        OnlineTitleInfo title;
        if (!j_title_node.is_object()) return title;

        title.titleId = safe_get_string(j_title_node, "titleId");
        title.pfn = safe_get_string(j_title_node, "pfn");
        title.type = safe_get_string(j_title_node, "type");
        title.name = safe_get_string(j_title_node, "name");
        title.windowsPhoneProductId = safe_get_string(j_title_node, "windowsPhoneProductId");
        title.modernTitleId = safe_get_string(j_title_node, "modernTitleId");
        title.mediaItemType = safe_get_string(j_title_node, "mediaItemType");
        title.minutesPlayed = safe_get_string(j_title_node, "minutesPlayed"); 

        if (j_title_node.contains("detail") && j_title_node["detail"].is_object()) {
            title.detail = TitleDetail::fromJson(j_title_node["detail"]); 
        } else {
            LOG(LogWarning) << "OnlineTitleInfo::fromJson: Oggetto 'detail' non trovato o non è un oggetto nel JSON del titolo: " << title.name;
        }

        if (j_title_node.contains("titleHistory") && j_title_node["titleHistory"].is_object()) {
            title.titleHistory = TitleLastPlayedHistory::fromJson(j_title_node["titleHistory"]);
        }

        title.devices = safe_get_string_vector(j_title_node, "devices");

        return title;
    }

    TitleHistoryResponse TitleHistoryResponse::fromJson(const nj::json& j) {
        TitleHistoryResponse response;
        if (!j.is_object()) return response;

        response.xuid = safe_get_string(j, "xuid");
        if (j.contains("titles") && j["titles"].is_array()) {
            for (const auto& title_json : j["titles"]) {
                response.titles.push_back(OnlineTitleInfo::fromJson(title_json));
            }
        }
        return response;
    }

} // namespace Xbox
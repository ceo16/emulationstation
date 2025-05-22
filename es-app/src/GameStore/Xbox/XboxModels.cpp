#include "GameStore/Xbox/XboxModels.h"
#include "Log.h" 
#include "utils/TimeUtil.h" 
#include "utils/StringUtil.h"
#include <iomanip>      
#include <sstream>  
#include <set> //     

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

    // Non loggare l'intero JSON qui a meno che non sia strettamente per debug temporaneo,
    // può essere molto verboso.
    // LOG(LogDebug) << "TitleDetail::fromJson - JSON grezzo ricevuto: " << j_detail_object.dump(2);

    parsed_detail.description = safe_get_string(j_detail_object, "description");
    parsed_detail.publisherName = safe_get_string(j_detail_object, "publisherName");
    parsed_detail.developerName = safe_get_string(j_detail_object, "developerName");
    parsed_detail.minAge = safe_get_int(j_detail_object, "minAge");

    std::string pc_specific_product_id = "";
    std::string first_product_id_fallback = "";

    if (j_detail_object.contains("availabilities") && j_detail_object["availabilities"].is_array()) {
        const auto& availabilities_array = j_detail_object["availabilities"];
        for (const auto& availability_entry : availabilities_array) {
            if (!availability_entry.is_object()) continue;

            if (first_product_id_fallback.empty()) { // Salva il primo come fallback
                first_product_id_fallback = safe_get_string(availability_entry, "ProductId");
            }

            if (availability_entry.contains("Platforms") && availability_entry["Platforms"].is_array()) {
                for (const auto& platform_node : availability_entry["Platforms"]) {
                    if (platform_node.is_string() && platform_node.get<std::string>() == "PC") {
                        pc_specific_product_id = safe_get_string(availability_entry, "ProductId");
                        LOG(LogDebug) << "TitleDetail::fromJson - Trovato ProductId specifico PC: [" << pc_specific_product_id << "]";
                        goto found_pc_product_id_label; // Trovato, esci dai cicli
                    }
                }
            }
        }
    found_pc_product_id_label:;
    } else {
        LOG(LogWarning) << "TitleDetail::fromJson: Chiave 'availabilities' non trovata o non è un array nel JSON detail.";
    }

    if (!pc_specific_product_id.empty()) {
        parsed_detail.productId = pc_specific_product_id;
    } else if (!first_product_id_fallback.empty()) {
        parsed_detail.productId = first_product_id_fallback;
        LOG(LogWarning) << "TitleDetail::fromJson - Nessun ProductId PC specifico trovato. Usato il primo ProductId disponibile: [" << parsed_detail.productId << "]";
    } else {
        // Fallback estremo se 'availabilities' non c'è o è vuoto
        parsed_detail.productId = safe_get_string(j_detail_object, "ProductId");
        if (parsed_detail.productId.empty()) {
            parsed_detail.productId = safe_get_string(j_detail_object, "productId"); // Prova lowercase
        }
        if (!parsed_detail.productId.empty()) {
             LOG(LogWarning) << "TitleDetail::fromJson: ProductId estratto direttamente da j_detail_object (fallback estremo): [" << parsed_detail.productId << "]";
        }
    }
    
    if (parsed_detail.productId.empty()) {
        LOG(LogError) << "TitleDetail::fromJson: ProductId finale NON TROVATO.";
    } else {
        LOG(LogInfo) << "TitleDetail::fromJson - ProductId finale assegnato a TitleDetail: [" << parsed_detail.productId << "]";
    }

    // Gestione ReleaseDate (la vostra logica esistente sembra ok, ma assicuratevi che Utils::Time::timeToISO8601 esista o usate la vostra logica put_time)
    if (j_detail_object.contains("releaseDate")) {
        if (j_detail_object["releaseDate"].is_string()) {
            parsed_detail.releaseDate = j_detail_object["releaseDate"].get<std::string>();
        } else if (j_detail_object["releaseDate"].is_object() && j_detail_object["releaseDate"].contains("$date")) {
            long long ms = 0;
            // ... (vostra logica per estrarre ms da $date) ...
            if (j_detail_object["releaseDate"]["$date"].is_number()) {
                ms = j_detail_object["releaseDate"]["$date"].get<long long>();
            } else if (j_detail_object["releaseDate"]["$date"].is_string()) {
                try { ms = std::stoll(j_detail_object["releaseDate"]["$date"].get<std::string>()); } catch (...) { ms = 0; }
            }
            if (ms > 0) {
                // Esempio usando Utils::Time se avete una funzione per convertire epoch ms in ISO string
                // parsed_detail.releaseDate = Utils::Time::epochMsToISO8601(ms); 
                // Altrimenti, la vostra logica con put_time:
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
            } else { parsed_detail.releaseDate = ""; }
        } else { parsed_detail.releaseDate = ""; }
    } else { parsed_detail.releaseDate = ""; }

    return parsed_detail;
}

    TitleLastPlayedHistory TitleLastPlayedHistory::fromJson(const nj::json& j) {
        TitleLastPlayedHistory history;
        if (!j.is_object()) return history;
        history.lastTimePlayed = safe_get_string(j, "lastTimePlayed"); 
        return history;
    }

   OnlineTitleInfo OnlineTitleInfo::fromJson(const nj::json& j_title_node) {
    OnlineTitleInfo title;
    if (!j_title_node.is_object()) return title;

    title.name = safe_get_string(j_title_node, "name");
    LOG(LogDebug) << "OnlineTitleInfo::fromJson - Parsing title: " << title.name;

    title.titleId = safe_get_string(j_title_node, "titleId");
    title.pfn = safe_get_string(j_title_node, "pfn");
    title.type = safe_get_string(j_title_node, "type");
    title.windowsPhoneProductId = safe_get_string(j_title_node, "windowsPhoneProductId");
    title.modernTitleId = safe_get_string(j_title_node, "modernTitleId");
    title.mediaItemType = safe_get_string(j_title_node, "mediaItemType");
    title.minutesPlayed = safe_get_string(j_title_node, "minutesPlayed");

    // Popola title.detail CHIAMANDO la TitleDetail::fromJson modificata
    if (j_title_node.contains("detail") && j_title_node["detail"].is_object()) {
        LOG(LogDebug) << "OnlineTitleInfo::fromJson - Calling TitleDetail::fromJson for title: " << title.name;
        title.detail = TitleDetail::fromJson(j_title_node["detail"]);
    } else {
        LOG(LogWarning) << "OnlineTitleInfo::fromJson: Oggetto 'detail' non trovato o non è un oggetto nel JSON del titolo: " << title.name;
    }

    // Popola CORRETTAMENTE title.devices basandosi sulle "availabilities" DENTRO "detail"
    title.devices.clear();
    std::set<std::string> found_platforms_set; // Usiamo un set per evitare duplicati

    if (j_title_node.contains("detail") && j_title_node["detail"].is_object() &&
        j_title_node["detail"].contains("availabilities") && j_title_node["detail"]["availabilities"].is_array()) {
        
        for (const auto& availability_entry : j_title_node["detail"]["availabilities"]) {
            if (availability_entry.contains("Platforms") && availability_entry["Platforms"].is_array()) {
                for (const auto& platform_node : availability_entry["Platforms"]) {
                    if (platform_node.is_string()) {
                        found_platforms_set.insert(platform_node.get<std::string>());
                    }
                }
            }
        }
        for(const std::string& pf_str : found_platforms_set) {
            title.devices.push_back(pf_str); // Aggiungi al vettore finale
        }
    } else {
         LOG(LogWarning) << "OnlineTitleInfo::fromJson - No 'availabilities' in 'detail' for title: " << title.name << ". Attempting to use top-level 'devices' field.";
         // Fallback alla vostra logica originale se "availabilities" non è presente in "detail"
         title.devices = safe_get_string_vector(j_title_node, "devices");
    }
    
    if (!title.devices.empty()){
         LOG(LogInfo) << "OnlineTitleInfo::fromJson - Title '" << title.name << "' (PFN: " << title.pfn 
                      << ") processed. Identified devices: [" << Utils::String::vectorToCommaString(title.devices) 
                      << "]. PC ProductID in detail: [" << title.detail.productId << "]";
    } else {
         LOG(LogWarning) << "OnlineTitleInfo::fromJson - Title '" << title.name << "' (PFN: " << title.pfn 
                         << ") has no devices listed after parsing all sources.";
    }

    if (j_title_node.contains("titleHistory") && j_title_node["titleHistory"].is_object()) {
        title.titleHistory = TitleLastPlayedHistory::fromJson(j_title_node["titleHistory"]);
    }
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
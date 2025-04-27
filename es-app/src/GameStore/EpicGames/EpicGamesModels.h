#pragma once
#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H

#include <string>
#include <vector>
#include <map>
#include "../../json.hpp" // Include nlohmann/json per parsing
#include "../../Log.h"      // Per LOG

// Usiamo un namespace per evitare conflitti di nomi
namespace EpicGames
{
    // Da AssetsResponse.cs
    struct Asset {
        std::string appName;
        std::string labelName;
        std::string buildVersion;
        std::string catalogItemId;
        std::string ns; // 'namespace' è una keyword C++, usiamo 'ns'
        std::string assetId;

        // Helper per il parsing JSON (implementazione in .cpp o inline se semplice)
        static Asset fromJson(const nlohmann::json& j);
    };

    // Da CatalogResponse.cs
    struct Image {
        std::string url;
        std::string type;
        int width = 0;
        int height = 0;

        static Image fromJson(const nlohmann::json& j);
    };

    struct Category {
        std::string path;

        static Category fromJson(const nlohmann::json& j);
    };

    struct ReleaseInfo {
        std::string appId;
        std::vector<std::string> platform;
        std::string dateAdded; // Data come stringa ISO 8601 Z

        static ReleaseInfo fromJson(const nlohmann::json& j);
    };

    struct CustomAttribute {
        std::string key;   // Chiave dell'attributo (es. "publisherName")
        std::string type;  // Tipo di dato (es. "STRING")
        std::string value; // Valore

        // La risposta JSON è una mappa, la convertiamo in un vettore nel parsing
        static CustomAttribute fromJson(const std::string& k, const nlohmann::json& j_val);
    };

    struct CatalogItem {
        std::string id; // Dovrebbe corrispondere al CatalogItemId
        std::string title;
        std::string description;
        std::vector<Image> keyImages;
        std::vector<Category> categories;
        std::string ns; // namespace
        std::string status;
        std::string creationDate;
        std::string lastModifiedDate;
        std::vector<CustomAttribute> customAttributes; // Convertito da mappa
        std::string entitlementName;
        std::string entitlementType;
        std::string itemType;
        std::vector<ReleaseInfo> releaseInfo;
        std::string developer; // Campo diretto, se presente
        std::string developerId;
        bool endOfSupport = false;

        // Campi derivati popolati dopo il parsing iniziale
        std::string publisher; // Da cercare in customAttributes
        std::string releaseDate; // Da cercare/parsare da releaseInfo o customAttributes

        static CatalogItem fromJson(const nlohmann::json& j);
    };

    // Helper per fare il parsing in modo sicuro
    template<typename T>
    T safe_get(const nlohmann::json& j, const char* key, T default_value) {
        if (j.contains(key) && !j.at(key).is_null()) {
            try {
                return j.at(key).get<T>();
            } catch (const nlohmann::json::exception& e) {
                 LOG(LogError) << "JSON parsing error for key '" << key << "': " << e.what();
                return default_value;
            }
        }
        return default_value;
    }

} // namespace EpicGames

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H
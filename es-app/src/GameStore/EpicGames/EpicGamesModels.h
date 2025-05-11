#pragma once
#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>      // <<< AGGIUNTO: Per std::exception
#include "utils/StringUtil.h" // <<< AGGIUNTO: Se safe_get o il parsing lo usano
#include "../../json.hpp"  // Include nlohmann/json per parsing
#include "../../Log.h"       // Per LOG

// Usiamo un namespace per evitare conflitti di nomi
namespace EpicGames
{
    // --- Definizione Helper safe_get (messa qui per essere usabile sotto) ---
    // Helper template per ottenere valori JSON in modo sicuro
    template<typename T>
    T safe_get(const nlohmann::json& j, const std::string& key, const T& default_value) {
        if (j.contains(key) && !j.at(key).is_null()) {
            try {
                return j.at(key).get<T>();
            } catch (const nlohmann::json::exception& e) {
                // Usiamo LogWarning invece di LogError per errori di parsing di campo singolo
                LOG(LogWarning) << "JSON safe_get warning for key '" << key << "': " << e.what();
                return default_value;
            }
        }
        return default_value;
    }
    // Specializzazione per const char* / std::string
    template<>
    inline std::string safe_get<std::string>(const nlohmann::json& j, const std::string& key, const std::string& default_value) {
         if (j.contains(key)) {
            const auto& val = j.at(key);
            if (val.is_string()) {
                return val.get<std::string>();
            } else if (!val.is_null()) {
                 try { return val.dump(); } catch (...) { return default_value; }
            }
         }
         return default_value;
    }


    // --- Struct Asset ---
    struct Asset {
        std::string appName;
        std::string labelName;
        std::string buildVersion;
        std::string catalogItemId;
        std::string ns; // 'namespace' è una keyword C++, usiamo 'ns'
        std::string assetId;
        std::string offerId; // <<< AGGIUNTO: Sembra utile dal parsing precedente

        // *** Definizione INLINE della funzione fromJson ***
        static inline Asset fromJson(const nlohmann::json& j) {
            Asset asset;
            // Usa la funzione safe_get definita sopra
            asset.appName       = safe_get<std::string>(j, "appName", "");
            asset.labelName     = safe_get<std::string>(j, "labelName", "");
            asset.buildVersion  = safe_get<std::string>(j, "buildVersion", "");
            asset.catalogItemId = safe_get<std::string>(j, "catalogItemId", "");
            asset.ns            = safe_get<std::string>(j, "namespace", ""); // Chiave JSON è "namespace"
            asset.assetId       = safe_get<std::string>(j, "assetId", "");
            asset.offerId       = safe_get<std::string>(j, "offerId", ""); // Campo aggiunto

            // Log se mancano dati essenziali
            if (asset.appName.empty()) LOG(LogWarning) << "Asset::fromJson: JSON missing 'appName'";
            // Il log per catalogItemId/ns è spesso troppo verboso, commentato
            // if (asset.catalogItemId.empty()) LOG(LogWarning) << "Asset::fromJson: Missing 'catalogItemId' for appName: " << asset.appName;
            // if (asset.ns.empty()) LOG(LogWarning) << "Asset::fromJson: Missing 'namespace' for appName: " << asset.appName;
            return asset;
        }
    };

    // --- Struct Image ---
    struct Image {
        std::string url;
        std::string type;
        int width = 0;
        int height = 0;
        // Aggiungi md5 se presente nell'API e utile
        // std::string md5;

        // *** Definizione INLINE della funzione fromJson ***
        static inline Image fromJson(const nlohmann::json& j) {
            Image img;
            img.url    = safe_get<std::string>(j, "url", "");
            img.type   = safe_get<std::string>(j, "type", "");
            img.width  = safe_get<int>(j, "width", 0);
            img.height = safe_get<int>(j, "height", 0);
            // img.md5 = safe_get<std::string>(j, "md5", ""); // Se necessario
            return img;
        }
    };

    // --- Struct Category ---
    struct Category {
        std::string path;

        // *** Definizione INLINE della funzione fromJson ***
        static inline Category fromJson(const nlohmann::json& j) {
            Category cat;
            cat.path = safe_get<std::string>(j, "path", "");
            return cat;
        }
    };

    // --- Struct ReleaseInfo ---
    struct ReleaseInfo {
        std::string appId;
        std::vector<std::string> platform;
        std::string dateAdded; // Data come stringa ISO 8601 Z

        // *** Definizione INLINE della funzione fromJson ***
        static inline ReleaseInfo fromJson(const nlohmann::json& j) {
            ReleaseInfo info;
            info.appId = safe_get<std::string>(j, "appId", "");
            if (j.contains("platform") && j.at("platform").is_array()) {
                 try { info.platform = j.at("platform").get<std::vector<std::string>>(); }
                 catch (...) { info.platform = {}; LOG(LogWarning) << "ReleaseInfo::fromJson: Failed to parse 'platform' array."; }
            } else { info.platform = {}; }
            info.dateAdded = safe_get<std::string>(j, "dateAdded", "");
            return info;
        }
    };

    // --- Struct CustomAttribute ---
    struct CustomAttribute {
        std::string key;
        std::string type;
        std::string value;

        // *** Definizione INLINE della funzione fromJson ***
        // Nota: Riceve chiave e valore separati perché nel JSON originale è una mappa
        static inline CustomAttribute fromJson(const std::string& k, const nlohmann::json& j_val) {
             CustomAttribute attr;
             attr.key = k;
             attr.type = safe_get<std::string>(j_val, "type", "UNKNOWN");
             if (j_val.contains("value")) {
                 const auto& valueField = j_val["value"];
                 if (valueField.is_string()) { attr.value = valueField.get<std::string>(); }
                 else if (valueField.is_boolean()) { attr.value = valueField.get<bool>() ? "true" : "false"; }
                 else if (valueField.is_number()) { attr.value = valueField.dump(); }
                 else if (valueField.is_null()){ attr.value = ""; }
                 else { attr.value = valueField.dump(); } // Dump per array/object
             } else {
                 attr.value = "";
             }
             return attr;
        }
    };

    // --- Struct CatalogItem ---
    struct CatalogItem {
        std::string id;
        std::string title;
        std::string description;
        std::vector<Image> keyImages;
        std::vector<Category> categories;
        std::string ns; // namespace
        std::string status;
        std::string creationDate;
        std::string lastModifiedDate;
        std::vector<CustomAttribute> customAttributes;
        std::string entitlementName;
        std::string entitlementType;
        std::string itemType;
        std::vector<ReleaseInfo> releaseInfo;
        std::string developer;
        std::string developerId;
        bool endOfSupport = false;

        // Campi derivati popolati dopo il parsing iniziale
        std::string publisher;
        std::string releaseDate;
		std::string fanartUrl;          // URL per la migliore immagine di sfondo/hero
             std::string boxartUrl;          // URL per la boxart principale (verticale/tall)
             std::string bannerUrl;          // URL per un banner/immagine orizzontale (potrebbe essere la thumbnail)
             std::string logoUrl;            // URL per il logo del gioco (se trovato)
             std::string thumbnailUrl;       // URL per la thumbnail (spesso una keyImage di tipo "Thumbnail")
             std::string videoUrl;           // URL per il video principale (es. mp4)
             std::vector<std::string> screenshotUrls; // Lista degli URL degli screenshot full-size

        // *** Definizione INLINE della funzione fromJson ***
        static inline CatalogItem fromJson(const nlohmann::json& j) {
             CatalogItem item;
             item.id                 = safe_get<std::string>(j, "id", "");
             item.title              = safe_get<std::string>(j, "title", "");
             item.description        = safe_get<std::string>(j, "description", "");
             item.ns                 = safe_get<std::string>(j, "namespace", "");
             item.status             = safe_get<std::string>(j, "status", "");
             item.creationDate       = safe_get<std::string>(j, "creationDate", "");
             item.lastModifiedDate   = safe_get<std::string>(j, "lastModifiedDate", "");
             item.entitlementName    = safe_get<std::string>(j, "entitlementName", "");
             item.entitlementType    = safe_get<std::string>(j, "entitlementType", "");
             item.itemType           = safe_get<std::string>(j, "itemType", "");
             item.developer          = safe_get<std::string>(j, "developerDisplayName", ""); // Prova prima questo
             item.developerId        = safe_get<std::string>(j, "developerId", "");
             item.endOfSupport       = safe_get<bool>(j, "endOfSupport", false);


             if (j.contains("keyImages") && j.at("keyImages").is_array()) {
                 for (const auto& imgJson : j.at("keyImages")) {
                     try { item.keyImages.push_back(Image::fromJson(imgJson)); }
                     catch(const std::exception& e) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse an image for item " << item.id << ": " << e.what(); }
                     catch(...) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse an image for item " << item.id << " (unknown exception)"; }
                 }
             }

             if (j.contains("categories") && j.at("categories").is_array()) {
                 for (const auto& catJson : j.at("categories")) {
                      try { item.categories.push_back(Category::fromJson(catJson)); }
                      catch(const std::exception& e) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse a category for item " << item.id << ": " << e.what(); }
                      catch(...) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse a category for item " << item.id << " (unknown exception)"; }
                 }
             }

             if (j.contains("releaseInfo") && j.at("releaseInfo").is_array()) {
                 for (const auto& relJson : j.at("releaseInfo")) {
                      try { item.releaseInfo.push_back(ReleaseInfo::fromJson(relJson)); }
                      catch(const std::exception& e) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse a releaseInfo for item " << item.id << ": " << e.what(); }
                      catch(...) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse a releaseInfo for item " << item.id << " (unknown exception)"; }
                 }
             }

             item.publisher = ""; item.releaseDate = ""; // Inizializza
             if (j.contains("customAttributes") && j.at("customAttributes").is_object()) {
                 for (auto const& [key, val] : j.at("customAttributes").items()) {
                      try {
                          CustomAttribute attr = CustomAttribute::fromJson(key, val);
                          item.customAttributes.push_back(attr);
                          std::string lowerKey = Utils::String::toLower(key); // Usa utility se disponibile
                          if (item.publisher.empty() && (lowerKey == "publishername" || lowerKey == "publisher")) { item.publisher = attr.value; }
                          if (item.releaseDate.empty() && (lowerKey == "releasedate" || lowerKey == "pcreleasedate")) { item.releaseDate = attr.value; }
                           // Fallback per developer se non trovato direttamente sopra
                          if (item.developer.empty() && (lowerKey == "developername" || lowerKey == "developer")) { item.developer = attr.value; }

                      } catch(const std::exception& e) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse custom attribute '" << key << "' for item " << item.id << ": " << e.what(); }
                        catch(...) { LOG(LogWarning) << "CatalogItem::fromJson: Failed to parse custom attribute '" << key << "' for item " << item.id << " (unknown exception)"; }
                 }
             }

              // Fallback per data rilascio da releaseInfo se non trovata negli attributi custom
             if (item.releaseDate.empty() && !item.releaseInfo.empty()) {
                 std::string firstDate = "";
                 for(const auto& ri : item.releaseInfo) {
                     bool isWindows = false;
                     for(const auto& p : ri.platform) { if (p == "Windows") { isWindows = true; break; } }
                     if (firstDate.empty() && !ri.dateAdded.empty()) { firstDate = ri.dateAdded; }
                     if(isWindows && !ri.dateAdded.empty()) { item.releaseDate = ri.dateAdded; break; }
                 }
                 if(item.releaseDate.empty()) { item.releaseDate = firstDate; }
             }
             return item;
        }
    };

} // namespace EpicGames

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESMODELS_H
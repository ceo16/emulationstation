// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesModels.h
#pragma once

#include <string>
#include <vector>
#include "json.hpp"
#include "utils/StringUtil.h" 
#include "utils/base64.h"
#include "Log.h" // Aggiunto per LOG da dentro i parser

namespace EAGames
{
    struct AuthTokenResponse {
        std::string access_token;
        std::string token_type;
        long expires_in = 0;
        std::string refresh_token;
        std::string id_token;
        std::string pid_id; // Persona ID (estratto da id_token)

        static AuthTokenResponse fromJson(const nlohmann::json& j) {
            AuthTokenResponse res;
            if (j.contains("access_token") && j.at("access_token").is_string())
                res.access_token = j.at("access_token").get<std::string>();
            if (j.contains("token_type") && j.at("token_type").is_string())
                res.token_type = j.at("token_type").get<std::string>();
            if (j.contains("expires_in") && j.at("expires_in").is_number())
                res.expires_in = j.at("expires_in").get<long>();
            if (j.contains("refresh_token") && j.at("refresh_token").is_string())
                res.refresh_token = j.at("refresh_token").get<std::string>();
            
            if (j.contains("id_token") && j.at("id_token").is_string()) {
                res.id_token = j.at("id_token").get<std::string>();
                std::vector<std::string> parts = Utils::String::split(res.id_token, '.');
                if (parts.size() >= 2) {
                    try {
                        std::string decoded_payload = ::base64_decode(parts[1]);
                        auto payload_json = nlohmann::json::parse(decoded_payload);
                        // Il PID è nel token id come "pid_id" o talvolta "sub" o "user_id" a seconda del flusso OAuth.
                        // Per EA Juno, il PID che ci interessa è di solito `pid` o `sub` (subject).
                        // Dal tuo Auth.cpp, sembra che tu lo prenda da GraphQL separatamente.
                        // Se l'id_token contiene il PID, eccolo:
                        if (payload_json.contains("pid") && payload_json.at("pid").is_string()) { // EA potrebbe usare 'pid'
                             res.pid_id = payload_json.at("pid").get<std::string>();
                        } else if (payload_json.contains("sub") && payload_json.at("sub").is_string()){ // Standard OIDC
                             res.pid_id = payload_json.at("sub").get<std::string>();
                        }
                        // Potresti anche loggare `decoded_payload` per vedere tutti i campi disponibili.
                    } catch (const std::exception& e) {
                         LOG(LogError) << "EAGamesModels: Failed to parse PID from id_token: " << e.what();
                    }
                }
            }
            return res;
        }
    };

    struct GameEntitlement {
        // Campi dal vecchio formato (alcuni potrebbero non essere più usati direttamente o rinominati)
        // std::string entitlementType; // Sostituito da product.baseItem.gameType
        // std::string masterTitleId;   // Potrebbe essere ancora utile per alcune logiche, ma non direttamente nella risposta GraphQL per "ownedGameProducts" item
        // std::string offerPath;       // Non presente direttamente

        // Campi basati sulla nuova risposta GraphQL per "ownedGameProducts"
        std::string originOfferId;      // Era offerId
        std::string productId;          // Da product.id
        std::string title;              // Da product.name (era name)
        std::string gameSlug;           // Da product.gameSlug
        std::string gameType;           // Da product.baseItem.gameType
        std::vector<std::string> ownershipMethods; // Da product.gameProductUser.ownershipMethods
        std::string entitlementId_Juno; // Da product.gameProductUser.entitlementId (rinominato per evitare collisioni se avevi un altro entitlementId)
        
        // Campi originali che potresti voler mantenere se usati altrove, ma potrebbero non essere popolati da questa specifica API
        std::string grantDate;          // Non presente nella risposta GraphQL mostrata
        std::string status;             // Non presente direttamente per l'item, ma la query filtra per ACTIVE

        static GameEntitlement fromJson(const nlohmann::json& itemJson) {
            GameEntitlement ge;
            try {
                ge.originOfferId = itemJson.value("originOfferId", "");

                if (itemJson.contains("product") && itemJson["product"].is_object()) {
                    const auto& productJson = itemJson["product"];
                    ge.productId = productJson.value("id", "");
                    ge.title = productJson.value("name", "");
                    ge.gameSlug = productJson.value("gameSlug", "");

                    if (productJson.contains("baseItem") && productJson["baseItem"].is_object()) {
                        ge.gameType = productJson["baseItem"].value("gameType", "");
                    }

                    if (productJson.contains("gameProductUser") && productJson["gameProductUser"].is_object()) {
                        const auto& gpuJson = productJson["gameProductUser"];
                        ge.entitlementId_Juno = gpuJson.value("entitlementId", "");
                        if (gpuJson.contains("ownershipMethods") && gpuJson["ownershipMethods"].is_array()) {
                            for (const auto& methodNode : gpuJson["ownershipMethods"]) {
                                if (methodNode.is_string()) {
                                    ge.ownershipMethods.push_back(methodNode.get<std::string>());
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EAGamesModels: Exception parsing GameEntitlement item: " << e.what() 
                              << " - JSON item: " << itemJson.dump(2);
            }
            return ge;
        }
    };

    struct AccountEntitlementsResponse {
        std::vector<GameEntitlement> entitlements;

        static AccountEntitlementsResponse fromJson(const nlohmann::json& jsonResponse) {
            AccountEntitlementsResponse aer;
            try {
                // Naviga fino all'array "items"
                if (jsonResponse.contains("data") &&
                    jsonResponse["data"].is_object() &&
                    jsonResponse["data"].contains("me") &&
                    jsonResponse["data"]["me"].is_object() &&
                    jsonResponse["data"]["me"].contains("ownedGameProducts") &&
                    jsonResponse["data"]["me"]["ownedGameProducts"].is_object() &&
                    jsonResponse["data"]["me"]["ownedGameProducts"].contains("items") &&
                    jsonResponse["data"]["me"]["ownedGameProducts"]["items"].is_array()) {

                    const auto& itemsArray = jsonResponse["data"]["me"]["ownedGameProducts"]["items"];
                    for (const auto& itemNode : itemsArray) {
                        GameEntitlement ent = GameEntitlement::fromJson(itemNode);
                        
                        // Filtra qui se necessario, ad esempio per gameType == "BASE_GAME"
                        // La query GraphQL già filtra per type:[DIGITAL_FULL_GAME, PACKAGED_FULL_GAME,...]
                        // ma un controllo aggiuntivo qui può essere utile.
                        // Ad esempio, se vuoi solo i giochi base:
                        if (!ent.gameType.empty() && (ent.gameType == "BASE_GAME" || ent.gameType == "Default") ) { // "Default" a volte è usato da EA per giochi base
                           if (!ent.title.empty() && !ent.originOfferId.empty()) { // Assicurati che ci siano dati minimi
                                aer.entitlements.push_back(ent);
                            } else {
                                LOG(LogWarning) << "EAGamesModels: Saltato GameEntitlement con titolo o originOfferId mancante durante il parsing dell'array. Titolo: '" << ent.title << "', OfferId: '" << ent.originOfferId << "'";
                            }
                        } else {
                             LOG(LogDebug) << "EAGamesModels: Saltato GameEntitlement con gameType non base: '" << ent.gameType << "' per il gioco '" << ent.title << "'";
                        }
                    }
                } else {
                    LOG(LogError) << "EAGamesModels (AccountEntitlementsResponse): Struttura JSON inattesa o 'items' mancante per ownedGameProducts. JSON ricevuto (parziale): " << jsonResponse.dump(2).substr(0, 500);
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EAGamesModels (AccountEntitlementsResponse): Eccezione durante il parsing: " << e.what();
            }
            return aer;
        }
    };
    
    // Le struct StorePageMetadataOffers, GameStoreData, InstalledGameInfo, EADesktopSettings
    // rimangono come nel tuo file originale, poiché non sono direttamente coinvolte
    // nel parsing della risposta di `getOwnedGames` dall'endpoint GraphQL.
    // Se anche queste devono essere aggiornate per nuovi endpoint, sarà un passo successivo.

    struct StorePageMetadataOffers {
        std::string offerId;
        std::string offerType;
        // ... (come nel tuo file originale) ...
    };

    struct GameStoreData {
        std::string offerId;
        std::string masterTitleId;
        std::string title;
        std::string description;
        std::string developer;
        std::string publisher;
        std::string releaseDate;
        std::string imageUrl;
        std::string backgroundImageUrl;
        std::vector<std::string> genres;
        std::vector<StorePageMetadataOffers> offers;
        bool isGame = false;

        static GameStoreData fromJson(const nlohmann::json& j, const std::string& country = "US", const std::string& language = "en_US") {
            // ... (mantieni la tua implementazione originale di GameStoreData::fromJson) ...
            // Questa implementazione è complessa e specifica per gli endpoint di store metadata,
            // che potrebbero essere diversi da quelli di entitlement.
            // Per brevità, la ometto qui, ma deve rimanere nel tuo file.
            // Se usi questa struct per i dati dei giochi dalla lista entitlements,
            // dovrai adattare GameEntitlement::fromJson per popolare una GameStoreData
            // o avere una funzione di conversione.
            // Per ora, ci concentriamo su GameEntitlement e AccountEntitlementsResponse.
            GameStoreData gsd; // Implementazione placeholder
             const nlohmann::json* gameDetailsNode = &j;
             if (j.contains("metadata") && j.is_object() && !j.at("metadata").is_null()) {
                 gameDetailsNode = &j.at("metadata");
                 if (j.contains("offers") && j.at("offers").is_array()) {
                     for (const auto& offerNode : j.at("offers")) {
                         StorePageMetadataOffers offer;
                         if (offerNode.contains("offerId") && offerNode.at("offerId").is_string())
                             offer.offerId = offerNode.at("offerId").get<std::string>();
                         if (offerNode.contains("offerType") && offerNode.at("offerType").is_string())
                             offer.offerType = offerNode.at("offerType").get<std::string>();
                         gsd.offers.push_back(offer);
                     }
                 }
             }
             if (gameDetailsNode->contains("offerId") && gameDetailsNode->at("offerId").is_string())
                 gsd.offerId = gameDetailsNode->at("offerId").get<std::string>();
             if (gameDetailsNode->contains("masterTitleId") && gameDetailsNode->at("masterTitleId").is_string())
                 gsd.masterTitleId = gameDetailsNode->at("masterTitleId").get<std::string>();
             if (gameDetailsNode->contains("i18n")) {
                 const auto& i18n = gameDetailsNode->at("i18n");
                 if (i18n.contains("displayName") && i18n.at("displayName").is_string())
                     gsd.title = i18n.at("displayName").get<std::string>();
                 if (i18n.contains("longDescription") && i18n.at("longDescription").is_string())
                     gsd.description = i18n.at("longDescription").get<std::string>();
                 else if (i18n.contains("description") && i18n.at("description").is_string())
                     gsd.description = i18n.at("description").get<std::string>();
             }
             if (gsd.title.empty() && gameDetailsNode->contains("itemName") && gameDetailsNode->at("itemName").is_string())
                 gsd.title = gameDetailsNode->at("itemName").get<std::string>();
             if (gameDetailsNode->contains("customAttributes")) {
                 const auto& ca = gameDetailsNode->at("customAttributes");
                 if (ca.contains("developerFacetKey") && ca.at("developerFacetKey").is_array() && !ca.at("developerFacetKey").empty())
                     gsd.developer = ca.at("developerFacetKey")[0].get<std::string>();
                 if (ca.contains("publisherFacetKey") && ca.at("publisherFacetKey").is_array() && !ca.at("publisherFacetKey").empty())
                     gsd.publisher = ca.at("publisherFacetKey")[0].get<std::string>();
                 if (ca.contains("genreFacetKey") && ca.at("genreFacetKey").is_array()) {
                     for (const auto& genreNode : ca.at("genreFacetKey")) {
                         if (genreNode.is_string()) gsd.genres.push_back(genreNode.get<std::string>());
                     }
                 }
                 if (ca.contains("imageInfo") && ca.at("imageInfo").is_array()) {
                     std::string imageServerBase = gameDetailsNode->value("imageServer", "");
                     if (imageServerBase.empty() && gameDetailsNode->contains("imageServer") && gameDetailsNode->at("imageServer").is_string()) { 
                         imageServerBase = gameDetailsNode->at("imageServer").get<std::string>();
                     }
                     for (const auto& imgInfoNode : ca.at("imageInfo")) {
                         std::string type = imgInfoNode.value("type", "");
                         std::string name = imgInfoNode.value("name", "");
                         if (!name.empty() && !imageServerBase.empty()) {
                             if ((type == "packart" || type == "boxart") && gsd.imageUrl.empty())
                                 gsd.imageUrl = imageServerBase + "/" + name;
                             else if (type == "backgroundImage" && gsd.backgroundImageUrl.empty())
                                 gsd.backgroundImageUrl = imageServerBase + "/" + name;
                         }
                     }
                 }
             }
             if (gameDetailsNode->contains("publishing") && gameDetailsNode->at("publishing").is_object()) {
                  const auto& pub = gameDetailsNode->at("publishing");
                  if (pub.contains("softwareList") && pub.at("softwareList").is_object()) {
                     const auto& sl = pub.at("softwareList");
                     if (sl.contains("software") && sl.at("software").is_array() && !sl.at("software").empty()) {
                          const auto& software = sl.at("software")[0];
                          if (software.contains("releaseDate") && software.at("releaseDate").is_string())
                               gsd.releaseDate = software.at("releaseDate").get<std::string>();
                     }
                  }
             }
             std::string productType = gameDetailsNode->value("productType", "");
             std::string offerType = gameDetailsNode->value("offerType", "");
             if (productType == "Base Game" || productType == "Project Titan") {
                 gsd.isGame = true;
             } else if (!offerType.empty()) {
                 std::string offerTypeLower = Utils::String::toLower(offerType);
                 if (offerTypeLower.find("edition") != std::string::npos || offerTypeLower.find("basegame") != std::string::npos || offerTypeLower.find("base_game") != std::string::npos) {
                     gsd.isGame = true;
                 }
             }
             if (gsd.title.empty() && (!gsd.offerId.empty() || !gsd.masterTitleId.empty())) {
                 gsd.title = "EA Game (" + (gsd.masterTitleId.empty() ? gsd.offerId : gsd.masterTitleId) + ")";
             }
            return gsd;
        }
    };

    struct InstalledGameInfo { // Mantieni come nel tuo file originale
        std::string id;
        std::string name;
        std::string installPath;
        std::string executablePath;
        std::string launchParameters;
        std::string version;
        std::string multiplayerId;
    };

    struct EADesktopSettings { // Mantieni come nel tuo file originale
        std::string HardwareId;
        std::string LastGameInstallPath;
        std::vector<std::string> ContentInstallPath;
    };

} // namespace EAGames
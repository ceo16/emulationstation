#pragma once

#include <string>
#include <vector>
#include "json.hpp"
#include "utils/StringUtil.h"
#include "utils/base64.h"
#include "Log.h"
#include "utils/TimeUtil.h"

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
        std::string originOfferId;
        std::string productId;
        std::string title;
        std::string gameSlug;
        std::string gameType;
        
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
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EAGamesModels: Exception parsing GameEntitlement item: " << e.what();
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

    struct GameStoreData {
        // --- MODIFICATO ---
        // Questa struct è PERFETTA così com'è. Rappresenta i dati
        // che otteniamo dalla chiamata di DETTAGLIO (la Fase 2).
        std::string offerId;
        std::string contentId; // <-- Contiene il dato che ci serve!
        std::string masterTitleId;
        std::string title;
        std::string description;
        std::string developer;
        std::string publisher;
        std::string releaseDate;
        std::string imageUrl;
        std::string backgroundImageUrl;
        std::vector<std::string> genres;
        bool isGame = false;

        // La funzione fromJson qui è corretta, perché legge da 'legacyOffers'
        // che è la fonte del contentId.
static GameStoreData fromJson(const nlohmann::json& j) {
            GameStoreData gsd;
             try {
                if (!j.contains("data") || !j["data"].is_object()) return gsd;
                const auto& dataNode = j["data"];
                if (dataNode.contains("legacyOffers") && dataNode["legacyOffers"].is_array() && !dataNode["legacyOffers"].empty()) {
                    const auto& legacyOffer = dataNode["legacyOffers"][0];
                    if (legacyOffer.is_object()) {
                        gsd.offerId = legacyOffer.value("offerId", "");
                        gsd.title = legacyOffer.value("displayName", "");
                        gsd.contentId = legacyOffer.value("contentId", ""); // Questo è corretto per la chiamata di dettaglio!
                    }
                }
             } catch (const std::exception& e) {
                 LOG(LogError) << "EAGamesModels (GameStoreData::fromJson): Eccezione: " << e.what();
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

 struct SubscriptionDetails {
        std::string tier;
        time_t endTime = 0;
        bool isActive = false;
        std::string level;

        static SubscriptionDetails fromJson(const nlohmann::json& subJson) {
            SubscriptionDetails details;
            try {
                std::string status = subJson.value("status", "");
                if (status == "ACTIVE") {
                    details.isActive = true;
                    details.level = subJson.value("level", "");
                    
                    if (details.level == "STANDARD") {
                        details.tier = "standard";
                    } else if (details.level == "ORIGIN_ACCESS_PREMIER") {
                        details.tier = "premium";
                    } else {
                        details.tier = details.level;
                    }

                    std::string timeStr = subJson.value("end", "");
                    if (!timeStr.empty()) {
                        // Questa riga ora compilerà correttamente
                        details.endTime = Utils::Time::stringToTime(timeStr, "%Y-%m-%dT%H:%M:%S.000Z");
                    }
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EAGamesModels: Eccezione durante il parsing di SubscriptionDetails: " << e.what();
            }
            return details;
        }
    };

     struct SubscriptionGame {
        std::string name;
        std::string offerId;
        std::string slug;
        
        static SubscriptionGame fromJson(const nlohmann::json& gameJson) {
            SubscriptionGame game;
            try {
                game.slug = gameJson.value("slug", "");
                if (gameJson.contains("products") && gameJson["products"].is_object() &&
                    gameJson["products"].contains("items") && gameJson["products"]["items"].is_array()) {
                    for (const auto& product : gameJson["products"]["items"]) {
                        std::string offerId = product.value("originOfferId", "");
                        std::string name = product.value("name", "");
                        if (!name.empty() && !offerId.empty()) {
                            game.name = name;
                            game.offerId = offerId;
                            return game;
                        }
                    }
                }
             } catch (const nlohmann::json::exception& e) {
                 LOG(LogError) << "SubscriptionGame Parser: Eccezione JSON: " << e.what();
             }
             return game;
        }
    };
}
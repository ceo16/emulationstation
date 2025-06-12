// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesModels.h
#pragma once

#include <string>
#include <vector>
#include "json.hpp"
#include "utils/StringUtil.h" 
#include "utils/base64.h"
#include "Log.h" // Aggiunto per LOG da dentro i parser
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

static GameStoreData fromJson(const nlohmann::json& j) {
    GameStoreData gsd;
    gsd.isGame = false; // Default

    try {
        if (!j.contains("data") || !j["data"].is_object()) {
            LOG(LogError) << "EAGamesModels (GameStoreData::fromJson JUNO): 'data' field missing or not an object in JUNO response.";
            return gsd; // Ritorna oggetto vuoto/invalido
        }
        const auto& dataNode = j["data"];

        // Estrai da legacyOffers (assumendo che sia un array e prendiamo il primo elemento)
        if (dataNode.contains("legacyOffers") && dataNode["legacyOffers"].is_array() && !dataNode["legacyOffers"].empty()) {
            const auto& legacyOffer = dataNode["legacyOffers"][0];

            if (legacyOffer.is_object()) { // Controllo aggiuntivo
                gsd.offerId = legacyOffer.value("offerId", ""); // Era 'id' nella query, mappato a offerId
                gsd.title = legacyOffer.value("displayName", "");
                // gsd.masterTitleId = legacyOffer.value("primaryMasterTitleId", ""); // Potresti volerlo qui

                std::string displayType = legacyOffer.value("displayType", "");
                if (displayType.find("Game") != std::string::npos || displayType.find("GAME") != std::string::npos) { // Semplice controllo
                    gsd.isGame = true;
                }

                // Tentativo di estrarre descrizioni (questi percorsi sono ipotetici per JUNO, da verificare!)
                if (legacyOffer.contains("metadata") && legacyOffer["metadata"].is_object()) {
                    const auto& metadata = legacyOffer["metadata"];
                    gsd.description = metadata.value("longDescription", "");
                    if (gsd.description.empty()) {
                        gsd.description = metadata.value("shortDescription", "");
                    }
                }
                
                // Tentativo di estrarre data, sviluppatore, editore (ipotetico!)
                if (legacyOffer.contains("publishing") && legacyOffer["publishing"].is_object()) {
                    const auto& publishing = legacyOffer["publishing"];
                    gsd.releaseDate = publishing.value("publishingDate", "");
                     if (publishing.contains("software") && publishing["software"].is_array() && !publishing["software"].empty()) {
                        const auto& software = publishing["software"][0];
                        if (software.is_object()) {
                           gsd.developer = software.value("softwareDeveloper", "");
                           gsd.publisher = software.value("softwarePublisher", "");
                        }
                    }
                }

                // Tentativo di estrarre immagini e generi (ipotetico!)
                 if (legacyOffer.contains("localizableProperties") && legacyOffer["localizableProperties"].is_object()) {
                    const auto& locProps = legacyOffer["localizableProperties"];
                    if (locProps.contains("packArt") && locProps["packArt"].is_array() && !locProps["packArt"].empty()) {
                        if (locProps["packArt"][0].is_object() && locProps["packArt"][0].contains("url")) {
                           gsd.imageUrl = locProps["packArt"][0].value("url", "");
                        }
                    }
                    if (locProps.contains("backgroundImage") && locProps["backgroundImage"].is_array() && !locProps["backgroundImage"].empty()) {
                         if (locProps["backgroundImage"][0].is_object() && locProps["backgroundImage"][0].contains("url")) {
                           gsd.backgroundImageUrl = locProps["backgroundImage"][0].value("url", "");
                        }
                    }
                    if (locProps.contains("genres") && locProps["genres"].is_array()) {
                        for (const auto& genreNode : locProps["genres"]) {
                            if (genreNode.is_object() && genreNode.contains("string") && genreNode["string"].is_string()) {
                                gsd.genres.push_back(genreNode["string"].get<std::string>());
                            } else if (genreNode.is_string()) { // Se fosse direttamente un array di stringhe
                                gsd.genres.push_back(genreNode.get<std::string>());
                            }
                        }
                    }
                }
            }
        }

        // Estrai da gameProducts (assumendo array e primo elemento)
        if (dataNode.contains("gameProducts") && dataNode["gameProducts"].is_object() &&
            dataNode["gameProducts"].contains("items") && dataNode["gameProducts"]["items"].is_array() &&
            !dataNode["gameProducts"]["items"].empty()) {

            const auto& productItem = dataNode["gameProducts"]["items"][0];
            if (productItem.is_object()) { // Controllo aggiuntivo
                // Sovrascrivi/popola il titolo se quello da gameProducts è migliore o mancante
                if (gsd.title.empty() && productItem.contains("name")) {
                    gsd.title = productItem.value("name", "");
                }
                // Popola masterTitleId dal productId di gameProducts
                gsd.masterTitleId = productItem.value("id", "");

                // Popola offerId se non già da legacyOffers o se diverso/più specifico
                if (gsd.offerId.empty() && productItem.contains("originOfferId")) {
                     gsd.offerId = productItem.value("originOfferId", "");
                }
                // Potresti voler prendere gameSlug e baseItem.title se hai campi in GameStoreData
                // std::string gameSlug = productItem.value("gameSlug", "");
            }
        }

        // Log di riepilogo dei dati parsati per debug
        LOG(LogDebug) << "GameStoreData::fromJson JUNO - Parsed: title='" << gsd.title
                      << "', offerId='" << gsd.offerId
                      << "', masterTitleId='" << gsd.masterTitleId
                      << "', isGame=" << gsd.isGame
                      << "', dev='" << gsd.developer
                      << "', pub='" << gsd.publisher
                      << "', releaseDate='" << gsd.releaseDate
                      << "', genres=" << Utils::String::vectorToCommaString(gsd.genres)
                      << "', imageURL='" << gsd.imageUrl
                      << "', bgImageURL='" << gsd.backgroundImageUrl
                      << "', desc_len=" << gsd.description.length();


    } catch (const nlohmann::json::exception& e) {
        LOG(LogError) << "EAGamesModels (GameStoreData::fromJson JUNO): Errore durante il parsing del JSON JUNO: " << e.what()
                      << " - JSON problematico (o parte): " << j.dump(2).substr(0, 1000);
        // Restituisce gsd parzialmente popolato o vuoto come indicatore di errore
    } catch (const std::exception& e) {
        LOG(LogError) << "EAGamesModels (GameStoreData::fromJson JUNO): Eccezione generica: " << e.what();
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

        if (gameJson.contains("products") && gameJson.at("products").is_object() &&
            gameJson.at("products").contains("items") && gameJson.at("products").at("items").is_array())
        {
            for (const auto& product : gameJson.at("products").at("items"))
            {
                std::string offerId = product.value("originOfferId", "");
                std::string name = product.value("name", "");

                if (!name.empty() && !offerId.empty()) {
                    game.name = name;
                    game.offerId = offerId;
                    // LOG di debug per confermare
                    LOG(LogDebug) << "SubscriptionGame Parser: Trovato gioco valido -> " << name;
                    return game;
                }
            }
        }
        // Se arriva qui, significa che non ha trovato prodotti validi per questo slug
        LOG(LogWarning) << "SubscriptionGame Parser: Nessun prodotto valido trovato per lo slug: " << game.slug;

    } catch (const nlohmann::json::exception& e) {
        LOG(LogError) << "SubscriptionGame Parser: Eccezione JSON: " << e.what();
    }
    return game; // Restituisce un gioco vuoto se non trova nulla
}
};

} // namespace EAGames
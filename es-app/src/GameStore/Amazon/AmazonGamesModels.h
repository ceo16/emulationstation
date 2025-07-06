#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_MODELS_H
#define ES_APP_GAMESTORE_AMAZON_MODELS_H

#include "json.hpp"
#include <string>
#include <vector>
#include <map>

namespace Amazon
{
    // --- DeviceRegistrationRequest (verificato e corretto) ---
    struct DeviceRegistrationRequest {
        struct RegistrationData {
            std::string app_name;
            std::string app_version;
            std::string device_model;
            std::string device_name;
            std::string device_serial;
            std::string device_type;
            std::string domain;
            std::string os_version;
        };
        struct AuthData {
            std::string access_token;
        };
        RegistrationData registration_data;
        AuthData auth_data;
        nlohmann::json user_context_map = nlohmann::json::object();
        std::vector<std::string> requested_extensions;
        std::vector<std::string> requested_token_type;
    };
    inline void to_json(nlohmann::json& j, const DeviceRegistrationRequest& p) {
        j = nlohmann::json{
            {"registration_data", {
                {"app_name", p.registration_data.app_name},
                {"app_version", p.registration_data.app_version},
                {"device_model", p.registration_data.device_model},
                {"device_name", p.registration_data.device_name},
                {"device_serial", p.registration_data.device_serial},
                {"device_type", p.registration_data.device_type},
                {"domain", p.registration_data.domain},
                {"os_version", p.registration_data.os_version}
            }},
            {"auth_data", {
                {"access_token", p.auth_data.access_token}
            }},
            {"user_context_map", p.user_context_map},
            {"requested_extensions", p.requested_extensions},
            {"requested_token_type", p.requested_token_type}
        };
    }
	
	struct TokenRefreshRequest
{
    std::string source_token_type = "refresh_token";
    std::string requested_token_type = "access_token";
    std::string source_token; // Qui andrà il nostro refresh_token
    std::string app_name = "AGSLauncher";
    std::string app_version = "1.1.133.2-9e2c3a3";
};

inline void to_json(nlohmann::json& j, const TokenRefreshRequest& p) {
    j = nlohmann::json{
        {"source_token_type", p.source_token_type},
        {"requested_token_type", p.requested_token_type},
        {"source_token", p.source_token},
        {"app_name", p.app_name},
        {"app_version", p.app_version}
    };
}

    // --- EntitlementsRequest (RISCRITTO DA ZERO PER CORRISPONDERE A PLAYNITE) ---
    struct EntitlementsRequest {
        std::string Operation = "GetEntitlements";
        std::string clientId = "Sonic";
        int syncPoint = 0;
        std::string nextToken;
        int maxResults = 500;
        std::string keyId = "d5dc8b8b-86c8-4fc4-ae93-18c0def5314d";
        std::string hardwareHash;
        bool disableStateFilter = true;
    };
    inline void to_json(nlohmann::json& j, const EntitlementsRequest& p) {
        j = nlohmann::json{
            {"Operation", p.Operation},
            {"clientId", p.clientId},
            {"syncPoint", p.syncPoint},
            {"maxResults", p.maxResults},
            {"keyId", p.keyId},
            {"hardwareHash", p.hardwareHash},
            {"disableStateFilter", p.disableStateFilter}
        };
        if (!p.nextToken.empty()) {
            j["nextToken"] = p.nextToken;
        }
    }

    // --- Strutture di Risposta (verificate e corrette) ---
    struct GameEntitlement {
        std::string id;
        std::string product_title;
        std::string product_imageUrl;
        std::string product_productLine;
    };
    struct EntitlementsResponse {
        std::vector<GameEntitlement> entitlements;
        std::string nextToken;
    };
inline void from_json(const nlohmann::json& j, GameEntitlement& p) {
    // La vecchia riga è commentata via, non serve più.
    // p.id = j.value("id", ""); 

    if (j.contains("product")) 
    {
        // CORREZIONE: Leggi l'ID e il titolo dall'oggetto "product" annidato.
        const auto& productJson = j["product"];
        p.id = productJson.value("id", ""); // <-- Legge l'ID CORRETTO (amzn1.adg.product...)
        p.product_title = productJson.value("title", "Unknown Title");
        p.product_imageUrl = productJson.value("imageUrl", "");
        p.product_productLine = productJson.value("productLine", "");
    }
    else
    {
        // Fallback nel caso improbabile che manchi l'oggetto "product"
        p.id = j.value("id", "");
        p.product_title = "Unknown Title";
    }
}
    inline void from_json(const nlohmann::json& j, EntitlementsResponse& p) {
        if (j.contains("entitlements")) {
            j.at("entitlements").get_to(p.entitlements);
        }
        p.nextToken = j.value("nextToken", "");
    }
    
    struct InstalledGameInfo {
        std::string id;
        std::string title;
        std::string installDirectory;
    };
}


#endif
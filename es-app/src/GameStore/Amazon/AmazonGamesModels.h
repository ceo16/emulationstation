#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_MODELS_H
#define ES_APP_GAMESTORE_AMAZON_MODELS_H

#include "json.hpp" // nlohmann/json
#include <string>
#include <vector>
#include <map>

namespace Amazon
{
    // Modello per la richiesta di registrazione del dispositivo (da DeviceRegistration.cs)
    struct DeviceRegistrationRequest {
        struct RegistrationData {
            std::string app_name = "AGSLauncher for Windows";
            std::string app_version = "1.0.0";
            std::string device_model = "Windows";
            std::string device_name; // Verrà popolato con il nome del computer
            std::string device_serial; // Verrà popolato con un GUID della macchina
            std::string device_type = "A2UMVHOX7UP4V7"; // Magic string da Playnite
            std::string domain = "Device";
            std::string os_version = "10.0"; // Semplificato
        };

        struct AuthData {
            std::string access_token;
        };

        RegistrationData registration_data;
        AuthData auth_data;
        nlohmann::json user_context_map = nlohmann::json::object();
        std::vector<std::string> requested_extensions = { "customer_info", "device_info" };
        std::vector<std::string> requested_token_type = { "bearer", "mac_dms" };
    };

    // Serializzatore per DeviceRegistrationRequest
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

    // Modello per la richiesta dei giochi posseduti (da Entitlements.cs)
    struct EntitlementsRequest {
        bool ownershipOnly = true;
        std::string status = "ENTITLED";
        std::string type = "PURCHASE";
        int limit = 100;
        std::string nextToken; // Per la paginazione
    };

    inline void to_json(nlohmann::json& j, const EntitlementsRequest& p) {
        j = nlohmann::json{
            {"ownershipOnly", p.ownershipOnly},
            {"status", p.status},
            {"type", p.type},
            {"limit", p.limit}
        };
        if (!p.nextToken.empty()) {
            j["nextToken"] = p.nextToken;
        }
    }

    // Modello per la risposta dei giochi posseduti
    struct GameEntitlement {
        std::string id;
        std::string product_title;
        std::string product_imageUrl;
    };

    struct EntitlementsResponse {
        std::vector<GameEntitlement> entitlements;
        std::string nextToken;
    };

    // Deserializzatori per la risposta
    inline void from_json(const nlohmann::json& j, GameEntitlement& p) {
        p.id = j.value("id", "");
        if (j.contains("product")) {
            p.product_title = j["product"].value("title", "Unknown Title");
            p.product_imageUrl = j["product"].value("imageUrl", "");
        }
    }

    inline void from_json(const nlohmann::json& j, EntitlementsResponse& p) {
        if (j.contains("entitlements")) {
            j.at("entitlements").get_to(p.entitlements);
        }
        p.nextToken = j.value("nextToken", "");
    }
    
    // Struttura per contenere i dati di un gioco installato localmente
    struct InstalledGameInfo {
        std::string id;
        std::string title;
        std::string installDirectory;
    };
}

#endif //ES_APP_GAMESTORE_AMAZON_MODELS_H
#pragma once
#ifndef ES_APP_GAMESTORE_GOG_MODELS_H
#define ES_APP_GAMESTORE_GOG_MODELS_H

#include "json.hpp"
#include <string>
#include <vector>

namespace GOG
{
    // --- Modelli per i giochi della libreria ---
    struct LibraryGame
    {
        struct GameInfo {
            std::string id;
            std::string title;
            std::string image;
        };
        GameInfo game;
    };
    
    struct LibraryGameResponse {
        struct Embedded {
            std::vector<LibraryGame> items;
        };
        Embedded _embedded;
        int pages;
        int page;
    };

    inline void from_json(const nlohmann::json& j, LibraryGame::GameInfo& p) {
        p.id = j.at("id").get<std::string>();
        p.title = j.at("title").get<std::string>();
        p.image = "https:" + j.at("image").get<std::string>();
    }
    inline void from_json(const nlohmann::json& j, LibraryGame& p) {
        j.at("game").get_to(p.game);
    }
    inline void from_json(const nlohmann::json& j, LibraryGameResponse::Embedded& p) {
        j.at("items").get_to(p.items);
    }
    inline void from_json(const nlohmann::json& j, LibraryGameResponse& p) {
        j.at("_embedded").get_to(p._embedded);
        j.at("pages").get_to(p.pages);
        j.at("page").get_to(p.page);
    }

    // --- Modello per le informazioni dell'account ---
    struct AccountInfo {
        bool isLoggedIn;
        std::string username;
    };
    inline void from_json(const nlohmann::json& j, AccountInfo& p) {
        p.isLoggedIn = j.value("isLoggedIn", false);
        p.username = j.value("username", "");
    }

    // --- Struttura per i giochi installati ---
    struct InstalledGameInfo {
        std::string id;
        std::string name;
        std::string installDirectory;
    };
}

#endif // ES_APP_GAMESTORE_GOG_MODELS_H
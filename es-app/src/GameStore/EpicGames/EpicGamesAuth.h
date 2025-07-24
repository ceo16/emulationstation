// Sostituisci completamente il tuo emulationstation-master/es-app/src/GameStore/EpicGames/EpicGamesAuth.h

#pragma once
#ifndef ES_APP_GAMESTORE_EPICGAMES_EPICGAMESAUTH_H
#define ES_APP_GAMESTORE_EPICGAMES_EPICGAMESAUTH_H

#include <string>
#include <functional>
#include <chrono>
#include <filesystem>
#include "json.hpp"

class EpicGamesAuth {
public:
    EpicGamesAuth(std::function<void(const std::string&)> setStateCallback);
    EpicGamesAuth();
    ~EpicGamesAuth();

    bool exchangeAuthCodeForToken(const std::string& authCode);
    bool refreshAccessToken();
    bool loadTokenData();

    // --- Funzioni per la UI ---
    void logout();
    std::string getUsername() const; // Restituisce il nome utente per la UI
    std::string getDisplayName() const; // Restituisce il nome utente per la UI
    bool processWebViewRedirect(const std::string& redirectUrl);

    // --- Funzioni Statiche ---
    static std::string getInitialLoginUrl();
    
    // --- Getters Esistenti ---
    std::string getAccessToken() const;
    bool isAuthenticated() const;
    std::string getAccountId() const;
	void clearAllTokenData();
    std::string getRefreshToken() const;
	static std::string getAuthorizationCodeUrl();
private:
    void saveTokenData();
    bool processTokenResponse(const nlohmann::json& response, bool isRefreshing = false);

    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mAccountId;
    std::string mDisplayName;
    std::chrono::time_point<std::chrono::system_clock> mTokenExpiry;
    bool mHasValidTokenInfo;

    std::string mTokenStoragePath;

    // Costanti della classe
    static const std::string EPIC_CLIENT_ID;
    static const std::string EPIC_CLIENT_SECRET;
    static const std::string TOKEN_ENDPOINT_URL;
};

#endif // ES_APP_GAMESTORE_EPICGAMES_EPICGAMESAUTH_H
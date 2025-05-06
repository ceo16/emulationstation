#pragma once

#ifndef ES_CORE_EPICGAMESAUTH_H
#define ES_CORE_EPICGAMESAUTH_H

#include <string>
#include <functional>
#include <chrono>
#include <filesystem>
#include "json.hpp"

using json = nlohmann::json;

class EpicGamesAuth {
public:
    // Costruttori dal tuo file originale
    EpicGamesAuth(std::function<void(const std::string&)> setStateCallback);
    EpicGamesAuth(); // Costruttore di default
    ~EpicGamesAuth();

    std::string getAuthorizationUrl(std::string& state_out_param_not_really_used);
    
    // Modificato per riflettere il tuo uso originale: scambia il codice e restituisce successo/fallimento.
    // Il token viene poi recuperato tramite il getter.
    bool exchangeAuthCodeForToken(const std::string& authCode);

    bool refreshAccessToken();

    std::string getAccessToken() const;
    std::string getRefreshToken() const;
    std::string getAccountId() const;
    std::string getDisplayName() const;
    std::chrono::time_point<std::chrono::system_clock> getTokenExpiry() const;

    bool isAuthenticated() const;
    void clearAllTokenData();
    bool loadTokenData();

    // std::string getCurrentState() const; // Se mAuthState è un membro

private:
    std::string generateRandomState();
    void saveTokenData();
    bool processTokenResponse(const nlohmann::json& response, bool isRefreshing = false);

    // Membri
    std::function<void(const std::string&)> mSetStateCallback; // Dal tuo codice
    // std::string mAuthState; // Dal tuo codice, se lo usi ancora internamente

    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mAccountId;
    std::string mDisplayName;
    std::chrono::time_point<std::chrono::system_clock> mTokenExpiry;
    bool mHasValidTokenInfo;

    std::filesystem::path mTokenStoragePath;

    const std::string EPIC_CLIENT_ID = "34a02cf8f4414e29b15921876da36f9a";
    const std::string EPIC_CLIENT_SECRET = "daafbccc737745039dffe53d94fc76cf";
    const std::string TOKEN_ENDPOINT_URL = "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token";
    const std::string AUTHORIZE_URL_MANUAL_CODE = "https://www.epicgames.com/id/api/redirect?clientId=" + EPIC_CLIENT_ID + "&responseType=code";

    // Nomi file come nel tuo .cpp
    // Potrebbero essere static const std::string qui o definiti nel .cpp
    static const std::string ACCESS_TOKEN_FILENAME_DEF; // Uso _DEF per evitare conflitti se li definisci anche nel cpp
    static const std::string REFRESH_TOKEN_FILENAME_DEF;
    static const std::string ACCOUNT_ID_FILENAME_DEF;
    static const std::string EXPIRY_FILENAME_DEF;
};

#endif // ES_CORE_EPICGAMESAUTH_H
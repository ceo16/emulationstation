#ifndef ES_APP_GAMESTORE_XBOX_AUTH_H
#define ES_APP_GAMESTORE_XBOX_AUTH_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include "utils/FileSystemUtil.h" // Per std::filesystem::path

// Forward declaration
class Window;

class XboxAuth
{
public:
    XboxAuth(std::function<void(const std::string&)> setStateCallback = nullptr);

    ~XboxAuth();

    // --- Costanti per l'autenticazione ---
    static const std::string CLIENT_ID;
    static const std::string REDIRECT_URI;
    static const std::string SCOPE;
    static const std::string LIVE_AUTHORIZE_URL;
    static const std::string LIVE_TOKEN_URL;
    static const std::string XBOX_USER_AUTHENTICATE_URL;
    static const std::string XBOX_XSTS_AUTHORIZE_URL;

    // --- Nomi dei file per il salvataggio dei token ---
    static const std::string LIVE_TOKENS_FILENAME_DEF;
    static const std::string XSTS_TOKENS_FILENAME_DEF;
    static const std::string USER_INFO_FILENAME_DEF; // Per XUID, UHS etc.

    bool isAuthenticated() const;
    std::string getAuthorizationUrl(std::string& state_out); // Simile a Epic, per il flusso manuale del codice
    bool exchangeAuthCodeForTokens(const std::string& authCode); // Scambia codice auth per token Live
    bool authenticateXSTS(); // Ottieni token XSTS usando token Live
    bool refreshTokens(); // Gestisce il refresh sia dei token Live che XSTS

    std::string getLiveAccessToken() const;
    std::string getXstsToken() const;
    std::string getXUID() const;
    std::string getUserHash() const; // UHS

    void loadTokenData();
    void saveTokenData();
    void clearAllTokenData();

private:
    // Funzione di callback per aggiornare lo stato (es. in UI)
    std::function<void(const std::string&)> mSetStateCallback;

    // Token Live
    std::string mLiveAccessToken;
    std::string mLiveRefreshToken;
    std::chrono::time_point<std::chrono::system_clock> mLiveTokenExpiry;

    // Token XSTS e info utente
    std::string mXstsToken;
    std::chrono::time_point<std::chrono::system_clock> mXstsTokenExpiry;
    std::string mUserXUID;
    std::string mUserHash; // UHS (User Hash)
    // Altri campi da XSTS DisplayClaims se necessari

    bool mHasTriedAutoLogin; // Flag per evitare tentativi multipli di login automatico
    std::string mTokenStoragePath;

    // Metodi helper privati per le richieste HTTP (verranno implementati in .cpp)
    // bool requestLiveTokens(const std::string& grantType, const std::string& codeOrRefreshToken, ...);
    // bool requestXstsTokenStep1(...);
    // bool requestXstsTokenStep2(...);
    // bool processLiveTokenResponse(const nlohmann::json& response);
    // bool processXstsResponse(const nlohmann::json& response);
};

#endif // ES_APP_GAMESTORE_XBOX_AUTH_H
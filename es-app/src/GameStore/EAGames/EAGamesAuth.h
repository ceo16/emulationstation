#pragma once

#include <string>
#include <functional>
#include <vector> // Per std::vector in calculateFnv1aHash
#include "Window.h" // Assicurati che Window sia definito o forward-declared se necessario

namespace EAGames {

    class EAGamesAuth {
    public:
        EAGamesAuth(Window* window);
		void postToUiThread(std::function<void()> func);

        bool isUserLoggedIn() const;
        std::string getAccessToken() const;
        std::string getRefreshToken() const;
        std::string getPidId() const;
        std::string getPersonaId() const;
        std::string getUserName() const;

		

        void login(std::function<void(bool success, const std::string& message)> callback);
        void logout();

        void StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished);

        void RefreshTokens(std::function<void(bool success, const std::string& message)> callback);

        static unsigned short GetLocalRedirectPort();

    private:
        Window* mWindow;
        std::string mAccessToken;
        std::string mRefreshToken;
        std::string mPid;
        std::string mPersonaId;
        std::string mUserName;
        time_t mTokenExpiryTime;

        bool mIsLoggedIn;

        static const std::string EA_AUTH_BASE_URL;
        static const std::string EA_TOKEN_URL;
        static const std::string OAUTH_CLIENT_ID;
        static const std::string OAUTH_CLIENT_SECRET;

        static unsigned short s_localRedirectPort;

        void loadCredentials();
        void saveCredentials();
        void clearCredentials();

        std::string generatePcSign();
        bool getWindowsHardwareInfo(std::string& bbm, std::string& bsn, int& gid, std::string& hsn, std::string& msn, std::string& mac, std::string& osn, std::string& osi_timestamp_str);
        std::string calculateFnv1aHash(const std::vector<std::string>& components);

        // MODIFICA QUI: Aggiungi il secondo parametro std::string
        void exchangeCodeForToken(const std::string& authCode,
                                  const std::string& redirectUriForTokenExchange, // <-- Aggiunto questo parametro
                                  std::function<void(bool success, const std::string& message)> callback);
        void fetchUserIdentity(std::function<void(bool success, const std::string& message)> callback);

        std::string getLocalRedirectUri() const;
    };

} // namespace EAGames
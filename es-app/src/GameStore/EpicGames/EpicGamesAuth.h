#pragma once
#ifndef ES_CORE_EPICGAMESAUTH_H
#define ES_CORE_EPICGAMESAUTH_H

#include <string>
#include <functional>

class EpicGamesAuth {
public:
    EpicGamesAuth(std::function<void(const std::string&)> setStateCallback);
    EpicGamesAuth();
    ~EpicGamesAuth();

    std::string getAuthorizationUrl(std::string& state);
    bool getAccessToken(const std::string& authCode, std::string& accessToken);
    std::string getAccessToken() const;

    std::string getCurrentState() const { return mAuthState; }

private:
    std::string generateRandomState();
    char base64Character(uint32_t index);
    std::string base64Encode(const std::string& input);

    void saveToken(const std::string& accessToken);
    void loadToken();

    std::function<void(const std::string&)> mSetStateCallback;
    std::string mAccessToken;
    std::string mAuthState;
    static const std::string STATE_FILE_NAME;
};

#endif // ES_CORE_EPICGAMESAUTH_H
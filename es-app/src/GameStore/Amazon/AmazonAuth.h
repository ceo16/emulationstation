#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_AUTH_H
#define ES_APP_GAMESTORE_AMAZON_AUTH_H

#include <string>
#include <functional>
#include "Window.h"

class AmazonAuth
{
public:
    AmazonAuth(Window* window);
    ~AmazonAuth();

    void startLoginFlow(std::function<void(bool success)> on_complete);
    void logout();

    bool isAuthenticated() const;
    std::string getAccessToken() const;

private:
    void loadTokens();
    void saveTokens();
    void clearTokens();
    
    std::string extractInitialToken(const std::string& urlFragment);
    void exchangeInitialToken(const std::string& initialToken, std::function<void(bool success)> on_complete);

    Window* mWindow;
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mTokensPath;
};

#endif // ES_APP_GAMESTORE_AMAZON_AUTH_H
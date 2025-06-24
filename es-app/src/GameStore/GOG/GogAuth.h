#pragma once
#ifndef ES_APP_GAMESTORE_GOG_AUTH_H
#define ES_APP_GAMESTORE_GOG_AUTH_H

#include "GameStore/GOG/GogModels.h"
#include <string>
#include <functional>
#include <atomic>
#include "Window.h"

class GuiWebViewAuthLogin;

class GogAuth
{
public:
    GogAuth(Window* window);
    
    void login(std::function<void(bool success)> on_complete);
    void logout();
    bool isAuthenticated();
    GOG::AccountInfo getAccountInfo();
    
private:
    bool checkAuthentication();

    Window* mWindow;
    bool mIsAuthenticated;
    GOG::AccountInfo mAccountInfo;
    GuiWebViewAuthLogin* mWebView = nullptr;
    std::atomic<bool> mIsCheckingAuth{false};
};

#endif // ES_APP_GAMESTORE_GOG_AUTH_H
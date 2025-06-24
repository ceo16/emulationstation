#pragma once
#ifndef ES_APP_GAMESTORE_GOG_AUTH_H
#define ES_APP_GAMESTORE_GOG_AUTH_H

#include "GameStore/GOG/GogModels.h"
#include <string>
#include <functional>
#include <atomic>
#include "Window.h"

// L'enum può rimanere fuori se preferisci, ma è buona pratica metterlo
// dentro la classe o in un namespace per evitare conflitti di nomi.
enum class GogLoginState {
    NOT_LOGGED_IN,
    WAITING_FOR_ACCOUNT_PAGE,
    FETCHING_ACCOUNT_INFO,
    LOGIN_SUCCESSFUL
};

class GuiWebViewAuthLogin;

class GogAuth
{
public:
    GogAuth(Window* window);
    ~GogAuth();

    void login(std::function<void(bool success)> on_complete);
    void logout();
    bool isAuthenticated();
    GOG::AccountInfo getAccountInfo();

private:
    // --- CORREZIONE: Funzioni separate per controlli sincroni e asincroni ---

    
    // CORREZIONE CHIAVE: mLoginState è ora un membro della classe
    GogLoginState mLoginState; 

    Window* mWindow;
    bool mIsAuthenticated;
    GOG::AccountInfo mAccountInfo;
    GuiWebViewAuthLogin* mWebView = nullptr;
    std::atomic<bool> mIsCheckingAuth{false};
	 bool mInitialNavigationDone; // Aggiungi questa linea
};

#endif // ES_APP_GAMESTORE_GOG_AUTH_H
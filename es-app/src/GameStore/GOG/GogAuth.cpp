#include "GogAuth.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "Log.h"
#include "HttpReq.h"
#include "Window.h"
#include <thread>

GogAuth::GogAuth(Window* window) : mWindow(window) {
    mIsAuthenticated = checkAuthentication();
}

void GogAuth::login(std::function<void(bool success)> on_complete) {
    const std::string loginUrl = "https://www.gog.com/account/";
    const std::string watchUrl = "https://www.gog.com/account"; 

    if (mWebView != nullptr) {
        LOG(LogWarning) << "GOG Auth: Tentativo di avviare un nuovo login mentre uno è già in corso.";
        return;
    }

    // --- CHIAMATA CORRETTA AL COSTRUTTORE ---
    // Usiamo la firma esatta, passando una stringa vuota per l'ultimo parametro che non ci serve.
    mWebView = new GuiWebViewAuthLogin(mWindow, loginUrl, "GOG.com", watchUrl, 
                                       GuiWebViewAuthLogin::AuthMode::GOG_LOGIN_POLLING, 
                                       true, ""); // visible = true, fragmentIdentifier = ""
    
    mWebView->setOnLoginFinishedCallback([this, on_complete](bool, const std::string&) {
        if (mIsCheckingAuth) {
            return;
        }

        mIsCheckingAuth = true;
        std::thread([this, on_complete] {
            bool isLoggedIn = checkAuthentication();
            if (isLoggedIn) {
                mWindow->postToUiThread([this, on_complete] {
                    if (mWebView) {
                        mWebView->close();
                        mWebView = nullptr;
                    }
                    if (on_complete) on_complete(true);
                });
            }
            mIsCheckingAuth = false;
        }).detach();
    });

    mWindow->pushGui(mWebView);
}

void GogAuth::logout() {
    // Il logout di GOG è complesso perché si basa sui cookie.
    // Il modo più sicuro per non causare crash è semplicemente resettare lo stato
    // e informare l'utente. I cookie verranno cancellati al prossimo avvio/login.
    mIsAuthenticated = false;
    mAccountInfo = GOG::AccountInfo();
    LOG(LogInfo) << "[GOG Auth] Logout eseguito. I cookie verranno cancellati al prossimo login.";
}

bool GogAuth::isAuthenticated() {
    return mIsAuthenticated;
}

GOG::AccountInfo GogAuth::getAccountInfo() {
    if (!mIsAuthenticated) {
        checkAuthentication();
    }
    return mAccountInfo;
}

bool GogAuth::checkAuthentication() {
    LOG(LogDebug) << "[GOG Auth] Controllo stato autenticazione tramite API...";
    
    HttpReqOptions options;
    options.useCookieManager = true; 
    HttpReq req("https://menu.gog.com/v1/account/basic", &options);
    req.wait();

    if (req.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto accountInfo = nlohmann::json::parse(req.getContent()).get<GOG::AccountInfo>();
            if (accountInfo.isLoggedIn) {
                LOG(LogInfo) << "[GOG Auth] SUCCESSO. Utente autenticato: " << accountInfo.username;
                mAccountInfo = accountInfo;
                mIsAuthenticated = true;
                return true;
            }
        } catch (const std::exception& e) {
            LOG(LogDebug) << "[GOG Auth] Errore parsing (normale se non loggati): " << e.what();
        }
    }
    
    mIsAuthenticated = false;
    mAccountInfo.username = "";
    return false;
}
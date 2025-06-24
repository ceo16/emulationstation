#include "GogAuth.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "Log.h"
#include "HttpReq.h"
#include "Window.h"
#include <thread>
#include "Paths.h"
#include "utils/FileSystemUtil.h"
#include <fstream>

// Aggiungi l'implementazione della nuova funzione privata
std::string GogAuth::getAuthFilePath() const {
    return Paths::getHomePath() + "/.emulationstation/gog_auth.json";
}


// SOSTITUISCI IL TUO COSTRUTTORE ATTUALE CON QUESTO
GogAuth::GogAuth(Window* window) 
    : mWindow(window), 
      mIsAuthenticated(false),
      mLoginState(GogLoginState::NOT_LOGGED_IN),
      mInitialNavigationDone(false)
{
    LOG(LogInfo) << "[GOG Auth] Istanza di GogAuth creata.";
    
    // --- INIZIO LOGICA DI CARICAMENTO ---
    const std::string authFile = getAuthFilePath();
    if (Utils::FileSystem::exists(authFile)) {
        LOG(LogInfo) << "[GOG Auth] Trovato file di autenticazione. Tento di caricare la sessione.";
        try {
            std::ifstream file(authFile);
            nlohmann::json data = nlohmann::json::parse(file);
            mAccountInfo = data.get<GOG::AccountInfo>();
            
            if (mAccountInfo.isLoggedIn) {
                mIsAuthenticated = true;
                LOG(LogInfo) << "[GOG Auth] Sessione ripristinata con successo per l'utente: " << mAccountInfo.username;
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "[GOG Auth] Errore nel caricare la sessione salvata: " << e.what();
            // In caso di errore, assicurati che lo stato sia pulito.
            mIsAuthenticated = false;
            mAccountInfo = GOG::AccountInfo();
        }
    }
    // --- FINE LOGICA DI CARICAMENTO ---
}

GogAuth::~GogAuth() {
    if (mWebView) {
        mWebView->close();
    }
}

void GogAuth::login(std::function<void(bool success)> on_complete) {
    const std::string loginUrl = "https://www.gog.com/account/";
    const std::string apiUrl = "https://menu.gog.com/v1/account/basic";

    if (mWebView != nullptr) { return; }

    mInitialNavigationDone = false; // Resetta la bandierina all'inizio di ogni tentativo
    mWebView = new GuiWebViewAuthLogin(mWindow, loginUrl, "GOG.com", "", GuiWebViewAuthLogin::AuthMode::GOG_LOGIN_POLLING);
    
    mLoginState = GogLoginState::WAITING_FOR_ACCOUNT_PAGE;

    mWebView->setNavigationCompletedCallback([this, on_complete, apiUrl](bool isSuccess, const std::string& url) {
        if (!isSuccess) { return; }

        LOG(LogDebug) << "[GOG Auth] Navigazione completata a: " << url;

        // --- NUOVA LOGICA CON BANDIERINA ---
        // Se questa è la prima navigazione in assoluto, la ignoriamo e alziamo la bandierina.
        if (!mInitialNavigationDone) {
            LOG(LogInfo) << "[GOG Auth] Caricamento pagina di login iniziale completato. In attesa dell'input dell'utente.";
            mInitialNavigationDone = true;
            return;
        }
        
        // D'ora in poi, agiamo solo sulle navigazioni successive alla prima.
        if (mLoginState == GogLoginState::WAITING_FOR_ACCOUNT_PAGE) {
            LOG(LogInfo) << "[GOG Auth] Rilevata azione utente. Verifico lo stato API...";
            mLoginState = GogLoginState::FETCHING_ACCOUNT_INFO;
            mWebView->navigate(apiUrl);
            return;
        }

        if (mLoginState == GogLoginState::FETCHING_ACCOUNT_INFO && url.rfind(apiUrl, 0) == 0) {
            LOG(LogInfo) << "[GOG Auth] Raggiunto endpoint API. Estraggo JSON...";
            
            mWebView->getHtmlContent([this, on_complete](const std::string& pageContent) {
                bool loggedIn = false;
                try {
                    auto accountInfo = nlohmann::json::parse(pageContent).get<GOG::AccountInfo>();
                    
                    if (accountInfo.isLoggedIn) {
                        LOG(LogInfo) << "[GOG Auth] SUCCESSO! Login confermato: " << accountInfo.username;
                        mAccountInfo = accountInfo;
                        mIsAuthenticated = true;
                        loggedIn = true;
						LOG(LogInfo) << "[GOG Auth] Salvo la sessione su disco...";
                nlohmann::json dataToSave = accountInfo;
                std::ofstream file(getAuthFilePath());
                file << dataToSave.dump(4); // dump(4) per un file leggibile
                file.close();
                LOG(LogInfo) << "[GOG Auth] Sessione salvata con successo.";
                    } else {
                        LOG(LogWarning) << "[GOG Auth] API dice che l'utente non è loggato. Torno in attesa.";
                        mLoginState = GogLoginState::WAITING_FOR_ACCOUNT_PAGE;
                    }
                } catch (const std::exception& e) {
                    LOG(LogError) << "[GOG Auth] Errore parsing JSON: " << e.what();
                    mLoginState = GogLoginState::WAITING_FOR_ACCOUNT_PAGE;
                }

                if (loggedIn) {
                    mWindow->postToUiThread([this, on_complete] {
                        if (mWebView) { mWebView->close(); mWebView = nullptr; }
                        if (on_complete) { on_complete(true); }
                    });
                }
            });
        }
    });

    mWindow->pushGui(mWebView);
}

void GogAuth::logout() {
    mIsAuthenticated = false;
    mAccountInfo = GOG::AccountInfo();

    const std::string authFile = getAuthFilePath();
    if (Utils::FileSystem::exists(authFile)) {
        Utils::FileSystem::removeFile(authFile);
        LOG(LogInfo) << "[GOG Auth] File di autenticazione rimosso. Logout completato.";
    } else {
        LOG(LogInfo) << "[GOG Auth] Logout eseguito (nessuna sessione salvata da rimuovere).";
    }
}

bool GogAuth::isAuthenticated() {
    // Semplicemente restituisce lo stato che abbiamo salvato durante il login.
    return mIsAuthenticated;
}

GOG::AccountInfo GogAuth::getAccountInfo() {
    // Semplicemente restituisce le info dell'account salvate durante il login.
    return mAccountInfo;
}

// --- NUOVA VERSIONE SINCRONA (per controlli rapidi) ---

// Include Standard e SDK prima
#include <functional>
#include <string>
#include <vector>
#include <cctype> 
#include <cstring> 

#ifdef _WIN32
  #include <SDL.h>
  #include <SDL_syswm.h>
  #include <WebView2EnvironmentOptions.h> 
#endif

// Include di EmulationStation Core
#include "renderers/Renderer.h"
#include "resources/Font.h"   
#include "Log.h"               
#include "Settings.h"         
#include "Paths.h"             
#include "utils/StringUtil.h"   
#include "Window.h"             
#include "ThemeData.h"  
#include "utils/FileSystemUtil.h" 
// #include "LocaleES.h" 

// Header della nostra classe
#include "guis/GuiWebViewAuthLogin.h"


GuiWebViewAuthLogin::GuiWebViewAuthLogin(Window* window, const std::string& initialUrl, const std::string& storeNameForLogging, const std::string& watchRedirectPrefix, AuthMode mode, bool visible, const std::string& fragmentIdentifier)
    : GuiComponent(window),
      mLoading(false),
      mCurrentInitialUrl(initialUrl),
      mStoreNameForLogging(storeNameForLogging),
      mWatchRedirectPrefix(watchRedirectPrefix),
      mCachedLoadingMsgText(""),
      mRedirectSuccessfullyHandled(false), // <-- VIRGOLA AGGIUNTA QUI
      mAuthMode(mode),
      mSteamCookieDomain(""), // <-- VIRGOLA AGGIUNTA QUI
	  mAuthCode(""),
	  mIsVisible(visible), // Inizializza la nuova variabile
	  mFragmentIdentifier(fragmentIdentifier) // Salva il nuovo parametro
#ifdef _WIN32
    , mParentHwnd(nullptr),
      mWebViewEnvironment(nullptr),
      mWebViewController(nullptr),
      mWebView(nullptr),
      mNavigationStartingToken({0}),
      mNavigationCompletedToken({0}),
      mProcessFailedToken({0})
#endif
{
    float width = Renderer::getScreenWidth() * 0.8f;
    float height = Renderer::getScreenHeight() * 0.85f;
    setSize(width, height);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2.0f, (Renderer::getScreenHeight() - mSize.y()) / 2.0f);
    init();
}

void GuiWebViewAuthLogin::setSteamCookieDomain(const std::string& domain) {
    if (mAuthMode == AuthMode::FETCH_STEAM_COOKIE) {
        mSteamCookieDomain = domain;
    }
}


GuiWebViewAuthLogin::~GuiWebViewAuthLogin()
{
#ifdef _WIN32
    closeWebView();
#endif
}

void GuiWebViewAuthLogin::init()
{
    mLoading = true; 
#ifdef _WIN32
    if (!mWebViewController) {
        LOG(LogInfo) << "[" << mStoreNameForLogging << "] GuiWebViewAuthLogin: Chiamata a initializeWebView().";
        if (!initializeWebView()) { 
            LOG(LogError) << "[" << mStoreNameForLogging << "] GuiWebViewAuthLogin: initializeWebView() ha fallito all'avvio.";
            mLoading = false; 
            if (mOnLoginFinishedCallback) {
                if (mWindow) mWindow->postToUiThread([this] { 
                    if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Inizializzazione WebView2 (chiamata init)"); 
                });
                else this->mOnLoginFinishedCallback(false, "Errore Inizializzazione WebView2 (chiamata init)");
            }
        }
    } else {
        LOG(LogInfo) << "[" << mStoreNameForLogging << "] GuiWebViewAuthLogin: WebView già inizializzata. Rendendola visibile e navigando.";
        if (mWebViewController) mWebViewController->put_IsVisible(TRUE); 
        navigateTo(mCurrentInitialUrl);
    }
#else
    LOG(LogError) << "[" << mStoreNameForLogging << "] GuiWebViewAuthLogin: WebView2 non supportato su questa piattaforma.";
    mLoading = false;
    if (mOnLoginFinishedCallback) {
        if (mWindow) mWindow->postToUiThread([this] {
            if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Piattaforma non supportata per WebView");
        });
        else this->mOnLoginFinishedCallback(false, "Piattaforma non supportata per WebView");
    }
#endif
}

void GuiWebViewAuthLogin::setOnLoginFinishedCallback(const std::function<void(bool success, const std::string& tokenOrError)>& callback)
{
    mOnLoginFinishedCallback = callback;
}

void GuiWebViewAuthLogin::setWatchRedirectPrefix(const std::string& prefix)
{
    LOG(LogDebug) << "[" << mStoreNameForLogging << "] Setting watch prefix to: " << prefix;
    mWatchRedirectPrefix = prefix;
    mRedirectSuccessfullyHandled = false; // Resetta il flag per il passo successivo
}

void GuiWebViewAuthLogin::render(const Transform4x4f& parentTrans)
{
	if (!mIsVisible) {
        return; // Non disegnare nulla se invisibile
    }
    Transform4x4f trans = parentTrans * getTransform();
    Renderer::setMatrix(trans); 
    
    Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0x202020DD, 0x202020DD); 

    if (mLoading && !mWebViewController) { 
        std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM); 
        if (font) {
             unsigned int color = 0xDDDDDDFF; 
             std::string msg = "[" + mStoreNameForLogging + "] " + "Inizializzazione WebView..."; 
             
             if (!mTextCache || mCachedLoadingMsgText != msg) { 
                mTextCache = std::unique_ptr<TextCache>(font->buildTextCache(msg, 0.0f, 0.0f, color));
                mCachedLoadingMsgText = msg; 
             }

             if (mTextCache) {
                Vector2f textSize = mTextCache->metrics.size; 
                // Calcolo di pos, assicurandosi che i componenti siano float per la costruzione di Vector2f
                Vector2f pos(
                    static_cast<float>((mSize.x() - textSize.x()) / 2.0f), 
                    static_cast<float>((mSize.y() - textSize.y()) / 2.0f)
                ); 
                
                Transform4x4f textMatrix = trans;
                textMatrix.translate(Vector3f(pos.x(), pos.y(), 0.0f)); // translate prende Vector3f
                Renderer::setMatrix(textMatrix);
                font->renderTextCache(mTextCache.get());
                Renderer::setMatrix(trans); 
             }
        }
        LOG(LogDebug) << "[" << mStoreNameForLogging << "] Schermata di caricamento WebView (mLoading && !mWebViewController)";
    } else if (mLoading) {
        LOG(LogDebug) << "[" << mStoreNameForLogging << "] Pagina WebView in caricamento...";
    }

    GuiComponent::renderChildren(trans); 
}

void GuiWebViewAuthLogin::update(int deltaTime) { GuiComponent::update(deltaTime); }

bool GuiWebViewAuthLogin::input(InputConfig* config, Input input) {
    if (input.value != 0 && (config->isMappedTo("b", input) || (input.id == SDLK_ESCAPE && input.type == TYPE_KEY))) {
        LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] GuiWebViewAuthLogin: Indietro/Esc premuto dall'utente.";
        if (this->mOnLoginFinishedCallback) { 
            if (this->mWindow) this->mWindow->postToUiThread([this] { 
                 if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Annullato (" + this->mStoreNameForLogging + ")");
            });
            else this->mOnLoginFinishedCallback(false, "Annullato (" + this->mStoreNameForLogging + ")");
        }
        this->closeWebView(); 
        if (this->mWindow) this->mWindow->removeGui(this); 
        return true; 
    }
    return GuiComponent::input(config, input);
}

void GuiWebViewAuthLogin::onSizeChanged() {
    GuiComponent::onSizeChanged(); 
#ifdef _WIN32
    if (mWebViewController) resizeWebView();
#endif
}

void GuiWebViewAuthLogin::navigate(const std::string& url)
{
#ifdef _WIN32
    if (mWebView) {
        LOG(LogDebug) << "[" << mStoreNameForLogging << "] Navigazione manuale a: " << url;
        mWebView->Navigate(Utils::String::convertToWideString(url).c_str());
    }
#endif
}

#ifdef _WIN32
// --- Implementazioni WebView2 ---
bool GuiWebViewAuthLogin::initializeWebView() {
    SDL_Window* sdlWindow = Renderer::getSDLWindow(); 
    if (!sdlWindow) { 
        LOG(LogError) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere SDL_Window da Renderer."; 
        return false; 
    }

    SDL_SysWMinfo wmInfo; 
    SDL_VERSION(&wmInfo.version); 
    if (SDL_GetWindowWMInfo(sdlWindow, &wmInfo) != SDL_TRUE) {
        LOG(LogError) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere SDL_SysWMinfo: " << SDL_GetError(); 
        return false; 
    }
    this->mParentHwnd = wmInfo.info.win.window; 
    if (!this->mParentHwnd) { 
        LOG(LogError) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere HWND dalla finestra SDL."; 
        return false; 
    }
    LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Parent HWND ottenuto: " << (void*)this->mParentHwnd;

    std::string profileFolderName = "default_profile";
    if (!this->mStoreNameForLogging.empty()) {
        profileFolderName = "store_";
        for (char ch : this->mStoreNameForLogging) {
            if (std::isalnum(static_cast<unsigned char>(ch))) profileFolderName += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            else profileFolderName += '_';
        }
    }
    std::string appName = "EmulationStation"; 
    std::string userDataRootPath = Paths::getHomePath() + "/.emulationstation/webview_profiles/"; 
    std::string userProfilePath = userDataRootPath + appName + "/" + profileFolderName;
    
    LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] User Data Folder per WebView: " << userProfilePath;
    
    if (!Utils::FileSystem::exists(userDataRootPath)) Utils::FileSystem::createDirectory(userDataRootPath);
    if (!Utils::FileSystem::exists(userProfilePath)) Utils::FileSystem::createDirectory(userProfilePath);
    
    std::wstring userDataFolderW = Utils::String::convertToWideString(userProfilePath); 

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions> optionsBase = 
        Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>(); 
    
    if (optionsBase) {
        HRESULT ssoHr = optionsBase->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);
        if (SUCCEEDED(ssoHr)) {
            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] WebView2: Impostato AllowSingleSignOnUsingOSPrimaryAccount a FALSE su ICoreWebView2EnvironmentOptions.";
        } else {
            LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] WebView2: Fallimento nell'impostare AllowSingleSignOnUsingOSPrimaryAccount su ICoreWebView2EnvironmentOptions. HR: 0x" << std::hex << ssoHr << ". L'errore EDGE_IDENTITY potrebbe persistere.";
        }
    } else {
        LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] Impossibile creare CoreWebView2EnvironmentOptions base.";
    }

    LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Tentativo di creare l'ambiente WebView2...";
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataFolderW.c_str(), optionsBase.Get(), 
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT { 
                LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] CreateCoreWebView2EnvironmentCompletedHandler - Result: 0x" << std::hex << result;
                if (FAILED(result)) { 
                    LOG(LogError) << "[" << this->mStoreNameForLogging << "] Creazione ambiente WebView2 fallita. HR: 0x" << std::hex << result;
                    this->mLoading = false; 
                    if (this->mOnLoginFinishedCallback) {
                        if (this->mWindow) this->mWindow->postToUiThread([this, result] {
                            if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Creazione Ambiente WebView2 (HR: 0x" + Utils::String::toHexString(result) + ")");
                        });
                        else this->mOnLoginFinishedCallback(false, "Errore Creazione Ambiente WebView2 (HR: 0x" + Utils::String::toHexString(result) + ")");
                    }
                    return result;
                }
                this->mWebViewEnvironment = env; 
                LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Ambiente WebView2 creato con successo.";

                LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Tentativo di creare il controller WebView2...";
                HRESULT createControllerResult = this->mWebViewEnvironment->CreateCoreWebView2Controller(this->mParentHwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT resultCtrl, ICoreWebView2Controller* ctrl) -> HRESULT { 
                            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] CreateCoreWebView2ControllerCompletedHandler - Result: 0x" << std::hex << resultCtrl;
                            if (FAILED(resultCtrl)) { 
                                LOG(LogError) << "[" << this->mStoreNameForLogging << "] Creazione controller WebView2 fallita. HR: 0x" << std::hex << resultCtrl;
                                this->mLoading = false; 
                                if (this->mOnLoginFinishedCallback) {
                                     if (this->mWindow) this->mWindow->postToUiThread([this, resultCtrl] {
                                        if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Creazione Controller WebView2 (HR: 0x" + Utils::String::toHexString(resultCtrl) + ")");
                                     });
                                     else this->mOnLoginFinishedCallback(false, "Errore Creazione Controller WebView2 (HR: 0x" + Utils::String::toHexString(resultCtrl) + ")");
                                }
                                if(this->mWebViewEnvironment) this->mWebViewEnvironment.Reset(); 
                                return resultCtrl;
                            }
                            this->mWebViewController = ctrl; 
                            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Controller WebView2 creato con successo.";

                            HRESULT getWebViewResult = this->mWebViewController->get_CoreWebView2(&this->mWebView);
                            LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] get_CoreWebView2 - Result: 0x" << std::hex << getWebViewResult;
                            if (FAILED(getWebViewResult)) { 
                                LOG(LogError) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere CoreWebView2. HR: 0x" << std::hex << getWebViewResult;
                                if(this->mWebViewController) this->mWebViewController.Reset(); 
                                if(this->mWebViewEnvironment) this->mWebViewEnvironment.Reset();
                                this->mLoading = false; 
                                if (this->mOnLoginFinishedCallback) {
                                    if (this->mWindow) this->mWindow->postToUiThread([this, getWebViewResult] {
                                        if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Ottenimento CoreWebView2 (HR: 0x" + Utils::String::toHexString(getWebViewResult) + ")");
                                    });
                                    else this->mOnLoginFinishedCallback(false, "Errore Ottenimento CoreWebView2 (HR: 0x" + Utils::String::toHexString(getWebViewResult) + ")");
                                }
                                return getWebViewResult;
                            }
                            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] ICoreWebView2 ottenuto con successo.";

  // --- INIZIO CODICE AGGIUNTO PER CANCELLARE I COOKIE ---
                            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Tentativo di cancellare i cookie per il profilo WebView.";
                            if (this->mWebView) {
                                Microsoft::WRL::ComPtr<ICoreWebView2CookieManager> cookieManager;
                                HRESULT hr_cookie = E_FAIL;

                                // Interroga per l'interfaccia versionata che contiene get_CookieManager
                                // SOSTITUISCI ICoreWebView2_2 CON L'INTERFACCIA CORRETTA SE DIVERSA
                                // (es. ICoreWebView2_3, ICoreWebView2_5, ecc., a seconda di dove l'hai trovato nel tuo WebView2.h)
                                Microsoft::WRL::ComPtr<ICoreWebView2_2> webView_versioned;
                                if (SUCCEEDED(this->mWebView.As(&webView_versioned)) && webView_versioned) {
                                    LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Ottenuta interfaccia ICoreWebView2_2 (o versione superiore).";
                                    hr_cookie = webView_versioned->get_CookieManager(&cookieManager);
                                } else {
                                    LOG(LogError) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere l'interfaccia ICoreWebView2_2 (o versione superiore) per CookieManager.";
                                    // Se this->mWebView.As() fallisce, hr_cookie rimane E_FAIL o l'HRESULT del fallimento.
                                }

                                if (SUCCEEDED(hr_cookie) && cookieManager) {
    // AGGIUNGI QUESTO CONTROLLO: Cancella i cookie solo se NON siamo in modalità scraping
    if (mAuthMode != AuthMode::FETCH_STEAM_GAMES_JSON) 
    {
        cookieManager->DeleteAllCookies();
        LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Chiamata a DeleteAllCookies() per il profilo WebView eseguita con successo.";
    }
} else {
                                    LOG(LogError) << "[" << this->mStoreNameForLogging << "] Fallimento nell'ottenere CookieManager. HR: 0x" << std::hex << hr_cookie;
                                    LOG(LogError) << "[" << this->mStoreNameForLogging << "] Assicurati che l'interfaccia ICoreWebView2_N interrogata sia quella corretta che espone get_CookieManager nella tua versione dell'SDK WebView2.";
                                }
                            } else {
                                LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] mWebView è null, impossibile cancellare i cookie.";
								}
                            // --- FINE CODICE AGGIUNTO PER CANCELLARE I COOKIE ---

                            Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
                            this->mWebView->get_Settings(&settings);
                            if (settings) {
                                Microsoft::WRL::ComPtr<ICoreWebView2Settings2> settings2;
                                if (SUCCEEDED(settings.As(&settings2))) { 
                                   settings2->put_UserAgent(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");
                                   LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] User-Agent per WebView2 impostato.";
                                } else {
                                   LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere ICoreWebView2Settings2 per UserAgent. put_UserAgent non chiamato.";
                                }
                                settings->put_AreDevToolsEnabled(TRUE);
                                LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] DevTools per WebView2 abilitati.";
                            } else {
                                LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] Impossibile ottenere ICoreWebView2Settings.";
                            }
                            
                            this->mWebView->add_ProcessFailed(Microsoft::WRL::Callback<ICoreWebView2ProcessFailedEventHandler>(
                                [this](ICoreWebView2 * sender, ICoreWebView2ProcessFailedEventArgs * args) -> HRESULT { 
                                COREWEBVIEW2_PROCESS_FAILED_KIND kind;
                                args->get_ProcessFailedKind(&kind);
                                LOG(LogError) << "[" << this->mStoreNameForLogging << "] WebView ProcessFailed! Kind: " << kind;
                                if (this->mOnLoginFinishedCallback) {
                                    if (this->mWindow) this->mWindow->postToUiThread([this, kind] {
                                        if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Processo WebView (Kind: " + std::to_string(kind) + ")");
                                    });
                                    else this->mOnLoginFinishedCallback(false, "Errore Processo WebView (Kind: " + std::to_string(kind) + ")");
                                }
                                this->closeWebView(); 
                                if (this->mWindow) this->mWindow->removeGui(this); 
                                return S_OK;
                            }).Get(), &this->mProcessFailedToken); 
                            LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Gestore ProcessFailed aggiunto.";

this->mWebView->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
    [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
        PWSTR uriRaw = nullptr;
        args->get_Uri(&uriRaw);
        std::string uriA = Utils::String::convertFromWideString(std::wstring(uriRaw ? uriRaw : L""));
        CoTaskMemFree(uriRaw);

        LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] WebView NavigationStarting: " << uriA;

        if (this->mRedirectSuccessfullyHandled)
            return S_OK;

        // --- NUOVA LOGICA PER GOG ---
        // GOG ha un flusso diverso. Non cerchiamo un token, ma aspettiamo
        // che l'utente arrivi alla pagina dell'account.
        if (mAuthMode == AuthMode::GOG_LOGIN_POLLING && uriA.rfind(this->mWatchRedirectPrefix, 0) == 0)
        {
            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Rilevato URL di redirect GOG. Avvio controllo autenticazione.";
            // Non chiudiamo la finestra qui. Invochiamo il callback.
            // Sarà GogAuth a fare il controllo API e a decidere quando chiudere.
            if (this->mOnLoginFinishedCallback) {
                this->mWindow->postToUiThread([this, uriA] {
                    if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(true, uriA);
                });
            }
            // Non impostiamo mRedirectSuccessfullyHandled = true perché il controllo potrebbe ripetersi
            // Non chiudiamo la finestra. Lasciamo che l'utente continui a navigare.
            return S_OK;
        }


        // --- LOGICA ESISTENTE PER AMAZON E ALTRI ---
        // Questo blocco viene eseguito solo se NON siamo in modalità GOG.
        if (!this->mWatchRedirectPrefix.empty() && uriA.rfind(this->mWatchRedirectPrefix, 0) == 0)
        {
            std::string resultData;
            bool success = false;

            if (mAuthMode == AuthMode::AMAZON_OAUTH_FRAGMENT)
            {
                std::string accessToken = Utils::String::getUrlParam(uriA, "openid.oa2.access_token");
                if (!accessToken.empty())
                {
                    LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Rilevato redirect con access_token per Amazon.";
                    resultData = uriA;
                    success = true;
                }
            }
            else // Modalità DEFAULT per ?code=...
            {
                std::string code = Utils::String::getUrlParam(uriA, "code");
                if (!code.empty())
                {
                    LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Rilevato redirect con parametro 'code'.";
                    this->mAuthCode = code;
                    resultData = uriA;
                    success = true;
                }
            }

            if (success)
            {
                this->mRedirectSuccessfullyHandled = true;
                if (this->mOnLoginFinishedCallback) {
                    this->mWindow->postToUiThread([this, success, resultData] {
                        if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(success, resultData);
                    });
                }
                this->mWindow->postToUiThread([this] { this->closeWebView(); delete this; });
                return S_OK;
            }
        }
        
        // Logica per Steam
        if (mAuthMode == AuthMode::FETCH_STEAM_COOKIE && uriA.find("steamcommunity.com") != std::string::npos) {
             LOG(LogInfo) << "[Steam] In navigazione su steamcommunity.com";
        }

        return S_OK;
    }).Get(), &this->mNavigationStartingToken);
LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Gestore NavigationStarting aggiunto.";
                           

                           this->mWebView->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
    [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
        this->mLoading = false;
        BOOL isSuccess = FALSE;
        args->get_IsSuccess(&isSuccess);
        PWSTR srcUriRaw = nullptr;
        sender->get_Source(&srcUriRaw);
        std::string srcUri = Utils::String::convertFromWideString(std::wstring(srcUriRaw ? srcUriRaw : L""));
        CoTaskMemFree(srcUriRaw);

        LOG(LogInfo) << "[" << mStoreNameForLogging << "] WebView NavigationCompleted. URI: " << srcUri << ", Success: " << (isSuccess ? "true" : "false");

        // Assicurati che la navigazione sia avvenuta con successo per continuare con l'analisi della pagina.
        if (!isSuccess) {
            LOG(LogError) << "[" << mStoreNameForLogging << "] Navigazione fallita. Non eseguo script o callbacks di successo.";
            if (mOnLoginFinishedCallback && !mRedirectSuccessfullyHandled) { // Solo se non gestito e non per reindirizzamento
                mWindow->postToUiThread([this, srcUri] {
                    mOnLoginFinishedCallback(false, "Navigazione fallita: " + srcUri);
                });
            }
            close(); // Chiudi la WebView in caso di navigazione fallita non gestita.
            return S_OK;
        }
        
        // --- GESTIONE SPECIFICA PER AUTH_MODE (DOPO IL COMPLETAMENTO DELLA NAVIGAZIONE) ---

        // 1. Modalità FETCH_STEAM_COOKIE (Login Steam e recupero dati profilo)
        // Questo si attiva quando la WebView naviga a una pagina che indica un login Steam riuscito (es. profilo o home).
        if (mAuthMode == AuthMode::FETCH_STEAM_COOKIE && !mRedirectSuccessfullyHandled) {
            // Controlla se l'URL attuale indica che l'utente è loggato su Steam Community o Store.
            bool isSteamLoggedInPage = 
                srcUri.find("steamcommunity.com/profiles/") != std::string::npos || // Pagina profilo numerica
                srcUri.find("steamcommunity.com/id/") != std::string::npos ||     // Pagina profilo con vanity URL
                srcUri == "https://steamcommunity.com/" ||                        // Pagina home community
                srcUri == "https://store.steampowered.com/";                     // Pagina home store

            if (isSteamLoggedInPage) {
                LOG(LogInfo) << "[Steam] Pagina di login/profilo Steam rilevata. Esecuzione script per dati profilo...";
                mRedirectSuccessfullyHandled = true; // Segna come gestito per prevenire esecuzioni multiple

                // Script per estrarre nome profilo e SteamID.
                // Questo script è progettato per funzionare su pagine Steam post-login.
                const char* profile_script = R"JS((function(){
                    var el_name = document.querySelector('.actual_persona_name'); // Elemento comune su steamcommunity.com
                    if (!el_name) el_name = document.querySelector('.playerAvatar.playerAvatar.profile_page.medium > img'); // Alternativa per Steam store se il nome è nell'alt dell'avatar
                    
                    var profileName = el_name ? el_name.innerText.trim() : null;
                    if (!profileName && el_name && el_name.alt) profileName = el_name.alt.trim(); // Se è un'immagine avatar

                    var steamId = window.g_steamID; // Variabile globale comune nelle pagine Steam per lo SteamID64
                    if (!steamId) {
                        // Alternativa: estrai da URL se è del tipo steamcommunity.com/profiles/<SteamID64>/
                        var match = window.location.href.match(/steamcommunity\.com\/profiles\/(\d+)/);
                        if (match && match[1]) steamId = match[1];
                    }

                    if(profileName && steamId){
                        return JSON.stringify({'strProfileName':profileName,'strSteamId':steamId});
                    }
                    return null; // Ritorna null se i dati non sono trovati
                })())JS";
                
                this->executeScriptAndGetResult(profile_script, [this](const std::string& jsonString) {
                    if (mOnLoginFinishedCallback) {
                        mWindow->postToUiThread([this, jsonString] {
                            // La 'success' della callback dipende dal fatto che lo script abbia restituito dati validi.
                            mOnLoginFinishedCallback(!jsonString.empty() && jsonString != "null", jsonString);
                        });
                    }
                    close(); // Chiudi la WebView dopo aver tentato di ottenere i dati del profilo.
                });
                return S_OK; // Evento gestito.
            } else {
                // Se la pagina non è ancora una di login riuscito, non facciamo nulla.
                // L'utente deve interagire con la WebView per effettuare il login.
                LOG(LogDebug) << "[" << mStoreNameForLogging << "] Pagina non è di login riuscito, attendo interazione utente. URI: " << srcUri;
            }
            // Non chiamare S_OK e non settare mRedirectSuccessfullyHandled = true qui se l'utente non ha ancora fatto il login.
            // Lascia che la WebView continui a navigare finché non si raggiunge una pagina di successo.
        }

        // 2. Modalità FETCH_STEAM_GAMES_JSON (Scraping della Libreria Giochi)
        // Si attiva quando la WebView naviga alla pagina specifica della libreria giochi.
        if (mAuthMode == AuthMode::FETCH_STEAM_GAMES_JSON && !mRedirectSuccessfullyHandled) {
            // Controlla se l'URL è la pagina dei giochi che vogliamo scrapare.
            // steamcommunity.com/profiles/<SteamID64>/games/?tab=all
            // o steamcommunity.com/id/<vanity_url>/games/?tab=all
            if (srcUri.find("/games") != std::string::npos && (srcUri.find("steamcommunity.com/profiles/") != std::string::npos || srcUri.find("steamcommunity.com/id/") != std::string::npos)) {
                LOG(LogInfo) << "[Steam] Pagina giochi caricata per scraping. Eseguo script.";
                mRedirectSuccessfullyHandled = true; // Impedisce esecuzioni multiple dello script.

                // Lo script per estrarre il JSON dei giochi.
    const char* games_script = R"JS((function() {
    try {
        // Tentativo 1: Cerca la variabile globale 'rgGames', che è il metodo più comune.
        if (typeof rgGames !== 'undefined' && Array.isArray(rgGames) && rgGames.length > 0) {
            return JSON.stringify(rgGames);
        }

        // Tentativo 2: Se il primo fallisce, cerca i dati direttamente nel codice HTML,
        // dentro i tag <script>. Questo è un approccio molto più robusto.
        const scripts = document.getElementsByTagName('script');
        for (let i = 0; i < scripts.length; i++) {
            const scriptContent = scripts[i].innerHTML;
            // Cerchiamo la riga esatta in cui Valve definisce la variabile.
            if (scriptContent.includes('var rgGames = ')) {
                // Usiamo un'espressione regolare per estrarre l'array [...] in modo pulito.
                const match = scriptContent.match(/var rgGames = (\[.*\]);/);
                if (match && match[1]) {
                    // Trovato! Restituisce la stringa JSON dei giochi.
                    return match[1];
                }
            }
        }

        // Tentativo 3 (Fallback): In alcune vecchie versioni della pagina, i dati erano
        // in un attributo di un elemento specifico. Questo è il metodo originale di Playnite.
        const configElement = document.querySelector('#gameslist_config');
        if (configElement && configElement.hasAttribute('data-profile-gameslist')) {
            return configElement.getAttribute('data-profile-gameslist');
        }

    } catch (e) {
        // Se si verifica un errore durante lo script, non bloccare tutto.
        return null;
    }

    // Se nessuno dei tentativi ha funzionato, restituisce null.
    return null;
})())JS";
                
                this->executeScriptAndGetResult(games_script, [this](const std::string& jsonString) {
                    if (mOnLoginFinishedCallback) {
                        mWindow->postToUiThread([this, jsonString] {
                            mOnLoginFinishedCallback(!jsonString.empty() && jsonString != "null", jsonString);
                        });
                    }
                    close(); // Chiudi la WebView dopo aver tentato di ottenere i dati dei giochi.
                });
                return S_OK; // Evento gestito.
            }
        }
        
        // 3. Logica generica di reindirizzamento OAuth (es. Xbox Live)
        // Questa parte è già nel NavigationStarting, ma se per qualche ragione la gestisci anche qui,
        // assicurati che !mRedirectSuccessfullyHandled sia rispettato per non duplicare le chiamate.
        // La versione che ti ho dato per NavigationStarting dovrebbe già catturare e chiudere la WebView.
        // Quindi questo blocco potrebbe essere superfluo qui se la tua logica principale è in NavigationStarting.
        if (!this->mWatchRedirectPrefix.empty() && srcUri.rfind(this->mWatchRedirectPrefix, 0) == 0 && !this->mRedirectSuccessfullyHandled) {
            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Rilevato URI di redirect OAuth in NavigationCompleted.";
            // Qui dovresti ricreare o chiamare la logica di estrazione del codice e la callback,
            // simile a quanto fai in NavigationStarting per questi flussi.
            // Per ora, ci basiamo sulla logica di NavigationStarting per questi.
            // Mantiene il flag 'mRedirectSuccessfullyHandled' per non duplicare l'azione.
        }

        // 4. Logica di fallback o per mNavigationCompletedCallback generica (se non gestita da altri blocchi)
        if (mNavigationCompletedCallback && !mRedirectSuccessfullyHandled) { // Aggiunto !mRedirectSuccessfullyHandled
            mWindow->postToUiThread([this, isSuccess, srcUri] {
                if (mNavigationCompletedCallback) mNavigationCompletedCallback(isSuccess, srcUri);
            });
            // Non chiamare close() qui, a meno che non sia l'ultima logica rimanente e la WebView debba chiudersi.
            // Se la WebView è per interazione utente, dovrebbe rimanere aperta.
            // close(); // ATTENZIONE: Questo chiuderebbe la WebView anche se l'utente deve ancora interagire!
            return S_OK;
        }

        // Se nessun AuthMode specifico o redirect è stato gestito da questa funzione,
        // e la WebView deve chiudersi solo in caso di errore, ma la navigazione è fallita:
        // Se non è un flusso gestito da mRedirectSuccessfullyHandled, e la navigazione è fallita, chiudi.
        if (!isSuccess && mOnLoginFinishedCallback && !mRedirectSuccessfullyHandled) {
            mWindow->postToUiThread([this, srcUri] {
                if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Navigazione Fallita: " + srcUri);
            });
            close(); // Chiudi la WebView in caso di navigazione fallita non gestita.
            return S_OK;
        }

        return S_OK;
    }).Get(), &mNavigationCompletedToken);
                            LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Gestore NavigationCompleted aggiunto.";
                            
                            this->resizeWebView(); 
                            this->mWebViewController->put_IsVisible(mIsVisible ? TRUE : FALSE);
                            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Setup WebView2 completato. Pronto per la navigazione iniziale.";
                            this->navigateTo(this->mCurrentInitialUrl); 
                            return S_OK;
                        }).Get());

                if(FAILED(createControllerResult)) { 
                    LOG(LogError) << "[" << this->mStoreNameForLogging << "] Chiamata a mWebViewEnvironment->CreateCoreWebView2Controller fallita. HR: 0x" << std::hex << createControllerResult;
                    this->mLoading = false; 
                    if (this->mOnLoginFinishedCallback) {
                        if (this->mWindow) this->mWindow->postToUiThread([this, createControllerResult] {
                            if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Chiamata CreateCoreWebView2Controller (HR: 0x" + Utils::String::toHexString(createControllerResult) + ")");
                        });
                        else this->mOnLoginFinishedCallback(false, "Errore Chiamata CreateCoreWebView2Controller (HR: 0x" + Utils::String::toHexString(createControllerResult) + ")");
                    }
                    if(this->mWebViewEnvironment) this->mWebViewEnvironment.Reset(); 
                }
                return createControllerResult; 
            }).Get());

    if (FAILED(hr)) { 
        LOG(LogError) << "[" << this->mStoreNameForLogging << "] Chiamata iniziale a CreateCoreWebView2EnvironmentWithOptions fallita. HR: 0x" << std::hex << hr;
        this->mLoading = false; 
        if (this->mOnLoginFinishedCallback) {
            if (this->mWindow) this->mWindow->postToUiThread([this, hr] {
                if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Chiamata CreateCoreWebView2Environment (HR: 0x" + Utils::String::toHexString(hr) + ")");
            });
            else this->mOnLoginFinishedCallback(false, "Errore Chiamata CreateCoreWebView2Environment (HR: 0x" + Utils::String::toHexString(hr) + ")");
        }
        return false; 
    }
    LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] initializeWebView: Processo di creazione ambiente avviato.";
    return true; 
}

void GuiWebViewAuthLogin::closeWebView() { 
    LOG(LogDebug) << "[" << mStoreNameForLogging << "] Tentativo di chiudere WebView.";
    if (mWebView) { 
        if (mNavigationStartingToken.value != 0) { 
            mWebView->remove_NavigationStarting(mNavigationStartingToken); 
            mNavigationStartingToken.value = 0; 
            LOG(LogDebug) << "[" << mStoreNameForLogging << "] Rimossa callback NavigationStarting.";
        }
        if (mNavigationCompletedToken.value != 0) { 
            mWebView->remove_NavigationCompleted(mNavigationCompletedToken); 
            mNavigationCompletedToken.value = 0; 
            LOG(LogDebug) << "[" << mStoreNameForLogging << "] Rimossa callback NavigationCompleted.";
        }
        if (mProcessFailedToken.value != 0) { 
            mWebView->remove_ProcessFailed(mProcessFailedToken); 
            mProcessFailedToken.value = 0; 
            LOG(LogDebug) << "[" << mStoreNameForLogging << "] Rimossa callback ProcessFailed.";
        }
    }
    if (mWebViewController) { 
        mWebViewController->Close(); 
        LOG(LogDebug) << "[" << mStoreNameForLogging << "] Chiamato mWebViewController->Close().";
    }
    mWebView.Reset(); 
    mWebViewController.Reset(); 
    mWebViewEnvironment.Reset(); 
    LOG(LogInfo) << "[" << mStoreNameForLogging << "] GuiWebViewAuthLogin: Puntatori WebView resettati.";
}

void GuiWebViewAuthLogin::navigateTo(const std::string& url) { 
    if (mWebView) {
        LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] GuiWebViewAuthLogin: Tentativo di navigazione WebView a: " << url; 
        HRESULT hr = mWebView->Navigate(Utils::String::convertToWideString(url).c_str()); 
        LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Risultato chiamata mWebView->Navigate: 0x" << std::hex << hr; 
        if (FAILED(hr)) {
            LOG(LogError) << "[" << this->mStoreNameForLogging << "] Fallimento chiamata mWebView->Navigate. HR: 0x" << std::hex << hr;
            this->mLoading = false;
            if (this->mOnLoginFinishedCallback) {
                if (this->mWindow) this->mWindow->postToUiThread([this, hr] { 
                    if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(false, "Errore Navigazione WebView (HR: 0x" + Utils::String::toHexString(hr) + ")");
                });
                else this->mOnLoginFinishedCallback(false, "Errore Navigazione WebView (HR: 0x" + Utils::String::toHexString(hr) + ")");
            }
            this->closeWebView(); 
            if (this->mWindow) this->mWindow->removeGui(this); 
        } else {
            this->mLoading = true; 
        }
    } else {
        LOG(LogWarning) << "[" << this->mStoreNameForLogging << "] WebView non ancora pronta per navigare. URL salvato: " << url;
        this->mCurrentInitialUrl = url; 
    }
}

void GuiWebViewAuthLogin::resizeWebView() { 
    if (mWebViewController) {
        RECT bounds;
        bounds.left = static_cast<LONG>(getPosition().x());
        bounds.top = static_cast<LONG>(getPosition().y());
        bounds.right = static_cast<LONG>(getPosition().x() + getSize().x());
        bounds.bottom = static_cast<LONG>(getPosition().y() + getSize().y());

        LOG(LogDebug) << "[" << mStoreNameForLogging << "] Resize WebView to Bounds: L=" << bounds.left << " T=" << bounds.top << " R=" << bounds.right << " B=" << bounds.bottom;

        HRESULT hr = mWebViewController->put_Bounds(bounds);
        if (FAILED(hr)) LOG(LogError) << "[" << mStoreNameForLogging << "] Fallimento put_Bounds in resizeWebView. HR: 0x" << std::hex << hr;
    }
}

void GuiWebViewAuthLogin::setNavigationCompletedCallback(const std::function<void(bool isSuccess, const std::string& url)>& callback)
{
    mNavigationCompletedCallback = callback;
}

void GuiWebViewAuthLogin::executeScript(const std::string& script)
{
    if (mWebView) {
        mWebView->ExecuteScript(Utils::String::convertToWideString(script).c_str(), nullptr);
    }
}

void GuiWebViewAuthLogin::executeScriptAndGetResult(const std::string& script, const std::function<void(const std::string& text)>& callback)
{
    if (!mWebView) {
        if (callback) mWindow->postToUiThread([callback] { callback(""); });
        return;
    }

    // Usiamo l'approccio "paziente" a tentativi che avevi nel tuo getTextAsync originale.
    // È la soluzione più robusta per le pagine web dinamiche.
    auto tryGetScriptResult = std::make_shared<std::function<void(int)>>();
    *tryGetScriptResult = [this, script, callback, tryGetScriptResult](int retriesLeft) {
        if (retriesLeft <= 0) {
            LOG(LogError) << "[" << mStoreNameForLogging << "] SCRAPING FALLITO: Impossibile ottenere il risultato dello script dopo i tentativi.";
            if (callback) mWindow->postToUiThread([callback] { callback(""); });
            return;
        }

        mWebView->ExecuteScript(Utils::String::convertToWideString(script).c_str(), Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [this, callback, tryGetScriptResult, retriesLeft](HRESULT errorCode, LPCWSTR result) -> HRESULT {
				 LOG(LogDebug) << "[WebViewScraping-DEBUG] Raw script result: " << (result ? Utils::String::convertFromWideString(result) : "NULL") << " | HR: 0x" << std::hex << errorCode;
                std::string scriptResult;
                if (SUCCEEDED(errorCode) && result) {
                    // Eseguiamo la pulizia della stringa JSON
                    scriptResult = Utils::String::convertFromWideString(result);
                    if (scriptResult.length() > 2 && scriptResult.front() == '"' && scriptResult.back() == '"') {
                        scriptResult = scriptResult.substr(1, scriptResult.length() - 2);
                    }
                    scriptResult = Utils::String::replace(scriptResult, "\\\"", "\"");
                    scriptResult = Utils::String::replace(scriptResult, "\\\\", "\\");
                }

                // Se abbiamo un risultato valido (non vuoto e non "null"), abbiamo finito!
                if (!scriptResult.empty() && scriptResult != "null") {
                    LOG(LogInfo) << "[" << mStoreNameForLogging << "] SCRAPING RIUSCITO: Dati del profilo ottenuti!";
                    if (callback) {
                        mWindow->postToUiThread([callback, scriptResult] { callback(scriptResult); });
                    }
                    return S_OK;
                }
                
                // Se non abbiamo ancora trovato il risultato, aspettiamo e riproviamo.
                LOG(LogDebug) << "Dati non ancora pronti (Tentativo " << (25 - retriesLeft + 1) << "/25). Nuovo tentativo tra 200ms...";
                SDL_Delay(200); // Attesa bloccante ma efficace in questo contesto
                mWindow->postToUiThread([tryGetScriptResult, retriesLeft] { (*tryGetScriptResult)(retriesLeft - 1); });
                return S_OK;
            }).Get());
    };

    // Avviamo il primo tentativo con una pazienza di 5 secondi totali (25 * 200ms)
    (*tryGetScriptResult)(25);
}

void GuiWebViewAuthLogin::getTextAsync(const std::function<void(const std::string& text)>& callback)
{
#ifdef _WIN32
    if (!mWebView) {
        if (callback) mWindow->postToUiThread([callback] { callback(""); });
        return;
    }

    // Usiamo un piccolo trucco con un contatore per riprovare a leggere il testo
    // per un massimo di 2 secondi (20 tentativi ogni 100ms)
    auto tryGetText = std::make_shared<std::function<void(int)>>();
    *tryGetText = [this, callback, tryGetText](int retriesLeft) {
        if (retriesLeft <= 0) {
            LOG(LogError) << "[" << mStoreNameForLogging << "] Impossibile ottenere il testo della pagina dopo vari tentativi.";
            if (callback) mWindow->postToUiThread([callback] { callback(""); });
            return;
        }

        const wchar_t* script = L"document.documentElement.innerText";
        mWebView->ExecuteScript(script, Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [this, callback, tryGetText, retriesLeft](HRESULT errorCode, LPCWSTR result) -> HRESULT {
                std::string pageText;
                if (SUCCEEDED(errorCode) && result) {
                    std::wstring wResult(result);
                    // Se il risultato NON è vuoto o "null", abbiamo trovato il testo!
                    if (!wResult.empty() && wResult != L"null") {
                        pageText = Utils::String::convertFromWideString(wResult);
                        LOG(LogInfo) << "[" << mStoreNameForLogging << "] Contenuto pagina ottenuto con successo!";
                        if (callback) {
                            mWindow->postToUiThread([callback, pageText] { callback(pageText); });
                        }
                        return S_OK;
                    }
                }
                
                // Se non abbiamo ancora trovato il testo, aspettiamo 100ms e riproviamo.
                LOG(LogDebug) << "Testo non ancora pronto, nuovo tentativo tra 100ms...";
                SDL_Delay(100);
                mWindow->postToUiThread([tryGetText, retriesLeft] { (*tryGetText)(retriesLeft - 1); });
                return S_OK;
            }).Get());
    };

    // Avvia il primo tentativo
    (*tryGetText)(20);
#endif
}
void GuiWebViewAuthLogin::close()
{
#ifdef _WIN32
    closeWebView();
#endif
    if (mWindow) {
        mWindow->removeGui(this);
    }
}
 ICoreWebView2Controller* GuiWebViewAuthLogin::getWebViewController() {
 #ifdef _WIN32
     return mWebViewController.Get();
 #else
     return nullptr;
 #endif
 }
void GuiWebViewAuthLogin::getSteamCookies() {
    if (!mWebView) return;
    Microsoft::WRL::ComPtr<ICoreWebView2CookieManager> cookieManager;
    Microsoft::WRL::ComPtr<ICoreWebView2_2> webView_versioned;
    if (SUCCEEDED(mWebView.As(&webView_versioned)) && webView_versioned) {
        webView_versioned->get_CookieManager(&cookieManager);
    } else { return; }

    cookieManager->GetCookies(nullptr, Microsoft::WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>(
        [this](HRESULT hr, ICoreWebView2CookieList* cookieList) -> HRESULT {
            if (FAILED(hr) || !cookieList) return hr;
            UINT cookieCount;
            cookieList->get_Count(&cookieCount);
            
            std::string fullCookieString;
            bool foundLoginCookie = false;

            for (UINT i = 0; i < cookieCount; ++i) {
                // ... (codice per estrarre e costruire la fullCookieString, come nella risposta precedente) ...
            }

            if (foundLoginCookie) {
                LOG(LogInfo) << "[Steam] Stringa cookie completa costruita. Invio alla callback e chiusura.";
                mRedirectSuccessfullyHandled = true;
                if (mOnLoginFinishedCallback) {
                    mWindow->postToUiThread([this, fullCookieString] {
                        mOnLoginFinishedCallback(true, fullCookieString);
                    });
                }
                // Chiudiamo la WebView QUI, dopo aver ottenuto il risultato.
                mWindow->postToUiThread([this] { this->close(); });
            }
            return S_OK;
        }).Get());
 }
#endif // _WIN32
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


GuiWebViewAuthLogin::GuiWebViewAuthLogin(Window* window, const std::string& initialUrl, const std::string& storeNameForLogging, const std::string& watchRedirectPrefix)
    : GuiComponent(window), 
      mLoading(false),
      mCurrentInitialUrl(initialUrl),
      mStoreNameForLogging(storeNameForLogging),
      mWatchRedirectPrefix(watchRedirectPrefix),
      mCachedLoadingMsgText(""),
	  mRedirectSuccessfullyHandled(false)
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
                                    cookieManager->DeleteAllCookies();
                                    LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Chiamata a DeleteAllCookies() per il profilo WebView eseguita con successo.";
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

                            this->mWebView->add_NavigationStarting( Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
    [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
        PWSTR uriRaw = nullptr; args->get_Uri(&uriRaw);
        std::string uriA = Utils::String::convertFromWideString(std::wstring(uriRaw ? uriRaw : L""));
        CoTaskMemFree(uriRaw);

        LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] WebView NavigationStarting: " << uriA;
        this->mLoading = true; // Considera una gestione più granulare dello stato di caricamento

        // Aggiungi il controllo !this->mRedirectSuccessfullyHandled per maggiore robustezza
        if (!this->mRedirectSuccessfullyHandled && !this->mWatchRedirectPrefix.empty() && uriA.rfind(this->mWatchRedirectPrefix, 0) == 0) {
            LOG(LogInfo) << "[" << this->mStoreNameForLogging << "] Rilevato URI di redirect (" << this->mWatchRedirectPrefix << "): " << uriA;
            this->mRedirectSuccessfullyHandled = true;

            if (this->mOnLoginFinishedCallback) {
                if (this->mWindow) {
                    this->mWindow->postToUiThread([this, uriA] {
                        if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(true, uriA);
                    });
                } else {
                    // Fallback se mWindow è null, anche se in un contesto GUI dovrebbe esistere
                    if (this->mOnLoginFinishedCallback) this->mOnLoginFinishedCallback(true, uriA);
                }
            }
                       if (this->mWindow) {
                this->mWindow->postToUiThread([this] {
                    this->closeWebView();
                    // Controlla di nuovo mWindow perché potrebbe essere stato invalidato
                    if (this->mWindow) {
                        this->mWindow->removeGui(this);
                    }
                });
            } else {
                // Se mWindow è null, non si può posticipare. Chiamata diretta (potenzialmente rischiosa se non sull'UI thread).
                // Tuttavia, le callback di WebView2 sono generalmente sull'UI thread.
                this->closeWebView();
                // Non si può chiamare removeGui se mWindow è null o la GUI è già stata rimossa.
            }
            // Anche se non annulliamo la navigazione, una volta pianificata la callback e la chiusura,
            // abbiamo terminato con questo evento specifico.
            return S_OK;
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

        // NUOVA LOGICA: Se il nuovo callback è impostato, usiamo quello e ci fermiamo.
        if (mNavigationCompletedCallback) {
            mWindow->postToUiThread([this, isSuccess, srcUri] {
                if (mNavigationCompletedCallback) mNavigationCompletedCallback(isSuccess, srcUri);
            });
            return S_OK;
        }

        // VECCHIA LOGICA: Altrimenti, prosegui con la gestione errori per il vecchio sistema.
        if (mRedirectSuccessfullyHandled) { return S_OK; }
        if (!isSuccess && mOnLoginFinishedCallback) {
            mWindow->postToUiThread([this, srcUri] {
                if (mOnLoginFinishedCallback) mOnLoginFinishedCallback(false, "Navigazione Fallita: " + srcUri);
            });
            close();
        }
        return S_OK;
    }).Get(), &mNavigationCompletedToken);
                            LOG(LogDebug) << "[" << this->mStoreNameForLogging << "] Gestore NavigationCompleted aggiunto.";
                            
                            this->resizeWebView(); 
                            this->mWebViewController->put_IsVisible(TRUE);
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
#endif // _WIN32
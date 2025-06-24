#pragma once
#ifndef ES_APP_GUIS_GUI_WEBVIEW_AUTH_LOGIN_H
#define ES_APP_GUIS_GUI_WEBVIEW_AUTH_LOGIN_H

// PRIMA DI TUTTO: Include per tipi fondamentali usati come membri
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>      // Definisce HWND
  
  #include <wrl/client.h>   // Per Microsoft::WRL::ComPtr
  #include <wrl/implements.h> // Per Microsoft::WRL::Make
  #include <wrl.h> 
#include <WebView2EnvironmentOptions.h>
  #include "WebView2.h"       // Definisce ICoreWebView2*, EventRegistrationToken, etc.
#endif

// Poi gli include standard C++
#include <functional>
#include <string>
#include <vector>
#include <memory> // Per std::unique_ptr

// Infine gli include di EmulationStation
#include "GuiComponent.h" 
#include "resources/Font.h" // Includi Font.h, che definisce TextCache

// Forward declaration per COREWEBVIEW2_WEB_ERROR_STATUS se non incluso globalmente da WebView2.h
// Di solito WebView2.h definisce questo enum.
#ifndef __corewebview2_h__ 
  // Se __corewebview2_h__ non è definito da WebView2.h (improbabile con l'SDK corretto),
  // potresti dover definire l'enum qui o assicurarti che WebView2.h sia incluso correttamente.
#endif


class GuiWebViewAuthLogin : public GuiComponent
{
public:
 enum class AuthMode {
        DEFAULT,
        FETCH_STEAM_COOKIE,
		 FETCH_STEAM_GAMES_JSON, // <-- AGGIUNGI QUESTA RIGA
		  OAUTH_CODE_IN_QUERY,    // Standard: cerca ?code=...
        AMAZON_OAUTH_FRAGMENT,  // Per Amazon: cerca #...access_token=...
        GOG_LOGIN_POLLING
    };



        GuiWebViewAuthLogin(Window* window, const std::string& initialUrl, const std::string& storeNameForLogging, const std::string& watchRedirectPrefix = "", AuthMode mode = AuthMode::DEFAULT, bool visible = true, const std::string& fragmentIdentifier = "");

	
    virtual ~GuiWebViewAuthLogin();

    void render(const Transform4x4f& parentTrans) override;
    void update(int deltaTime) override;
    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;

    void init(); 
    void setOnLoginFinishedCallback(const std::function<void(bool success, const std::string& dataOrError)>& callback);
	void getTextAsync(const std::function<void(const std::string& text)>& callback);
	void setNavigationCompletedCallback(const std::function<void(bool isSuccess, const std::string& url)>& callback);
	void navigate(const std::string& url);
	void close();
	void setWatchRedirectPrefix(const std::string& prefix);
	ICoreWebView2Controller* getWebViewController();
	void setSteamCookieDomain(const std::string& domain);
    void executeScript(const std::string& script);
    void executeScriptAndGetResult(const std::string& script, const std::function<void(const std::string&)>& callback);
	const std::string& getAuthCode() const { return mAuthCode; }


private: 
#ifdef _WIN32
    bool initializeWebView();
    void closeWebView();
    void resizeWebView();
#endif
    void navigateTo(const std::string& url);
    
    bool mLoading;
    std::function<void(bool success, const std::string& dataOrError)> mOnLoginFinishedCallback;
    std::string mCurrentInitialUrl;
    std::string mStoreNameForLogging;
    std::string mWatchRedirectPrefix; 
    std::unique_ptr<TextCache> mTextCache; 
    std::string mCachedLoadingMsgText; // NUOVA MODIFICA: Per tracciare il testo del TextCache
	bool mRedirectSuccessfullyHandled;
	std::function<void(bool isSuccess, const std::string& url)> mNavigationCompletedCallback;

#ifdef _WIN32
    HWND mParentHwnd;
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> mWebViewEnvironment;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> mWebViewController;
    Microsoft::WRL::ComPtr<ICoreWebView2> mWebView;
    EventRegistrationToken mNavigationStartingToken;
    EventRegistrationToken mNavigationCompletedToken;
    EventRegistrationToken mProcessFailedToken; 
#endif
     AuthMode mAuthMode;
     std::string mAuthCode;
	 std::string mFragmentIdentifier; 
    std::string mSteamCookieDomain;
	bool mIsVisible;

#ifdef _WIN32
    void getSteamCookies(); // Funzione helper solo per Steam
#endif

};

#endif // ES_APP_GUIS_GUI_WEBVIEW_AUTH_LOGIN_H
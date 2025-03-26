//  es-app/src/WebView/WebViewGuiComponent.cpp
#include "WebView/WebViewGuiComponent.h"
#include "Window.h"
#include "Log.h"
#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h> // DWMWA_WINDOW_CORNER_PREFERENCE
#endif

void ViewController::startEpicGamesLogin() {
    //  ...

    WebViewGuiComponent* webView = new WebViewGuiComponent(mWindow);

    //  Set the size and position of the webview
    int webViewWidth = mWindow->getWidth() * 0.8;  //  80% of window width
    int webViewHeight = mWindow->getHeight() * 0.8; //  80% of window height
    int webViewX = (mWindow->getWidth() - webViewWidth) / 2;
    int webViewY = (mWindow->getHeight() - webViewHeight) / 2;
    webView->setBounds(webViewX, webViewY, webViewWidth, webViewHeight);

    mWindow->pushGui(webView);
    webView->loadUrl(EpicGamesStoreAPI::LOGIN_URL);

    //  ...
}
WebViewGuiComponent::WebViewGuiComponent(Window* window) : GuiComponent(window) {
#ifdef _WIN32
    // Get the HWND of the EmulationStation window
    HWND hwnd = (HWND)window->getHwnd();
    m_hwnd = hwnd;

    // Create a WebView2 Environment
    HRESULT env_result = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, Callback<ICoreWebView2EnvironmentCreationCompletedHandler>(
        [this](HRESULT errorCode, ICoreWebView2Environment* environment) -> HRESULT {
            if (errorCode != S_OK) {
                LOG(LogError) << "Failed to create WebView2 Environment: " << errorCode;
                return E_FAIL;
            }

            m_webViewEnvironment = environment;
            // Create a WebView2 Controller
            HRESULT controller_result = m_webViewEnvironment->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT errorCode, ICoreWebView2Controller* controller) -> HRESULT {
                    if (errorCode != S_OK) {
                        LOG(LogError) << "Failed to create WebView2 Controller: " << errorCode;
                        return E_FAIL;
                    }
                    m_webViewcontroller = controller;
                    m_webViewcontroller->get_CoreWebView2(&m_webView);

                    // Resize WebView2 to match GuiComponent
                    RECT bounds;
                    GetClientRect(m_hwnd, &bounds);
                    m_webViewcontroller->put_Bounds(bounds);

                    // Register event handlers
                    EventRegistrationToken token;
                    m_webView->AddNavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                            BOOL is_success;
                            args->get_IsSuccess(&is_success);
                            if (is_success) {
                                wil::unique_cotaskmem_string uri;
                                m_webView->get_Source(&uri);
                                handleNavigation(std::wstring(uri.get()).begin(), std::wstring(uri.get()).end());
                            }
                            return S_OK;
                        }).Get(), &token);

                    return S_OK;
                }).Get());
            return S_OK;
        }).Get());
#endif
}

WebViewGuiComponent::~WebViewGuiComponent() {
#ifdef _WIN32
    if (m_webViewcontroller) {
        m_webViewcontroller->Close();
        m_webViewcontroller = nullptr;
    }
    m_webViewEnvironment = nullptr;
    m_webView = nullptr;
#endif
}

void WebViewGuiComponent::loadUrl(const std::string& url) {
#ifdef _WIN32
    if (m_webView) {
        m_webView->Navigate(std::wstring(url.begin(), url.end()).c_str());
    }
#endif
}

void WebViewGuiComponent::setNavigationCallback(std::function<void(const std::string&)> callback) {
    mNavigationCallback = callback;
}

void WebViewGuiComponent::setCloseCallback(std::function<void()> callback) {
    mCloseCallback = callback;
}

void WebViewGuiComponent::handleNavigation(const std::string& url) {
    if (mNavigationCallback) {
        mNavigationCallback(url);
    }
    //  Example:
    //  if (url.find(EpicGamesStoreAPI::AUTH_CODE_URL) == 0) {
    //      //  Estrarre il codice di autorizzazione dall'URL (implementa questa funzione)
    //      std::string authorizationCode = extractAuthorizationCode(url);
    //      if (mNavigationCallback) {
    //          mNavigationCallback(authorizationCode);
    //      }
    //  }
}

void WebViewGuiComponent::handleClose() {
    if (mCloseCallback) {
        mCloseCallback();
    }
}

std::string WebViewGuiComponent::GetCurrentUrl() {
#ifdef _WIN32
    if (m_webView) {
        wil::unique_cotaskmem_string uri;
        m_webView->get_Source(&uri);
        return std::wstring(uri.get()).begin(), std::wstring(uri.get()).end();
    }
#endif
    return "";
}

void WebViewGuiComponent::setBounds(int x, int y, int width, int height) {
#ifdef _WIN32
    if (m_webViewcontroller) {
        RECT bounds = { x, y, x + width, y + height };
        m_webViewcontroller->put_Bounds(bounds);
    }
#endif
}

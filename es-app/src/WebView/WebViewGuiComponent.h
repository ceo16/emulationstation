#ifndef EMULATIONSTATION_WEBVIEWGUICOMPONENT_H
#define EMULATIONSTATION_WEBVIEWGUICOMPONENT_H

#include "GuiComponent.h"
#include <string>
#include <functional>

#ifdef _WIN32
#include <wrl.h>
#include <wil/com.h>
#include <Microsoft.Web.WebView2.Core.h>
#endif

class WebViewGuiComponent : public GuiComponent {
public:
    WebViewGuiComponent(Window* window);
    ~WebViewGuiComponent();

    void loadUrl(const std::string& url);
    void setNavigationCallback(std::function<void(const std::string&)> callback);
    void setCloseCallback(std::function<void()> closeCallback);
    std::string GetCurrentUrl();

private:
#ifdef _WIN32
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> m_webViewcontroller;
    wil::com_ptr<ICoreWebView2> m_webView;
    HWND m_hwnd; // Handle to the WebView2 window
#endif

    std::function<void(const std::string&)> mNavigationCallback;
    std::function<void()> mCloseCallback;
    void handleNavigation(const std::string& url);
    void handleClose();
};

#endif // EMULATIONSTATION_WEBVIEWGUICOMPONENT_H

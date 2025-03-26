#ifndef EMULATIONSTATION_WEBVIEWGUICOMPONENT_H
#define EMULATIONSTATION_WEBVIEWGUICOMPONENT_H

#include "GuiComponent.h"
#include <string>
#include <functional>

class WebViewGuiComponent : public GuiComponent {
public:
    WebViewGuiComponent(Window* window);
    ~WebViewGuiComponent();

    void loadUrl(const std::string& url);
    void setNavigationCallback(std::function<void(const std::string&)> callback);
    void setCloseCallback(std::function<void()> closeCallback);
    std::string GetCurrentUrl(); // to get current URL

private:
    //  ... (Membri per gestire la libreria di visualizzazione web)
    //  Esempio:
    //  QtWebEngineView* webView; // Se si usa Qt WebEngine

    std::function<void(const std::string&)> mNavigationCallback;
    std::function<void()> mCloseCallback;
    void handleNavigation(const std::string& url);
    void handleClose();
};

#endif // EMULATIONSTATION_WEBVIEWGUICOMPONENT_H

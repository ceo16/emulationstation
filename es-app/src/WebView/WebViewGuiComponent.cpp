#include "WebViewGuiComponent.h"
#include "Window.h"
// #include <QtWebEngineWidgets/QWebEngineView> // Esempio per Qt WebEngine

WebViewGuiComponent::WebViewGuiComponent(Window* window) : GuiComponent(window) {
    // Inizializza la visualizzazione web (Qt WebEngine example)
    // webView = new QWebEngineView(this);
    // webView->resize(Renderer::getScreenWidth() * 0.8, Renderer::getScreenHeight() * 0.8); // Adjust size as needed
    // QObject::connect(webView, &QWebEngineView::urlChanged, [this](const QUrl& url) {
    //     handleNavigation(url.toString().toStdString());
    // });
    // QObject::connect(webView, &QWebEngineView::destroyed, [this]() {
    //     handleClose();
    // });

    // Add the webview to the GuiComponent
    // this->addChild(webView);
}

WebViewGuiComponent::~WebViewGuiComponent() {
    // Pulisci la visualizzazione web
    // delete webView;
}

void WebViewGuiComponent::loadUrl(const std::string& url) {
    // Carica l'URL nella visualizzazione web
    // webView->load(QUrl(QString::fromStdString(url)));
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
    //      //  Estrarre il codice di autorizzazione dall'URL
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
    // return webView->url().toString().toStdString();
    return "";
}

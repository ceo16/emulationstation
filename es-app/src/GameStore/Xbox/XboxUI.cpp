#include "GameStore/Xbox/XboxUI.h"
#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/Xbox/XboxAuth.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"          // Per GuiMsgBox e GuiMsgBoxIcon
#include "guis/GuiTextEditPopup.h"   // Per GuiTextEditPopup
#include "guis/GuiBusyInfoPopup.h"
#include "Log.h"
#include "LocaleES.h"
#include "utils/Platform.h"        // Per Utils::Platform::openUrl
#include "utils/StringUtil.h"      // Per Utils::String::toUpper
#include "renderers/Renderer.h"              // Per Renderer::getScreenWidth/Height, Renderer::swapBuffers()
#include "Settings.h"              // Per Settings::getInstance() se usato per OSK

// NON includere <SDL.h> o SdlEvents.h qui se non invii eventi SDL da XboxUI per l'autenticazione

XboxUI::XboxUI(Window* window, XboxStore* store) :
    GuiComponent(window),
    mStore(store),
    mMenu(window, _("XBOX LIVE")), // Titolo per MenuComponent
    mLastAuthStatus(false)       // Inizializza mLastAuthStatus
{
    mWindow = window; // Inizializza il membro mWindow

    XboxAuth* auth = getAuth();
    if (!mStore || !auth) {
        LOG(LogError) << "XboxUI Constructor: XboxStore o XboxAuth non validi!";
        if (mWindow) {
            mWindow->pushGui(new GuiMsgBox(mWindow,
                _("ERRORE CRITICO XBOX") + std::string("\n") + _("Impossibile inizializzare il menu Xbox."),
                _("OK"),
                [this]() { delete this; },
                GuiMsgBoxIcon::ICON_ERROR // Usa l'enum corretto
            ));
        }
        return;
    }
    mLastAuthStatus = auth->isAuthenticated(); // Imposta lo stato iniziale

    setSize(Renderer::getScreenWidth() * 0.7f, Renderer::getScreenHeight() * 0.7f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
    mMenu.setSize(mSize.x(), mSize.y());
    mMenu.setPosition(0, 0);
    addChild(&mMenu);
    buildMenu();
}

XboxAuth* XboxUI::getAuth() const {
    return mStore ? mStore->getAuth() : nullptr;
}

void XboxUI::buildMenu() {
    mMenu.clear();
    XboxAuth* auth = getAuth();

    if (!auth) {
        mMenu.addEntry(_("ERRORE AUTENTICAZIONE"), false, nullptr);
        mMenu.addButton(_("INDIETRO"), _("esci"), [this] { delete this; });
        return;
    }

    if (auth->isAuthenticated()) {
        std::string xuid = auth->getXUID();
        mMenu.addEntry(_("UTENTE: ") + (xuid.empty() ? "N/D" : xuid), false, nullptr, "", false, false, _("XUID utente connesso"));
        mMenu.addEntry(_("LOGOUT"), true, [this] { optionLogout(); });
        mMenu.addEntry(_("AGGIORNA LISTA GIOCHI"), true, [this] { optionRefreshGamesList(); });
    } else {
        mMenu.addEntry(_("LOGIN / AUTENTICAZIONE"), true, [this] { optionLogin(); });
        mMenu.addEntry(_("INSERISCI CODICE AUTENTICAZIONE"), true, [this] { optionEnterAuthCodeSincrono(); });
    }
    mMenu.addButton(_("INDIETRO"), _("torna indietro"), [this] { delete this; });
}

void XboxUI::optionLogin() {
    LOG(LogDebug) << "XboxUI::optionLogin - Inizio. mWindow: " << static_cast<void*>(mWindow);
    XboxAuth* auth = getAuth();
    if (!auth || !mWindow) {
        LOG(LogError) << "XboxUI::optionLogin - Auth o mWindow non validi.";
        return;
    }

    std::string state_unused;
    std::string authUrl = auth->getAuthorizationUrl(state_unused);
    LOG(LogDebug) << "XboxUI::optionLogin - Auth URL: " << (authUrl.empty() ? "VUOTO" : authUrl.c_str());

    if (authUrl.empty()) {
        mWindow->pushGui(new GuiMsgBox(mWindow,
            _("ERRORE LOGIN XBOX") + std::string("\n") + _("Impossibile ottenere l'URL di autenticazione."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        return;
    }

    std::string msg = _("Verrai reindirizzato al sito di Microsoft...\n" /* Messaggio completo omesso per brevità */);
    mWindow->pushGui(new GuiMsgBox(mWindow,
        _("ISTRUZIONI LOGIN XBOX") + std::string("\n\n") + msg,
        _("APRI BROWSER"), [this, authUrl] { Utils::Platform::openUrl(authUrl); },
        _("ANNULLA"), nullptr,
         GuiMsgBoxIcon::ICON_INFORMATION // << USA ICON_NONE PER EVITARE CRASH CON info.svg
    ));
    LOG(LogDebug) << "XboxUI::optionLogin - GuiMsgBox con istruzioni mostrato.";
}

void XboxUI::optionEnterAuthCodeSincrono() {
    LOG(LogDebug) << "XboxUI::optionEnterAuthCodeSincrono - Inizio. mWindow: " << static_cast<void*>(mWindow);
    XboxAuth* auth = getAuth();
    if (!auth) { /* ... (gestione errore come nella risposta precedente) ... */ return; }
    if (auth->isAuthenticated()) { /* ... (messaggio "già autenticato" con ICON_NONE) ... */ return; }
    if (!mWindow) { /* ... (errore mWindow nullo) ... */ return; }

    std::string acceptBtnText = _("INVIA");

    LOG(LogDebug) << "XboxUI::optionEnterAuthCodeSincrono - Creazione GuiTextEditPopup.";
    mWindow->pushGui(new GuiTextEditPopup(mWindow,
        _("INSERISCI CODICE AUTENTICAZIONE XBOX"), "",
        [this, auth](const std::string& code) { // okCallback
            if (!code.empty()) {
                LOG(LogInfo) << "XboxUI (okCallback): Codice inserito: [" << code << "]";
                if (!mWindow || !auth) { /* ... (log errore e return) ... */ return; }

                GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(mWindow, _("AUTENTICAZIONE IN CORSO..."));
                mWindow->pushGui(busyPopup);
                if(mWindow) { mWindow->render(); Renderer::swapBuffers(); }

                bool success = auth->exchangeAuthCodeForTokens(code); // BLOCCANTE
                LOG(LogInfo) << "XboxUI (okCallback): Risultato autenticazione: " << success;

                if (mWindow && mWindow->peekGui() == busyPopup) { delete busyPopup; }
                else { if (busyPopup) delete busyPopup; }
                busyPopup = nullptr;

                if (success) {
                    LOG(LogInfo) << "XboxUI (okCallback): Autenticazione riuscita.";
                    if (mWindow) mWindow->pushGui(new GuiMsgBox(mWindow,
                        _("XBOX LOGIN") + std::string("\n") + _("Autenticazione Xbox riuscita!"),
                        _("OK"), nullptr,  GuiMsgBoxIcon::ICON_INFORMATION)); // ICON_NONE
                    // buildMenu() verrà chiamato da update() a causa del cambio di mLastAuthStatus
                } else {
                    LOG(LogError) << "XboxUI (okCallback): Autenticazione fallita.";
                    if (mWindow) mWindow->pushGui(new GuiMsgBox(mWindow,
                        _("XBOX LOGIN") + std::string("\n") + _("Autenticazione Xbox fallita. Riprova."),
                        _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
                }
            }
        },
        false,
        acceptBtnText.c_str() // O solo acceptBtnText se _() dà char* e non hai errori C2664
    ));
    LOG(LogDebug) << "XboxUI::optionEnterAuthCodeSincrono - GuiTextEditPopup creato.";
}

void XboxUI::optionLogout() {
    LOG(LogDebug) << "XboxUI::optionLogout - Inizio.";
    XboxAuth* auth = getAuth();
    if (auth && mWindow) {
        auth->clearAllTokenData();
        mWindow->pushGui(new GuiMsgBox(mWindow,
            _("XBOX LOGOUT") + std::string("\n") + _("Logout effettuato."),
            _("OK"), nullptr,  GuiMsgBoxIcon::ICON_INFORMATION // ICON_NONE
        ));
    }
    // buildMenu() verrà chiamato da update()
}

void XboxUI::optionRefreshGamesList() {
    LOG(LogDebug) << "XboxUI::optionRefreshGamesList - Inizio.";
    if (!mStore || !mWindow) return;
    XboxAuth* auth = getAuth();
    if (!auth || !auth->isAuthenticated()) {
        mWindow->pushGui(new GuiMsgBox(mWindow,
            _("XBOX") + std::string("\n") + _("Devi essere autenticato per aggiornare."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_WARNING));
        return;
    }
    GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(mWindow, _("AGGIORNAMENTO LIBRERIA XBOX..."));
    mWindow->pushGui(busyPopup);
    mStore->refreshGamesListAsync(); // Invia SDL_XBOX_REFRESH_COMPLETE
    delete this; // Chiudi XboxUI, il busyPopup è gestito da main.cpp
}

bool XboxUI::input(InputConfig* config, Input input) {
    if (mMenu.input(config, input)) return true;
    if (config->isMappedTo("b", input) && input.value != 0) {
        delete this; return true;
    }
    return GuiComponent::input(config, input);
}

void XboxUI::update(int deltaTime) {
    GuiComponent::update(deltaTime); // Chiama updateChildren, incluso mMenu

    XboxAuth* auth = getAuth();
    if (auth) {
        bool currentAuthStatus = auth->isAuthenticated();
        if (mLastAuthStatus != currentAuthStatus) {
            LOG(LogInfo) << "XboxUI::update - Stato autenticazione cambiato. Ricostruzione menu.";
            buildMenu();
            mLastAuthStatus = currentAuthStatus;
        }
    }
}
void XboxUI::rebuildMenu() {
    LOG(LogDebug) << "XboxUI::rebuildMenu chiamata.";
    buildMenu(); // Chiama direttamente buildMenu
}

void XboxUI::render(const Transform4x4f& parentTrans) {
    Transform4x4f trans = parentTrans * getTransform();
    // Opzionale: Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0x000000A0, 0x000000A0, false, Renderer::BlendFactor::BLEND_SRC_ALPHA, Renderer::BlendFactor::BLEND_ONE_MINUS_SRC_ALPHA);
    GuiComponent::renderChildren(trans);
}

std::vector<HelpPrompt> XboxUI::getHelpPrompts() {
    return mMenu.getHelpPrompts();
}

// void XboxUI::rebuildMenu() { // Potrebbe non servire più se update() gestisce il refresh
//     if (mMenu.getTheme()){ buildMenu(); }
// }
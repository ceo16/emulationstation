#include "GameStore/Xbox/XboxUI.h"
#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/Xbox/XboxAuth.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
// #include "guis/GuiTextEditPopup.h" // Rimosso: non più necessario
#include "guis/GuiBusyInfoPopup.h"
#include "Log.h"
#include "LocaleES.h"
#include "utils/Platform.h" // Potrebbe non servire più, ma lascialo se usato altrove
#include "utils/StringUtil.h"
#include "renderers/Renderer.h"
#include "Settings.h"

// ... (restanti include e globali come pfnExclusionListGlobal) ...

XboxUI::XboxUI(Window* window, XboxStore* store) :
    GuiComponent(window),
    mStore(store),
    mMenu(window, _("XBOX LIVE")),
    mLastAuthStatus(false)
{
    mWindow = window;

    XboxAuth* auth = getAuth();
    if (!mStore || !auth) {
        LOG(LogError) << "XboxUI Constructor: XboxStore o XboxAuth non validi!";
        if (mWindow) {
            mWindow->pushGui(new GuiMsgBox(mWindow,
                _("ERRORE CRITICO XBOX") + std::string("\n") + _("Impossibile inizializzare il menu Xbox."),
                _("OK"),
                [this]() { delete this; },
                GuiMsgBoxIcon::ICON_ERROR
            ));
        }
        return;
    }
    mLastAuthStatus = auth->isAuthenticated();

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
        mMenu.addEntry(_("UTENTE: ") + (xuid.empty() ? _("N/D") : xuid), false, nullptr, "", false, false, _("XUID utente connesso"));
        mMenu.addEntry(_("LOGOUT"), true, [this] { optionLogout(); });
        mMenu.addEntry(_("AGGIORNA LISTA GIOCHI"), true, [this] { optionRefreshGamesList(); });
    } else {
        // NUOVO: Unico punto di login via WebView
        mMenu.addEntry(_("LOGIN XBOX LIVE"), true, [this] { optionLogin(); });
        // Rimosso: optionEnterAuthCodeSincrono() non più necessario
    }
    mMenu.addButton(_("INDIETRO"), _("torna indietro"), [this] { delete this; });
}

// MODIFICATO: Ora avvia direttamente il flusso di autenticazione WebView
void XboxUI::optionLogin() {
    LOG(LogDebug) << "XboxUI::optionLogin - Avvio login Xbox Live via WebView.";
    XboxAuth* auth = getAuth();
    if (!auth || !mWindow) {
        LOG(LogError) << "XboxUI::optionLogin - Auth o mWindow non validi.";
        mWindow->pushGui(new GuiMsgBox(mWindow,
            _("ERRORE LOGIN XBOX") + std::string("\n") + _("Impossibile avviare il processo di login."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        return;
    }

    // Chiamata diretta al metodo di autenticazione WebView di XboxAuth
    auth->authenticateWithWebView(mWindow);

    LOG(LogDebug) << "XboxUI::optionLogin - Chiamato authenticateWithWebView. Attendere il callback.";
}

// RIMOSSO/DEPRECATO: Questo metodo non è più necessario con il flusso WebView completo
// void XboxUI::optionEnterAuthCodeSincrono() { /* ... */ }


void XboxUI::optionLogout() {
    LOG(LogDebug) << "XboxUI::optionLogout - Inizio.";
    XboxAuth* auth = getAuth();
    if (auth && mWindow) {
        auth->clearAllTokenData();
        mWindow->pushGui(new GuiMsgBox(mWindow,
            _("XBOX LOGOUT") + std::string("\n") + _("Logout effettuato."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION
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
    mStore->refreshGamesListAsync(); // Questa funzione invia SDL_XBOX_REFRESH_COMPLETE al thread principale

    // IMPORTANTE: Non fare delete this qui. La UI di XboxUI deve rimanere aperta mentre il popup è mostrato.
    // La gestione della chiusura del busyPopup e dell'aggiornamento della UI avverrà
    // nel loop di gestione degli eventi SDL nel tuo main.cpp.
}

bool XboxUI::input(InputConfig* config, Input input) {
    if (mMenu.input(config, input)) return true;
    if (input.value != 0 && (config->isMappedTo("b", input) || (input.id == SDLK_ESCAPE && input.type == TYPE_KEY))) {
        LOG(LogDebug) << "XboxUI: Chiusura per input 'Indietro'.";
        delete this;
        return true;
    }
    return GuiComponent::input(config, input);
}

void XboxUI::update(int deltaTime) {
    GuiComponent::update(deltaTime);

    XboxAuth* auth = getAuth();
    if (auth) {
        bool currentAuthStatus = auth->isAuthenticated();
        if (mLastAuthStatus != currentAuthStatus) {
            LOG(LogInfo) << "XboxUI::update - Stato autenticazione Xbox cambiato. Ricostruzione menu.";
            buildMenu();
            mLastAuthStatus = currentAuthStatus;
        }
    }
}
void XboxUI::rebuildMenu() {
    LOG(LogDebug) << "XboxUI::rebuildMenu chiamata.";
    buildMenu();
}

void XboxUI::render(const Transform4x4f& parentTrans) {
    Transform4x4f trans = parentTrans * getTransform();
    GuiComponent::renderChildren(trans);
}

std::vector<HelpPrompt> XboxUI::getHelpPrompts() {
    return mMenu.getHelpPrompts();
}
#include "GameStore/Steam/SteamUI.h"
#include "GameStore/Steam/SteamStore.h"
#include "GameStore/Steam/SteamAuth.h"

#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiBusyInfoPopup.h"
#include "LocaleES.h"
#include "Log.h"
#include "Window.h"
#include "GuiComponent.h" // Per GuiComponent*
#include "utils/StringUtil.h"
#include <functional>
#include <string>
#include <thread> // Per il thread di validazione API Key

SteamUI::SteamUI() {
    LOG(LogDebug) << "SteamUI: Costruttore";
}

// Funzione helper statica per chiudere il menu corrente e riaprirlo
void SteamUI::reloadSettingsMenu(Window* window, SteamStore* store, GuiComponent* currentMenu) {
    LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Inizio. CurrentMenu: " << currentMenu;
    if (!window || !store) {
        LOG(LogError) << "SteamUI: reloadSettingsMenu - Window o Store sono nulli.";
        return;
    }
    if (currentMenu) {
        LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Rimozione e deallocazione del menu GuiSettings corrente.";
        window->removeGui(currentMenu); // PRIMA: Rimuovi dalla pila della GUI
        delete currentMenu;             // DOPO: Dealloca la memoria
        currentMenu = nullptr;
    } else {
        LOG(LogWarning) << "SteamUI: reloadSettingsMenu - currentMenu era già nullo, nessuna azione di rimozione.";
    }

    LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Chiamata a store->showStoreUI per mostrare il menu aggiornato.";
    store->showStoreUI(window); // Chiedi allo store di mostrare di nuovo la sua UI principale (che ricreerà il menu)
}

// IMPLEMENTAZIONE: Metodo optionLogin
void SteamUI::optionLogin(Window* window, SteamStore* store, GuiSettings* currentMenu) {
    LOG(LogDebug) << "SteamUI::optionLogin - Avvio login Steam via WebView.";
    SteamAuth* auth = store->getAuth();
    if (!auth || !window || !store || !currentMenu) {
        LOG(LogError) << "SteamUI::optionLogin - Prerequisiti non validi.";
        window->pushGui(new GuiMsgBox(window,
            _("ERRORE LOGIN STEAM") + std::string("\n") + _("Impossibile avviare il processo di login."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        return;
    }

    auth->authenticateWithWebView(window);

    // Chiudi il menu attuale subito per evitare interazioni doppie.
    // Il menu verrà riaperto automaticamente dal callback di SteamAuth (tramite reloadSettingsMenu)
    // dopo che il login WebView è completato e lo stato di autenticazione è cambiato.
    LOG(LogDebug) << "SteamUI: optionLogin - Chiudo il menu corrente (" << currentMenu << "). Attendo callback da WebView.";
    currentMenu->close(); // Chiude il GuiSettings corrente.
}

// IMPLEMENTAZIONE: Metodo optionLogout
void SteamUI::optionLogout(Window* window, SteamStore* store, GuiSettings* currentMenu) {
    LOG(LogDebug) << "SteamUI::optionLogout - Inizio.";
    SteamAuth* auth = store->getAuth();
    if (auth && window && store && currentMenu) {
        auth->clearCredentials(); // Pulisce tutte le credenziali
        window->pushGui(new GuiMsgBox(window,
            _("STEAM LOGOUT") + std::string("\n") + _("Logout effettuato."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION
        ));
        reloadSettingsMenu(window, store, currentMenu); // Ricarica il menu per riflettere lo stato di logout
    } else {
        LOG(LogError) << "SteamUI::optionLogout - Prerequisiti non validi.";
    }
}

// IMPLEMENTAZIONE: Metodo optionRefreshGamesList
void SteamUI::optionRefreshGamesList(Window* window, SteamStore* store, GuiSettings* currentMenu) {
    LOG(LogDebug) << "SteamUI::optionRefreshGamesList - Inizio. Verifica prerequisiti.";
    Log::flush(); // FORZA IL LOG A ESSERE SCRITTO IMMEDIATAMENTE

    SteamAuth* auth = store->getAuth();
    if (!store || !window || !auth || !auth->isAuthenticated() || !currentMenu) {
        LOG(LogError) << "SteamUI::optionRefreshGamesList - Prerequisiti non validi o non autenticato. Ritorno.";
        Log::flush(); // FORZA IL LOG
        window->pushGui(new GuiMsgBox(window,
            _("STEAM") + std::string("\n") + _("Devi essere autenticato per aggiornare la lista giochi."),
            _("OK"), nullptr, GuiMsgBoxIcon::ICON_WARNING));
        return;
    }

    LOG(LogDebug) << "SteamUI::optionRefreshGamesList - Prerequisiti OK. Creazione e visualizzazione busy popup.";
    Log::flush(); // FORZA IL LOG
    GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("AGGIORNAMENTO LIBRERIA STEAM..."));
    window->pushGui(busyPopup);
    // Non c'è bisogno di flush qui, postToUiThread gestisce il suo contesto, ma per estrema sicurezza...
    Log::flush(); 

    currentMenu->close(); // Chiude il menu delle impostazioni subito dopo aver avviato il popup.
    LOG(LogDebug) << "SteamUI: optionRefreshGamesList - Chiuso menu impostazioni (" << currentMenu << "). Avvio refresh asincrono.";
    Log::flush(); // FORZA IL LOG PRIMA DI AVVIARE IL THREAD ASINCRONO

    store->refreshSteamGamesListAsync(); // Questa funzione invia un evento SDL_USEREVENT al thread principale
}

// Metodo principale per mostrare il menu delle impostazioni Steam
void SteamUI::showSteamSettingsMenu(Window* window, SteamStore* store)
{
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Inizio.";
    if (!store) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu chiamato con store nullo.";
        window->pushGui(new GuiMsgBox(window,
                                     _("ERRORE") + std::string("\n\n") + _("Impossibile caricare le impostazioni Steam."),
                                     _("OK"), nullptr));
        return;
    }

    SteamAuth* auth = store->getAuth();
    if (!auth) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu: SteamAuth nullo fornito da SteamStore.";
        window->pushGui(new GuiMsgBox(window,
                                     _("ERRORE") + std::string("\n\n") + _("Modulo di autenticazione Steam non disponibile."),
                                     _("OK"), nullptr));
        return;
    }

    bool currentAuthState = auth->isAuthenticated();
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Stato letto da auth->isAuthenticated(): " << currentAuthState;
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Valore di auth->hasCredentials(): " << auth->hasCredentials();
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Valore di auth->getUserPersonaName(): " << auth->getUserPersonaName();
    
    GuiSettings* menu = new GuiSettings(window, Utils::String::toUpper(_("IMPOSTAZIONI ACCOUNT STEAM")));
    LOG(LogDebug) << "SteamUI: GuiSettings menu creato: " << menu;

    // --- Sezione Stato Autenticazione ---
    std::string authStatusText;
    if (auth->isAuthenticated()) {
        authStatusText = std::string(_("STATO: AUTENTICATO COME ")) + auth->getUserPersonaName();
    } else if (auth->hasCredentials()) {
        std::string apiKeyStatus = auth->getApiKey().empty() ? _("NO") : _("SI");
        std::string steamIdStr = auth->getSteamId();
        std::string steamIdStatus = steamIdStr.empty() ? _("NO") : steamIdStr;

        authStatusText = _("STATO: CREDENZIALI PRESENTI (API Key: ") + apiKeyStatus +
                         _(", SteamID: ") + steamIdStatus + _(") - NON VALIDATE");
    } else {
        authStatusText = _("STATO: NON AUTENTICATO (NESSUNA CREDENZIALE)");
    }
    LOG(LogDebug) << "SteamUI: Stato autenticazione: " << authStatusText;
    menu->addEntry(authStatusText, false, nullptr);

    // --- Opzioni di Login/Logout ---
    if (auth->isAuthenticated()) {
        menu->addEntry(_("LOGOUT"), true,
            [this, window, store, menu_ptr = menu] { // Cattura 'this' per chiamare optionLogout
                this->optionLogout(window, store, menu_ptr);
            });

        menu->addEntry(_("AGGIORNA LISTA GIOCHI ONLINE"), true,
            [this, window, store, menu_ptr = menu] { // Cattura 'this' per chiamare optionRefreshGamesList
                this->optionRefreshGamesList(window, store, menu_ptr);
            });
    } else {
        menu->addEntry(_("LOGIN STEAM"), true,
            [this, window, store, menu_ptr = menu] { // Cattura 'this' per chiamare optionLogin
                this->optionLogin(window, store, menu_ptr);
            });

        // Opzione per Configurare/Modificare API Key e SteamID (per il login manuale/API Key)
        menu->addEntry(_("CONFIGURA API KEY E STEAMID"), true, [window, auth, store, menu_ptr = menu] {
            std::string currentApiKey = auth->getApiKey();
            std::string currentSteamId = auth->getSteamId();

            auto apiKeyPopup = new GuiTextEditPopup(window, _("STEAM API KEY"), currentApiKey,
                [window, auth, store, menu_ptr, currentSteamId_cap = currentSteamId](const std::string& newApiKey) {
                    auto steamIdPopup = new GuiTextEditPopup(window, _("STEAMID64"), currentSteamId_cap,
                        [window, auth, store, newApiKey_cap = newApiKey, menu_ptr](const std::string& newSteamId) {
                            if (newApiKey_cap.empty() || newSteamId.empty()) {
                                window->pushGui(new GuiMsgBox(window,
                                    _("ERRORE") + std::string("\n\n") + _("API KEY e STEAMID64 non possono essere vuoti."),
                                    _("OK"), nullptr));
                                reloadSettingsMenu(window, store, menu_ptr);
                                return;
                            }
                            auth->setCredentials(newApiKey_cap, newSteamId);
                            GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("VALIDAZIONE IN CORSO...\nAttendere prego."));
                            window->pushGui(busyPopup);
                            std::thread([auth, window, busyPopup, store, menu_ptr]() {
                                bool isValid = auth->validateAndSetAuthentication();
                                window->postToUiThread([window, busyPopup, isValid, store, menu_ptr, auth]() {
                                    if (window->peekGui() == busyPopup) {
                                        window->removeGui(busyPopup);
                                        delete busyPopup;
                                    }
                                    if (isValid) {
                                        std::string successMsg = _("Credenziali Steam validate e salvate!\nUtente: ") + auth->getUserPersonaName();
                                        window->pushGui(new GuiMsgBox(window, _("SUCCESSO") + std::string("\n\n") + successMsg, _("OK"), [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
                                    } else {
                                        std::string errorDetailMsg = _("API Key o SteamID64 non validi, oppure errore di rete.\nVerifica le credenziali e la connessione.");
                                        window->pushGui(new GuiMsgBox(window, _("ERRORE VALIDAZIONE") + std::string("\n\n") + errorDetailMsg, _("OK"), [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
                                    }
                                });
                            }).detach();
                        }, false, _("ES. 76561197960287930").c_str());
                    window->pushGui(steamIdPopup);
                }, false, _("ES. XXXXXXXXXXXXXXXXXXXXXXXX").c_str());
            window->pushGui(apiKeyPopup);
        });

        // Opzione per Cancellare Credenziali (se presenti)
        if (auth->hasCredentials()) {
            menu->addEntry(_("CANCELLA CREDENZIALI SALVATE"), true, [window, auth, store, menu_ptr = menu] {
                std::string confirmMessage = _("Sei sicuro di voler cancellare le credenziali Steam salvate?");
                window->pushGui(new GuiMsgBox(window,
                    confirmMessage,
                    _("SI"),
                    [window, auth, store, menu_ptr]() {
                        auth->clearCredentials();
                        reloadSettingsMenu(window, store, menu_ptr);
                    },
                    _("NO"), nullptr, "", nullptr, GuiMsgBoxIcon::ICON_QUESTION
                ));
            });
        }
    }
    
    // Opzione "COME OTTENERE API KEY E STEAMID64?"
    menu->addEntry(_("COME OTTENERE API KEY E STEAMID64?"), true, [window] {
        std::string helpMsg = "";
        helpMsg += _("Per accedere ai dati della tua libreria Steam:\n\n");
        helpMsg += _("1. VAI AL SITO WEB DI STEAM:\n");
        helpMsg += _("   - Login tramite browser web.\n");
        helpMsg += _("   - Per la CHIAVE API (se necessaria per altre funzioni):\n");
        helpMsg += _("     Visita: https://steamcommunity.com/dev/apikey\n");
        helpMsg += _("     Accetta i termini e genera la chiave.\n");
        helpMsg += _("   - Per il tuo STEAMID64 (ID numerico):\n");
        helpMsg += _("     Il tuo SteamID64 si trova nell'URL del tuo profilo Steam, ad es:\n");
        helpMsg += _("     https://steamcommunity.com/profiles/VULNERABILE\n");
        helpMsg += _("     (VULNERABILE è il tuo SteamID64 numerico).\n");
        helpMsg += _("     Puoi anche usare siti come steamid.io per convertirlo.\n\n");
        helpMsg += _("2. INSERISCI LE CREDENZIALI IN EMULATIONSTATION:");
        
        GuiMsgBox* msgBox = new GuiMsgBox(window,
                                         helpMsg,
                                         _("OK"),
                                         nullptr,
                                         GuiMsgBoxIcon::ICON_INFORMATION);
        if (msgBox) {
            window->pushGui(msgBox);
        }
    });

    window->pushGui(menu);
}
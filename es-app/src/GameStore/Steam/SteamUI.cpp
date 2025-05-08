#include "GameStore/Steam/SteamUI.h"   // Header corrispondente
#include "GameStore/Steam/SteamStore.h" // Per interagire con lo store
#include "GameStore/Steam/SteamAuth.h"   // Per interagire con l'autenticazione

#include "guis/GuiSettings.h"          // Componente base per menu di impostazioni
#include "guis/GuiTextEditPopup.h"     // Per l'input di API Key e SteamID
#include "guis/GuiMsgBox.h"            // Per mostrare messaggi informativi o di errore
#include "LocaleES.h"                  // Per le traduzioni _("...")
#include "Log.h"                       // Per il logging
#include "Window.h"                    // Per accedere all'istanza della finestra
#include "GuiComponent.h"              // Per delete currentMenu
#include "utils/StringUtil.h"          // Per Utils::String::toUpper
#include <functional> // Assicurati che <functional> sia incluso (GuiMsgBox.h dovrebbe farlo, ma non fa male)
#include <string>     // Assicurati che <string> sia incluso

SteamUI::SteamUI()
{
    LOG(LogDebug) << "SteamUI: Costruttore";
}

// Funzione helper statica per chiudere il menu corrente e riaprirlo
void SteamUI::reloadSettingsMenu(Window* window, SteamStore* store, GuiComponent* currentMenu)
{
    LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Inizio. CurrentMenu: " << currentMenu;
    if (!window || !store) {
        LOG(LogError) << "SteamUI: reloadSettingsMenu - Window o Store sono nulli.";
        return;
    }
    if (currentMenu) {
        LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Cancellazione del menu GuiSettings corrente.";
        delete currentMenu; // Prova a cancellare
        currentMenu = nullptr;
    } else {
         LOG(LogWarning) << "SteamUI: reloadSettingsMenu - currentMenu era già nullo?";
    }

    // Ora, invece di creare un SteamUI() temporaneo, chiama DI NUOVO il metodo
    // showStoreUI dello store. Sarà lo store a usare la sua mUI membro per
    // mostrare il menu aggiornato. Questo è un flusso più pulito.
    LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Chiamata a store->showStoreUI per mostrare il menu aggiornato.";
    store->showStoreUI(window); // Chiedi allo store di mostrare di nuovo la sua UI principale
}

void SteamUI::showSteamSettingsMenu(Window* window, SteamStore* store)
{
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Inizio.";
    if (!store) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu chiamato con store nullo.";
        // Firma probabile (window, title, msg, callback) - VERIFICATA
        window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("Impossibile caricare le impostazioni Steam."), nullptr));
        return;
    }

    SteamAuth* auth = store->getAuth();
    if (!auth) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu: SteamAuth nullo fornito da SteamStore.";
        window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("Modulo di autenticazione Steam non disponibile."), nullptr));
        return;
    }

bool currentAuthState = auth->isAuthenticated();
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Stato letto da auth->isAuthenticated(): " << currentAuthState;
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Valore di auth->hasCredentials(): " << auth->hasCredentials();
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Valore di auth->getUserPersonaName(): " << auth->getUserPersonaName();
	
	
    // CORREZIONE: Usa Utils::String::toUpper e passa std::string al costruttore
    GuiSettings* menu = new GuiSettings(window, Utils::String::toUpper(_("IMPOSTAZIONI ACCOUNT STEAM")));
    LOG(LogDebug) << "SteamUI: GuiSettings menu creato: " << menu;

    // --- Sezione Stato Autenticazione ---
    std::string authStatusText;
    if (auth->isAuthenticated()) {
        // CORREZIONE: Usa std::string() per forzare la concatenazione corretta se necessario
        authStatusText = std::string(_("STATO: AUTENTICATO COME ")) + auth->getUserPersonaName();
    } else if (auth->hasCredentials()) {
        // CORREZIONE: Usa variabili intermedie e _() per tradurre
        std::string apiKeyStatus = auth->getApiKey().empty() ? _("NO") : _("SI");
        std::string steamIdStr = auth->getSteamId();
        std::string steamIdStatus = steamIdStr.empty() ? _("NO") : steamIdStr;

        authStatusText = _("STATO: CREDENZIALI PRESENTI (API Key: ") + apiKeyStatus +
                         _(", SteamID: ") + steamIdStatus + _(") - NON VALIDATE");
    } else {
        authStatusText = _("STATO: NON AUTENTICATO (NESSUNA CREDENZIALE)");
    }
    LOG(LogDebug) << "SteamUI: Stato autenticazione: " << authStatusText;
    menu->addEntry(authStatusText, false, nullptr); // Sostituisce addSubtitle

    // --- Opzione per Configurare/Modificare API Key e SteamID ---
    menu->addEntry(_("CONFIGURA API KEY E STEAMID"), true, [window, auth, store, menu_ptr = menu] {
        LOG(LogDebug) << "SteamUI: Entry 'CONFIGURA API KEY E STEAMID' selezionata.";
        std::string currentApiKey = auth->getApiKey();
        LOG(LogDebug) << "SteamUI: API Key corrente (da auth): " << (currentApiKey.empty() ? "VUOTA" : "PRESENTE");

        auto apiKeyPopup = new GuiTextEditPopup(window, _("STEAM API KEY"), currentApiKey,
            [window, auth, store, menu_ptr](const std::string& newApiKey) {
                // CORREZIONE: Aggiunta cattura newApiKey nella lambda interna
                LOG(LogInfo) << "SteamUI: API Key popup callback eseguita. API Key inserita: " << (newApiKey.empty() ? "VUOTA" : "*** (nascosta)");

                std::string currentSteamId = auth->getSteamId();
                LOG(LogDebug) << "SteamUI: SteamID corrente (da auth): " << currentSteamId;

                auto steamIdPopup = new GuiTextEditPopup(window, _("STEAMID64"), currentSteamId,
                    [window, auth, store, newApiKey, menu_ptr](const std::string& newSteamId) { // newApiKey è catturato
                        LOG(LogInfo) << "SteamUI: SteamID popup callback eseguita. SteamID inserito: " << newSteamId;

                        if (newApiKey.empty() || newSteamId.empty()) {
                            LOG(LogWarning) << "SteamUI: API Key o SteamID vuoti.";
                            // Firma probabile (window, title, msg, callback)
                            window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("API KEY e STEAMID64 non possono essere vuoti."), nullptr));
                            return;
                        }

                        LOG(LogInfo) << "SteamUI: Chiamata a auth->setCredentials con APIKey (presente) e SteamID: " << newSteamId;
                        auth->setCredentials(newApiKey, newSteamId);

                        // Mostra "Attendere"
                        GuiMsgBox* busyPopup = new GuiMsgBox(window, _("VALIDAZIONE IN CORSO..."), _("Attendere prego."), nullptr); // Firma probabile (window, title, msg, callback)
                        window->pushGui(busyPopup);
                        LOG(LogDebug) << "SteamUI: Mostrato busyPopup, avvio validazione.";

                        // VALIDAZIONE (SINCRONA - TODO: rendere asincrona)
                        bool isValid = auth->validateAndSetAuthentication();
                        LOG(LogInfo) << "SteamUI: Validazione terminata da SteamAuth. Risultato: " << (isValid ? "VALIDO" : "NON VALIDO");

                        if (busyPopup) { delete busyPopup; busyPopup = nullptr; } // Cancella il busyPopup

                        if (isValid) {
                            std::string successMsg = _("Credenziali Steam validate e salvate!\nUtente: ") + auth->getUserPersonaName();
                            LOG(LogInfo) << "SteamUI: Validazione successo: " << successMsg;
                            // Firma probabile (window, title, msg, callback)
                            window->pushGui(new GuiMsgBox(window, _("SUCCESSO"), successMsg,
                                [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
                        } else {
                            LOG(LogError) << "SteamUI: Validazione fallita da SteamAuth.";
                            // Firma probabile (window, title, msg, callback)
                            window->pushGui(new GuiMsgBox(window, _("ERRORE VALIDAZIONE"), _("API Key o SteamID64 non validi, oppure errore di rete.\nVerifica le credenziali e la connessione."),
                                [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
                        }
                    }, false, _("ES. 76561197960287930").c_str()); // .c_str() per placeholder
                window->pushGui(steamIdPopup);
            }, false, _("ES. XXXXXXXXXXXXXXXXXXXXXXXX").c_str()); // .c_str() per placeholder
        window->pushGui(apiKeyPopup);
    });

    // --- Opzione per Validare Credenziali (se già inserite e non autenticato) ---
    if (auth->hasCredentials() && !auth->isAuthenticated()) {
        menu->addEntry(_("VALIDA CREDENZIALI SALVATE"), true, [window, auth, store, menu_ptr = menu] {
            LOG(LogDebug) << "SteamUI: Entry 'VALIDA CREDENZIALI SALVATE' selezionata.";
            GuiMsgBox* busyPopup = new GuiMsgBox(window, _("VALIDAZIONE IN CORSO..."), _("Attendere prego."), nullptr); // Firma probabile (window, title, msg, callback)
            window->pushGui(busyPopup);
            LOG(LogDebug) << "SteamUI: Mostrato busyPopup (valida salvate), avvio validazione.";

            bool isValid = auth->validateAndSetAuthentication(); // Sincrono
            LOG(LogInfo) << "SteamUI: Validazione (valida salvate) terminata da SteamAuth. Risultato: " << (isValid ? "VALIDO" : "NON VALIDO");

            if (busyPopup) { delete busyPopup; busyPopup = nullptr; }

            if (isValid) {
                std::string successMsg = _("Credenziali Steam validate!\nUtente: ") + auth->getUserPersonaName();
                LOG(LogInfo) << "SteamUI: Validazione (valida salvate) successo: " << successMsg;
                // Firma probabile (window, title, msg, callback)
                window->pushGui(new GuiMsgBox(window, _("SUCCESSO"), successMsg,
                    [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
            } else {
                LOG(LogError) << "SteamUI: Validazione (valida salvate) fallita da SteamAuth.";
                // Firma probabile (window, title, msg, callback)
                window->pushGui(new GuiMsgBox(window, _("FALLIMENTO VALIDAZIONE"), _("API Key o SteamID64 non validi, oppure errore di rete."),
                    [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); }));
            }
        });
    }


if (auth->hasCredentials()) {
    menu->addEntry(_("CANCELLA CREDENZIALI SALVATE"), true, [window, auth, store, menu_ptr = menu] {
        LOG(LogDebug) << "SteamUI: Entry 'CANCELLA CREDENZIALI SALVATE' selezionata.";

        std::function<void()> yesCallback = [window, auth, store, menu_ptr]() {
            LOG(LogInfo) << "SteamUI: Conferma cancellazione ricevuta (SÌ).";
            auth->clearCredentials();
            reloadSettingsMenu(window, store, menu_ptr);
        };
        std::function<void()> noCallback = nullptr;
        std::string yesButtonText = "SI";
        std::string noButtonText = "NO";

        window->pushGui(new GuiMsgBox(window,
            _("CONFERMA CANCELLAZIONE"),
            _("SI"),
            yesCallback,
            _("NO"),
            noCallback,
            _("Sei sicuro di voler cancellare le credenziali Steam salvate?"),
            nullptr,
            GuiMsgBoxIcon::ICON_QUESTION
        ));

    });
}

    // --- Guida per ottenere API Key e SteamID ---
    menu->addEntry(_("COME OTTENERE API KEY E STEAMID64?"), true, [window] {
        LOG(LogDebug) << "SteamUI: Entry 'COME OTTENERE API KEY E STEAMID64?' selezionata.";
        std::string helpMsg = "";
        helpMsg += _("Per accedere ai dati della tua libreria Steam, EmulationStation necessita di due informazioni:\n\n");
        helpMsg += _("1. UNA CHIAVE API STEAM (API KEY):\n");
        helpMsg += _("   - Visita il sito: https://steamcommunity.com/dev/apikey\n");
        helpMsg += _("   - Accedi con il tuo account Steam.\n");
        helpMsg += _("   - Inserisci un nome di dominio (es. 'emulationstation' o il tuo nome) e accetta i termini di servizio.\n");
        helpMsg += _("   - Copia la chiave generata.\n\n");
        helpMsg += _("2. IL TUO STEAMID64:\n");
        helpMsg += _("   - Questo è un identificatore numerico unico per il tuo account.\n");
        helpMsg += _("   - Puoi trovarlo facilmente usando siti web come:\n");
        helpMsg += _("     * steamid.io\n");
        helpMsg += _("     * steamidfinder.com\n");
        helpMsg += _("   - Inserisci il link al tuo profilo Steam o il tuo nome utente personalizzato per trovarlo.\n\n");
        helpMsg += _("Una volta ottenuti, inseriscili nelle apposite voci di questo menu.");

        // Firma probabile (window, title, msg, callback)
        GuiMsgBox* msgBox = new GuiMsgBox(window, _("GUIDA CREDENZIALI STEAM"), helpMsg, nullptr);
        if (msgBox) {
             // CORREZIONE Errore 5 & 6: Rimuovi setHyperlinksEnabled se non esiste
             // msgBox->setHyperlinksEnabled(true);
             window->pushGui(msgBox);
        } else {
            LOG(LogError) << "SteamUI: Errore creazione GuiMsgBox per guida.";
        }
    });

    // --- Opzione per Aggiornare Lista Giochi (se autenticato) ---
    if (auth->isAuthenticated()) {
        menu->addEntry(_("AGGIORNA LISTA GIOCHI ONLINE"), true, [window, store] {
            LOG(LogInfo) << "SteamUI: Richiesto aggiornamento lista giochi online (TODO: implementare).";
            // Firma probabile (window, title, msg, callback)
            window->pushGui(new GuiMsgBox(window, _("OPERAZIONE AVVIATA"), _("L'aggiornamento della libreria Steam è in corso in background... (TODO: implementare feedback e logica asincrona)"), nullptr));
            // store->refreshOnlineGamesListAsync(); // Chiamata futura
        });
    }

    LOG(LogDebug) << "SteamUI: Tutte le entry aggiunte a GuiSettings. Visualizzazione menu.";
    window->pushGui(menu);
}
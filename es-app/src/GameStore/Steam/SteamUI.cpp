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
#include "guis/GuiBusyInfoPopup.h" // Già incluso, ottimo per i popup di attesa senza interazione

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

    LOG(LogDebug) << "SteamUI: reloadSettingsMenu - Chiamata a store->showStoreUI per mostrare il menu aggiornato.";
    store->showStoreUI(window); // Chiedi allo store di mostrare di nuovo la sua UI principale
}

void SteamUI::showSteamSettingsMenu(Window* window, SteamStore* store)
{
    LOG(LogDebug) << "SteamUI: showSteamSettingsMenu - Inizio.";
    if (!store) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu chiamato con store nullo.";
        // window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("Impossibile caricare le impostazioni Steam."), nullptr)); // ORIGINALE
        window->pushGui(new GuiMsgBox(window, 
                                      _("ERRORE") + std::string("\n\n") + _("Impossibile caricare le impostazioni Steam."), // Messaggio
                                      _("OK"),                                                                            // Testo pulsante
                                      nullptr));                                                                        // Azione
        return;
    }

    SteamAuth* auth = store->getAuth();
    if (!auth) {
        LOG(LogError) << "SteamUI::showSteamSettingsMenu: SteamAuth nullo fornito da SteamStore.";
        // window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("Modulo di autenticazione Steam non disponibile."), nullptr)); // ORIGINALE
        window->pushGui(new GuiMsgBox(window, 
                                      _("ERRORE") + std::string("\n\n") + _("Modulo di autenticazione Steam non disponibile."), // Messaggio
                                      _("OK"),                                                                               // Testo pulsante
                                      nullptr));                                                                           // Azione
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

    // --- Opzione per Configurare/Modificare API Key e SteamID ---
    menu->addEntry(_("CONFIGURA API KEY E STEAMID"), true, [window, auth, store, menu_ptr = menu] {
        LOG(LogDebug) << "SteamUI: Entry 'CONFIGURA API KEY E STEAMID' selezionata.";
        std::string currentApiKey = auth->getApiKey();
        LOG(LogDebug) << "SteamUI: API Key corrente (da auth): " << (currentApiKey.empty() ? "VUOTA" : "PRESENTE");

        auto apiKeyPopup = new GuiTextEditPopup(window, _("STEAM API KEY"), currentApiKey,
            [window, auth, store, menu_ptr](const std::string& newApiKey) {
                LOG(LogInfo) << "SteamUI: API Key popup callback eseguita. API Key inserita: " << (newApiKey.empty() ? "VUOTA" : "*** (nascosta)");

                std::string currentSteamId = auth->getSteamId();
                LOG(LogDebug) << "SteamUI: SteamID corrente (da auth): " << currentSteamId;

                auto steamIdPopup = new GuiTextEditPopup(window, _("STEAMID64"), currentSteamId,
                    [window, auth, store, newApiKey, menu_ptr](const std::string& newSteamId) { 
                        LOG(LogInfo) << "SteamUI: SteamID popup callback eseguita. SteamID inserito: " << newSteamId;

                        if (newApiKey.empty() || newSteamId.empty()) {
                            LOG(LogWarning) << "SteamUI: API Key o SteamID vuoti.";
                            // window->pushGui(new GuiMsgBox(window, _("ERRORE"), _("API KEY e STEAMID64 non possono essere vuoti."), nullptr)); // ORIGINALE
                            window->pushGui(new GuiMsgBox(window,
                                                          _("ERRORE") + std::string("\n\n") + _("API KEY e STEAMID64 non possono essere vuoti."), // Messaggio
                                                          _("OK"),                                                                             // Testo pulsante
                                                          nullptr));                                                                         // Azione
                            return;
                        }

                        LOG(LogInfo) << "SteamUI: Chiamata a auth->setCredentials con APIKey (presente) e SteamID: " << newSteamId;
                        auth->setCredentials(newApiKey, newSteamId);
                        
                        // Mostra "Attendere"
                        // GuiMsgBox* busyPopup = new GuiMsgBox(window, _("VALIDAZIONE IN CORSO..."), _("Attendere prego."), nullptr); // ORIGINALE
                        // Per un popup di attesa senza interazione, GuiBusyInfoPopup è meglio:
                        GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("VALIDAZIONE IN CORSO...\nAttendere prego."));
                        window->pushGui(busyPopup);
                        LOG(LogDebug) << "SteamUI: Mostrato busyPopup, avvio validazione.";

                        bool isValid = auth->validateAndSetAuthentication();
                        LOG(LogInfo) << "SteamUI: Validazione terminata da SteamAuth. Risultato: " << (isValid ? "VALIDO" : "NON VALIDO");

                        if (busyPopup) { 
                            if (busyPopup) { delete busyPopup; busyPopup = nullptr; }  // Usa il metodo close() per GuiBusyInfoPopup se disponibile, o delete.
                                                // Per GuiMsgBox sarebbe delete busyPopup;
                                                // Per GuiBusyInfoPopup, è meglio che si chiuda da solo o con un metodo close()
                                                // Se GuiBusyInfoPopup non ha un metodo close(), allora delete va bene ma assicurati che sia sicuro.
                                                // In genere, i popup gestiti da window->pushGui si auto-eliminano o vengono eliminati dallo stack.
                                                // Per sicurezza, se GuiBusyInfoPopup è un GuiComponent base, delete è ok.
                                                // Dato che lo chiudiamo subito, il delete è più diretto.
                            delete busyPopup; 
                            busyPopup = nullptr; 
                        } 

                        if (isValid) {
                            std::string successMsg = _("Credenziali Steam validate e salvate!\nUtente: ") + auth->getUserPersonaName();
                            LOG(LogInfo) << "SteamUI: Validazione successo: " << successMsg;
                            // window->pushGui(new GuiMsgBox(window, _("SUCCESSO"), successMsg, [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // ORIGINALE
                            window->pushGui(new GuiMsgBox(window,
                                                          _("SUCCESSO") + std::string("\n\n") + successMsg, // Messaggio
                                                          _("OK"),                                         // Testo pulsante
                                                          [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // Azione
                        } else {
                            LOG(LogError) << "SteamUI: Validazione fallita da SteamAuth.";
                            std::string errorDetailMsg = _("API Key o SteamID64 non validi, oppure errore di rete.\nVerifica le credenziali e la connessione.");
                            // window->pushGui(new GuiMsgBox(window, _("ERRORE VALIDAZIONE"), errorDetailMsg, [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // ORIGINALE
                             window->pushGui(new GuiMsgBox(window, 
                                                          _("ERRORE VALIDAZIONE") + std::string("\n\n") + errorDetailMsg, // Messaggio
                                                          _("OK"),                                                        // Testo pulsante
                                                          [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // Azione
                        }
                    }, false, _("ES. 76561197960287930").c_str()); 
                window->pushGui(steamIdPopup);
            }, false, _("ES. XXXXXXXXXXXXXXXXXXXXXXXX").c_str()); 
        window->pushGui(apiKeyPopup);
    });

    // --- Opzione per Validare Credenziali (se già inserite e non autenticato) ---
    if (auth->hasCredentials() && !auth->isAuthenticated()) {
        menu->addEntry(_("VALIDA CREDENZIALI SALVATE"), true, [window, auth, store, menu_ptr = menu] {
            LOG(LogDebug) << "SteamUI: Entry 'VALIDA CREDENZIALI SALVATE' selezionata.";
            // GuiMsgBox* busyPopup = new GuiMsgBox(window, _("VALIDAZIONE IN CORSO..."), _("Attendere prego."), nullptr); // ORIGINALE
            GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("VALIDAZIONE IN CORSO...\nAttendere prego."));
            window->pushGui(busyPopup);
            LOG(LogDebug) << "SteamUI: Mostrato busyPopup (valida salvate), avvio validazione.";

            bool isValid = auth->validateAndSetAuthentication(); 
            LOG(LogInfo) << "SteamUI: Validazione (valida salvate) terminata da SteamAuth. Risultato: " << (isValid ? "VALIDO" : "NON VALIDO");

            if (busyPopup) { delete busyPopup; busyPopup = nullptr; }

            if (isValid) {
                std::string successMsg = _("Credenziali Steam validate!\nUtente: ") + auth->getUserPersonaName();
                LOG(LogInfo) << "SteamUI: Validazione (valida salvate) successo: " << successMsg;
                // window->pushGui(new GuiMsgBox(window, _("SUCCESSO"), successMsg, [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // ORIGINALE
                window->pushGui(new GuiMsgBox(window,
                                              _("SUCCESSO") + std::string("\n\n") + successMsg, // Messaggio
                                              _("OK"),                                         // Testo pulsante
                                              [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // Azione
            } else {
                LOG(LogError) << "SteamUI: Validazione (valida salvate) fallita da SteamAuth.";
                std::string errorDetailMsg = _("API Key o SteamID64 non validi, oppure errore di rete.");
                // window->pushGui(new GuiMsgBox(window, _("FALLIMENTO VALIDAZIONE"), errorDetailMsg, [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // ORIGINALE
                window->pushGui(new GuiMsgBox(window,
                                              _("FALLIMENTO VALIDAZIONE") + std::string("\n\n") + errorDetailMsg, // Messaggio
                                              _("OK"),                                                            // Testo pulsante
                                              [window, store, menu_ptr]() { reloadSettingsMenu(window, store, menu_ptr); })); // Azione
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
            // std::function<void()> noCallback = nullptr; // Già così se non specificato per il secondo pulsante GuiMsgBox
            // std::string yesButtonText = "SI"; // Non serve, _("SI") è più corretto per traduzione
            // std::string noButtonText = "NO";

            std::string confirmMessage = _("Sei sicuro di voler cancellare le credenziali Steam salvate?");
            // Se vuoi un titolo:
            // std::string confirmMessage = _("CONFERMA CANCELLAZIONE") + std::string("\n\n") + _("Sei sicuro di voler cancellare le credenziali Steam salvate?");
            
            // La chiamata originale era confusa con i parametri.
            // window->pushGui(new GuiMsgBox(window,
            //     _("CONFERMA CANCELLAZIONE"), // Questo diventava messaggio
            //     _("SI"),                     // Questo diventava pulsante 1
            //     yesCallback,
            //     _("NO"),                     // Questo diventava pulsante 2
            //     noCallback,                  // Azione pulsante 2 (nullptr)
            //     _("Sei sicuro di voler cancellare le credenziali Steam salvate?"), // Questo diventava pulsante 3!
            //     nullptr,
            //     GuiMsgBoxIcon::ICON_QUESTION
            // ));

            // Correzione per un box con messaggio e due pulsanti "SI" / "NO"
            window->pushGui(new GuiMsgBox(window,
                confirmMessage,                 // Testo del messaggio principale
                _("SI"),                        // Testo Pulsante 1
                yesCallback,                    // Azione Pulsante 1
                _("NO"),                        // Testo Pulsante 2
                nullptr,                        // Azione Pulsante 2 (solo chiude)
                "",                             // Testo Pulsante 3 (vuoto per non averlo)
                nullptr,                        // Azione Pulsante 3
                GuiMsgBoxIcon::ICON_QUESTION
            ));
        });
    }

  menu->addEntry(_("COME OTTENERE API KEY E STEAMID64?"), true, [window] {
    LOG(LogInfo) << "SteamUI_Guida: Azione menu avviata.";

    std::string helpMsg = ""; // Il tuo testo di aiuto lungo
    helpMsg += _("Per accedere ai dati della tua libreria Steam,\n");
    helpMsg += _("EmulationStation necessita di due informazioni:\n");
	helpMsg += _("UNA CHIAVE API STEAM (API KEY):\n");
    helpMsg += _(" - Visita il sito: https://steamcommunity.com/dev/apikey\n");
    helpMsg += _(" - Accedi con il tuo account Steam.\n");
    helpMsg += _(" - Inserisci le credenziali e accetta i termini di servizio.\n");
	helpMsg += _(" - Copia la chiave generata.\n");
	helpMsg += _(" - Copia il nome del dominio.");


    LOG(LogInfo) << "SteamUI_Guida: Messaggio di aiuto creato. Pronto per GuiMsgBox.";


    GuiMsgBox* msgBox = new GuiMsgBox(window, 
                                      helpMsg,    // Questo è il testo principale del messaggio
                                      _("OK"),    // Questo è il testo per il pulsante "OK"
                                      nullptr);   // Callback per il pulsante (si chiude)
                                      // Potresti dover aggiungere un argomento GuiMsgBoxIcon se necessario, es: GuiMsgBoxIcon::ICON_INFORMATION

    if (msgBox) {
        LOG(LogInfo) << "SteamUI_Guida: GuiMsgBox creato. Eseguo window->pushGui().";
        window->pushGui(msgBox);
    } else {
        LOG(LogError) << "SteamUI_Guida: Errore durante la creazione di GuiMsgBox.";
    }

    LOG(LogInfo) << "SteamUI_Guida: Azione menu terminata.";
});

// --- Opzione per Aggiornare Lista Giochi (se autenticato) ---
if (auth && auth->isAuthenticated()) { 
    menu->addEntry(_("AGGIORNA LISTA GIOCHI ONLINE"), true,
        [window, store, menu_ptr = menu] { 
            LOG(LogInfo) << "SteamUI: Pulsante 'AGGIORNA LISTA GIOCHI ONLINE' premuto.";

            if (!store) {
                LOG(LogError) << "SteamUI: Store è nullo nel callback!";
                return;
            }

            // MOSTRA DI NUOVO IL TUO POPUP
            GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("AGGIORNAMENTO LIBRERIA STEAM IN CORSO..."));
            window->pushGui(busyPopup);

            if (menu_ptr) {
                LOG(LogDebug) << "SteamUI: Chiusura del menu impostazioni prima dell'aggiornamento asincrono.";
                menu_ptr->close(); 
            }

            std::thread refreshThread([store, window, busyPopup /* Cattura busyPopup! */]() { 
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
                if (store) {
                    LOG(LogInfo) << "SteamUI: Avvio store->refreshSteamGamesListAsync()...";
                    store->refreshSteamGamesListAsync();
                    // Il busyPopup DEVE essere chiuso dal gestore eventi SDL_STEAM_REFRESH_COMPLETE
                    // oppure, se l'evento non è affidabile, la logica in refreshSteamGamesListAsync
                    // deve trovare un modo per segnalare al thread principale di chiudere busyPopup.
                    // ATTENZIONE: Non chiudere busyPopup direttamente da questo thread se è un GuiComponent
                    // perché le modifiche alla UI devono avvenire nel thread principale.
                } else {
                    LOG(LogError) << "SteamUI: Store diventato nullo prima del lancio asincrono!";
                    // In caso di errore qui, il busyPopup potrebbe rimanere bloccato se l'evento non parte.
                    // Sarebbe ideale avere un modo per inviare comunque l'evento di completamento (con stato di errore)
                    // o inviare un evento specifico di errore che chiuda il popup.
                    // Per ora, ci affidiamo a refreshSteamGamesListAsync che invii l'evento.
                }
            });
            refreshThread.detach();
        }); 
}
    LOG(LogDebug) << "SteamUI: Tutte le entry aggiunte a GuiSettings. Visualizzazione menu.";
    window->pushGui(menu);
}
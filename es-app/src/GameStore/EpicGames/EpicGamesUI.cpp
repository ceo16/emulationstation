#include "GameStore/EpicGames/EpicGamesUI.h"
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "guis/GuiSettings.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiBusyInfoPopup.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "components/ButtonComponent.h"
#include "components/TextComponent.h"
#include "Window.h"
#include "Log.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "HttpReq.h" // Necessario per le richieste HTTP
#include "json.hpp"  // Necessario per il parsing del JSON
#include <SDL.h>

#include <thread> // Necessario per std::thread

EpicGamesUI::EpicGamesUI() {}
EpicGamesUI::~EpicGamesUI() {}

void EpicGamesUI::showMainMenu(Window* window, EpicGamesStore* store) {
    if (!store || !store->getAuth()) { return; }

    auto* menu = new GuiSettings(window, "EPIC GAMES STORE");
    EpicGamesAuth* auth = store->getAuth();
    
    auto reloadSystemView = [window, menu] {
        if (menu && window->peekGui() == menu) menu->close();
        ViewController::get()->reloadAll(window);
    };
    
    std::string status = auth->isAuthenticated() ? "Autenticato: " + auth->getDisplayName() : "Non autenticato";
    menu->addEntry(status, false, nullptr);
    menu->addEntry(" ", false, nullptr);

    if (!auth->isAuthenticated()) {
        menu->addEntry("ACCEDI CON EPIC GAMES", true,
            [window, auth, reloadSystemView]() {
                const std::string initialLoginUrl = "https://www.epicgames.com/id/login";
                const std::string personalUrlPart = "epicgames.com/account/personal";
                const std::string finalRedirectUrlPart = "id/api/redirect?clientId=";

                // Creiamo la WebView SENZA watch prefix. Questo bypassa la vecchia logica di auto-chiusura.
                auto* webView = new GuiWebViewAuthLogin(window, initialLoginUrl, "EpicGames", "");

                // Usiamo il NUOVO callback flessibile
                webView->setNavigationCompletedCallback(
                    [=](bool success, const std::string& navigatedUrl) mutable {
                        if (!success || navigatedUrl == "user_cancelled") {
                             if (auto* wv = dynamic_cast<GuiWebViewAuthLogin*>(window->peekGui())) wv->close();
                             return;
                        }
                        
                        auto* wvInstance = dynamic_cast<GuiWebViewAuthLogin*>(window->peekGui());
                        if (!wvInstance) return;

                        // Fase finale: Siamo sulla pagina del codice JSON
                       if (navigatedUrl.find(finalRedirectUrlPart) != std::string::npos) {
    LOG(LogInfo) << "[EpicLoginFlow] Rilevato URL finale. Estraggo il contenuto...";
    wvInstance->setNavigationCompletedCallback(nullptr);
    
  wvInstance->getTextAsync([=](const std::string& pageContent) mutable {
    wvInstance->close();
    
    if (pageContent.empty()) {
        window->pushGui(new GuiMsgBox(window, "Login Fallito: Timeout nella lettura del codice.", "OK"));
        return;
    }
    
    try {
        // La WebView restituisce un JSON impacchettato in una stringa, es: "\"{\\\"key\\\":...}\""
        // Dobbiamo fare un doppio parsing per estrarlo correttamente.
        
        // 1. Parsa la stringa esterna
        auto outerJson = nlohmann::json::parse(pageContent);
        // 2. Estrai il contenuto, che è il vero JSON
        std::string innerJsonString = outerJson.get<std::string>();
        // 3. Parsa il JSON interno e prendi il codice
        std::string code = nlohmann::json::parse(innerJsonString).value("authorizationCode", "");

        if (!code.empty() && auth->exchangeAuthCodeForToken(code)) {
            window->pushGui(new GuiMsgBox(window, "LOGIN COMPLETATO!", "OK", reloadSystemView));
        } else {
            throw std::runtime_error("Codice non trovato nel JSON o scambio fallito.");
        }
    } catch (const nlohmann::json::exception& e) {
        LOG(LogError) << "Errore parsing JSON: " << e.what() << "\nDati ricevuti: " << pageContent;
        window->pushGui(new GuiMsgBox(window, std::string("Login Fallito (Errore JSON):\n") + e.what(), "OK"));
    }
});
}
                        // Fase intermedia: Siamo sulla pagina dell'account
                        else if (navigatedUrl.find(personalUrlPart) != std::string::npos) {
                             LOG(LogInfo) << "[EpicLoginFlow] Rilevata pagina account. Navigo per ottenere il codice.";
                             wvInstance->navigate(auth->getAuthorizationCodeUrl());
                        }
                    });
                
                window->pushGui(webView);
            }, 
            "iconBrowser");
    } else {
        // --- OPZIONI POST-LOGIN ---
menu->addEntry(
            _("Scarica/Aggiorna Libreria Giochi Online"),
            true,
            [window, store, menu]() { // Non serve catturare 'auth' qui
                LOG(LogInfo) << "User triggered 'Scarica/Aggiorna Libreria Giochi Online' (Authenticated)";

                // 1. Mostra il Popup di attesa
                GuiBusyInfoPopup* busyPopup = new GuiBusyInfoPopup(window, _("AGGIORNAMENTO EPIC IN CORSO..."));
                window->pushGui(busyPopup);

                // 2. Chiudi il menu corrente
                if(menu) menu->close();

                // 3. Avvia il Task Asincrono DOPO un piccolo ritardo
                std::thread delayedLaunchThread([store, window]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    LOG(LogInfo) << "Starting refreshGamesListAsync (Authenticated)";
                    if (store) {
                        store->refreshGamesListAsync();
                    } else { LOG(LogError) << "Delayed Launch: Store pointer became null!"; }
                });
                delayedLaunchThread.detach();
                LOG(LogDebug) << "UI Action Lambda finished, delayed launch thread detached.";

            },
            "iconSync");
			 menu->addEntry("LOGOUT", true, [auth, menu, reloadSystemView] {
            auth->logout();
            if (menu) menu->close();
            reloadSystemView();
        }, "iconLogout");
    } // Fine if (isEpicAuthenticated)

    window->pushGui(menu);
} // Fine showMainMenu

// --- Funzione showGameList (Invariata, ma assicurati che getGamesList funzioni) ---
void EpicGamesUI::showGameList(Window* window, EpicGamesStore* store) {
 LOG(LogDebug) << "EpicGamesUI::showGameList";

 if (!store) {
     LOG(LogError) << "EpicGamesUI::showGameList - store pointer is null!";
     return;
 }

 auto menu = new GuiSettings(window, "Epic Games Library");

 // TODO: Qui dovremmo idealmente chiamare una funzione che *ottiene*
 // la lista giochi aggiornata, magari triggerando le chiamate API
 // se non già fatto. Per ora, usiamo getGamesList che potrebbe
 // restituire solo quelli trovati localmente.
 std::vector<FileData*> games = store->getGamesList();
 LOG(LogDebug) << "showGameList: store->getGamesList() returned " << games.size() << " games.";

 if (games.empty()) {
     menu->addEntry("Nessun gioco trovato.", false, nullptr);
     // Aggiungi messaggio per indicare che serve login/sync?
 } else {
     for (FileData* game : games) {
         if (game) {
             menu->addEntry(game->getMetadata().get(MetaDataId::Name), false, nullptr);
         }
     }
 }
 window->pushGui(menu);
}
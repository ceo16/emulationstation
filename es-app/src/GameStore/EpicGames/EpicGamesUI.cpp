#include "GameStore/EpicGames/EpicGamesUI.h"
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "components/ButtonComponent.h"        // Includi componenti usati
#include "components/TextComponent.h"
#include "components/ComponentList.h"
#include "guis/GuiSettings.h"
#include "Window.h"
#include "Log.h"
#include "FileData.h"
#include "guis/GuiMsgBox.h"                // Include per MsgBox
#include "guis/GuiTextEditPopup.h"         // Include per TextEditPopup
#include "utils/StringUtil.h"              // Include per StringUtil::trim
#include <string>
#include <functional>
#include <cstring>                         // Per strlen


EpicGamesUI::EpicGamesUI() {
}

EpicGamesUI::~EpicGamesUI() {
}

// --- Funzione showMainMenu MODIFICATA per Login Alternativo ---
void EpicGamesUI::showMainMenu(Window* window, EpicGamesStore* store) {
 LOG(LogDebug) << "EpicGamesUI::showMainMenu";

 if (!store) {
     LOG(LogError) << "EpicGamesUI::showMainMenu - store pointer is null!";
     return;
 }

 auto menu = new GuiSettings(window, "Epic Games Store");

 // --- Logica per la voce di Login ---
 menu->addEntry("Login to Epic Games", true,
    [window, store]() { // Cattura window e store

        // 1. Mostra messaggio informativo
        std::string msg =
            "Si aprirà il browser per l'accesso a Epic Games.\n\n"
            "Dopo aver effettuato l'accesso, Epic mostrerà una pagina con un CODICE DI AUTORIZZAZIONE (una lunga stringa di lettere e numeri).\n\n"
            "Questo codice NON è facilmente copiabile. Dovrai **SCRIVERLO A MANO** nella finestra successiva.\nPresta molta attenzione!";

        window->pushGui(new GuiMsgBox(window, msg, "HO CAPITO, APRI IL BROWSER",
            [window, store]() { // Eseguita dopo OK sul messaggio

                // 2. Apri il browser con l'URL diretto
                store->startLoginFlow();

                // 3. Mostra la finestra per SCRIVERE il codice
                std::string initialValue = "";
                std::string acceptButtonText = "CONFERMA CODICE";
                std::string title = "Inserisci Codice Autorizzazione Epic";
                // Il prompt è stato rimosso dal costruttore

                // Crea il popup con 6 argomenti
                auto* codePopup = new GuiTextEditPopup(window, title, initialValue,
                    [window, store](const std::string& enteredCode) { // Eseguita dopo CONFERMA CODICE
                        if (!enteredCode.empty()) {
                            std::string cleanCode = Utils::String::trim(enteredCode);
                            LOG(LogDebug) << "Received authorization code from user input: '" << cleanCode << "'";
                            store->processAuthCode(cleanCode);
                        } else {
                            LOG(LogWarning) << "User submitted empty authorization code.";
                            window->pushGui(new GuiMsgBox(window, "Codice non inserito.", "OK"));
                        }
                    }, false, acceptButtonText.c_str() // 6 argomenti: window, titolo, valore iniziale, callback, multiline, testo bottone OK
                  // <<< RIMOSSO: prompt.c_str()
                ); // Fine costruttore GuiTextEditPopup

                window->pushGui(codePopup); // Questa riga ora funzionerà

            } // Fine lambda GuiMsgBox
        )); // Fine pushGui GuiMsgBox

    }, // Fine lambda principale addEntry
 "iconFolder");
// --- NUOVA VOCE: Aggiorna Libreria Giochi ---
 if (store && store->getAuth() && !store->getAuth()->getAccessToken().empty()) {
        menu->addEntry(
            "Scarica/Aggiorna Libreria Giochi Online",
            true,
            [window, store, menu]() { // Cattura anche menu
                LOG(LogInfo) << "User triggered 'Aggiorna Libreria Giochi'";

                // --- Popup Rimosso ---
                // GuiInfoPopup* infoPopup = new GuiInfoPopup(window, "Avvio aggiornamento...", 4000);
                // window->pushGui(infoPopup);
                // ---------------------

                // Mostra un messaggio bloccante semplice (opzionale)
                window->pushGui(new GuiMsgBox(window, "Avvio aggiornamento libreria Epic in background.\nLa lista si aggiornerà al termine.", "OK", [menu] {
                    // Chiudi il menu DOPO che l'utente preme OK sul messaggio
                    if(menu) menu->close();
                }));

                // Chiama la funzione asincrona
                store->refreshGamesListAsync(); // Chiamata come prima

                // Non chiudere il menu qui, ma nel callback del MsgBox
                // if(menu) menu->close();

            },
        "iconSync");
    }
    // --- Fine Nuova Voce ---
 // --- Fine voce di Login ---

 // Aggiungi qui altre voci se necessario...
 // Esempio: Verifica stato login prima di mostrare altre voci
 /*
 if (store->isAuthenticated()) { // Metodo ipotetico
     menu->addEntry("Mostra Libreria", true, [window, store]{
         showGameList(window, store);
     });
 }
 */

 window->pushGui(menu);
}

// --- Funzione showLogin (Probabilmente non più necessaria) ---
void EpicGamesUI::showLogin(Window* window, EpicGamesStore* store) {
 LOG(LogWarning) << "EpicGamesUI::showLogin called, redirecting to showMainMenu.";
 showMainMenu(window, store);
}

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
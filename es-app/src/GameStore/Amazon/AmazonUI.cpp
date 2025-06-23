#include "GameStore/Amazon/AmazonUI.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/Amazon/AmazonGamesStore.h"

#include "guis/GuiSettings.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "Log.h"
#include "LocaleES.h"

AmazonUI::AmazonUI(Window* window) : mWindow(window)
{
    // Otteniamo un puntatore allo store di Amazon
    GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
    mStore = nullptr;
    if (gsm) {
        GameStore* baseStore = gsm->getStore("amazon");
        if (baseStore) {
            mStore = dynamic_cast<AmazonGamesStore*>(baseStore);
        }
    }
}

void AmazonUI::openAmazonStoreMenu()
{
    if (!mStore) {
        LOG(LogError) << "AmazonUI: Impossibile ottenere l'istanza di AmazonGamesStore.";
        // CORREZIONE: Usa il costruttore corretto (testo, bottone1, funzione1)
        mWindow->pushGui(new GuiMsgBox(mWindow, _("LO STORE AMAZON NON E' STATO INIZIALIZZATO."), _("OK"), nullptr));
        return;
    }

    auto s = new GuiSettings(mWindow, "AMAZON GAMES STORE");

    if (mStore->isAuthenticated()) {
        // --- Utente Autenticato ---
        
        s->addEntry(_("DISCONNETTI ACCOUNT"), true, [this, s] {
            mStore->logout();
            // CORREZIONE: Usa il costruttore corretto
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ACCOUNT DISCONNESSO CORRETTAMENTE."), _("OK"), [this, s] {
                delete s; 
                auto new_ui = new AmazonUI(mWindow);
                new_ui->openAmazonStoreMenu();
                delete new_ui;
            }));
        });

        s->addEntry(_("SINCRONIZZA LIBRERIA"), true, [this] {
            GuiComponent* msgBox = new GuiMsgBox(mWindow, _("SINCRONIZZAZIONE IN CORSO..."), _("ANNULLA"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION);
            mWindow->pushGui(msgBox);

            mStore->syncGames([this, msgBox](bool success) {
                delete msgBox; 
                
                if (success) {
                    // CORREZIONE: Usa il costruttore corretto
                    mWindow->pushGui(new GuiMsgBox(mWindow, _("LIBRERIA SINCRONIZZATA CORRETTAMENTE."), _("OK"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION));
                } else {
                    // CORREZIONE: Usa il costruttore corretto - QUESTA ERA LA RIGA DELL'ERRORE
                    mWindow->pushGui(new GuiMsgBox(mWindow, _("IMPOSSIBILE SINCRONIZZARE LA LIBRERIA."), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
                }
            });
        });

    } else {
        // --- Utente NON Autenticato ---

        s->addEntry(_("ACCEDI ALL'ACCOUNT"), true, [this, s] {
            mStore->login([this, s](bool success) {
                if (success) {
                    // CORREZIONE: Usa il costruttore corretto
                    mWindow->pushGui(new GuiMsgBox(mWindow, _("LOGIN EFFETTUATO CORRETTAMENTE."), _("OK"), [this, s] {
                        delete s;
                        auto new_ui = new AmazonUI(mWindow);
                        new_ui->openAmazonStoreMenu();
                        delete new_ui;
                    }, GuiMsgBoxIcon::ICON_INFORMATION));
                } else {
                    // CORREZIONE: Usa il costruttore corretto
                    mWindow->pushGui(new GuiMsgBox(mWindow, _("IMPOSSIBILE EFFETTUARE IL LOGIN."), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
                }
            });
        });
    }

    mWindow->pushGui(s);
}
#include "GameStore/GOG/GogUI.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/GOG/GogGamesStore.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "Log.h"
#include "LocaleES.h"

GogUI::GogUI(Window* window) 
    : GuiSettings(window, "GOG.COM STORE")
{
    mStore = nullptr;
    GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
    if (gsm) {
        GameStore* baseStore = gsm->getStore("gog");
        if (baseStore) {
            mStore = dynamic_cast<GogGamesStore*>(baseStore);
        }
    }

    if (!mStore) {
        LOG(LogError) << "GogUI: Impossibile ottenere l'istanza di GogGamesStore.";
        // CORREZIONE: Usa il costruttore corretto (testo, bottone1, funzione1)
        mWindow->pushGui(new GuiMsgBox(mWindow, _("LO STORE GOG NON E' STATO INIZIALIZZATO."), _("OK"), nullptr));
        close();
        return;
    }
    
    buildMenu();
}

void GogUI::buildMenu()
{
    mMenu.clear();

    if (mStore->isAuthenticated())
    {
        addEntry(_("DISCONNETTI ACCOUNT (RICHIEDE RIAVVIO)"), false, [this] { performLogout(); });
        addEntry(_("SINCRONIZZA LIBRERIA ONLINE"), false, [this] { syncGames(); });
    }
    else
    {
        addEntry(_("ACCEDI ALL'ACCOUNT"), false, [this] { performLogin(); });
    }
}

void GogUI::performLogout()
{
    mStore->logout();
    buildMenu();
    // CORREZIONE: Usa il costruttore corretto
    mWindow->pushGui(new GuiMsgBox(mWindow, _("PER COMPLETARE IL LOGOUT, RIAVVIARE L'APPLICAZIONE."), _("OK"), nullptr));
}

void GogUI::performLogin()
{
    mStore->login([this](bool success) {
        if (success) {
            buildMenu();
        } else {
            // CORREZIONE: Usa il costruttore corretto
            mWindow->pushGui(new GuiMsgBox(mWindow, _("LOGIN FALLITO"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        }
    });
}

void GogUI::syncGames()
{
    GuiComponent* msgBox = new GuiMsgBox(mWindow, _("SINCRONIZZAZIONE IN CORSO..."), _("ANNULLA"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION);
    mWindow->pushGui(msgBox);

    mStore->syncGames([this, msgBox](bool success) {
        delete msgBox;
        if (success) {
            // CORREZIONE: Usa il costruttore corretto
            mWindow->pushGui(new GuiMsgBox(mWindow, _("SINCRONIZZAZIONE COMPLETATA"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION));
        } else {
            // CORREZIONE: Usa il costruttore corretto
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE DI SINCRONIZZAZIONE"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        }
    });
}
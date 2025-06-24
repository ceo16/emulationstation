#include "GameStore/Amazon/AmazonUI.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/Amazon/AmazonGamesStore.h"

#include "guis/GuiSettings.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "Log.h"
#include "LocaleES.h"

AmazonUI::AmazonUI(Window* window) 
    : GuiSettings(window, "AMAZON GAMES STORE")
{
    GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
    mStore = nullptr;
    if (gsm) {
        GameStore* baseStore = gsm->getStore("amazon");
        if (baseStore) {
            mStore = dynamic_cast<AmazonGamesStore*>(baseStore);
        }
    }

    if (!mStore) {
        LOG(LogError) << "AmazonUI: Impossibile ottenere l'istanza di AmazonGamesStore.";
        mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("OK"), nullptr));
        close();
        return;
    }
    
    buildMenu();
}

void AmazonUI::buildMenu()
{
    mMenu.clear();

    if (mStore->isAuthenticated())
    {
        addEntry(_("DISCONNETTI ACCOUNT"), false, [this] { performLogout(); });
        addEntry(_("SINCRONIZZA LIBRERIA"), false, [this] { syncGames(); });
    }
    else
    {
        addEntry(_("ACCEDI ALL'ACCOUNT"), false, [this] { performLogin(); });
    }
}

void AmazonUI::performLogout()
{
    mStore->logout();
    buildMenu(); 
}

void AmazonUI::performLogin()
{
    GuiComponent* msgBox = new GuiMsgBox(mWindow, _("ATTENDERE..."), _("ANNULLA"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION);
    mWindow->pushGui(msgBox);

    mStore->login([this, msgBox](bool success) {
        delete msgBox;
        if (success) {
            // --- CORREZIONE: Usa ICON_INFORMATION invece di ICON_SUCCESS ---
            mWindow->pushGui(new GuiMsgBox(mWindow, _("LOGIN EFFETTUATO"), _("OTTIMO!"), [this] {
                buildMenu();
            }, GuiMsgBoxIcon::ICON_INFORMATION)); 
        } else {
            mWindow->pushGui(new GuiMsgBox(mWindow, _("LOGIN FALLITO"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        }
    });
}

void AmazonUI::syncGames()
{
    GuiComponent* msgBox = new GuiMsgBox(mWindow, _("SINCRONIZZAZIONE IN CORSO..."), _("ANNULLA"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION);
    mWindow->pushGui(msgBox);

    mStore->syncGames([this, msgBox](bool success) {
        delete msgBox;
        if (success) {
            // --- CORREZIONE: Usa ICON_INFORMATION invece di ICON_SUCCESS ---
             mWindow->pushGui(new GuiMsgBox(mWindow, _("SINCRONIZZAZIONE COMPLETATA"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_INFORMATION));
        } else {
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE DI SINCRONIZZAZIONE"), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        }
    });
}
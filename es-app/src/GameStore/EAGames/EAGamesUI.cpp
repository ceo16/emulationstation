// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesUI.cpp
#include "GameStore/EAGames/EAGamesUI.h" 
#include "GameStore/EAGames/EAGamesStore.h"
#include "GameStore/GameStoreManager.h"

#include "guis/GuiMsgBox.h"
#include "guis/GuiInfoPopup.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Log.h" 
#include "ApiSystem.h"
#include "views/ViewController.h" 
#include "SystemData.h"   
#include "Gamelist.h"  

EAGamesUI::EAGamesUI(Window* window) : GuiSettings(window, "EA GAMES STORE"), mStore(nullptr)
{
    GameStore* storeBase = GameStoreManager::getInstance(mWindow)->getStore(EAGamesStore::STORE_ID);
    if (storeBase) {
        mStore = dynamic_cast<EAGamesStore*>(storeBase);
    }
    
    initializeMenu();
}

void EAGamesUI::initializeMenu()
{
    if (!mStore) 
    {
        setSubTitle(_("Could not initialize the EA Games module."));
        addEntry(_("OK"), false, [this] { delete this; });
        return;
    }

    auto theme = ThemeData::getMenuTheme();
    auto smallFont = theme->TextSmall.font;
    auto font = theme->Text.font;
    unsigned int color = theme->Text.color;

    if (mStore->IsUserLoggedIn())
    {
        setSubTitle(_("Logged in to your EA Account"));

        // =======================================================================
        // ** SOLUZIONE FINALE: Layout a spazio condiviso **
        
        // 1. Crea la riga manualmente
        ComponentListRow row;
        
        // 2. Crea l'etichetta di sinistra e il valore di destra
        auto statusTitle = std::make_shared<TextComponent>(mWindow, _("EA PLAY STATUS"), font, color, ALIGN_LEFT);
        mSubscriptionLabel = std::make_shared<TextComponent>(mWindow, _("LOADING..."), smallFont, 0x777777FF, ALIGN_RIGHT);
        
        // 3. Aggiungi ENTRAMBI i componenti alla riga con il flag di espansione a 'true'.
        //    Questo li costringe a dividersi lo spazio disponibile invece di spingersi a vicenda.
        row.addElement(statusTitle, true);
        row.addElement(mSubscriptionLabel, true);
        
        // 4. Aggiungi la riga al menu
        addRow(row);
        // =======================================================================

        mStore->getSubscriptionDetails([this](const EAGames::SubscriptionDetails& details) {
            std::string tierName = (details.tier == "premium") ? "EA Play Pro" : "EA Play";
            std::string labelText = _("NO ACTIVE SUBSCRIPTION");
            unsigned int color = 0xDD2222FF;
            
            if (details.isActive) {
                labelText = tierName + " (" + _("ACTIVE") + ")";
                color = 0x22DD22FF;
            }
            mSubscriptionLabel->setText(labelText);
            mSubscriptionLabel->setColor(color);
        });

        addEntry(" ", false);

        mEaPlaySwitch = std::make_shared<SwitchComponent>(mWindow);
        mEaPlaySwitch->setState(Settings::getInstance()->getBool("EAPlay.Enabled"));
        addWithLabel(_("INCLUDE EA PLAY IN GAME LIST"), mEaPlaySwitch);
        
        addSaveFunc([this] {
            if (mEaPlaySwitch) {
                Settings::getInstance()->setBool("EAPlay.Enabled", mEaPlaySwitch->getState());
            }
        });

        addEntry(" ", false);

        addEntry(_("REFRESH GAME LIST"), true, [this]() { processImportGames(); });
        addEntry(_("LOGOUT"), true, [this]() { processLogout(); });
    }
    else
    {
        setSubTitle(_("You are not logged in"));
        addEntry(_("LOGIN TO EA ACCOUNT"), true, [this]() { processStartLoginFlow(); });
    }
}


void EAGamesUI::processStartLoginFlow()
{
    if (!mStore) return;

    GuiInfoPopup* waitPopup = new GuiInfoPopup(mWindow, _("Initiating EA Login..."), 10000);
    mWindow->pushGui(waitPopup);

    mStore->StartLoginFlow([this, waitPopup](bool success, const std::string& message) {
        if (mWindow->peekGui() == waitPopup) {
             delete waitPopup;
        }
        onLoginFinished(success, message);
    });
}

void EAGamesUI::onLoginFinished(bool success, const std::string& message)
{
    std::string finalMessage = message;
    if (message.empty()) {
        finalMessage = success ? _("Login process completed successfully.") : _("Login process failed or was cancelled.");
    }
    
    mWindow->pushGui(new GuiMsgBox(mWindow, finalMessage, std::string(_("OK")), nullptr));
    
    delete this;
    mWindow->pushGui(new EAGamesUI(mWindow));
}

void EAGamesUI::processLogout()
{
    if (!mStore) return;
    mStore->Logout(); 
    mWindow->pushGui(new GuiMsgBox(mWindow, _("You have been logged out."), std::string(_("OK")), 
        [this] {
            delete this;
            mWindow->pushGui(new EAGamesUI(mWindow));
        }));
}

void EAGamesUI::processImportGames()
{
    if (!mStore) return;
    if (!mStore->IsUserLoggedIn()) { 
        mWindow->pushGui(new GuiMsgBox(mWindow, _("YOU NEED TO BE LOGGED IN TO IMPORT GAMES."), std::string(_("OK")), nullptr));
        return;
    }
    
    GuiInfoPopup* waitPopup = new GuiInfoPopup(mWindow, _("Syncing EA games... This may take a while."), 180000);
    mWindow->pushGui(waitPopup);

    mStore->SyncGames([this, waitPopup](bool success) { 
        if (waitPopup) { delete waitPopup; }
        
        std::string msg = success ? _("EA Games synced successfully!") : _("Failed to sync EA Games.");
        onImportGamesFinished(success, msg);
    });
}

void EAGamesUI::onImportGamesFinished(bool success, const std::string& message)
{
    mWindow->pushGui(new GuiMsgBox(mWindow, message, std::string(_("OK")), nullptr));

    if (success) {
        SystemData* eaSystem = SystemData::getSystem(EAGamesStore::STORE_ID);
        if (eaSystem && mStore) { 
            FolderData* root = eaSystem->getRootFolder();
            if (root) {
                root->clear();
                std::vector<FileData*> gamesFromStore = mStore->getGamesList(); 
                for (FileData* game : gamesFromStore) {
                    root->addChild(game, false); 
                }
                eaSystem->updateDisplayedGameCount(); 
                updateGamelist(eaSystem);
            } 
            
            ViewController::get()->reloadGameListView(eaSystem);
            ViewController::get()->goToGameList(eaSystem);
        } else {
            ViewController::get()->reloadAll(mWindow); 
        }
    }
}
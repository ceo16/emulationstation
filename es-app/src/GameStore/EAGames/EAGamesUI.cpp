// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesUI.cpp
#include "GameStore/EAGames/EAGamesUI.h" 
#include "GameStore/EAGames/EAGamesStore.h"
#include "GameStore/GameStoreManager.h"

#include "Window.h"
#include "components/ButtonComponent.h"
#include "components/TextComponent.h"
#include "components/MenuComponent.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h" 
#include "guis/GuiMsgBox.h"
#include "guis/GuiInfoPopup.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Log.h" 
#include "ApiSystem.h"
#include "views/ViewController.h" 
#include "SystemData.h"   
#include "Gamelist.h"  
#include "utils/TimeUtil.h"
#include "views/SystemView.h" 
#include "components/SwitchComponent.h"

// Assicurati che EAGamesStore::STORE_ID sia definito in EAGamesStore.cpp come:
// const std::string EAGamesStore::STORE_ID = "EAGamesStore"; 
// E che GameStoreManager.cpp registri lo store usando questa costante:
// mStores[EAGamesStore::STORE_ID] = new EAGamesStore(mWindow);

EAGamesUI::EAGamesUI(Window* window) : GuiComponent(window),
    mBackground(window, ":/frame.png"), 
    mGrid(window, Vector2i(1, 3)),
    mStore(nullptr)
{
    LOG(LogInfo) << "EAGamesUI: Constructor called."; // Log generico per vedere se il costruttore parte
    LOG(LogDebug) << "EAGamesUI: Attempting to get store with ID: '" << EAGamesStore::STORE_ID << "'";
    GameStore* storeBase = GameStoreManager::getInstance(mWindow)->getStore(EAGamesStore::STORE_ID);
    LOG(LogDebug) << "EAGamesUI: storeBase pointer from getStore: " << storeBase; // VEDIAMO QUESTO

    if (storeBase) {
        mStore = dynamic_cast<EAGamesStore*>(storeBase);
        LOG(LogDebug) << "EAGamesUI: mStore pointer after dynamic_cast: " << mStore; // E QUESTO
    } else {
        mStore = nullptr; 
        LOG(LogWarning) << "EAGamesUI: getStore returned nullptr for ID: '" << EAGamesStore::STORE_ID << "'";
    }

    if (!mStore) {
        LOG(LogError) << "EAGamesUI: mStore is NULL. EAGamesStore not found or dynamic_cast failed. STORE_ID used: '" << EAGamesStore::STORE_ID << "'. Value of storeBase was: " << storeBase;
        // ... resto della UI di errore ...
        setSize(Renderer::getScreenWidth() * 0.6f, Renderer::getScreenHeight() * 0.4f);
        setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
        addChild(&mBackground); 
        mGrid.clearChildren(); 
        mTitle = std::make_shared<TextComponent>(mWindow, _("EA STORE ERROR"), Font::get(FONT_SIZE_MEDIUM), 0xFF0000FF, ALIGN_CENTER);
        mGrid.setEntry(mTitle, Vector2i(0,0),false,true);
        auto errMsg = std::make_shared<TextComponent>(mWindow, _("EA Store module not loaded correctly."), Font::get(FONT_SIZE_SMALL), 0xAAAAAAFF, ALIGN_CENTER);
        mGrid.setEntry(errMsg, Vector2i(0,1),false,true);
        auto okBtn = std::make_shared<ButtonComponent>(mWindow, _("OK"), _("DISMISS"), [this] { delete this; });
        mGrid.setEntry(okBtn, Vector2i(0,2),true,true, Vector2i(1,1), GridFlags::BORDER_TOP); 
        addChild(&mGrid); 
        mGrid.setSize(mSize);
        mGrid.setRowHeightPerc(0, 0.3f); 
        mGrid.setRowHeightPerc(1, 0.4f);
        mGrid.setRowHeightPerc(2, 0.3f);
        return;
    }

    // Setup UI normale (come prima)
    float width = Renderer::getScreenWidth() * 0.8f;
    float height = Renderer::getScreenHeight() * 0.7f;
    setSize(width, height);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);

    addChild(&mBackground);
    addChild(&mGrid);

    mTitle = std::make_shared<TextComponent>(mWindow, _("EA GAMES STORE"), Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1,1), GridFlags::BORDER_BOTTOM);

    mMenu = std::make_shared<MenuComponent>(mWindow, "");
    mGrid.setEntry(mMenu, Vector2i(0, 1), true, true);

    mVersionInfo = std::make_shared<TextComponent>(mWindow, "", Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
    mGrid.setEntry(mVersionInfo, Vector2i(0, 2), false, true, Vector2i(1,1), GridFlags::BORDER_TOP);
    
    mGrid.setRowHeightPerc(0, 0.15f);
    mGrid.setRowHeightPerc(1, 0.75f);
    mGrid.setRowHeightPerc(2, 0.10f);

    buildMenu();
}

EAGamesUI::~EAGamesUI()
{
    // Questo codice viene eseguito quando la finestra si chiude (es. premendo "BACK")
    // Se abbiamo creato l'interruttore (cioè se l'utente era loggato)...
    if (mEaPlaySwitch)
    {
        // ...leggi il suo stato finale e salva l'impostazione.
        LOG(LogDebug) << "Saving EAPlay.Enabled setting state: " << (mEaPlaySwitch->getState() ? "true" : "false");
        Settings::getInstance()->setBool("EAPlay.Enabled", mEaPlaySwitch->getState());
        Settings::getInstance()->saveFile();
    }
}

// ... (il resto del file EAGamesUI.cpp rimane invariato rispetto alla v10/ultima versione corretta per la compilazione) ...

void EAGamesUI::onSizeChanged()
{
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
    mGrid.setSize(mSize);
}

void EAGamesUI::buildMenu()
{
    if (!mStore) return;

    mMenu->clear();

    if (mStore->IsUserLoggedIn())
    {
        mVersionInfo->setText(_("LOGGED IN TO EA ACCOUNT"));
        mVersionInfo->setColor(0x00C000FF);

        // Mostra lo stato dell'abbonamento
        auto subscriptionLabel = std::make_shared<TextComponent>(mWindow, _("LOADING..."), Font::get(FONT_SIZE_SMALL), 0xAAAAAAFF, ALIGN_CENTER);
        mMenu->addWithLabel(_("EA PLAY STATUS"), subscriptionLabel);
        mStore->getSubscriptionDetails([this, subscriptionLabel](const EAGames::SubscriptionDetails& details) {
            std::string labelText = _("NO ACTIVE SUBSCRIPTION");
            unsigned int color = 0xDD2222FF;
            if (details.isActive) {
                std::string tierName = (details.tier == "premium") ? "EA Play Pro" : "EA Play";
                labelText = tierName + " (" + _("ACTIVE") + ")";
                color = 0x22DD22FF;
            }
            subscriptionLabel->setText(labelText);
            subscriptionLabel->setColor(color);
            mMenu->onSizeChanged();
        });

        mMenu->addEntry(" ");

        // Interruttore per il Catalogo EA Play
        auto eaPlaySwitch = std::make_shared<SwitchComponent>(mWindow);
        eaPlaySwitch->setState(Settings::getInstance()->getBool("EAPlay.Enabled"));
        mMenu->addWithLabel(_("INCLUDE EA PLAY IN GAME LIST"), eaPlaySwitch);
        
      

        mMenu->addEntry(" ");

        mMenu->addEntry(_("REFRESH GAME LIST"), true, [this]() {
            this->processImportGames();
        });
        mMenu->addEntry(_("LOGOUT"), true, [this]() {
            this->processLogout();
        });
    }
    else
    {
        mVersionInfo->setText(_("NOT LOGGED IN"));
        mVersionInfo->setColor(0xC00000FF);
        mMenu->addEntry(_("LOGIN TO EA ACCOUNT"), true, [this]() {
            this->processStartLoginFlow();
        });
    }

    mMenu->addEntry(_("BACK"), true, [this]() {
        delete this;
    });
}


void EAGamesUI::addEAPlayEntries()
{
    if (!mStore) return;

    auto subscriptionLabel = std::make_shared<TextComponent>(mWindow, _("LOADING SUBSCRIPTION STATUS..."), Font::get(FONT_SIZE_SMALL), 0xAAAAAAFF, ALIGN_CENTER);
    mMenu->addWithLabel(_("EA PLAY"), subscriptionLabel);

    mStore->getSubscriptionDetails([this, subscriptionLabel](const EAGames::SubscriptionDetails& details) {
        // --- ECCO LA CORREZIONE ---
        // Rimuoviamo il mWindow->postToUiThread(...) perché la callback è già sul thread giusto.
        // Eseguiamo l'aggiornamento della UI direttamente.
        
        if (details.isActive) {
            std::string tierName = (details.tier == "premium") ? "EA Play Pro" : "EA Play";
            std::string labelText;
            if (details.endTime < 86400) {
                labelText = tierName + " (" + _("ACTIVE") + ")";
            } else {
                std::string expiry = Utils::Time::timeToString(details.endTime, "%d/%m/%Y");
                labelText = tierName + " (" + _("EXPIRES ON") + " " + expiry + ")";
            }
            
            subscriptionLabel->setText(labelText);
            subscriptionLabel->setColor(0x22DD22FF);

            // Questa parte ora verrà eseguita correttamente
            mMenu->addEntry(_("REFRESH GAME LIST (INC. EA PLAY)"), true, [this]() {
                this->processImportGames(); 
            });

        } else {
            subscriptionLabel->setText(_("NO ACTIVE SUBSCRIPTION"));
            subscriptionLabel->setColor(0xDD2222FF);
        }
        mMenu->onSizeChanged();
    });
}

void EAGamesUI::processStartLoginFlow()
{
    if (!this->mStore) return;

    GuiInfoPopup* waitPopup = new GuiInfoPopup(this->mWindow, _("Initiating EA Login..."), 10000);
    this->mWindow->pushGui(waitPopup);

    this->mStore->StartLoginFlow([this, waitPopup](bool success, const std::string& message) {
        if (this->mWindow->peekGui() == waitPopup) {
             delete waitPopup;
        }
        this->onLoginFinished(success, message);
    });
}

void EAGamesUI::onLoginFinished(bool success, const std::string& message)
{
    std::string finalMessage = message;
    if (message.empty()) {
        finalMessage = success ? _("Login process completed successfully.") : _("Login process failed or was cancelled.");
    }
    
    this->mWindow->pushGui(new GuiMsgBox(this->mWindow, finalMessage, std::string(_("OK")), nullptr));
    
    this->buildMenu(); 
}

void EAGamesUI::processLogout()
{
    if (!this->mStore) return;
    this->mStore->Logout(); 
    this->mWindow->pushGui(new GuiMsgBox(this->mWindow, _("You have been logged out."), std::string(_("OK")), nullptr));
    this->buildMenu();
}

void EAGamesUI::processImportGames()
{
    if (!this->mStore) return;
    if (!this->mStore->IsUserLoggedIn()) { 
        this->mWindow->pushGui(new GuiMsgBox(this->mWindow, _("YOU NEED TO BE LOGGED IN TO IMPORT GAMES."), std::string(_("OK")), nullptr));
        return;
    }
    
    GuiInfoPopup* waitPopup = new GuiInfoPopup(this->mWindow, _("Syncing EA games... This may take a while."), 180000);
    this->mWindow->pushGui(waitPopup);

    this->mStore->SyncGames([this, waitPopup](bool success) { 
        if (waitPopup) { delete waitPopup; }
        
        std::string msg = success ? _("EA Games synced successfully!") : _("Failed to sync EA Games.");
        this->onImportGamesFinished(success, msg);
    });
}

void EAGamesUI::onImportGamesFinished(bool success, const std::string& message)
{
    this->mWindow->pushGui(new GuiMsgBox(this->mWindow, message, std::string(_("OK")), nullptr));

    if (success) {
        SystemData* eaSystem = SystemData::getSystem(EAGamesStore::STORE_ID);
        if (eaSystem && mStore) { 
            FolderData* root = eaSystem->getRootFolder();
            if (root) {
                LOG(LogInfo) << "EAGamesUI: Clearing and repopulating EA system's root folder with synced games.";

                // 1. Pulisce la vecchia lista dalla vista
                root->clear();
                
                // 2. Ottiene la NUOVA lista (già processata e pulita) dallo Store
                std::vector<FileData*> gamesFromStore = mStore->getGamesList(); 
                LOG(LogInfo) << "EAGamesUI: Adding " << gamesFromStore.size() << " games from EAGamesStore to EA system's root folder.";

                // 3. Aggiunge i nuovi giochi alla vista
                for (FileData* game : gamesFromStore) {
                    root->addChild(game, false); 
                }
                
                // 4. Aggiorna il conteggio e salva il gamelist.xml corretto
                eaSystem->updateDisplayedGameCount(); 
                updateGamelist(eaSystem);
            } 
            
            LOG(LogInfo) << "EAGamesUI: Reloading and navigating to EA game list view.";
            ViewController::get()->reloadGameListView(eaSystem);
            ViewController::get()->goToGameList(eaSystem);
        } else {
            ViewController::get()->reloadAll(this->mWindow); 
        }
    }
}


bool EAGamesUI::input(InputConfig* config, Input input)
{
    if (!mStore && mMenu == nullptr) { 
         if (mGrid.input(config, input)) { 
             return true;
         }
         return GuiComponent::input(config, input);
    }

    if (mMenu && mMenu->input(config, input)) {
        return true;
    }

    if (input.value != 0 && config->isMappedTo("b", input)) {
        delete this; 
        return true;
    }
    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> EAGamesUI::getHelpPrompts()
{
    std::vector<HelpPrompt> final_prompts; 

    if (!mStore && mMenu == nullptr) { 
        final_prompts.push_back(HelpPrompt("a", _("DISMISS"))); 
        return final_prompts;
    }
    
    if (mMenu) {
        final_prompts = mMenu->getHelpPrompts();
    }
    
    final_prompts.push_back(HelpPrompt("b", _("BACK")));
    return final_prompts;
}
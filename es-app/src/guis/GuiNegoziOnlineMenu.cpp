#include "guis/GuiNegoziOnlineMenu.h"
#include "Log.h" // Per logging se necessario
#include "LocaleES.h" // Per _()
#include "guis/GuiMsgBox.h"
// GameStoreManager.h è già incluso tramite GuiNegoziOnlineMenu.h
#include "GameStore/EAGames/EAGamesStore.h" 
#include "guis/GuiWebViewAuthLogin.h"

GuiNegoziOnlineMenu::GuiNegoziOnlineMenu(Window* window) :
    GuiComponent(window),
    mMenu(window, _("NEGOZI ONLINE")) // Titolo del sottomenu
{
    addChild(&mMenu);
    loadMenuEntries();

    // Imposta dimensione e posizione del menu
    float width = Renderer::getScreenWidth() * 0.6f; // Larghezza 60% dello schermo
    float height = Renderer::getScreenHeight() * 0.5f; // Altezza 50% dello schermo
    setSize(width, height);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2); // Centrato
}

GuiNegoziOnlineMenu::~GuiNegoziOnlineMenu()
{
}

void GuiNegoziOnlineMenu::loadMenuEntries()
{
    // Voce per GAME STORE (se vuoi mantenere la distinzione)
    // mMenu.addEntry(_("SELEZIONE STORE"), true, [this] { // "STORE SELECTION"
    //     GameStoreManager::get()->showStoreSelectionUI(mWindow);
    //     // Se vuoi che questo sottomenu si chiuda dopo aver selezionato una voce:
    //     // delete this;
    // }, "iconFolder"); // Usa un'icona appropriata se disponibile

    // Voce per EPIC GAMES STORE
    mMenu.addEntry(_("EPIC GAMES STORE"), true, [this] {
        // CORREZIONE: Usa getInstance(mWindow)
        GameStoreManager* gsm = GameStoreManager::getInstance(mWindow); 
        if (gsm) { // Controlla sempre se l'istanza è valida
            GameStore* store = gsm->getStore("EpicGamesStore");
            if (store) {
                store->showStoreUI(mWindow);
            } else {
                LOG(LogError) << "EpicGamesStore non trovato nel GameStoreManager!";
                // Mostra messaggio di errore
            }
        }
    }, "iconGames"); 

      mMenu.addEntry(_("STEAM STORE"), true, [this] {
        // CORREZIONE: Usa getInstance(mWindow)
        GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
        if (gsm) {
            GameStore* store = gsm->getStore("SteamStore");
            if (store) { 
                store->showStoreUI(mWindow);
            } else {
                LOG(LogError) << "SteamStore non registrato nel GameStoreManager!";
                mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("STEAM STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
            }
        }
    }, "iconGames"); 
   mMenu.addEntry(_("XBOX STORE"), true, [this] {
        // CORREZIONE: Usa getInstance(mWindow)
        GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
        if (gsm) {
            GameStore* store = gsm->getStore("XboxStore");
            if (store) { 
                store->showStoreUI(mWindow);
            } else {
                LOG(LogError) << "XboxStore non registrato nel GameStoreManager!";
                mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("XBOX STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
            }
        }
    // La parentesi graffa di chiusura della lambda per XBOX STORE è qui
    }, "iconGames");
	
mMenu.addEntry("APRI PAGINA TEST WEBVIEW", true, [this] {
    std::string testUrl = "https://www.google.com"; 
    std::string storeIdentifier = "TestWebView";
    
    // DICHIARA dummyRedirectPrefix QUI
    std::string dummyRedirectPrefix = ""; // Puoi usare una stringa vuota se non ti serve un prefisso specifico per il test di Google

    LOG(LogInfo) << "Tentativo di aprire la pagina di test WebView: " << testUrl;

    // Ora la chiamata al costruttore con 4 argomenti è corretta
    GuiWebViewAuthLogin* webViewTestGui = new GuiWebViewAuthLogin(mWindow, testUrl, storeIdentifier, dummyRedirectPrefix); 
    
    webViewTestGui->setOnLoginFinishedCallback([this, storeIdentifier](bool success, const std::string& dataOrError) {
        LOG(LogInfo) << "Callback dalla WebView (" << storeIdentifier << "): Successo - " << (success ? "Si" : "No") << ", Dati/Errore: " << dataOrError;
        
        std::string titleText = _("RISULTATO TEST WEBVIEW"); 
        std::string bodyText = storeIdentifier + ": " + (success ? "Token/Dati: " : "Errore: ") + dataOrError;
        std::string fullMessageText = titleText + "\n\n" + bodyText;

        mWindow->pushGui(new GuiMsgBox(mWindow, 
            fullMessageText, 
            _("OK"), 
            nullptr, 
            GuiMsgBoxIcon::ICON_INFORMATION 
        ));
    });
    
    mWindow->pushGui(webViewTestGui); 
    webViewTestGui->init(); 
});
    

mMenu.addEntry(_("EA GAME STORE"), true, [this] {
    GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
    if (gsm) {
        // USA LA COSTANTE STORE_ID PER RECUPERARE
        GameStore* store = gsm->getStore(EAGamesStore::STORE_ID); 
        if (store) { 
            store->showStoreUI(mWindow);
        } else {
            LOG(LogError) << "EAGamesStore con ID '" << EAGamesStore::STORE_ID << "' non registrato nel GameStoreManager!";
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("EAGame STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
        }
    }
}, "iconGames");
    // Aggiungi un'opzione per chiudere questo sottomenu
    mMenu.addEntry(_("CHIUDI"), false, [this] { // "CLOSE"
        delete this;
    });
}

bool GuiNegoziOnlineMenu::input(InputConfig* config, Input input)
{
    if (mMenu.input(config, input))
        return true;

    if (config->isMappedTo(BUTTON_BACK, input) && input.value != 0)
    {
        delete this;
        return true;
    }
    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiNegoziOnlineMenu::getHelpPrompts()
{
    return mMenu.getHelpPrompts();
}

void GuiNegoziOnlineMenu::onSizeChanged()
{
    mMenu.setSize(mSize);
    mMenu.setPosition(0,0);
    GuiComponent::onSizeChanged();
}
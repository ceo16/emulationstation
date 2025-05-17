#include "guis/GuiNegoziOnlineMenu.h"
#include "Log.h" // Per logging se necessario
#include "LocaleES.h" // Per _()
#include "guis/GuiMsgBox.h"
// GameStoreManager.h è già incluso tramite GuiNegoziOnlineMenu.h

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
        GameStoreManager::get()->getStore("EpicGamesStore")->showStoreUI(mWindow);
        // delete this; // Opzionale, come sopra
    }, "iconGames"); // Usa un'icona appropriata se disponibile

      mMenu.addEntry(_("STEAM STORE"), true, [this] {
        if (GameStoreManager::get()->getStore("SteamStore")) { // Controlla se lo store è registrato
            GameStoreManager::get()->getStore("SteamStore")->showStoreUI(mWindow);
        } else {
            LOG(LogError) << "SteamStore non registrato nel GameStoreManager!";
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("STEAM STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
        }
    }, "iconGames"); // TODO: Aggiungi un'icona per Steam (es. "iconSteam")
	      mMenu.addEntry(_("XBOX STORE"), true, [this] {
        if (GameStoreManager::get()->getStore("XboxStore")) { // Controlla se lo store è registrato
            GameStoreManager::get()->getStore("XboxStore")->showStoreUI(mWindow);
        } else {
            LOG(LogError) << "XboxStore non registrato nel GameStoreManager!";
            mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("XBOX STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
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
#include "guis/GuiNegoziOnlineMenu.h"
#include "Settings.h"
#include "components/SwitchComponent.h"
#include "LocaleES.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/EAGames/EAGamesStore.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "guis/GuiMsgBox.h"
#include "Log.h"
#include "GameStore/Amazon/AmazonUI.h"
#include "GameStore/GOG/GogUI.h"

GuiNegoziOnlineMenu::GuiNegoziOnlineMenu(Window* window) 
	: GuiSettings(window, _("IMPOSTAZIONI NEGOZI ONLINE")) // Titolo della finestra
{
	// =======================================================================
	// === SEZIONE 1: INTERRUTTORI PER ABILITARE/DISABILITARE GLI STORE ===
	// =======================================================================

    // --- Amazon Games Store Switch --- (NUOVO BLOCCO)
    auto enable_amazon_games = std::make_shared<SwitchComponent>(mWindow);
    enable_amazon_games->setState(Settings::getInstance()->getBool("EnableAmazonGames"));
    addWithLabel(_("VISUALIZZA AMAZON GAMES STORE"), enable_amazon_games);
    addSaveFunc([enable_amazon_games] {
    Settings::getInstance()->setBool("EnableAmazonGames", enable_amazon_games->getState());
    });

	// --- EA Games Store Switch ---
	auto enable_ea_games = std::make_shared<SwitchComponent>(mWindow);
	enable_ea_games->setState(Settings::getInstance()->getBool("EnableEAGamesStore"));
	addWithLabel(_("VISUALIZZA EA GAMES STORE"), enable_ea_games);
	addSaveFunc([enable_ea_games] {
		Settings::getInstance()->setBool("EnableEAGamesStore", enable_ea_games->getState());
	});

	// --- Epic Games Store Switch ---
	auto enable_epic_games = std::make_shared<SwitchComponent>(mWindow);
	enable_epic_games->setState(Settings::getInstance()->getBool("EnableEpicGamesStore"));
	addWithLabel(_("VISUALIZZA EPIC GAMES STORE"), enable_epic_games);
	addSaveFunc([enable_epic_games] {
		Settings::getInstance()->setBool("EnableEpicGamesStore", enable_epic_games->getState());
	});
	
	// --- GOG.com Store Switch --- (NUOVO BLOCCO)
    auto enable_gog = std::make_shared<SwitchComponent>(mWindow);
    enable_gog->setState(Settings::getInstance()->getBool("EnableGogStore"));
    addWithLabel(_("VISUALIZZA GOG.COM STORE"), enable_gog);
    addSaveFunc([enable_gog] {
        Settings::getInstance()->setBool("EnableGogStore", enable_gog->getState());
    });

	// --- Steam Store Switch ---
	auto enable_steam = std::make_shared<SwitchComponent>(mWindow);
	enable_steam->setState(Settings::getInstance()->getBool("EnableSteamStore"));
	addWithLabel(_("VISUALIZZA STEAM"), enable_steam);
	addSaveFunc([enable_steam] {
		Settings::getInstance()->setBool("EnableSteamStore", enable_steam->getState());
	});

	// --- Xbox Store Switch ---
	auto enable_xbox = std::make_shared<SwitchComponent>(mWindow);
	enable_xbox->setState(Settings::getInstance()->getBool("EnableXboxStore"));
	addWithLabel(_("VISUALIZZA XBOX GAME PASS"), enable_xbox);
	addSaveFunc([enable_xbox] {
		Settings::getInstance()->setBool("EnableXboxStore", enable_xbox->getState());
	});

	// Aggiunge una riga vuota per separare le sezioni
	addEntry(" ");
	addEntry("------------------------------------");
	addEntry(" ");

	// =======================================================================
	// === SEZIONE 2: I TUOI PULSANTI ESISTENTI PER ACCEDERE AGLI STORE ===
	// =======================================================================

     // Voce per AMAZON GAMES STORE (NUOVO BLOCCO)
    addEntry(_("APRI AMAZON GAMES STORE"), true, [this] {
        // Ora creiamo e apriamo la nostra nuova classe di menu
        mWindow->pushGui(new AmazonUI(mWindow));
    }, "iconGames");

	// Voce per EPIC GAMES STORE
	addEntry(_("APRI EPIC GAMES STORE"), true, [this] {
		GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
		if (gsm) {
			GameStore* store = gsm->getStore("EpicGamesStore");
			if (store) {
				store->showStoreUI(mWindow);
			} else {
				LOG(LogError) << "EpicGamesStore non trovato nel GameStoreManager!";
			}
		}
	}, "iconGames");
	
	 // Voce per GOG.COM STORE (NUOVO BLOCCO)
    addEntry(_("APRI GOG.COM STORE"), true, [this] {
        mWindow->pushGui(new GogUI(mWindow));
    }, "iconGames");

	// Voce per STEAM STORE
	addEntry(_("APRI STEAM STORE"), true, [this] {
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

	// Voce per XBOX STORE
	addEntry(_("APRI XBOX STORE"), true, [this] {
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
	}, "iconGames");

	// Voce per EA GAMES STORE
	addEntry(_("APRI EA GAMES STORE"), true, [this] {
		GameStoreManager* gsm = GameStoreManager::getInstance(mWindow);
		if (gsm) {
			GameStore* store = gsm->getStore(EAGamesStore::STORE_ID);
			if (store) {
				store->showStoreUI(mWindow);
			} else {
				LOG(LogError) << "EAGamesStore con ID '" << EAGamesStore::STORE_ID << "' non registrato nel GameStoreManager!";
				mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE"), _("EAGame STORE NON ANCORA IMPLEMENTATO CORRETTAMENTE.")));
			}
		}
	}, "iconGames");


}
GuiNegoziOnlineMenu::~GuiNegoziOnlineMenu()
{
	// Questo chiama tutte le funzioni registrate con addSaveFunc()
	// e poi salva il file di configurazione es_settings.cfg
	save();
	Settings::getInstance()->saveFile();
}
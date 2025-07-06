#include "GogGamesStore.h"
#include "SystemData.h"
#include "Log.h"
#include "utils/Platform.h"
#include "views/ViewController.h"
#include "FileData.h"
#include <future>
#include <thread>
#include <map>

// --- MODIFICA CHIAVE: Aggiunti tutti gli header necessari ---
#include "GameStore/GOG/GogAuth.h"
#include "GameStore/GOG/GogGamesAPI.h"
#include "GameStore/GOG/GogScanner.h"

GogGamesStore::GogGamesStore(Window* window) : mWindow(window) {}
GogGamesStore::~GogGamesStore() {}

bool GogGamesStore::init(Window* window) {
    mWindow = window;
    mAuth = std::make_unique<GogAuth>(mWindow);
    mApi = std::make_unique<GogGamesAPI>(mWindow, mAuth.get());
    mScanner = std::make_unique<GogScanner>();
    _initialized = true; 
    LOG(LogInfo) << "GOG Games Store Initialized.";
    return true;
}

void GogGamesStore::shutdown() {}

void GogGamesStore::showStoreUI(Window* window) {
    LOG(LogInfo) << "showStoreUI for GOG not implemented yet.";
}

std::string GogGamesStore::getStoreName() const {
    return "gog";
}

std::vector<FileData*> GogGamesStore::getGamesList() {
    SystemData* sys = SystemData::getSystem("gog");
    if (sys) return sys->getRootFolder()->getChildren();
    return std::vector<FileData*>();
}

bool GogGamesStore::launchGame(const std::string& gameId) {
    LOG(LogWarning) << "GOG direct launch from store object not yet implemented. Use system view.";
    return false;
}

bool GogGamesStore::installGame(const std::string& gameId) { return false; }
bool GogGamesStore::uninstallGame(const std::string& gameId) { return false; }
bool GogGamesStore::updateGame(const std::string& gameId) { return false; }

// Ora le chiamate -> funzioneranno perché il compilatore conosce la definizione completa delle classi
bool GogGamesStore::isAuthenticated() const { return mAuth && mAuth->isAuthenticated(); }

void GogGamesStore::login(std::function<void(bool)> on_complete) {
    if (mAuth) {
        mAuth->login([this, on_complete](bool success) {
            if (success) {
                syncGames(nullptr);
            }
            if (on_complete) {
                on_complete(success);
            }
        });
    }
}

void GogGamesStore::logout() {
    LOG(LogInfo) << "GOG logout not directly supported. Please clear browser/WebView cookies manually.";
    SystemData* sys = SystemData::getSystem("gog");
    if (sys) {
        sys->getRootFolder()->clear();
        ViewController::get()->reloadGameListView(sys);
    }
}

void GogGamesStore::syncGames(std::function<void(bool)> on_complete) {
    if (!isAuthenticated()) {
        if (on_complete) on_complete(false);
        return;
    }
    LOG(LogInfo) << "GOG Sync: Starting...";
    std::thread([this, on_complete]() {
        // Ora la chiamata ->findInstalledGames() funzionerà
        auto installedGames = mScanner->findInstalledGames(); 
        
        // E anche la chiamata ->getOwnedGames()
        mApi->getOwnedGames([this, installedGames, on_complete](std::vector<GOG::LibraryGame> onlineGames, bool success) {
            if (success) {
                mWindow->postToUiThread([this, onlineGames, installedGames] {
                    processGamesList(onlineGames, installedGames);
                });
            }
            if (on_complete) {
                mWindow->postToUiThread([on_complete, success] { on_complete(success); });
            }
        });
    }).detach();
}

void GogGamesStore::processGamesList(const std::vector<GOG::LibraryGame>& onlineGames, const std::vector<GOG::InstalledGameInfo>& installedGames) {
    SystemData* sys = SystemData::getSystem("gog");
    if (!sys) {
        LOG(LogError) << "[GOG Sync] Sistema 'gog' non trovato!";
        return;
    }

    FolderData* root = sys->getRootFolder();

    // 1. PULISCI LA VECCHIA LISTA
    // Questo è il passo fondamentale. Svuota la memoria da qualsiasi stato precedente o corrotto.
    LOG(LogInfo) << "[GOG Sync] Pulizia della lista giochi esistente prima della ricostruzione...";
    root->clear();

    // 2. PREPARA I DATI
    // Mappa dei giochi installati per una ricerca veloce
    std::set<std::string> installedGameIds;
    for (const auto& game : installedGames) {
        installedGameIds.insert(game.id);
    }
    LOG(LogInfo) << "[GOG Sync] Ricostruzione della lista basata su " << onlineGames.size() << " giochi della libreria online.";

// 3. RICOSTRUISCI LA LISTA DA ZERO
for (const auto& onlineGame : onlineGames) {
    bool isInstalled = (installedGameIds.count(onlineGame.game.id) > 0);
    
    // Il path viene già calcolato correttamente
    std::string path = isInstalled ? "gog_installed:/" + onlineGame.game.id : "gog_virtual:/" + onlineGame.game.id;

    FileData* fd = new FileData(FileType::GAME, path, sys);
    MetaDataList& mdl = fd->getMetadata();

    // Imposta i metadati
    mdl.set("storeId", onlineGame.game.id);
    mdl.set(MetaDataId::Name, onlineGame.game.title);
    mdl.set(MetaDataId::Image, onlineGame.game.image);
    mdl.set(MetaDataId::Installed, isInstalled ? "true" : "false");
    mdl.set(MetaDataId::Virtual, !isInstalled ? "true" : "false");

    // --- ECCO LA MODIFICA FONDAMENTALE ---
    // Imposta il comando di lancio CORRETTO in base allo stato di installazione
    // IMMEDIATAMENTE alla creazione dell'oggetto in memoria.
    if (isInstalled) {
        mdl.set(MetaDataId::LaunchCommand, "goggalaxy://launch/" + onlineGame.game.id);
    } else {
        mdl.set(MetaDataId::LaunchCommand, "goggalaxy://openGameView/" + onlineGame.game.id);
    }

    root->addChild(fd, false);
}
    
    // 4. AGGIORNA LA UI
    sys->updateDisplayedGameCount(); // Aggiorna il conteggio
    ViewController::get()->reloadGameListView(sys); 
    LOG(LogInfo) << "[GOG Sync] Ricostruzione completata. Conteggio finale: " << root->getChildren().size();
}
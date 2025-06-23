#include "AmazonGamesStore.h"
#include "SystemData.h"
#include "Log.h"
#include "utils/Platform.h"       // Header per Utils::Platform::ProcessStartInfo
#include "views/ViewController.h"
#include "Window.h"               // Header per la classe Window
#include <future>
#include <thread>

// Il costruttore ora è corretto per la tua classe base GameStore
AmazonGamesStore::AmazonGamesStore(Window* window) : mWindow(window) {}

AmazonGamesStore::~AmazonGamesStore() {
    shutdown();
}

bool AmazonGamesStore::init(Window* window) {
    mWindow = window;
    mAuth = std::make_unique<AmazonAuth>(mWindow);
    
    // --- MODIFICA CHIAVE ---
    // Passiamo mWindow al costruttore di AmazonGamesAPI
    mApi = std::make_unique<AmazonGamesAPI>(mWindow, mAuth.get());

    mScanner = std::make_unique<AmazonGamesScanner>();
    _initialized = true; 
    LOG(LogInfo) << "Amazon Games Store Initialized.";
    return true;
}

void AmazonGamesStore::shutdown() {
    for (auto& game : mGames) {
        delete game;
    }
    mGames.clear();
}

void AmazonGamesStore::showStoreUI(Window* window) {
    LOG(LogInfo) << "showStoreUI for Amazon not implemented yet.";
}

std::string AmazonGamesStore::getStoreName() const {
    return "amazon";
}

std::vector<FileData*> AmazonGamesStore::getGamesList() {
    return mGames;
}

bool AmazonGamesStore::launchGame(const std::string& gameId) {
    if (gameId.empty()) return false;
    LOG(LogInfo) << "Launching Amazon game with ID: " << gameId;
    std::string launchCommand = "start amazon-games://play/" + gameId;
    
    // CORREZIONE: Usa la classe ProcessStartInfo corretta dal tuo platform.h
    Utils::Platform::ProcessStartInfo(launchCommand).run();
    return true;
}

bool AmazonGamesStore::installGame(const std::string& gameId) {
    LOG(LogInfo) << "installGame for Amazon not implemented yet.";
    // CORREZIONE: Usa la classe ProcessStartInfo corretta
    Utils::Platform::ProcessStartInfo("start ms-windows-store://pdp/?productid=9NFQ1K0M448T").run();
    return false;
}

bool AmazonGamesStore::uninstallGame(const std::string& gameId) {
    LOG(LogInfo) << "uninstallGame for Amazon not implemented yet.";
    return false;
}

bool AmazonGamesStore::updateGame(const std::string& gameId) {
    LOG(LogInfo) << "updateGame for Amazon not implemented yet.";
    return false;
}

bool AmazonGamesStore::isAuthenticated() const {
    return mAuth && mAuth->isAuthenticated();
}

void AmazonGamesStore::login(std::function<void(bool)> on_complete) {
    if (mAuth) {
        mAuth->startLoginFlow([this, on_complete](bool success) {
            if (success) {
                syncGames(nullptr);
            }
            if (on_complete) {
                on_complete(success);
            }
        });
    }
}

void AmazonGamesStore::logout() {
    if (mAuth) mAuth->logout();
    shutdown(); 
    SystemData* sys = SystemData::getSystem("amazon");
    if (sys) {
        // CORREZIONE: Usa il metodo corretto dal tuo ViewController.h
        ViewController::get()->reloadGameListView(sys);
    }
}

void AmazonGamesStore::syncGames(std::function<void(bool)> on_complete) {
    if (!isAuthenticated()) {
        if (on_complete) on_complete(false);
        return;
    }

    LOG(LogInfo) << "Amazon Sync: Starting...";
    std::thread([this, on_complete]() {
        auto installedGames = mScanner->findInstalledGames();
        
        mApi->getOwnedGames([this, installedGames, on_complete](std::vector<Amazon::GameEntitlement> onlineGames, bool success) {
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

void AmazonGamesStore::processGamesList(const std::vector<Amazon::GameEntitlement>& onlineGames, const std::vector<Amazon::InstalledGameInfo>& installedGames) {
    SystemData* sys = SystemData::getSystem("amazon");
    if (!sys) {
        LOG(LogError) << "Amazon Store: System 'amazon' not found!";
        return;
    }

    FolderData* root = sys->getRootFolder();
    root->clear(); // Pulisce i giochi esistenti dalla vista

    // Puliamo la nostra cache interna per evitare puntatori duplicati
    for (auto game : mGames) {
        // Non cancelliamo il FileData qui, perché ora è gestito da root->clear()
    }
    mGames.clear();

    std::map<std::string, std::string> installedMap;
    for (const auto& game : installedGames) {
        installedMap[game.id] = game.installDirectory;
    }

    int processedCount = 0;
    for (const auto& onlineGame : onlineGames) {
        if (onlineGame.product_productLine == "Twitch:FuelEntitlement") {
            continue;
        }

        bool isInstalled = (installedMap.find(onlineGame.id) != installedMap.end());
        std::string path = isInstalled ? "amazon_installed:/" + onlineGame.id : "amazon_virtual:/" + onlineGame.id;

        FileData* fd = new FileData(FileType::GAME, path, sys);
        MetaDataList& mdl = fd->getMetadata();
        mdl.set(MetaDataId::Name, onlineGame.product_title);
        mdl.set(MetaDataId::Image, onlineGame.product_imageUrl);
        mdl.set(MetaDataId::Installed, isInstalled ? "true" : "false");
        mdl.set(MetaDataId::Virtual, !isInstalled ? "true" : "false");
        mdl.set("storeId", onlineGame.id);
		mdl.set(MetaDataId::LaunchCommand, "amazon-games://play/" + onlineGame.id);
        
        
        root->addChild(fd, false);
        processedCount++;
    }
    
    // --- MODIFICA FINALE ---
    // Chiamiamo la funzione con il numero corretto di argomenti (uno solo).
    ViewController::get()->reloadGameListView(sys);

    LOG(LogInfo) << "Amazon Sync: Processing complete. Total games processed: " << processedCount;
}
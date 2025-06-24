#include "AmazonGamesStore.h"
#include "SystemData.h"
#include "Log.h"
#include "utils/Platform.h" // Header corretto per ProcessStartInfo
#include "views/ViewController.h"
#include "FileData.h"
#include <future>
#include <thread>
#include <map>

AmazonGamesStore::AmazonGamesStore(Window* window) : mWindow(window) {}
AmazonGamesStore::~AmazonGamesStore() {}

bool AmazonGamesStore::init(Window* window) {
    mWindow = window;
    mAuth = std::make_unique<AmazonAuth>(mWindow);
    mApi = std::make_unique<AmazonGamesAPI>(mWindow, mAuth.get());
    mScanner = std::make_unique<AmazonGamesScanner>();
    _initialized = true; 
    return true;
}

void AmazonGamesStore::shutdown() {}

void AmazonGamesStore::showStoreUI(Window* window) {}

std::string AmazonGamesStore::getStoreName() const {
    return "amazon";
}

std::vector<FileData*> AmazonGamesStore::getGamesList() {
    SystemData* sys = SystemData::getSystem("amazon");
    if (sys) {
        // CORREZIONE: Il metodo corretto per ottenere i figli è getChildren()
        return sys->getRootFolder()->getChildren();
    }
    return std::vector<FileData*>();
}

bool AmazonGamesStore::launchGame(const std::string& gameId) {
    if (gameId.empty()) return false;
    std::string launchCommand = "start amazon-games://play/" + gameId;
    // CORREZIONE: Usiamo il metodo corretto dal tuo platform.h
    Utils::Platform::ProcessStartInfo(launchCommand).run();
    return true;
}

bool AmazonGamesStore::installGame(const std::string& gameId) {
    // CORREZIONE: Usiamo il metodo corretto dal tuo platform.h
    Utils::Platform::ProcessStartInfo("start ms-windows-store://pdp/?productid=9NFQ1K0M448T").run();
    return false;
}

bool AmazonGamesStore::uninstallGame(const std::string& gameId) { return false; }
bool AmazonGamesStore::updateGame(const std::string& gameId) { return false; }
bool AmazonGamesStore::isAuthenticated() const { return mAuth && mAuth->isAuthenticated(); }

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
    SystemData* sys = SystemData::getSystem("amazon");
    if (sys) {
        sys->getRootFolder()->clear();
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

    // Crea la nuova lista di giochi in un vettore temporaneo
    std::vector<FileData*> newGameList;
    std::map<std::string, std::string> installedMap;
    for (const auto& game : installedGames) {
        installedMap[game.id] = game.installDirectory;
    }

    int processedCount = 0;
    for (const auto& onlineGame : onlineGames) {
        if (onlineGame.product_productLine == "Twitch:FuelEntitlement") continue;

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
        
		std::string esSystemLanguage = Settings::getInstance()->getString("Language");
    if (!esSystemLanguage.empty()) {
        mdl.set(MetaDataId::Language, esSystemLanguage);
    }
        newGameList.push_back(fd);
        processedCount++;
    }
    
    // Ora che la nuova lista è pronta, aggiorniamo il sistema in modo "atomico"
    // Questo previene che la UI acceda a dati mentre vengono cancellati.
    sys->getRootFolder()->clear(); // Pulisce la vecchia lista
    for (auto game : newGameList) {
        sys->getRootFolder()->addChild(game, false); // Aggiunge i nuovi giochi
    }
    
    // Infine, notifichiamo alla UI che può ricaricare la vista in sicurezza
    ViewController::get()->reloadGameListView(sys);

    LOG(LogInfo) << "Amazon Sync: Processing complete. Total games processed: " << processedCount;
}
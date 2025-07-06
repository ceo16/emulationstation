#include "SystemData.h"

#include "SystemConf.h"
#include "utils/FileSystemUtil.h"
#include "utils/ThreadPool.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Gamelist.h"
#include "Log.h"
#include "utils/Platform.h"
#include "Settings.h"
#include "ThemeData.h"
#include "views/UIModeController.h"
#include <fstream>
#include "Window.h"
#include "LocaleES.h"
#include "utils/StringUtil.h"
#include "utils/Randomizer.h"
#include "views/ViewController.h"
#include "ThreadedHasher.h"
#include <unordered_set>
#include <algorithm>
#include <functional>
#include "SaveStateRepository.h"
#include "Paths.h"
#include "SystemRandomPlaylist.h"
#include "PlatformId.h"
#include <map>
#include <set>
#include <vector>
#include "HttpReq.h"
#include "GameStore/EpicGames/EpicGamesStoreAPI.h"
#include "GameStore/EpicGames/EpicGamesModels.h"
#include "MetaData.h"
#include "GameStore/GameStoreManager.h" // <--- AGGIUNGI QUESTO
#include "GameStore/EpicGames/EpicGamesStore.h" // <--- AGGIUNGI QUESTO
#include "GameStore/EpicGames/EpicGamesModels.h"
 #include "GameStore/Steam/SteamStore.h"
 #include "GameStore/Steam/SteamStoreAPI.h"
 #include "GameStore/Steam/SteamAuth.h"
 #include "GameStore/GameStore.h" 
 #include "FileData.h"
 #include "GameStore/Xbox/XboxAuth.h"
#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/GameStoreManager.h"
#include "GameStore/EAGames/EAGamesStore.h"
#include "GameStore/EAGames/EAGamesModels.h"
#include "GameStore/EAGames/EAGamesScanner.h"
#include "GameStore/Amazon/AmazonGamesStore.h"
#include "GameStore/GOG/GogGamesStore.h"
#include "GameStore/GOG/GogScanner.h"

#if WIN32
#include "Win32ApiSystem.h"
#endif

using namespace Utils;



const std::string SystemData::VIRTUAL_EPIC_PREFIX = "epic:/virtual/"; // Definizione della costante

static std::map<std::string, std::function<BindableProperty(SystemData*)>> properties =
{
	{ "name",				[] (SystemData* sys) { return sys->getName(); } },
	{ "fullName",			[] (SystemData* sys) { return sys->getFullName(); } },
	{ "manufacturer",		[] (SystemData* sys) { return sys->getSystemMetadata().manufacturer; } },
	{ "theme",				[] (SystemData* sys) { return sys->getThemeFolder(); } },
	{ "releaseYear",		[] (SystemData* sys) { return sys->getSystemMetadata().releaseYear <= 0 ? std::string() : std::to_string(sys->getSystemMetadata().releaseYear); } },
	{ "hardwareType",		[] (SystemData* sys) { return sys->getSystemMetadata().hardwareType; } },
	{ "command",			[] (SystemData* sys) { return sys->getSystemEnvData()->mLaunchCommand; } },
	{ "group",				[] (SystemData* sys) { return sys->getSystemEnvData()->mGroup; } },		
	{ "collection",			[] (SystemData* sys) { return sys->isCollection(); } },		
	{ "showManual",         [] (SystemData* sys) { return sys->getBoolSetting("ShowManualIcon"); } },
	{ "showSaveStates",     [] (SystemData* sys) { return sys->getBoolSetting("ShowSaveStates"); } },
	{ "showCheevos",        [] (SystemData* sys) { return sys->getShowCheevosIcon() && sys->getBoolSetting("ShowCheevosIcon"); } },
	{ "showFlags",          [] (SystemData* sys) { return sys->getShowFlags(); } },
	{ "showFavorites",      [] (SystemData* sys) { return sys->getShowFavoritesIcon(); } },
	{ "showGun",            [] (SystemData* sys) { return sys->getBoolSetting("ShowGunIconOnGames"); } },
	{ "showWheel",          [] (SystemData* sys) { return sys->getBoolSetting("ShowWheelIconOnGames"); } },
	{ "showTrackball",      [] (SystemData* sys) { return sys->getBoolSetting("ShowTrackballIconOnGames"); } },
	{ "showSpinner",      [] (SystemData* sys) { return sys->getBoolSetting("ShowSpinnerIconOnGames"); } },
	{ "showParentFolder",   [] (SystemData* sys) { return sys->getShowParentFolder(); } },
	{ "hasKeyboardMapping", [] (SystemData* sys) { return sys->hasKeyboardMapping(); } },
	{ "isCheevosSupported", [] (SystemData* sys) { return sys->isCheevosSupported(); } },
	{ "isNetplaySupported", [] (SystemData* sys) { return sys->isNetplaySupported(); } },
	{ "hasfilter",			[] (SystemData* sys) { auto idx = sys->getIndex(false); return idx != nullptr && idx->isFiltered(); } },
	{ "filter",				[] (SystemData* sys) { auto idx = sys->getIndex(false); return (idx != nullptr && idx->isFiltered() ? idx->getDisplayLabel(true) : BindableProperty::EmptyString); } },
};

VectorEx<SystemData*> SystemData::sSystemVector;
bool SystemData::IsManufacturerSupported = false;

std::string normalizeGameNameForEA(const std::string& name) {
    std::string lowerName = Utils::String::toLower(name);
    std::string result = "";
    for (char c : lowerName) {
        if (isalnum(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    // Gestiamo il caso specifico di Plants vs Zombies che ha nomi diversi
    if (result.find("piantecontrozombi") != std::string::npos || result.find("plantsvszombies") != std::string::npos) {
        return "plantsvszombies"; // Riconduciamo entrambi i nomi a un'unica chiave standard
    }
    return result;
}

SystemData::SystemData(const SystemMetadata& meta, SystemEnvironmentData* envData, std::vector<EmulatorData>* pEmulators, bool CollectionSystem, bool groupedSystem, bool withTheme, bool loadThemeOnlyIfElements, bool isItAStoreSystem)  :
	mMetadata(meta), mEnvData(envData), mIsCollectionSystem(CollectionSystem), mIsGameSystem(true), mIsStoreSystem(isItAStoreSystem) 
{
	mBindableRandom = nullptr;
	mSaveRepository = nullptr;
	mIsCheevosSupported = -1;
	mIsGroupSystem = groupedSystem;
	mGameListHash = 0;
	mGameCountInfo = nullptr;
	mSortId = Settings::getInstance()->getInt(getName() + ".sort");
	mGridSizeOverride = Vector2f(0, 0);

	mFilterIndex = nullptr;

	if (pEmulators != nullptr)
		mEmulators = *pEmulators;

	auto hiddenSystems = Utils::String::split(Settings::HiddenSystems(), ';');
	mHidden = (mIsCollectionSystem ? withTheme : (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), getName()) != hiddenSystems.cend()));

	loadFeatures();

	// if it's an actual system, initialize it, if not, just create the data structure
	if (!mIsCollectionSystem && mIsGameSystem)
	{
		mRootFolder = new FolderData(mEnvData->mStartPath, this);
		mRootFolder->getMetadata().set(MetaDataId::Name, mMetadata.fullName);

		std::unordered_map<std::string, FileData*> fileMap;
		fileMap[mEnvData->mStartPath] = mRootFolder;

		if (!Settings::ParseGamelistOnly())
		{
			populateFolder(mRootFolder, fileMap);

			if (!UIModeController::LoadEmptySystems())
			{
				if (mRootFolder->getChildren().size() == 0)
					return;

				if (mHidden && !Settings::HiddenSystemsShowGames())
					return;
			}
		}

		if (!Settings::IgnoreGamelist())
			parseGamelist(this, fileMap);

		if (Settings::RemoveMultiDiskContent())
			removeMultiDiskContent(fileMap);
	}
	else
	{
		// virtual systems are updated afterwards, we're just creating the data structure
		mRootFolder = new FolderData(mMetadata.fullName, this);
		mRootFolder->getMetadata().set(MetaDataId::Name, mMetadata.fullName);
	}

	mRootFolder->getMetadata().resetChangedFlag();

	if (withTheme && (!loadThemeOnlyIfElements || UIModeController::LoadEmptySystems() || mRootFolder->mChildren.size() > 0))
	{
		loadTheme();

		auto defaultView = Settings::getInstance()->getString(getName() + ".defaultView");
		auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(getName() + ".gridSize"));
		setSystemViewMode(defaultView, gridSizeOverride, false);

		setIsGameSystemStatus();

		if (Settings::PreloadMedias())
			getSaveStateRepository();
	}
} // Fine costruttore SystemData

SystemData::~SystemData()
{
	if (mBindableRandom)
		delete mBindableRandom;

	if (mRootFolder)
		delete mRootFolder;

	if (!mIsCollectionSystem && mEnvData != nullptr)
		delete mEnvData;

	if (mSaveRepository != nullptr)
		delete mSaveRepository;

	if (mGameCountInfo != nullptr)
		delete mGameCountInfo;

	if (mFilterIndex != nullptr)
		delete mFilterIndex;
}

bool SystemData::isStoreSystem() const
{
    return mIsStoreSystem;
}

void SystemData::removeMultiDiskContent(std::unordered_map<std::string, FileData*>& fileMap)
{	
	if (mEnvData == nullptr ||!(mEnvData->isValidExtension(".cue") || mEnvData->isValidExtension(".ccd") || mEnvData->isValidExtension(".gdi") || mEnvData->isValidExtension(".m3u")))
		return;

	StopWatch stopWatch("RemoveMultiDiskContent - "+ getName() +" :", LogDebug);

	std::vector<std::string> files;

	std::vector<FolderData*> folders;

	std::stack<FolderData*> stack;
	stack.push(mRootFolder);

	while (stack.size())
	{
		FolderData* current = stack.top();
		stack.pop();

		for (auto it : current->getChildren())
		{
			if (it->getType() == GAME && it->hasContentFiles())
			{
				for (auto ct : it->getContentFiles())
					files.push_back(ct);
			}
			else if (it->getType() == FOLDER)
			{
				folders.push_back((FolderData*)it);
				stack.push((FolderData*)it);
			}
		}
	}

	for (auto file : files)
	{
		auto it = fileMap.find(file);
		if (it != fileMap.cend())
		{
			delete it->second;
			fileMap.erase(it);
		}
	}

	// Remove empty folders
	for (auto folder = folders.crbegin(); folder != folders.crend(); ++folder)
	{
		if ((*folder)->getChildren().size())
			continue;
		
		auto it = fileMap.find((*folder)->getPath());
		if (it != fileMap.cend())
		{
			fileMap.erase(it);
			delete (*folder);
		}		
	}
}




  void SystemData::populateSteamVirtual(SystemData* system) {
  if (system == nullptr || system->getName() != "steam") {
  LOG(LogError) << "populateSteamVirtual called with invalid system!";
  return;
  }
 

  LOG(LogInfo) << "SystemData::populateSteamVirtual - STARTING";
 

  //  1. Get SteamStore instance from GameStoreManager
   GameStoreManager* storeManager = GameStoreManager::getInstance(nullptr); // Assumendo che sia già stato inizializzato in main
    SteamStore* steamStore = nullptr;
    if (storeManager) { // Controlla se storeManager non è nullo
        GameStore* baseStore = storeManager->getStore("SteamStore"); // Usa il nome corretto registrato
        if (baseStore) {
            steamStore = dynamic_cast<SteamStore*>(baseStore);
        }
    } else {
        LOG(LogError) << "populateSteamVirtual - GameStoreManager instance is null!";
        return; // Esci se il manager non è disponibile
    }
 

    if (!steamStore) {
  LOG(LogError) << "populateSteamVirtual - Could not get SteamStore instance from GameStoreManager!";
  return;
  }
 

  //  2. Find installed games
  std::vector<SteamInstalledGameInfo> installedGames = steamStore->findInstalledSteamGames();
  LOG(LogInfo) << "Found " << installedGames.size() << " installed Steam games.";
 

  //  3. Get online games (if authenticated)
  std::vector<Steam::OwnedGame> onlineGames;
  if (steamStore->getAuth() && steamStore->getAuth()->isAuthenticated() && !steamStore->getAuth()->getSteamId().empty() && !steamStore->getAuth()->getApiKey().empty()) {
  onlineGames = steamStore->getApi()->GetOwnedGames(steamStore->getAuth()->getSteamId(), steamStore->getAuth()->getApiKey()); // Use getApi()
  LOG(LogInfo) << "Obtained " << onlineGames.size() << " games from the online Steam library.";
  } else {
  LOG(LogWarning) << "Not authenticated with Steam, skipping online games.";
  }
 

  //  4. Create FileData objects and add them to the system's root folder
  FolderData* rootFolder = system->getRootFolder();
  if (!rootFolder) {
  LOG(LogError) << "SystemData::populateSteamVirtual - Root folder is null!";
  return;
  }
 

  for (const auto& installedGame : installedGames) {
  if (installedGame.appId == 0) continue;
  std::string pseudoPath = "steam://game/" + std::to_string(installedGame.appId);
  FileData* game = new FileData(FileType::GAME, pseudoPath, system);
  game->setMetadata(MetaDataId::Name, installedGame.name);
  game->setMetadata(MetaDataId::SteamAppId, std::to_string(installedGame.appId));
  game->setMetadata(MetaDataId::Installed, "true");
  game->setMetadata(MetaDataId::Virtual, "false");
  game->setMetadata(MetaDataId::Path, installedGame.libraryFolderPath + "/common/" + installedGame.installDir);
  game->setMetadata(MetaDataId::LaunchCommand, steamStore->getGameLaunchUrl(installedGame.appId));
  rootFolder->addChild(game);
  }
 

  for (const auto& onlineGame : onlineGames) {
  if (onlineGame.appId == 0) continue;
  if (rootFolder->FindByPath("steam://game/" + std::to_string(onlineGame.appId)) == nullptr) {
  FileData* game = new FileData(FileType::GAME, "steam://game/" + std::to_string(onlineGame.appId), system);
  game->setMetadata(MetaDataId::Name, onlineGame.name);
  game->setMetadata(MetaDataId::SteamAppId, std::to_string(onlineGame.appId));
  game->setMetadata(MetaDataId::Installed, steamStore->checkInstallationStatus(onlineGame.appId, installedGames) ? "true" : "false");
  game->setMetadata(MetaDataId::Virtual, "true");
  game->setMetadata(MetaDataId::LaunchCommand, steamStore->getGameLaunchUrl(onlineGame.appId));
  rootFolder->addChild(game);
  }
  }
 

  LOG(LogInfo) << "SystemData::populateSteamVirtual - ENDED";
 }
 
 
 
void SystemData::populateEpicGamesVirtual(EpicGamesStoreAPI* epicApi, const std::map<std::string, std::string>& existingNames)
{
    if (getName() != "epicgamestore") { /* ... */ return; }
    LOG(LogInfo) << "Populating virtual games for Epic Games Store system [" << getName() << "]";
    if (!epicApi || !epicApi->getAuth() || epicApi->getAuth()->getAccessToken().empty()) { /* ... */ return; }
    FolderData* root = getRootFolder();
    if (!root) { /* ... */ return; }

    LOG(LogInfo) << "Fetching full Epic Games library from API...";
    std::vector<EpicGames::Asset> libraryAssets;
    try {
        libraryAssets = epicApi->GetAllAssets();
    } catch (const std::exception& e) { LOG(LogError) << "Exception fetching Epic assets: " << e.what(); return; }
      catch(...) { LOG(LogError) << "Unknown exception fetching Epic assets."; return; }

    LOG(LogInfo) << "API returned " << libraryAssets.size() << " assets from Epic library.";
    if (libraryAssets.empty()) { LOG(LogWarning) << "No assets returned from Epic API or library is empty."; return; }

    int virtualGamesAdded = 0;
    int skippedCount = 0;
    // const std::string VIRTUAL_EPIC_PREFIX = "epic://virtual/"; // RIMUOVI QUESTA DEFINIZIONE LOCALE

    for (const auto& asset : libraryAssets)
    {
        std::string uniqueId = asset.appName; // Assumendo che asset.appName sia l'ID univoco per il path
        if (uniqueId.empty()) { LOG(LogWarning) << "Asset from Epic library has empty appName, skipping."; continue; }

        // Usa il membro statico della classe: SystemData::VIRTUAL_EPIC_PREFIX
        std::string pseudoPath = SystemData::VIRTUAL_EPIC_PREFIX; 
        if (!asset.ns.empty() && !asset.catalogItemId.empty() && !asset.appName.empty()) { // Preferisci il path più completo
             pseudoPath += HttpReq::urlEncode(asset.ns) + "/" + HttpReq::urlEncode(asset.catalogItemId) + "/" + HttpReq::urlEncode(asset.appName);
        } else if (!uniqueId.empty()) { // Fallback sull'appName se ns o catalogItemId mancano
             pseudoPath += HttpReq::urlEncode(uniqueId);
             LOG(LogWarning) << "Generating simplified pseudoPath for asset " << uniqueId << " due to missing ns/catalogItemId for path construction.";
        } else {
             LOG(LogWarning) << "Cannot generate pseudoPath for an Epic asset, skipping.";
             continue; 
        }

        if (existingNames.find(pseudoPath) == existingNames.end()) {
            LOG(LogDebug) << "Virtual game path not found in existing gamelist, adding: " << pseudoPath;
            FileData* virtualGame = new FileData(FileType::GAME, pseudoPath, this);

            MetaDataList& mdl = virtualGame->getMetadata();
               // --- CORREZIONE PER IL NOME ---
            if (!asset.labelName.empty()) {
                mdl.set(MetaDataId::Name, asset.labelName);
            } else if (!asset.appName.empty()) { // Fallback a appName se labelName è vuoto
                mdl.set(MetaDataId::Name, asset.appName);
            } else {
                LOG(LogWarning) << "Epic asset has empty labelName and appName. Cannot set name for: " << pseudoPath;
                // Considera di non aggiungere il gioco o di usare un placeholder se entrambi sono vuoti.
                // Per ora, procede senza nome se entrambi sono vuoti, ma è un caso limite.
            }
            // --- FINE CORREZIONE NOME ---
            
            mdl.set(MetaDataId::Installed, "false");
            mdl.set(MetaDataId::Virtual, "true");
            mdl.set(MetaDataId::EpicId, asset.appName); // Usato per il comando di lancio
            mdl.set(MetaDataId::EpicNamespace, asset.ns);
            mdl.set(MetaDataId::EpicCatalogId, asset.catalogItemId);
			mdl.set(MetaDataId::EpicId, asset.appName); 
			
			// Imposta un nome temporaneo o lascia vuoto se labelName/appName sono problematici
            std::string initialName = "[In attesa di metadati...]";
            if (!asset.labelName.empty() && asset.labelName != "Live-Windows") { // Evita "Live-Windows" se possibile
                initialName = asset.labelName;
            } else if (!asset.appName.empty() && asset.appName != "Live-Windows") {
                initialName = asset.appName;
            } else if (!asset.catalogItemId.empty()) { // Fallback a catalogItemId se i nomi sono pessimi
                initialName = asset.catalogItemId;
            }
            mdl.set(MetaDataId::Name, initialName);
            // Se hai aggiunto MetaDataId::EpicAppName e vuoi specificamente memorizzare asset.appName lì:
            // mdl.set(MetaDataId::EpicAppName, asset.appName);


            std::string installCommandUri = "com.epicgames.launcher://apps/";
            if (!asset.ns.empty() && !asset.catalogItemId.empty() && !asset.appName.empty()) {
                installCommandUri += HttpReq::urlEncode(asset.ns) + "%3A";
                installCommandUri += HttpReq::urlEncode(asset.catalogItemId) + "%3A";
                installCommandUri += HttpReq::urlEncode(asset.appName);
            } else { 
                installCommandUri += HttpReq::urlEncode(asset.appName); // Fallback se mancano dettagli
            }
            installCommandUri += "?action=launch&silent=true"; // Cambiato in launch, install era un esempio
            mdl.set(MetaDataId::LaunchCommand, installCommandUri);

            // Imposta lingua di sistema per il nuovo gioco virtuale
            std::string esSystemLanguage = Settings::getInstance()->getString("Language");
            if (esSystemLanguage.empty()) esSystemLanguage = SystemConf::getInstance()->get("system.language");
            if (!esSystemLanguage.empty()) mdl.set(MetaDataId::Language, esSystemLanguage);

            root->addChild(virtualGame);
            if (mFilterIndex != nullptr) {
                mFilterIndex->addToIndex(virtualGame);
            }
            virtualGamesAdded++;
        } else {
            LOG(LogDebug) << "Virtual game path already exists in gamelist (existingNames map), skipping add: " << pseudoPath;
            skippedCount++;
        }
    }
    LOG(LogInfo) << "Added " << virtualGamesAdded << " NEW virtual Epic game entries to system " << getName();
    LOG(LogInfo) << "Skipped adding " << skippedCount << " virtual games found in existingNames map (from gamelist.xml).";
    updateDisplayedGameCount(); // Aggiorna il conteggio dopo aver aggiunto i giochi
}

void SystemData::setIsGameSystemStatus()
{
	// we exclude non-game systems from specific operations
	// if/when there are more in the future, maybe this can be a more complex method, with a proper list
	// but for now a simple string comparison is more performant
	mIsGameSystem = (mMetadata.name != "retropie" && mMetadata.name != "lumaca");
}

void SystemData::populateFolder(FolderData* folder, std::unordered_map<std::string, FileData*>& fileMap)
{
	const std::string& folderPath = folder->getPath();

	if(!Utils::FileSystem::isDirectory(folderPath))
		return;
	/*
	// [Obsolete] make sure that this isn't a symlink to a thing we already have
	// Deactivated because it's slow & useless : users should to be carefull not to make recursive simlinks
	if (Utils::FileSystem::isSymlink(folderPath))
	{
		//if this symlink resolves to somewhere that's at the beginning of our path, it's gonna recurse
		if(folderPath.find(Utils::FileSystem::getCanonicalPath(folderPath)) == 0)
		{
			LOG(LogWarning) << "Skipping infinitely recursive symlink \"" << folderPath << "\"";
			return;
		}
	}
	*/
	std::string filePath;
	std::string extension;
	bool isGame;
	bool showHidden = Settings::ShowHiddenFiles();
	bool preloadMedias = Settings::PreloadMedias();

	auto shv = Settings::getInstance()->getString(getName() + ".ShowHiddenFiles");
	if (shv == "1") showHidden = true;
	else if (shv == "0") showHidden = false;

	Utils::FileSystem::fileList dirContent = Utils::FileSystem::getDirectoryFiles(folderPath);
	for (auto fileInfo : dirContent)
	{
		filePath = fileInfo.path;

		// skip hidden files and folders
		if(!showHidden && fileInfo.hidden)
			continue;

		//this is a little complicated because we allow a list of extensions to be defined (delimited with a space)
		//we first get the extension of the file itself:
		extension = Utils::String::toLower(Utils::FileSystem::getExtension(filePath));

		//fyi, folders *can* also match the extension and be added as games - this is mostly just to support higan
		//see issue #75: https://github.com/Aloshi/EmulationStation/issues/75

		isGame = false;
		if(mEnvData->isValidExtension(extension))
		{
			FileData* newGame = new FileData(GAME, filePath, this);

			// preventing new arcade assets to be added
			if(!newGame->isArcadeAsset())
			{
				folder->addChild(newGame);
				fileMap[filePath] = newGame;
				isGame = true;
			}
		}

		//add directories that also do not match an extension as folders
		if(!isGame && fileInfo.directory)
		{
			std::string fn = Utils::String::toLower(Utils::FileSystem::getFileName(filePath));

			// Never look in "artwork", reserved for mame roms artwork
			if (fn == "artwork")
				continue;

			if (preloadMedias && (!mHidden || Settings::HiddenSystemsShowGames()))
			{
				// Recurse list files in medias folder, just to let OS build filesystem cache 
				if (fn == "media" || fn == "medias")
				{
					Utils::FileSystem::getDirContent(filePath, true);
					continue;
				}

				// List files in folder, just to get OS build filesystem cache 
				if (fn == "manuals" || fn == "images" || fn == "videos" || Utils::String::startsWith(fn, "downloaded_"))
				{
					Utils::FileSystem::getDirectoryFiles(filePath);
					continue;
				}
			}

			// Don't loose time looking in downloaded_images, downloaded_videos & media folders
			if (fn == "media" || fn == "medias" || fn == "images" || fn == "manuals" || fn == "videos" || fn == "assets" || Utils::String::startsWith(fn, "downloaded_") || Utils::String::startsWith(fn, "."))
				continue;
			
			// Hardcoded optimisation : WiiU has so many files in content & meta directories
			if (mMetadata.name == "wiiu" && (fn == "content" || fn == "meta"))
				continue;

			// Hardcoded optimisation : vpinball 'roms' subfolder must be excluded
			if (mMetadata.name == "vpinball" && fn == "roms")
				continue;			

			FolderData* newFolder = new FolderData(filePath, this);
			populateFolder(newFolder, fileMap);

			//ignore folders that do not contain games
			if(newFolder->getChildren().size() == 0)
				delete newFolder;
			else 
			{
				const std::string& key = newFolder->getPath();
				if (fileMap.find(key) == fileMap.end())
				{
					folder->addChild(newFolder);
					fileMap[key] = newFolder;
				}
			}
		}
	}
}

FileFilterIndex* SystemData::getIndex(bool createIndex)
{
	if (mFilterIndex == nullptr && createIndex)
	{
		mFilterIndex = new FileFilterIndex();
		indexAllGameFilters(mRootFolder);
		mFilterIndex->setUIModeFilters();
	}

	return mFilterIndex;
}

void SystemData::deleteIndex()
{
	if (mFilterIndex != nullptr)
	{
		delete mFilterIndex;
		mFilterIndex = nullptr;
	}
}

void SystemData::indexAllGameFilters(const FolderData* folder)
{
	const std::vector<FileData*>& children = folder->getChildren();

	for(auto it = children.cbegin(); it != children.cend(); ++it)
	{
		switch((*it)->getType())
		{
			case GAME:   { mFilterIndex->addToIndex(*it); } break;
			case FOLDER: { indexAllGameFilters((FolderData*)*it); } break;
		}
	}
}

void SystemData::createGroupedSystems()
{
	auto hiddenSystems = Utils::String::split(Settings::HiddenSystems(), ';');

	std::map<std::string, std::vector<SystemData*>> map;

	for (auto sys : sSystemVector)
	{
		if (sys->isCollection() || sys->getSystemEnvData()->mGroup.empty())
			continue;
		
		if (Settings::getInstance()->getBool(sys->getSystemEnvData()->mGroup + ".ungroup") || Settings::getInstance()->getBool(sys->getName() + ".ungroup"))
			continue;

		if (sys->getName() == sys->getSystemEnvData()->mGroup)
		{
			sys->getSystemEnvData()->mGroup = "";
			continue;
		}		
		else if (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), sys->getName()) != hiddenSystems.cend())
			continue;
		
		map[sys->getSystemEnvData()->mGroup].push_back(sys);		
	}

	for (auto item : map)
	{	
		// Don't group if system count is only 1 		
		if (item.second.size() == 1 && Settings::getInstance()->HideUniqueGroups())
		{
			item.second[0]->getSystemEnvData()->mGroup = "";
			continue;
		}
		
		SystemData* system = nullptr;
		bool existingSystem = false;

		for (auto sys : sSystemVector)
		{
			if (sys->getName() == item.first)
			{
				existingSystem = true;
				system = sys;
				system->mIsGroupSystem = true;
				break;
			}
		}

		if (system == nullptr)
		{
			SystemEnvironmentData* envData = new SystemEnvironmentData;
			envData->mStartPath = "";
			envData->mLaunchCommand = "";

			SystemMetadata md;
			md.name = item.first;
			md.fullName = item.first;
			md.themeFolder = item.first;

			// Check if the system is described in es_systems but empty, to import metadata )
			auto sourceSystem = SystemData::loadSystem(item.first, false);
			if (sourceSystem != nullptr)
			{
				md.fullName = sourceSystem->getSystemMetadata().fullName;
				md.themeFolder = sourceSystem->getSystemMetadata().themeFolder;
				md.manufacturer = sourceSystem->getSystemMetadata().manufacturer;
				md.releaseYear = sourceSystem->getSystemMetadata().releaseYear;
				md.hardwareType = sourceSystem->getSystemMetadata().hardwareType;

				delete sourceSystem;
			}
			else if (item.second.size() > 0)
			{
				SystemData* syss = *item.second.cbegin();
				md.manufacturer = syss->getSystemMetadata().manufacturer;
				md.releaseYear = syss->getSystemMetadata().releaseYear;
				md.hardwareType = "system";
			}

			system = new SystemData(md, envData, nullptr, false, true);
			system->mIsGroupSystem = true;
			system->mIsGameSystem = false;
		}

		if (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), system->getName()) != hiddenSystems.cend())
		{
			system->mHidden = true;

			if (!existingSystem)
				sSystemVector.push_back(system);
						
			for (auto childSystem : item.second)
				childSystem->getSystemEnvData()->mGroup = "";

			continue;
		}

		FolderData* root = system->getRootFolder();

		for (auto childSystem : item.second)
		{
			auto children = childSystem->getRootFolder()->getChildren();
			if (children.size() > 0)
			{
				auto folder = new FolderData(childSystem->getRootFolder()->getPath(), childSystem, false);
				folder->setMetadata(childSystem->getRootFolder()->getMetadata());
				
				if (folder->getMetadata(MetaDataId::Desc).empty())
				{
					char trstring[1024];

					std::string games_list;

					int games_counter = 0;
					auto games = childSystem->getRootFolder()->getFilesRecursive(GAME, true);
					for (auto game : games)
					{
						games_counter++;
						if (games_counter == 3)
							break;

						games_list += "\n";
						games_list += "- " + game->getName();
					}

					games_counter = childSystem->getGameCountInfo()->totalGames;

					snprintf(trstring, 1024, ngettext(
						"This collection contains %i game:%s",
						"This collection contains %i games, including:%s", games_counter), games_counter, games_list.c_str());

					folder->setMetadata(MetaDataId::Desc, std::string(trstring));
				}

				root->addChild(folder);

				if (folder->getMetadata(MetaDataId::Image).empty())
				{
					auto theme = childSystem->getTheme();
					if (theme)
					{
						const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
						if (logoElem && logoElem->has("path"))
						{
							std::string path = logoElem->get<std::string>("path");
							folder->setMetadata(MetaDataId::Image, path);
							folder->setMetadata(MetaDataId::Thumbnail, path);
							folder->enableVirtualFolderDisplay(true);
						}
					}
				}

				for (auto child : children)
					folder->addChild(child, false);

				folder->getMetadata().resetChangedFlag();
			}
		}

		if (root->getChildren().size() > 0 && !existingSystem)
		{
			system->loadTheme();

			auto defaultView = Settings::getInstance()->getString(system->getName() + ".defaultView");
			auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(system->getName() + ".gridSize"));
			system->setSystemViewMode(defaultView, gridSizeOverride, false);

			sSystemVector.push_back(system);
		}
		
		root->getMetadata().resetChangedFlag();
	}
}

bool SystemData::loadFeatures()
{
	if (mEmulators.size() == 0)
		return false;

	if (mIsCollectionSystem || hasPlatformId(PlatformIds::IMAGEVIEWER) || hasPlatformId(PlatformIds::PLATFORM_IGNORE))
		return false;

	std::string systemName = this->getName();

	for (auto& emul : mEmulators)
	{
		emul.features = EmulatorFeatures::Features::none;
		emul.customFeatures.clear();

		for (auto& core : emul.cores)
		{
			core.netplay = false;
			core.features = EmulatorFeatures::Features::none;
			core.customFeatures.clear();
		}
	}

	if (!CustomFeatures::FeaturesLoaded)
		return false;

	for (auto& emul : mEmulators)
	{
		std::string name = emul.name;

		if (Utils::String::startsWith(emul.name, "lr-"))
			name = "libretro";

		auto it = CustomFeatures::EmulatorFeatures.find(name);
		if (it != CustomFeatures::EmulatorFeatures.cend())
		{
			emul.features = it->second.features;
			emul.customFeatures = it->second.customFeatures;			

			for (auto essystem : it->second.systemFeatures)
			{
				if (essystem.name != systemName)
					continue;

				emul.features = emul.features | essystem.features;
				for (auto feat : essystem.customFeatures)
					emul.customFeatures.push_back(feat);
			}

			for (auto& core : emul.cores)
			{
				for (auto escore : it->second.cores)
				{
					if (core.name != escore.name)
						continue;
					
					core.features = core.features | escore.features;

					for (auto feat : escore.customFeatures)
						core.customFeatures.push_back(feat);

					for (auto essystem : escore.systemFeatures)
					{
						if (essystem.name != systemName)
							continue;

						core.features = core.features | essystem.features;
						for (auto feat : essystem.customFeatures)
							core.customFeatures.push_back(feat);
					}

					core.netplay = (core.features & EmulatorFeatures::Features::netplay) == EmulatorFeatures::Features::netplay;
				}
			}
		}
	}

	return true;
}

bool SystemData::isCurrentFeatureSupported(EmulatorFeatures::Features feature)
{
	return isFeatureSupported(getEmulator(), getCore(), feature);
}

bool SystemData::hasFeatures()
{
	if (isCollection() || hasPlatformId(PlatformIds::PLATFORM_IGNORE))
		return false;

	for (auto emulator : mEmulators)
	{
		for (auto& core : emulator.cores)
			if (core.features != EmulatorFeatures::Features::none || core.customFeatures.size() > 0)
				return true;

		if (emulator.features != EmulatorFeatures::Features::none || emulator.customFeatures.size() > 0)
			return true;
	}

	return !CustomFeatures::FeaturesLoaded;
}

CustomFeatures SystemData::getCustomFeatures(std::string emulatorName, std::string coreName)
{
	CustomFeatures ret;

	if (emulatorName.empty() || emulatorName == "auto")
		emulatorName = getEmulator();

	if (coreName.empty() || coreName == "auto")
		coreName = getCore();

	for (auto emulator : mEmulators)
	{
		if (emulator.name == emulatorName)
		{
			for (auto ft : emulator.customFeatures)
				ret.push_back(ft);

			for (auto& core : emulator.cores)
				if (coreName == core.name)
					for (auto ft : core.customFeatures)
						ret.push_back(ft);

			break;
		}
	}

	ret.sort();
	return ret;
}

bool SystemData::isFeatureSupported(std::string emulatorName, std::string coreName, EmulatorFeatures::Features feature)
{
	if (emulatorName.empty() || emulatorName == "auto")
		emulatorName = getEmulator();

	if (coreName.empty() || coreName == "auto")
		coreName = getCore();

	for (auto emulator : mEmulators)
	{
		if (emulator.name == emulatorName)
		{
			for (auto& core : emulator.cores)
				if (coreName == core.name)
					if ((core.features & feature) == feature)
						return true;
			
			return (emulator.features & feature) == feature;
		}
	}

	return !CustomFeatures::FeaturesLoaded;
}

// Load custom additionnal config from es_systems_*.cfg files
void SystemData::loadAdditionnalConfig(pugi::xml_node& srcSystems)
{	
	std::vector<std::string> rootPaths = { Paths::getUserEmulationStationPath(), Paths::getEmulationStationPath() };
	for (auto rootPath : VectorHelper::distinct(rootPaths, [](auto x) { return x; }))
	{
		for (auto customPath : Utils::FileSystem::getDirContent(rootPath, false, false))
		{
			if (Utils::FileSystem::getExtension(customPath) != ".cfg")
				continue;

			if (!Utils::String::startsWith(Utils::FileSystem::getFileName(customPath), "es_systems_"))
				continue;

			pugi::xml_document doc;
			pugi::xml_parse_result res = doc.load_file(WINSTRINGW(customPath).c_str());
			if (!res)
			{
				LOG(LogError) << "Could not parse " << Utils::FileSystem::getFileName(customPath) << " file!";
				return;
			}

			pugi::xml_node systemList = doc.child("systemList");
			if (!systemList)
			{
				LOG(LogError) << Utils::FileSystem::getFileName(customPath) << " is missing the <systemList> tag !";
				return;
			}

			for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
			{
				if (!system.child("name"))
					continue;

				std::string name = system.child("name").text().get();
				if (name.empty())
					continue;

				bool found = false;

				// Remove existing one
				for (pugi::xml_node& srcSystem : srcSystems.children())
				{
					if (std::string(srcSystem.name()) != "system")
						continue;

					std::string srcName = srcSystem.child("name").text().get();
					if (srcName != name)
						continue;

					found = true;

					for (pugi::xml_node& child : system.children())
					{
						std::string tag = child.name();
						if (tag == "name")
							continue;

						srcSystem.remove_child(tag.c_str());

						if (tag == "emulators" || !std::string(child.text().get()).empty())
							srcSystem.append_copy(child);
					}

					break;
				}

				if (!found)
					srcSystems.append_copy(system);
			}
		}
	}
}

//creates systems from information located in a config file
 bool SystemData::loadConfig(Window* window) {
  deleteSystems();
  ThemeData::setDefaultTheme(nullptr);
  UIModeController::getInstance();  // Init UIModeController before loading systems
 
  std::string path = getConfigPath();
 
  LOG(LogInfo) << "Loading system config file " << path << "...";
 
  if (!Utils::FileSystem::exists(path)) {
  LOG(LogError) << "es_systems.cfg file does not exist!";
  return false;
  }
 
  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_file(WINSTRINGW(path).c_str());
 
  if (!res) {
  LOG(LogError) << "Could not parse es_systems.cfg file!";
  LOG(LogError) << res.description();
  return false;
  }
 
  // actually read the file
  pugi::xml_node systemList = doc.child("systemList");
  if (!systemList) {
  LOG(LogError) << "es_systems.cfg is missing the <systemList> tag!";
  return false;
  }
 
  loadAdditionnalConfig(systemList);
 
  std::vector<std::string> systemsNames;
 
  int systemCount = 0;
  for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system")) {
  systemsNames.push_back(system.child("fullname").text().get());
  systemCount++;
  }
 
  if (systemCount == 0) {
  LOG(LogError) << "no system found in es_systems.cfg";
  return false;
  }
 
  Utils::FileSystem::FileSystemCacheActivator fsc;
 
  CustomFeatures::loadEsFeaturesFile();
 
  int currentSystem = 0;
 
  typedef SystemData* SystemDataPtr;
 
  ThreadPool* pThreadPool = NULL;
  SystemDataPtr* systems = NULL;
 
  // Allow threaded loading only if processor threads > 1 so it does not apply on machines like Pi0.
  if (std::thread::hardware_concurrency() > 1 && Settings::ThreadedLoading()) {
  pThreadPool = new ThreadPool();
 
  systems = new SystemDataPtr[systemCount];
  for (int i = 0; i < systemCount; i++)
  systems[i] = nullptr;
 
  pThreadPool->queueWorkItem([] { CollectionSystemManager::get()->loadCollectionSystems(); });
  }
 
  int processedSystem = 0;
 
  for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system")) {
  if (pThreadPool != NULL) {
  pThreadPool->queueWorkItem([system, currentSystem, systems, &processedSystem] {
  systems[currentSystem] = loadSystem(system);
  processedSystem++;
  });
  } else {
  std::string fullname = system.child("fullname").text().get();
 
  if (window != NULL)
 window->renderSplashScreen(fullname, systemCount == 0 ? 0 : (float)currentSystem / (float)(systemCount + 2));
 
  std::string nm = system.child("name").text().get();
 
  SystemData* pSystem = loadSystem(system);
  if (pSystem != nullptr)
  sSystemVector.push_back(pSystem);
  if (pSystem->getName() == "epicgamestore") {
            window->setCurrentSystem(pSystem);  // Set the current system
            LOG(LogDebug) << "Set current system to epicgamestore";
        }
  }
 
  currentSystem++;
  }
 
  if (pThreadPool != NULL) {
  if (window != NULL) {
  pThreadPool->wait([window, &processedSystem, systemCount, &systemsNames] {
  int px = processedSystem - 1;
  if (px >= 0 && px < systemsNames.size())
  window->renderSplashScreen(systemsNames.at(px), (float)px / (float)(systemCount + 2));
  }, 50);
  } else
  pThreadPool->wait();
 
  for (int i = 0; i < systemCount; i++) {
  SystemData* pSystem = systems[i];
  if (pSystem != nullptr)
  sSystemVector.push_back(pSystem);
  }
 
  delete[] systems;
  delete pThreadPool;
 
  if (window != NULL)
  window->renderSplashScreen(_("Collections"), systemCount == 0 ? 0 : currentSystem / (float)(systemCount + 1));
  } else {
  if (window != NULL)
  window->renderSplashScreen(_("Collections"), systemCount == 0 ? 0 : currentSystem / (float)(systemCount + 1));
 
  CollectionSystemManager::get()->loadCollectionSystems();
  }
  
// --- AMAZON GAMES SYSTEM (LOGICA FINALE BASATA SUL NOME DEL GIOCO) ---
LOG(LogInfo) << "[AmazonDynamic] Checking/Creating Amazon Games system...";

if (Settings::getInstance()->getBool("EnableAmazonGames"))
{
    SystemData* amazonSystem = SystemData::getSystem("amazon");
    bool amazonSystemWasNewlyCreated = false;

    if (amazonSystem == nullptr) {
        LOG(LogInfo) << "[AmazonDynamic] Amazon system not found, creating dynamically...";
        // La tua logica di creazione è corretta e rimane invariata
        SystemMetadata md_amazon;
        md_amazon.name = "amazon";
        md_amazon.fullName = _("Amazon Games");
        md_amazon.themeFolder = "amazon";
        md_amazon.manufacturer = "Amazon";
        md_amazon.hardwareType = "pc";
        SystemEnvironmentData* envData_amazon = new SystemEnvironmentData();
        std::string exeDir = Utils::FileSystem::getParent(Paths::getExePath());
        std::string amazonGamelistDir = Utils::FileSystem::getGenericPath(exeDir + "/roms/amazon");
        Utils::FileSystem::createDirectory(amazonGamelistDir);
        envData_amazon->mStartPath = amazonGamelistDir;
        envData_amazon->mPlatformIds = {PlatformIds::PC};
        envData_amazon->mLaunchCommand = "";
        std::vector<EmulatorData> amazonEmulators_empty;
        amazonSystem = new SystemData(md_amazon, envData_amazon, &amazonEmulators_empty, false, false, true, true);
        if (amazonSystem) {
            amazonSystemWasNewlyCreated = true;
            sSystemVector.push_back(amazonSystem);
        } else {
            delete envData_amazon;
        }
    } else {
        // STRATEGIA CHIAVE: Pulisci sempre la lista per evitare corruzioni e duplicati.
        LOG(LogInfo) << "[AmazonDynamic] Amazon system already exists. Clearing game list before repopulation.";
        amazonSystem->getRootFolder()->clear();
    }

    if (amazonSystem != nullptr) {
        FolderData* root = amazonSystem->getRootFolder();

        // 1. CARICA LA CACHE DA gamelist.xml
        std::string gamelistPath = amazonSystem->getGamelistPath(false);
        if (Utils::FileSystem::exists(gamelistPath)) {
            std::unordered_map<std::string, FileData*> fileMap;
            parseGamelist(amazonSystem, fileMap);
            LOG(LogInfo) << "[AmazonDynamic] Popolato con " << root->getChildren().size() << " giochi da gamelist.xml.";
        }

        // 2. SCANSIONA I GIOCHI INSTALLATI LOCALMENTE
        std::map<std::string, Amazon::InstalledGameInfo> installedGameMap; // Chiave: Nome del gioco (lowercase)
        GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);
        if (gsm) {
            AmazonGamesStore* amazonStore = nullptr;
            GameStore* baseStore = gsm->getStore("amazon");
            if (baseStore) amazonStore = dynamic_cast<AmazonGamesStore*>(baseStore);
            if (amazonStore) {
                auto installedGames = amazonStore->getScanner()->findInstalledGames();
                for(const auto& game : installedGames) {
                    // Usa il nome del gioco in minuscolo come chiave per la mappa
                    installedGameMap[Utils::String::toLower(game.title)] = game;
                }
            }
        }
        LOG(LogInfo) << "[AmazonDynamic] Scansione locale ha trovato " << installedGameMap.size() << " giochi installati.";

        // 3. RICONCILIAZIONE BASATA SUL NOME
        LOG(LogInfo) << "[AmazonDynamic] Riconciliazione dello stato dei giochi per nome...";
        
        // A. Aggiorna i giochi esistenti nel gamelist
        for (auto& game : root->getChildren()) {
            std::string gameNameLower = Utils::String::toLower(game->getName());
            auto it = installedGameMap.find(gameNameLower);

            if (it != installedGameMap.end()) {
                // CORRISPONDENZA TROVATA: Il gioco è installato
                const auto& installedInfo = it->second;
                MetaDataList& mdl = game->getMetadata();
                mdl.set(MetaDataId::Installed, "true");
                mdl.set(MetaDataId::Virtual, "false");
                // Aggiorniamo il path e l'ID per coerenza, usando l'ID trovato dallo scanner
                game->setPath("amazon_installed:/" + installedInfo.id);
                mdl.set("storeId", installedInfo.id); 
                mdl.set(MetaDataId::LaunchCommand, "amazon-games://play/" + installedInfo.id);

                // Rimuovi il gioco dalla mappa per non ri-aggiungerlo dopo
                installedGameMap.erase(it);
            } else {
                // NESSUNA CORRISPONDENZA: Il gioco è solo virtuale
                MetaDataList& mdl = game->getMetadata();
                mdl.set(MetaDataId::Installed, "false");
                mdl.set(MetaDataId::Virtual, "true");
                // Assicuriamoci che il path sia corretto per un gioco virtuale
                std::string storeId = mdl.get("storeId");
                if (!storeId.empty()) {
                    game->setPath("amazon_virtual:/" + storeId);

                }
            }
        }
        
        // B. Aggiungi i giochi installati che non erano presenti nel gamelist
        for (const auto& pair : installedGameMap) {
            const Amazon::InstalledGameInfo& installedInfo = pair.second;
            LOG(LogInfo) << "[AmazonDynamic] Trovato nuovo gioco installato non nel gamelist: '" << installedInfo.title << "'. Aggiungo.";
            FileData* fd = new FileData(FileType::GAME, "amazon_installed:/" + installedInfo.id, amazonSystem);
            MetaDataList& mdl = fd->getMetadata();
            mdl.set("storeId", installedInfo.id);
            mdl.set(MetaDataId::Name, installedInfo.title);
            mdl.set(MetaDataId::Installed, "true");
            mdl.set(MetaDataId::Virtual, "false");
            mdl.set(MetaDataId::LaunchCommand, "amazon-games://play/" + installedInfo.id);
            root->addChild(fd, false);
        }
        
        LOG(LogInfo) << "[AmazonDynamic] Riconciliazione completata. Conteggio finale: " << root->getChildren().size();

        if (amazonSystemWasNewlyCreated) {
            amazonSystem->loadTheme();
        }
        amazonSystem->updateDisplayedGameCount();
    }
}
// --- FINE BLOCCO AMAZON ---

// --- GOG GAMES SYSTEM (LOGICA FINALE BASATA SUL MODELLO FUNZIONANTE DI EPIC/STEAM) ---
LOG(LogInfo) << "[GogDynamic] Checking/Creating GOG.com system...";

if (Settings::getInstance()->getBool("EnableGogStore"))
{
    SystemData* gogSystem = SystemData::getSystem("gog");
    bool gogSystemWasNewlyCreated = false;

    if (gogSystem == nullptr) {
        LOG(LogInfo) << "[GogDynamic] GOG system not found, creating dynamically...";
        // La tua logica di creazione è corretta e rimane invariata
        SystemMetadata md_gog;
        md_gog.name = "gog";
        md_gog.fullName = "GOG.com";
        md_gog.themeFolder = "gog";
        md_gog.manufacturer = "CD Projekt";
        md_gog.hardwareType = "pc";
        SystemEnvironmentData* envData_gog = new SystemEnvironmentData();
        std::string romPath = Utils::FileSystem::getParent(Paths::getExePath()) + "/roms/gog";
        Utils::FileSystem::createDirectory(romPath);
        envData_gog->mStartPath = romPath;
        std::vector<EmulatorData> gogEmulators_empty;
        gogSystem = new SystemData(md_gog, envData_gog, &gogEmulators_empty, false, false, true, true);

        if (gogSystem) {
            gogSystemWasNewlyCreated = true;
            sSystemVector.push_back(gogSystem);
        } else {
            delete envData_gog;
        }
    } else {
        // STRATEGIA CHIAVE COPIATA DA EPIC/STEAM: Se il sistema esiste già, puliscilo completamente.
        // Questo è il passo più importante per prevenire qualsiasi tipo di corruzione o duplicato.
        LOG(LogInfo) << "[GogDynamic] GOG system already exists. Clearing game list before repopulation.";
        gogSystem->getRootFolder()->clear();
    }

    if (gogSystem != nullptr) {
        FolderData* root = gogSystem->getRootFolder();

        // 1. CARICA LA CACHE DA gamelist.xml (la nostra fonte di verità per i giochi virtuali)
        std::string gamelistPath = gogSystem->getGamelistPath(false);
        if (Utils::FileSystem::exists(gamelistPath)) {
            // Poiché la lista è stata pulita (se il sistema esisteva), partiamo sempre da una base pulita.
            std::unordered_map<std::string, FileData*> fileMap;
            parseGamelist(gogSystem, fileMap);
            LOG(LogInfo) << "[GogDynamic] Popolato con " << root->getChildren().size() << " giochi da gamelist.xml.";
        }

        // 2. SCANSIONA I GIOCHI INSTALLATI LOCALMENTE
        std::map<std::string, GOG::InstalledGameInfo> installedGameMap;
        GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);
        if (gsm) {
            GogGamesStore* gogStore = nullptr;
            GameStore* baseStore = gsm->getStore("gog");
            if (baseStore) gogStore = dynamic_cast<GogGamesStore*>(baseStore);
            if (gogStore) {
                auto installedGames = gogStore->getScanner()->findInstalledGames();
                for(const auto& game : installedGames) {
                    installedGameMap[game.id] = game;
                }
            }
        }
        LOG(LogInfo) << "[GogDynamic] Scansione locale ha trovato " << installedGameMap.size() << " giochi installati.";

        // 3. RICONCILIAZIONE (Il cuore della logica)
        LOG(LogInfo) << "[GogDynamic] Riconciliazione dello stato dei giochi...";
        
        // A. Itera sui giochi INSTALLATI e aggiorna o crea le voci.
        for (const auto& pair : installedGameMap) {
            const std::string& storeId = pair.first;
            const GOG::InstalledGameInfo& installedInfo = pair.second;
            
            FileData* existingGame = nullptr;
            for(auto child : root->getChildren()) {
                // CORREZIONE DEFINITIVA per il bug del log: estrai l'ID dal path se manca nei metadati.
                std::string childStoreId = child->getMetadata().get("storeId");
                if (childStoreId.empty()) {
                    std::string path = child->getPath();
                    size_t pos = path.find_last_of('/');
                    if (pos != std::string::npos) {
                        childStoreId = path.substr(pos + 1);
                        child->getMetadata().set("storeId", childStoreId); // Salva l'ID corretto per il futuro
                    }
                }

                if(childStoreId == storeId) {
                    existingGame = child;
                    break;
                }
            }

            if (existingGame) {
                // Il gioco esiste (probabilmente caricato come virtuale dal gamelist), aggiornalo.
                MetaDataList& mdl = existingGame->getMetadata();
                mdl.set(MetaDataId::Installed, "true");
                mdl.set(MetaDataId::Virtual, "false");
                mdl.set(MetaDataId::Name, installedInfo.name);
                mdl.set(MetaDataId::LaunchCommand, "goggalaxy://launch/" + storeId);
                       existingGame->setPath("gog_installed:/" + storeId);
            } else {
                // Gioco installato non presente in lista, crealo.
                LOG(LogInfo) << "[GogDynamic] Trovato nuovo gioco installato non nel gamelist: '" << installedInfo.name << "'. Aggiungo.";
                FileData* fd = new FileData(FileType::GAME, "gog_installed:/" + storeId, gogSystem);
                MetaDataList& mdl = fd->getMetadata();
                mdl.set("storeId", storeId);
                mdl.set(MetaDataId::Name, installedInfo.name);
                mdl.set(MetaDataId::Installed, "true");
                mdl.set(MetaDataId::LaunchCommand, "goggalaxy://launch/" + storeId);
                root->addChild(fd, false);
            }
        }
        
        // B. Itera su tutti i giochi in lista e assicurati che quelli NON installati siano marcati come virtuali.
        for (auto& game : root->getChildren()) {
            std::string storeId = game->getMetadata().get("storeId");
            if (storeId.empty()) continue; // Dovrebbe essere già stato corretto sopra, ma per sicurezza

            if (installedGameMap.find(storeId) == installedGameMap.end()) {
                // Questo gioco è nel gamelist ma non è risultato installato.
                MetaDataList& mdl = game->getMetadata();
                mdl.set(MetaDataId::Installed, "false");
                mdl.set(MetaDataId::Virtual, "true");
                game->setPath("gog_virtual:/" + storeId);
            }
        }
        
        LOG(LogInfo) << "[GogDynamic] Riconciliazione completata. Conteggio finale: " << root->getChildren().size();

        if (gogSystemWasNewlyCreated) {
            gogSystem->loadTheme();
        }
        gogSystem->updateDisplayedGameCount();
    }
}
// --- FINE BLOCCO GOG ---
  
// --- All'interno di SystemData::loadConfig() o funzione equivalente ---
// Questo è l'inizio della sezione di codice che gestisce il sistema 'epicgamestore'


LOG(LogInfo) << "[EpicDynamic] Checking/Creating Epic Games Store system in loadConfig...";

SystemData* epicSystem = getSystem("epicgamestore"); // Cerca se il sistema esiste già
bool systemWasNewlyCreated = false;

if (epicSystem == nullptr) {
    LOG(LogInfo) << "[EpicDynamic] Epic Games Store system not found, creating dynamically...";
    
    SystemMetadata md_epic;
    md_epic.name = "epicgamestore";
    md_epic.fullName = _("Epic Games Store"); // Usa la tua funzione di traduzione _()
    md_epic.themeFolder = "epicgamestore";
    md_epic.manufacturer = "Epic Games";
    md_epic.hardwareType = "pc";

    SystemEnvironmentData* envData_epic = new SystemEnvironmentData();
    // Usa la tua logica originale per determinare la cartella dei gamelist/roms
    std::string exePath = Paths::getExePath(); 
    std::string exeDir = Utils::FileSystem::getParent(exePath);
    std::string gamelistSystemFolder = Utils::FileSystem::getGenericPath(exeDir + "/roms/" + md_epic.name); 
    envData_epic->mStartPath = gamelistSystemFolder;
    
    envData_epic->mPlatformIds = {PlatformIds::PC}; // Assumi che PlatformIds::PC sia definito
    envData_epic->mLaunchCommand = ""; // Il comando di lancio è specifico per gioco

    std::vector<EmulatorData> epicEmulators; // Solitamente vuota per sistemi store

    // Modifica i flag isDynamic e isStore come appropriato per la tua implementazione
    // (es. isDynamic=true, isStore=true)
    epicSystem = new SystemData(md_epic, envData_epic, &epicEmulators, false, false, true, true); 

    if (!epicSystem) {
        delete envData_epic; 
        LOG(LogError) << "[EpicDynamic] FATAL: Failed to dynamically create 'epicgamestore' SystemData object!";
        // Potresti voler uscire o saltare il resto della logica per Epic qui
    } else {
        systemWasNewlyCreated = true;
        LOG(LogInfo) << "[EpicDynamic] Dynamically created 'epicgamestore' system object successfully.";
    }
} else {
    LOG(LogInfo) << "[EpicDynamic] Epic Games Store system already exists. Clearing game list for repopulation with local/gamelist data.";
    // Se il sistema esiste già e vuoi assicurarti che venga popolato solo con dati "freschi" da questo avvio (escludendo virtuali non in gamelist)
    // Potresti volerlo pulire solo se è un sistema che gestisci dinamicamente.
    // Aggiungi un flag o un metodo a SystemData come 'isStoreSystem()' o 'isDynamicallyPopulatedStore()'
    // if (epicSystem->isStoreSystem()) { // Esempio di condizione
        epicSystem->getRootFolder()->clear(); 
        FileFilterIndex* existingIndex = epicSystem->getIndex(false); 
        if (existingIndex != nullptr) {
            delete existingIndex;
            epicSystem->mFilterIndex = nullptr; 
        }
    // }
}

// Procedi con il popolamento di base (gamelist + installati) solo se epicSystem è valido
if (epicSystem != nullptr) {
    LOG(LogInfo) << "[EpicDynamic] Populating (gamelist & installed games) for epicgamestore system [" << epicSystem->getName() << "]";
    try {
        // 1. Leggi il gamelist.xml (se esiste)
        //    Questa operazione popolerà epicSystem con i giochi e i metadati salvati precedentemente.
        //    Se i nomi erano "Live-Windows" nel file, verranno caricati così.
        std::string gamelistPath = epicSystem->getGamelistPath(false); 
        if (Utils::FileSystem::exists(gamelistPath)) {
            LOG(LogInfo) << "[EpicDynamic] Parsing Epic gamelist: " << gamelistPath;
            // Usa la tua funzione di parsing del gamelist come nella tua versione originale:
            std::unordered_map<std::string, FileData*> fileMap; 
            if (epicSystem->getRootFolder() != nullptr) {
                 fileMap[epicSystem->getRootFolder()->getPath()] = epicSystem->getRootFolder();
            }
            parseGamelist(epicSystem, fileMap); // La tua chiamata a parseGamelist
            LOG(LogInfo) << "[EpicDynamic] Game count after gamelist parse: " << epicSystem->getGameCount();
        } else {
            LOG(LogWarning) << "[EpicDynamic] Epic gamelist file not found at: " << gamelistPath << ". No games loaded from XML.";
        }

        // 2. Sincronizza con giochi Epic INSTALLATI localmente
        //    Questa logica aggiungerà giochi installati se non già presenti (da gamelist con path identico)
        //    o potrebbe aggiornare lo stato 'installed' di voci esistenti.
        GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);
        EpicGamesStore* epicGamesStore = nullptr; 
        if (gsm) {
            GameStore* baseStore = gsm->getStore("EpicGamesStore"); 
            if (baseStore) epicGamesStore = dynamic_cast<EpicGamesStore*>(baseStore);
        }

        if (epicGamesStore) {
            LOG(LogDebug) << "[EpicDynamic] Processing *installed* Epic Games from local machine...";
            int installedProcessed = 0;
            std::vector<EpicGamesStore::EpicGameInfo> installedEpicGames = epicGamesStore->getInstalledEpicGamesWithDetails();
            LOG(LogInfo) << "[EpicDynamic] Found " << installedEpicGames.size() << " installed Epic Games manifests.";
            FolderData* root = epicSystem->getRootFolder();
            if (root) {
                for (const auto& installedGame : installedEpicGames) {
                    std::string installedPath = Utils::FileSystem::getPreferredPath(installedGame.installDir);
                    if (installedPath.empty()) continue;

                    FileData* fd = root->FindByPath(installedPath); // Cerca se esiste già un FileData con questo path assoluto
                    if (!fd) { // Se non esiste, crealo
                        LOG(LogDebug) << "[EpicDynamic] Installed Epic game path not found, creating new FileData entry: " << installedPath;
                        fd = new FileData(FileType::GAME, installedPath, epicSystem);
                        root->addChild(fd);
                        // Non aggiungere a existingGamePathsFromGamelist qui, quella mappa era per populateEpicGamesVirtual
                    } else {
                        LOG(LogDebug) << "[EpicDynamic] Found existing FileData for installed Epic game path: " << installedPath;
                    }

                    if (fd) {
                        MetaDataList& mdl = fd->getMetadata();
                        mdl.set(MetaDataId::Name, installedGame.name); // Nome dall'info del gioco installato
                        mdl.set(MetaDataId::Installed, "true");
                        mdl.set(MetaDataId::Virtual, "false"); 
                        mdl.set(MetaDataId::LaunchCommand, installedGame.launchCommand);
                        mdl.set(MetaDataId::EpicId, installedGame.id); // Assumi che 'id' sia l'appName per il lancio
                        mdl.set(MetaDataId::EpicNamespace, installedGame.catalogNamespace);
                        mdl.set(MetaDataId::EpicCatalogId, installedGame.catalogItemId);
                        
                        std::string esSystemLanguage = Settings::getInstance()->getString("Language");
                        if (esSystemLanguage.empty()) esSystemLanguage = SystemConf::getInstance()->get("system.language");
                        if (!esSystemLanguage.empty() && mdl.get(MetaDataId::Language).empty()) {
                            mdl.set(MetaDataId::Language, esSystemLanguage);
                        }
                        mdl.setDirty(); 
                        installedProcessed++;
                    }
                }
            }
            LOG(LogInfo) << "[EpicDynamic] Finished processing " << installedProcessed << " installed Epic games. Game count now: " << epicSystem->getGameCount();
        } else {
            LOG(LogWarning) << "[EpicDynamic] EpicGamesStore instance not available, cannot process installed games.";
        }

        // 3. Aggiungi il sistema al vettore globale e carica il tema
        //    Questa operazione viene fatta indipendentemente dal fatto che il sistema sia vuoto,
        //    perché l'utente vuole che il sistema sia presente nel menu.
        if (systemWasNewlyCreated) {
            // Controlla se esiste già in sSystemVector per evitare duplicati se questa logica viene chiamata più volte
            bool alreadyInVector = false;
            for (const auto& sys : SystemData::sSystemVector) {
                if (sys == epicSystem) {
                    alreadyInVector = true;
                    break;
                }
            }
            if (!alreadyInVector) {
                 // Verifica ulteriore di collisione per nome se non si basa sul puntatore
                 bool nameCollision = false;
                 if (!alreadyInVector) { // Controlla collisione nome solo se non già in vettore per puntatore
                    for (const auto& sys : SystemData::sSystemVector) {
                        if (sys && sys->getName() == epicSystem->getName()) {
                            nameCollision = true;
                            break;
                        }
                    }
                 }

                if (!nameCollision) {
                    LOG(LogInfo) << "[EpicDynamic] Adding newly created 'epicgamestore' system to sSystemVector. Current game count: " << epicSystem->getGameCount();
                    sSystemVector.push_back(epicSystem); 
                    epicSystem->loadTheme();             
                    auto defaultView = Settings::getInstance()->getString(epicSystem->getName() + ".defaultView");
                    auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(epicSystem->getName() + ".gridSize"));
                    epicSystem->setSystemViewMode(defaultView, gridSizeOverride, false);
                } else {
                     LOG(LogError) << "[EpicDynamic] Name collision for 'epicgamestore'. New instance discarded.";
                     delete epicSystem; // Pulisci l'istanza appena creata perché non può essere aggiunta
                     epicSystem = nullptr; 
                }
            } else {
                 LOG(LogInfo) << "[EpicDynamic] Newly created 'epicgamestore' system object was already in sSystemVector (unexpected). Theme assumed loaded.";
                 epicSystem->loadTheme(); // Meglio caricarlo comunque per sicurezza se è stato appena creato
            }
         } else if (epicSystem) { // Sistema esistente, ripopolato (solo localmente/gamelist)
             LOG(LogInfo) << "[EpicDynamic] 'epicgamestore' system existed and was repopulated with local/gamelist data. Game count: " << epicSystem->getGameCount();
             // Se il sistema esisteva già, il suo tema dovrebbe essere stato caricato quando è stato aggiunto
             // originariamente a sSystemVector. Una nuova chiamata a loadTheme() qui potrebbe essere ridondante
             // o, in alcuni casi, indesiderata se il tema era già corretto.
             // Se si verificano problemi con il tema per i sistemi esistenti dopo la ripopolazione,
             // potremmo dover indagare ulteriormente, ma per ora rimuoviamo la chiamata problematica.
        
        }

        // 4. Aggiorna il conteggio visualizzato finale (se il sistema non è stato scartato/deallocato)
        if (epicSystem) {
            epicSystem->updateDisplayedGameCount(); // Chiamalo alla fine del popolamento del sistema
            LOG(LogInfo) << "[EpicDynamic] Final processing for epicgamestore at startup. Displayed game count: " << epicSystem->getGameCountInfo()->visibleGames;
        }

    } catch (const std::exception& e) {
        LOG(LogError) << "[EpicDynamic] Main exception during dynamic population of epicgamestore: " << e.what();
        if (systemWasNewlyCreated && epicSystem) { // Se è nuovo e c'è stato un errore, pulisci
            // Potresti voler rimuoverlo da sSystemVector se era stato aggiunto
            auto& vec = SystemData::sSystemVector;
            vec.erase(std::remove(vec.begin(), vec.end(), epicSystem), vec.end());
            delete epicSystem; 
            epicSystem = nullptr; 
        }
    } catch (...) {
        LOG(LogError) << "[EpicDynamic] Unknown main exception during dynamic population of epicgamestore.";
        if (systemWasNewlyCreated && epicSystem) { // Come sopra
            auto& vec = SystemData::sSystemVector;
            vec.erase(std::remove(vec.begin(), vec.end(), epicSystem), vec.end());
            delete epicSystem; 
            epicSystem = nullptr; 
        }
    }
} else if (epicSystem == nullptr && systemWasNewlyCreated) { 
    LOG(LogError) << "[EpicDynamic] EpicSystem object is unexpectedly null after dynamic creation block reported success (but system pointer is null).";
}
// --- Fine della sezione per il sistema 'epicgamestore' ---
 
// --- STEAM SYSTEM CREATION/POPULATION (Logica da rendere il più simile possibile a Epic) ---
LOG(LogInfo) << "[SteamDynamic] Checking/Creating/Populating Steam system...";

SystemData* steamSystem = SystemData::getSystem("steam");
bool steamSystemJustCreated = false; // Rinominato

if (steamSystem == nullptr) {
    LOG(LogInfo) << "[SteamDynamic] Steam system not found, creating dynamically...";
    SystemMetadata md_steam;
    md_steam.name = "steam";
    md_steam.fullName = _("Steam");
    md_steam.themeFolder = "steam";
    md_steam.manufacturer = "Valve";
    md_steam.hardwareType = "pc";

    SystemEnvironmentData* envData_steam = new SystemEnvironmentData();
    std::string exePath_s = Paths::getExePath();
    std::string exeDir_s = Utils::FileSystem::getParent(exePath_s);
    // Assicurati che questo path esista o che ES possa crearlo.
    // È dove si aspetta di trovare gamelist.xml per Steam.
    std::string steamRomPath_s = Utils::FileSystem::getGenericPath(exeDir_s + "/roms/steam");
    LOG(LogInfo) << "[SteamDynamic] Setting StartPath for Steam to: " << steamRomPath_s;
    envData_steam->mStartPath = steamRomPath_s;
    envData_steam->mSearchExtensions = {".steamgame"}; // O le tue estensioni fittizie
    envData_steam->mPlatformIds = {PlatformIds::PC};
    envData_steam->mLaunchCommand = "";

    std::vector<EmulatorData> steamEmulators_s_empty; // Rinominato
    steamSystem = new SystemData(md_steam, envData_steam, &steamEmulators_s_empty, false, false, true, false);

    if (!steamSystem) {
        delete envData_steam;
        LOG(LogError) << "[SteamDynamic] Failed to dynamically create 'steam' system object!";
        // return false; // O gestisci l'errore e non continuare con Steam
    } else {
        LOG(LogInfo) << "[SteamDynamic] Dynamically created 'steam' system object.";
        // Aggiungi il sistema appena creato a sSystemVector QUI.
        bool nameCollisionSteam = false;
        for (const auto& sys : SystemData::sSystemVector) {
            if (sys && sys->getName() == steamSystem->getName()) { nameCollisionSteam = true; break; }
        }
        if (!nameCollisionSteam) {
            SystemData::sSystemVector.push_back(steamSystem);
            LOG(LogInfo) << "[SteamDynamic] Added newly created Steam system to sSystemVector.";
        } else {
            LOG(LogWarning) << "[SteamDynamic] Steam system name collision (getSystem should have caught this). Using existing.";
            delete steamSystem;
            steamSystem = SystemData::getSystem("steam");
            if (!steamSystem) { LOG(LogError) << "[SteamDynamic] CRITICAL: Steam system null after collision handling."; /* gestisci */ }
        }
        steamSystemJustCreated = true;
    }
} else { // steamSystem già esisteva
    LOG(LogInfo) << "[SteamDynamic] Steam system already loaded. Clearing for repopulation.";
    if (steamSystem->getRootFolder()) steamSystem->getRootFolder()->clear();

    FileFilterIndex* existingIndexSteam = steamSystem->getIndex(false);
    if (existingIndexSteam != nullptr) {
        delete existingIndexSteam;
        steamSystem->mFilterIndex = nullptr;
    }
    steamSystem->updateDisplayedGameCount();
}

// Popola il sistema Steam se esiste (o è stato appena creato con successo)
if (steamSystem != nullptr) {
    LOG(LogInfo) << "[SteamDynamic] Initiating data population for 'steam' system...";
    try {
        // --- Inizio Logica di popolamento Steam ---
        // 1. LEGGI IL GAMELIST.XML DI STEAM (SE ESISTE) <<< QUESTO È IL PASSO CHIAVE
        std::string gamelistPathSteam = steamSystem->getGamelistPath(false); // Rinominato
        LOG(LogInfo) << "[SteamDynamic] Attempting to parse Steam gamelist: " << gamelistPathSteam;
        if (Utils::FileSystem::exists(gamelistPathSteam)) {
            std::unordered_map<std::string, FileData*> fileMapSteam; // Rinominato
            if (steamSystem->getRootFolder()) {
                fileMapSteam[steamSystem->getRootFolder()->getPath()] = steamSystem->getRootFolder();
            } else {
                 LOG(LogError) << "[SteamDynamic] Root folder for Steam is null before parsing gamelist! StartPath: " << steamSystem->getStartPath();
                 // Se mStartPath è valido, il costruttore di SystemData dovrebbe aver creato mRootFolder.
                 // Controlla se mStartPath è una directory valida e accessibile.
                 // Potrebbe essere necessario creare mRootFolder esplicitamente se non lo fa:
                 // if (!steamSystem->getStartPath().empty())
                 //    steamSystem->mRootFolder = new FolderData(steamSystem->getStartPath(), steamSystem);
                 // if (steamSystem->getRootFolder()) fileMapSteam[steamSystem->getRootFolder()->getPath()] = steamSystem->getRootFolder();
            }

            // Assicurati che Gamelist::parseGamelist sia accessibile
            if (steamSystem->getRootFolder()) { // Solo se rootFolder è valido
                 parseGamelist(steamSystem, fileMapSteam); // CHIAMA IL PARSING DEL GAMELIST PER STEAM
                 LOG(LogInfo) << "[SteamDynamic] Parsed Steam gamelist. Root folder child count now: " << steamSystem->getRootFolder()->getChildren().size();
            }
        } else {
            LOG(LogWarning) << "[SteamDynamic] Gamelist file not found for Steam at: " << gamelistPathSteam;
            // Se il gamelist non esiste, assicurati che rootFolder esista per l'API
            if (steamSystem && !steamSystem->getRootFolder() && !steamSystem->getStartPath().empty()) {
                 // Come sopra, il costruttore dovrebbe averlo gestito.
                 LOG(LogError) << "[SteamDynamic] Steam RootFolder is still NULL after gamelist check (gamelist missing).";
            }
        }
		     // --- INIZIO SEZIONE CRUCIALE DA AGGIUNGERE/MODIFICARE ---
        // 2. SCANSIONA I GIOCHI STEAM INSTALLATI (INDIPENDENTEMENTE DAL GAMELIST.XML)
       LOG(LogInfo) << "[SteamDynamic] Attempting to discover and add/update with installed Steam games from actual Steam installation...";

GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);
std::string steamStoreName = "SteamStore"; // O "steam" - VERIFICA il nome registrato!

    if (gsm != nullptr) {
            // GameStore* steamStoreProviderBase = storeManager->getStore(steamStoreName); // << ERRORE QUI, usa gsm
            // CORREZIONE:
            GameStore* steamStoreProviderBase = gsm->getStore(steamStoreName);

            if (steamStoreProviderBase != nullptr) {
                SteamStore* steamStoreConcrete = dynamic_cast<SteamStore*>(steamStoreProviderBase);

        if (steamStoreConcrete != nullptr) {
            LOG(LogDebug) << "[SteamDynamic] Calling SteamStore's findInstalledSteamGames method...";
            std::vector<SteamInstalledGameInfo> installedSteamGames = steamStoreConcrete->findInstalledSteamGames();
            LOG(LogInfo) << "[SteamDynamic] SteamStore::findInstalledSteamGames() identified " << installedSteamGames.size() << " installed game manifest entries.";

            FolderData* steamRootFolder = steamSystem->getRootFolder();
            if (steamRootFolder == nullptr) {
                LOG(LogError) << "[SteamDynamic] Steam root folder is NULL, cannot add games from installation scan.";
                // Considera di creare rootFolder qui se StartPath è valido e root è nullo,
                // ma idealmente il costruttore di SystemData o la logica di creazione dinamica
                // dovrebbero già averlo fatto se steamSystem->getStartPath() è corretto.
                // Esempio: if (!steamSystem->getStartPath().empty()) steamSystem->createRootFolder(); (se esiste)
            }
            
            if (steamRootFolder != nullptr) { // Procedi solo se rootFolder è valido
                int addedFromScan = 0;
                int alreadyExisted = 0;
                for (const auto& gameInfo : installedSteamGames) {
                    if (gameInfo.appId == 0 || gameInfo.name.empty() || !gameInfo.fullyInstalled) {
                        LOG(LogWarning) << "[SteamDynamic] Skipping incomplete or not fully installed game from scan: AppID " << gameInfo.appId << ", Name: '" << gameInfo.name << "'";
                        continue;
                    }

                // Il PATH rimane il riferimento interno di ES, non direttamente il comando di lancio.
                    std::string virtualPath = "steam:/launch/" + std::to_string(gameInfo.appId);
                    // IL COMANDO DI LANCIO è quello che ES userà per avviare il gioco.
                    std::string launchCommand = "steam://launch/" + std::to_string(gameInfo.appId); // Comando URL per Steam

                    FileData* existingGame = nullptr;
                    const std::vector<FileData*>& children = steamRootFolder->getChildren();
                    for (FileData* child : children) {
                        if (child->getPath() == virtualPath) {
                            existingGame = child;
                            break;
                        }
                    }

                    bool metadataActuallyChanged = false;

                     if (!existingGame) {
                        LOG(LogDebug) << "[SteamDynamic] Creating new FileData for installed game: '" << gameInfo.name << "' (AppID: " << gameInfo.appId << ", Path: " << virtualPath << ")";
                        
                        existingGame = new FileData(GAME, virtualPath, steamSystem); // Usa 'GAME' direttamente
                        existingGame->getMetadata().set(MetaDataId::Name, gameInfo.name);
                        existingGame->getMetadata().set(MetaDataId::SteamAppId, std::to_string(gameInfo.appId));
                        existingGame->getMetadata().set(MetaDataId::LaunchCommand, launchCommand); // <<< IMPOSTA IL LAUNCHCOMMAND
                        existingGame->getMetadata().set(MetaDataId::Installed, "true");
                        existingGame->getMetadata().set(MetaDataId::Virtual, "false");
                        
                        steamRootFolder->addChild(existingGame, false);
                        if (steamSystem->mFilterIndex) {
                            steamSystem->mFilterIndex->addToIndex(existingGame);
                        }
                        addedFromScan++;
                    } else {
                        LOG(LogDebug) << "[SteamDynamic] Game '" << gameInfo.name << "' (AppID: " << gameInfo.appId << ") already exists. Checking/Updating metadata.";
                                                
                        if (existingGame->getMetadata().get(MetaDataId::Name) != gameInfo.name) {
                            existingGame->getMetadata().set(MetaDataId::Name, gameInfo.name);
                            metadataActuallyChanged = true;
                        }
                        if (existingGame->getMetadata().get(MetaDataId::SteamAppId) != std::to_string(gameInfo.appId)) {
                            existingGame->getMetadata().set(MetaDataId::SteamAppId, std::to_string(gameInfo.appId));
                            metadataActuallyChanged = true;
                        }
                        if (existingGame->getMetadata().get(MetaDataId::LaunchCommand) != launchCommand) { // <<< AGGIORNA IL LAUNCHCOMMAND
                            existingGame->getMetadata().set(MetaDataId::LaunchCommand, launchCommand);
                            metadataActuallyChanged = true;
                        }
                        if (existingGame->getMetadata().get(MetaDataId::Installed) != "true") {
                            existingGame->getMetadata().set(MetaDataId::Installed, "true");
                            metadataActuallyChanged = true;
                        }
                        if (existingGame->getMetadata().get(MetaDataId::Virtual) != "false") {
                            existingGame->getMetadata().set(MetaDataId::Virtual, "false");
                            metadataActuallyChanged = true;
                        }

                        if (metadataActuallyChanged && steamSystem->mFilterIndex) {
                            LOG(LogDebug) << "[SteamDynamic] Metadata changed for existing game '" << gameInfo.name << "'. Re-indexing.";
                            steamSystem->mFilterIndex->removeFromIndex(existingGame);
                            steamSystem->mFilterIndex->addToIndex(existingGame);
                        }
                        alreadyExisted++;
                    }
                }
                LOG(LogInfo) << "[SteamDynamic] Added/Updated " << addedFromScan << " new games from Steam installation scan. " << alreadyExisted << " games already existed/updated.";
            }
        } else {
            LOG(LogError) << "[SteamDynamic] Could not dynamic_cast GameStore* to SteamStore*! Base pointer was from getStore('" << steamStoreName << "').";
        }
    } else {
        LOG(LogError) << "[SteamDynamic] Could not get " << steamStoreName << " provider from manager!";
    }
} else {
    if (gsm != nullptr) {
         LOG(LogError) << "[SteamDynamic] GameStoreManager instance is null (from get())!";
    } else {
         // LOG(LogWarning) << "[SteamDynamic] Steam store '" << steamStoreName << "' is not enabled by manager; skipping scan.";
    }
}
        // --- Fine Logica di popolamento Steam ---

        if (steamSystemJustCreated) {
            size_t steamGameCountAfterPopulation = 0;
            if (steamSystem->getRootFolder()) {
                 std::vector<FileData*> tempListSteam;
                 GetFileContext permissiveCtxSteam; permissiveCtxSteam.showHiddenFiles = true;
                 steamSystem->getRootFolder()->getFilesRecursiveWithContext(tempListSteam, GAME, &permissiveCtxSteam, false, steamSystem, true);
                 steamGameCountAfterPopulation = tempListSteam.size();
            }

            if (steamGameCountAfterPopulation > 0) {
                LOG(LogInfo) << "[SteamDynamic] Loading theme and adding (if new) populated steam system. Game count: " << steamGameCountAfterPopulation;
                steamSystem->loadTheme();
                // ... (setSystemViewMode per Steam) ...
            } else if (steamSystemJustCreated) {
                LOG(LogWarning) << "[SteamDynamic] Newly created steam system is empty after all population. May be hidden.";
            }
        }
        steamSystem->updateDisplayedGameCount();
        LOG(LogInfo) << "[SteamDynamic] Finished populating steam. Final game counts: Visible="
                     << (steamSystem->getGameCountInfo() ? steamSystem->getGameCountInfo()->visibleGames : -1)
                     << ", Total=" << (steamSystem->getGameCountInfo() ? steamSystem->getGameCountInfo()->totalGames : -1);

    } catch (const std::exception& e) {
        LOG(LogError) << "[SteamDynamic] Exception during dynamic population of steam: " << e.what();
    } catch (...) {
        LOG(LogError) << "[SteamDynamic] Unknown exception during dynamic population of steam.";
    }
	
}
// --- FINE BLOCCO STEAM ---

// --- XBOX SYSTEM CREATION/POPULATION ---
LOG(LogInfo) << "[XboxDynamic] Checking/Creating/Populating Xbox system...";

std::string esSystemLanguage = Settings::getInstance()->getString("Language"); 
if (esSystemLanguage.empty()) {
    esSystemLanguage = SystemConf::getInstance()->get("system.language");
}
if (esSystemLanguage.empty()) {
    LOG(LogWarning) << "[XboxPopulate] Lingua globale di EmulationStation non trovata, lo scraper potrebbe usare default.";
} else {
    LOG(LogInfo) << "[XboxPopulate] Lingua globale di EmulationStation rilevata: " << esSystemLanguage;
}

SystemData* xboxSystem = SystemData::getSystem("xbox");
bool xboxSystemJustCreated = false;

if (xboxSystem == nullptr) {
    LOG(LogInfo) << "[XboxDynamic] Xbox system not found, creating dynamically...";
    SystemMetadata md_xbox;
    md_xbox.name = "xbox";
    md_xbox.fullName = _("Xbox");
    md_xbox.themeFolder = "xbox";
    md_xbox.manufacturer = "Microsoft";
    md_xbox.hardwareType = "pc";

    SystemEnvironmentData* envData_xbox = new SystemEnvironmentData();
    std::string exePath_xbox = Paths::getExePath();
    std::string exeDir_xbox = Utils::FileSystem::getParent(exePath_xbox);
    std::string xboxGamelistDir = Utils::FileSystem::getGenericPath(exeDir_xbox + "/roms/xbox");
    
    if (!Utils::FileSystem::exists(xboxGamelistDir)) {
        Utils::FileSystem::createDirectory(xboxGamelistDir);
        LOG(LogInfo) << "[XboxDynamic] Created roms/xbox directory: " << xboxGamelistDir;
    }
    envData_xbox->mStartPath = xboxGamelistDir;
    envData_xbox->mPlatformIds = {PlatformIds::PC};
    envData_xbox->mLaunchCommand = "";

    std::vector<EmulatorData> xboxEmulators_empty;
    xboxSystem = new SystemData(md_xbox, envData_xbox, &xboxEmulators_empty, false, false, true, true);

    if (!xboxSystem) {
        delete envData_xbox;
        LOG(LogError) << "[XboxDynamic] Failed to dynamically create 'xbox' system object!";
    } else {
        LOG(LogInfo) << "[XboxDynamic] Dynamically created 'xbox' system object.";
        xboxSystemJustCreated = true;

        bool nameCollisionXbox = false;
        for (const auto& sys : SystemData::sSystemVector) {
            if (sys && sys->getName() == xboxSystem->getName()) { nameCollisionXbox = true; break; }
        }
        if (!nameCollisionXbox) {
            SystemData::sSystemVector.push_back(xboxSystem);
            LOG(LogInfo) << "[XboxDynamic] Added newly created Xbox system to sSystemVector.";
        } else {
            LOG(LogWarning) << "[XboxDynamic] Xbox system name collision after dynamic creation. Using existing system from vector.";
            delete xboxSystem; 
            xboxSystem = SystemData::getSystem("xbox"); 
            if (xboxSystem) {
                if (xboxSystem->getRootFolder()) xboxSystem->getRootFolder()->clear();
                FileFilterIndex* existingIndexXbox = xboxSystem->getIndex(false);
                if (existingIndexXbox != nullptr) {
                    delete existingIndexXbox;
                    xboxSystem->mFilterIndex = nullptr;
                }
                xboxSystem->updateDisplayedGameCount();
            } else {
                LOG(LogError) << "[XboxDynamic] CRITICAL: Xbox system is null after collision handling and re-fetch.";
            }
        }
    }
} else { 
    LOG(LogInfo) << "[XboxDynamic] Xbox system already loaded. Clearing for repopulation.";
    if (xboxSystem->getRootFolder()) {
        xboxSystem->getRootFolder()->clear();
    }
    FileFilterIndex* existingIndexXbox = xboxSystem->getIndex(false);
    if (existingIndexXbox != nullptr) {
        delete existingIndexXbox;
        xboxSystem->mFilterIndex = nullptr;
    }
    xboxSystem->updateDisplayedGameCount();
}

if (xboxSystem) {
    LOG(LogInfo) << "[XboxDynamic] Initiating data population for 'xbox' system ("
                 << (xboxSystemJustCreated ? "newly created" : "existing, repopulating")
                 << ").";
    try { 
        if (!xboxSystem->getRootFolder()) {
            if (!xboxSystem->getStartPath().empty()) {
                LOG(LogWarning) << "[XboxDynamic] Xbox RootFolder was null. Creating from StartPath: " << xboxSystem->getStartPath();
                if (!Utils::FileSystem::exists(xboxSystem->getStartPath())) {
                    Utils::FileSystem::createDirectory(xboxSystem->getStartPath());
                }
                if (!xboxSystem->mRootFolder) { 
                    xboxSystem->mRootFolder = new FolderData(xboxSystem->getStartPath(), xboxSystem);
                }
                if (xboxSystem->mRootFolder && (xboxSystem->mRootFolder->getMetadata().get(MetaDataId::Name).empty() ||
                                                xboxSystem->mRootFolder->getMetadata().get(MetaDataId::Name) == Utils::FileSystem::getFileName(xboxSystem->getStartPath())) ) {
                    xboxSystem->mRootFolder->getMetadata().set(MetaDataId::Name, xboxSystem->getFullName());
                }
            }
            if (!xboxSystem->getRootFolder()) {
                LOG(LogError) << "[XboxDynamic] CRITICAL: Xbox RootFolder is still null. Population aborted.";
                throw std::runtime_error("Xbox RootFolder could not be established for population.");
            }
        }

        FolderData* currentXboxRootFolder = xboxSystem->getRootFolder();

        std::string gamelistPathXbox = xboxSystem->getGamelistPath(true);
        LOG(LogInfo) << "[XboxDynamic] Attempting to parse Xbox gamelist (if exists): " << gamelistPathXbox;
        if (Utils::FileSystem::exists(gamelistPathXbox)) {
            std::unordered_map<std::string, FileData*> fileMapXbox_local;
            parseGamelist(xboxSystem, fileMapXbox_local);
            LOG(LogInfo) << "[XboxDynamic] Parsed Xbox gamelist. Root folder child count: " << currentXboxRootFolder->getChildren().size();
        } else {
            LOG(LogWarning) << "[XboxDynamic] Gamelist file not found for Xbox: " << gamelistPathXbox;
        }

        GameStoreManager* gsm = GameStoreManager::getInstance(nullptr); 
        XboxStore* xboxStoreConcrete = nullptr;
        if (gsm) {
            GameStore* baseStore = gsm->getStore("xbox"); // Usa nome sistema, non classe
            if (baseStore) xboxStoreConcrete = dynamic_cast<XboxStore*>(baseStore);
        }

        if (xboxStoreConcrete) {
            int addedInstalledCount = 0;
            int updatedInstalledCount = 0;
            
            // --- 1. Processa Giochi Installati (Questa parte rimane) ---
            LOG(LogInfo) << "[XboxDynamic] Discovering installed Xbox games...";
            std::vector<Xbox::InstalledXboxGameInfo> detectedGamesInfo = xboxStoreConcrete->findInstalledXboxGames();
            LOG(LogInfo) << "[XboxDynamic] Found " << detectedGamesInfo.size() << " installed UWP application(s).";

            for (const auto& gameInfo : detectedGamesInfo) {
                if (gameInfo.aumid.empty()) continue;
                std::string gameAUMIDPath = gameInfo.aumid;

                FileData* gameFileData = currentXboxRootFolder->FindByPath(gameAUMIDPath);
                
                if (gameFileData == nullptr) { 
                    gameFileData = new FileData(FileType::GAME, gameAUMIDPath, xboxSystem);
                    MetaDataList& metadata = gameFileData->getMetadata();
                    LOG(LogInfo) << "[XboxDynamic] Added NEW INSTALLED game: '" << gameInfo.displayName << "' (AUMID: " << gameAUMIDPath << ")";
                    
                    metadata.set(MetaDataId::Name, gameInfo.displayName);
                    if (!gameInfo.pfn.empty()) metadata.set(MetaDataId::XboxPfn, gameInfo.pfn);
                    if (!gameInfo.aumid.empty()) metadata.set(MetaDataId::XboxAumid, gameInfo.aumid);
                    metadata.set(MetaDataId::Installed, "true");
                    metadata.set(MetaDataId::Virtual, "false");
                    metadata.set(MetaDataId::LaunchCommand, gameAUMIDPath);
                    if (!esSystemLanguage.empty()) {
                        metadata.set(MetaDataId::Language, esSystemLanguage);
                    }
                    
                    currentXboxRootFolder->addChild(gameFileData, false);
                    if (xboxSystem->mFilterIndex) xboxSystem->mFilterIndex->addToIndex(gameFileData);
                    metadata.setDirty();
                    addedInstalledCount++;
                } else {
                    LOG(LogDebug) << "[XboxDynamic] Updating EXISTING INSTALLED game: '" << gameInfo.displayName << "' (AUMID: " << gameAUMIDPath << ")";
                    MetaDataList& currentMetaData = gameFileData->getMetadata();
                    bool metadataWasActuallyChanged = false;
                    
                    if (!gameInfo.displayName.empty() && currentMetaData.get(MetaDataId::Name) != gameInfo.displayName) {
                        currentMetaData.set(MetaDataId::Name, gameInfo.displayName);
                        metadataWasActuallyChanged = true;
                    }
                    if (!gameInfo.pfn.empty() && currentMetaData.get(MetaDataId::XboxPfn) != gameInfo.pfn) {
                        currentMetaData.set(MetaDataId::XboxPfn, gameInfo.pfn);
                        metadataWasActuallyChanged = true;
                    }
                    if (currentMetaData.get(MetaDataId::XboxAumid) != gameInfo.aumid && !gameInfo.aumid.empty()) {
                        currentMetaData.set(MetaDataId::XboxAumid, gameInfo.aumid);
                        metadataWasActuallyChanged = true;
                    }
                    if (currentMetaData.get(MetaDataId::Installed) != "true") {
                        currentMetaData.set(MetaDataId::Installed, "true");
                        metadataWasActuallyChanged = true;
                    }
                    if (currentMetaData.get(MetaDataId::Virtual) != "false") {
                        currentMetaData.set(MetaDataId::Virtual, "false");
                        metadataWasActuallyChanged = true;
                    }
                    if (currentMetaData.get(MetaDataId::LaunchCommand) != gameInfo.aumid && !gameInfo.aumid.empty()) {
                        currentMetaData.set(MetaDataId::LaunchCommand, gameInfo.aumid);
                        metadataWasActuallyChanged = true;
                    }

                    if (metadataWasActuallyChanged) {
                        currentMetaData.setDirty();
                        LOG(LogInfo) << "[XboxPopulateExisting] Metadata for existing installed game '" << currentMetaData.get(MetaDataId::Name) << "' was updated and marked dirty.";
                    }
                    updatedInstalledCount++;
                }
            } 
            LOG(LogInfo) << "[XboxDynamic] Finished processing installed Xbox games. Added: " << addedInstalledCount << ", Updated: " << updatedInstalledCount;

            // --- 2. Processa Giochi Virtuali ---
            // QUESTO BLOCCO È STATO RIMOSSO INTENZIONALMENTE PER EVITARE LA SINCRONIZZAZIONE ONLINE ALL'AVVIO.
            // La sincronizzazione dei giochi virtuali avverrà solo on-demand tramite la funzione refreshGamesListAsync.

        } else { 
            LOG(LogError) << "[XboxDynamic] Impossibile ottenere istanza XboxStore.";
        }
        
        // Caricamento tema e aggiornamento conteggio giochi
        if (xboxSystemJustCreated) {
            size_t gameCountCurrent = currentXboxRootFolder ? currentXboxRootFolder->getFilesRecursive(GAME, true).size() : 0;
            if (gameCountCurrent > 0 || UIModeController::LoadEmptySystems()) {
                LOG(LogDebug) << "[XboxDynamic] Loading theme for newly created Xbox system. Game count: " << gameCountCurrent;
                xboxSystem->loadTheme();
                auto defaultView = Settings::getInstance()->getString(xboxSystem->getName() + ".defaultView");
                auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(xboxSystem->getName() + ".gridSize"));
                xboxSystem->setSystemViewMode(defaultView, gridSizeOverride, false);
            } else {
                LOG(LogWarning) << "[XboxDynamic] Newly created Xbox system is empty and not configured to show empty systems.";
            }
        } else if (xboxSystem && !xboxSystem->getTheme()) { 
            LOG(LogDebug) << "[XboxDynamic] Xbox system existed. Ensuring theme is loaded if not already.";
            xboxSystem->loadTheme();
        }
        
        if (xboxSystem) {
            xboxSystem->updateDisplayedGameCount();
            LOG(LogInfo) << "[XboxDynamic] Finished all processing for 'xbox'. Final displayed game count: "
                         << (xboxSystem->getGameCountInfo() ? xboxSystem->getGameCountInfo()->visibleGames : -1);
        }

    } catch (const std::runtime_error& re) { 
        LOG(LogError) << "[XboxDynamic] Runtime error during Xbox population: " << re.what();
    } catch (const std::exception& e) {
        LOG(LogError) << "[XboxDynamic] Generic C++ exception during Xbox population: " << e.what();
    } catch (...) {
        LOG(LogError) << "[XboxDynamic] Unknown exception during Xbox population.";
    }
} else { 
     LOG(LogError) << "[XboxDynamic] Xbox system object is definitively null. Cannot proceed with Xbox population logic.";
}
// --- FINE BLOCCO XBOX ---
// --- BLOCCO EA GAMES PER SystemData::loadConfig ---
LOG(LogInfo) << "[EADynamic] Checking/Creating EA Games system...";
SystemData* eaSystem = SystemData::getSystem("EAGamesStore");
bool eaSystemWasNewlyCreated = false;

if (eaSystem == nullptr && Settings::getInstance()->getBool("EnableEAGamesStore")) {
    LOG(LogInfo) << "[EADynamic] EA Games system not found and is enabled, creating dynamically...";
    SystemMetadata md_ea;
    md_ea.name = "EAGamesStore";
    md_ea.fullName = _("EA Games"); 
    md_ea.themeFolder = "EAGamesStore"; 
    md_ea.manufacturer = "Electronic Arts";
    md_ea.hardwareType = "pc";
    SystemEnvironmentData* envData_ea = new SystemEnvironmentData();
    
    // CORREZIONE ERRORE DI BATTITURA
     std::string esPath = Paths::getExePath(); 
    std::string rootPath = Utils::FileSystem::getParent(esPath);
    std::string eaGamelistDir = Utils::FileSystem::getGenericPath(rootPath + "/roms/EAGamesStore");
    if (!Utils::FileSystem::exists(eaGamelistDir)) {
        Utils::FileSystem::createDirectory(eaGamelistDir);
    }
    
    envData_ea->mStartPath = eaGamelistDir;
    envData_ea->mPlatformIds = {PlatformIds::PC}; 
    envData_ea->mLaunchCommand = "";
    std::vector<EmulatorData> eaEmulators_empty; 
    eaSystem = new SystemData(md_ea, envData_ea, &eaEmulators_empty, false, false, true, true); 
    
    if (!eaSystem) {
        delete envData_ea;
    } else {
        eaSystemWasNewlyCreated = true;
        if (SystemData::sSystemVector.end() == std::find(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), eaSystem)) {
             SystemData::sSystemVector.push_back(eaSystem);
        } else {
            delete eaSystem;
            eaSystem = SystemData::getSystem("EAGamesStore");
        }
    }
}

if (eaSystem != nullptr) 
{
    // All'avvio, carichiamo solo la gamelist.
    if (eaSystem->getRootFolder()->getChildren().empty()) {
        std::string gamelistPath = eaSystem->getGamelistPath(false);
        if (Utils::FileSystem::exists(gamelistPath)) {
            LOG(LogInfo) << "[EADynamic] EA Games system is empty, parsing gamelist from cache...";
            
            // CORREZIONE CHIAMATA A PARSEGAMELIST
            std::unordered_map<std::string, FileData*> fileMap;
            eaSystem->getRootFolder()->createChildrenByFilenameMap(fileMap);
            parseGamelist(eaSystem, fileMap);
        }
    }
    
    if (eaSystemWasNewlyCreated) {
        eaSystem->loadTheme();
    }
}
// --- FINE BLOCCO EA GAMES ---

  if (SystemData::sSystemVector.size() > 0) {
  createGroupedSystems();
 
  // Load features before creating collections
  //loadFeatures();
 
  CollectionSystemManager::get()->updateSystemsList();
 
  for (auto sys : SystemData::sSystemVector) {
  auto theme = sys->getTheme();
  if (theme != nullptr) {
  ViewController::get()->onThemeChanged(theme);
  break;
  }
  }
  }
 
  if (window != nullptr && !ThreadedHasher::isRunning()) {
  int checkIndex = 0;
 
  if (Settings::CheevosCheckIndexesAtStart())
  checkIndex |= (int)ThreadedHasher::HASH_CHEEVOS_MD5;
 
  if (SystemConf::getInstance()->getBool("global.netplay") && Settings::NetPlayCheckIndexesAtStart())
  checkIndex |= (int)ThreadedHasher::HASH_NETPLAY_CRC;
 
  if (checkIndex != 0)
  ThreadedHasher::start(window, (ThreadedHasher::HasherType)checkIndex, false, true);
  }
 
  return true;
 }

SystemData* SystemData::loadSystem(std::string systemName, bool fullMode)
{
	std::string path = getConfigPath();
	if (!Utils::FileSystem::exists(path))
		return nullptr;

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(WINSTRINGW(path).c_str());
	if (!res)
		return nullptr;

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");
	if (!systemList)
		return nullptr;

	loadAdditionnalConfig(systemList);

	for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		std::string name = system.child("name").text().get();
		if (Utils::String::compareIgnoreCase(name, systemName) == 0)
			return loadSystem(system, fullMode);
	}

	return nullptr;
}

std::map<std::string, std::string> SystemData::getKnownSystemNames()
{
	std::map<std::string, std::string> ret;

	std::string path = getConfigPath();
	if (!Utils::FileSystem::exists(path))
		return ret;

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(WINSTRINGW(path).c_str());
	if (!res)
		return ret;

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");
	if (!systemList)
		return ret;

	loadAdditionnalConfig(systemList);

	for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		std::string name = system.child("name").text().get();
		if (name.empty())
			continue;

		std::string fullName = system.child("fullname").text().get();
		if (fullName.empty())
			continue;
		
		ret[name] = fullName;
	}

	return ret;
}

#define readList(x) Utils::String::splitAny(x, " \t\r\n,", true)

SystemData* SystemData::loadSystem(pugi::xml_node system, bool fullMode)
{
	std::string path, cmd; // , name, fullname, themeFolder;

	path = system.child("path").text().get();

	SystemMetadata md;
	md.name = system.child("name").text().get();
	md.fullName = system.child("fullname").text().get();
	md.manufacturer = system.child("manufacturer").text().get();
	md.releaseYear = Utils::String::toInteger(system.child("release").text().get());
	md.hardwareType = system.child("hardware").text().get();
	md.themeFolder = system.child("theme").text().as_string(md.name.c_str());

	// convert extensions list from a string into a vector of strings
	std::set<std::string> extensions;
	for (auto ext : readList(system.child("extension").text().get()))
	{
		std::string extlow = Utils::String::toLower(ext);
		if (extensions.find(extlow) == extensions.cend())
			extensions.insert(extlow);
	}

	cmd = system.child("command").text().get();

	// platform id list
	std::string platformList = system.child("platform").text().get();
	std::vector<std::string> platformStrs = readList(platformList);
	std::set<PlatformIds::PlatformId> platformIds;
	for (auto it = platformStrs.cbegin(); it != platformStrs.cend(); it++)
	{
		const char* str = it->c_str();
		PlatformIds::PlatformId platformId = PlatformIds::getPlatformId(str);

		if (platformId == PlatformIds::PLATFORM_IGNORE)
		{
			// when platform is ignore, do not allow other platforms
			platformIds.clear();

			if (md.name == "imageviewer")
				platformIds.insert(PlatformIds::IMAGEVIEWER);
			else
				platformIds.insert(platformId);

			break;
		}

		// if there appears to be an actual platform ID supplied but it didn't match the list, warn
		if (platformId != PlatformIds::PLATFORM_UNKNOWN)
			platformIds.insert(platformId);
		else if (str != NULL && str[0] != '\0' && platformId == PlatformIds::PLATFORM_UNKNOWN)
			LOG(LogWarning) << "  Unknown platform for system \"" << md.name << "\" (platform \"" << str << "\" from list \"" << platformList << "\")";
	}

	//convert path to generic directory seperators
	path = Utils::FileSystem::getGenericPath(path);

	//expand home symbol if the startpath contains ~
	if (path[0] == '~')
	{
		path.erase(0, 1);
		path.insert(0, Paths::getHomePath());
		path = Utils::FileSystem::getCanonicalPath(path);
	}

	//validate
	if (fullMode && (md.name.empty() || path.empty() || extensions.empty() || cmd.empty()))
	{
		LOG(LogError) << "System \"" << md.name << "\" is missing name, extension, or command!";
		return nullptr;
	}

	if (fullMode && !UIModeController::LoadEmptySystems() && !Utils::FileSystem::exists(path))
	{
		LOG(LogError) << "System \"" << md.name << "\" path does not exist !";
		return nullptr;
	}

	//create the system runtime environment data
	SystemEnvironmentData* envData = new SystemEnvironmentData;
	envData->mStartPath = path;
	envData->mSearchExtensions = extensions;
	envData->mLaunchCommand = cmd;
	envData->mPlatformIds = platformIds;
	envData->mGroup = system.child("group").text().get();
	
	// Emulators and cores
	std::vector<EmulatorData> systemEmulators;
	
	pugi::xml_node emulatorsNode = system.child("emulators");
	if (emulatorsNode == nullptr)
		emulatorsNode = system;

	if (emulatorsNode != nullptr)
	{
		for (pugi::xml_node emuNode = emulatorsNode.child("emulator"); emuNode; emuNode = emuNode.next_sibling("emulator"))
		{
			EmulatorData emulatorData;
			emulatorData.name = emuNode.attribute("name").value();
			emulatorData.customCommandLine = emuNode.attribute("command").value();
			emulatorData.features = EmulatorFeatures::Features::all;

			if (emuNode.attribute("incompatible_extensions"))
			{
				for (auto ext : readList(emuNode.attribute("incompatible_extensions").value()))
				{
					std::string extlow = Utils::String::toLower(ext);
					if (std::find(emulatorData.incompatibleExtensions.cbegin(), emulatorData.incompatibleExtensions.cend(), extlow) == emulatorData.incompatibleExtensions.cend())
						emulatorData.incompatibleExtensions.push_back(extlow);
				}
			}

			pugi::xml_node coresNode = emuNode.child("cores");
			if (coresNode == nullptr)
				coresNode = emuNode;

			if (coresNode != nullptr)
			{
				for (pugi::xml_node coreNode = coresNode.child("core"); coreNode; coreNode = coreNode.next_sibling("core"))
				{
					CoreData core;
					core.name = coreNode.text().as_string();
					core.netplay = coreNode.attribute("netplay") && strcmp(coreNode.attribute("netplay").value(), "true") == 0;
					core.isDefault = coreNode.attribute("default") && strcmp(coreNode.attribute("default").value(), "true") == 0;
					
					if (coreNode.attribute("incompatible_extensions"))
					{
						for (auto ext : readList(coreNode.attribute("incompatible_extensions").value()))
						{
							std::string extlow = Utils::String::toLower(ext);
							if (std::find(core.incompatibleExtensions.cbegin(), core.incompatibleExtensions.cend(), extlow) == core.incompatibleExtensions.cend())
								core.incompatibleExtensions.push_back(extlow);
						}
					}

					core.features = EmulatorFeatures::Features::all;
					core.customCommandLine = coreNode.attribute("command").value();

					emulatorData.cores.push_back(core);
				}
			}

			systemEmulators.push_back(emulatorData);
		}
	}

	SystemData* newSys = new SystemData(md, envData, &systemEmulators, false, false, fullMode, true);
    if (newSys->getName() == "epicgamestore")
    {
        LOG(LogDebug) << "SystemData::loadSystem - Loaded EPIC system: " << newSys->getName();
    }
	if (!fullMode)
		return newSys;

	//  [MODIFIED]  Remove or comment out this block!
  
  if (!UIModeController::LoadEmptySystems() && newSys->getRootFolder()->getChildren().size() == 0)
  {
   LOG(LogWarning) << "System \"" << md.name << "\" has no games! Ignoring it.";
   delete newSys;
   return nullptr;
  }
  
	

	if (!newSys->mIsCollectionSystem && newSys->mIsGameSystem && !md.manufacturer.empty() && !IsManufacturerSupported)
		IsManufacturerSupported = true;

	return newSys;
}

bool SystemData::hasDirtySystems()
{
	bool saveOnExit = !Settings::IgnoreGamelist() && Settings::SaveGamelistsOnExit();
	if (!saveOnExit)
		return false;

	for (unsigned int i = 0; i < sSystemVector.size(); i++)
	{
		SystemData* pData = sSystemVector.at(i);
		if (pData->mIsCollectionSystem)
			continue;
		
		if (hasDirtyFile(pData))
			return true;
	}

	return false;
}

void SystemData::deleteSystems()
{
	bool saveOnExit = !Settings::IgnoreGamelist() && Settings::SaveGamelistsOnExit();

	for (unsigned int i = 0; i < sSystemVector.size(); i++)
	{
		SystemData* pData = sSystemVector.at(i);
		pData->getRootFolder()->removeVirtualFolders();

		if (saveOnExit && !pData->mIsCollectionSystem)
			updateGamelist(pData);

		delete pData;
	}

	sSystemVector.clear();
	IsManufacturerSupported = false;
}

std::string SystemData::getConfigPath()
{
	std::string customPath = Paths::getUserEmulationStationPath() + "/es_systems_custom.cfg";
	if (Utils::FileSystem::exists(customPath))
		return customPath;

	std::string userdataPath = Paths::getUserEmulationStationPath() + "/es_systems.cfg";
	if(Utils::FileSystem::exists(userdataPath))
		return userdataPath;

	userdataPath = Paths::getEmulationStationPath() + "/es_systems.cfg";
	if (Utils::FileSystem::exists(userdataPath))
		return userdataPath;

	return "/etc/emulationstation/es_systems.cfg"; // Backward compatibility with Retropie
}

bool SystemData::isVisible()
{
    bool visible = true; // Initialize to true for clarity

    if (mIsCollectionSystem)
    {
        if (mMetadata.name != "favorites" && !UIModeController::getInstance()->isUIModeFull() && getGameCountInfo()->totalGames == 0)
            visible = false;  // Set visible to false if this condition is met
    }
    else if (isGroupChildSystem())
    {
        visible = false; // Set visible to false if this is a group child
    }
    else if (!mHidden && !mIsCollectionSystem && (UIModeController::LoadEmptySystems() || getGameCountInfo()->totalGames > 0))
    {
        visible = true; // Redundant, but makes logic clear
    }
    else
    {
        visible = !mHidden; // The corrected logic: base visibility on mHidden
    }

    // --- LOGGING ---
    LOG(LogDebug) << "System " << getName() << " isVisible(): " << visible
                  << ", mIsCollectionSystem: " << mIsCollectionSystem
                  << ", mHidden: " << mHidden
                  << ", gameCount: " << (getGameCountInfo() ? getGameCountInfo()->totalGames : 0);
    // --- END LOGGING ---

    return visible;
}

SystemData* SystemData::getNext() const
{
	auto it = getIterator();

	do 
	{
		it++;
		if (it == sSystemVector.cend())
			it = sSystemVector.cbegin();
	} 
	while (!(*it)->isVisible());
	
	// as we are starting in a valid gamelistview, this will always succeed, even if we have to come full circle.

	return *it;
}

SystemData* SystemData::getPrev() const
{
	auto it = getRevIterator();

	do 
	{
		it++;
		if (it == sSystemVector.crend())
			it = sSystemVector.crbegin();
	} 
	while (!(*it)->isVisible());
	// as we are starting in a valid gamelistview, this will always succeed, even if we have to come full circle.

	return *it;
}

std::string SystemData::getGamelistPath(bool forWrite) const
{
    // Ottieni la cartella che contiene l'eseguibile
    std::string exePath = Paths::getExePath();
    std::string exeDir = Utils::FileSystem::getParent(exePath);

    // Costruisci il percorso: [CartellaExe]/roms/[nome_sistema]/gamelist.xml
    std::string systemRomDir = exeDir + "/roms/" + getName(); // Usa getName() per il nome corretto!
    std::string filePath = systemRomDir + "/gamelist.xml";

    // Log per debug
    LOG(LogDebug) << "getGamelistPath (Portable Setup 'roms' sibling): Percorso per sistema '" << getName() << "' è: " << filePath;

    // Se richiesto per scrittura, assicurati che la cartella specifica del sistema esista
    if (forWrite) {
        // Controlla se la cartella di destinazione esiste prima di provare a crearla
        if (!Utils::FileSystem::exists(systemRomDir)) {
             LOG(LogInfo) << "getGamelistPath (Portable Setup 'roms' sibling): Creo cartella per gamelist sistema '" << getName() << "': " << systemRomDir;
             // createDirectory dovrebbe creare anche le cartelle intermedie (come 'roms') se necessario
             Utils::FileSystem::createDirectory(systemRomDir);
        } else if (!Utils::FileSystem::isDirectory(systemRomDir)) {
             // Logga un errore se il percorso esiste ma non è una cartella
             LOG(LogError) << "getGamelistPath (Portable Setup 'roms' sibling): Il percorso per la cartella roms del sistema esiste ma non è una directory: " << systemRomDir;
        }
    }
    // Restituisci sempre questo percorso costruito
    return filePath;
}

std::string SystemData::getThemePath() const
{
	// where we check for themes, in order:
	// 1. [SYSTEM_PATH]/theme.xml
	// 2. system theme from currently selected theme set [CURRENT_THEME_PATH]/[SYSTEM]/theme.xml
	// 3. default system theme from currently selected theme set [CURRENT_THEME_PATH]/theme.xml

	// first, check game folder
	
	if (!mEnvData->mStartPath.empty())
	{
		std::string rootThemePath = mRootFolder->getPath() + "/theme.xml";
		if (Utils::FileSystem::exists(rootThemePath))
			return rootThemePath;
	}

	// not in game folder, try system theme in theme sets
	std::string localThemePath = ThemeData::getThemeFromCurrentSet(mMetadata.themeFolder);
	if (Utils::FileSystem::exists(localThemePath))
		return localThemePath;

	// not system theme, try default system theme in theme set
	localThemePath = Utils::FileSystem::getParent(Utils::FileSystem::getParent(localThemePath)) + "/theme.xml";

	return localThemePath;
}

bool SystemData::hasGamelist() const
{
	return (Utils::FileSystem::exists(getGamelistPath(false)));
}

unsigned int SystemData::getGameCount() const
{
	return (unsigned int)mRootFolder->getFilesRecursive(GAME).size();
}

SystemData* SystemData::getRandomSystem()
{
	//  this is a bit brute force. It might be more efficient to just to a while (!gameSystem) do random again...
	unsigned int total = sSystemVector.count([](auto sys) { return sys->isGameSystem(); });

	// get random number in range
	int target = Randomizer::random(total);
	//int target = (int)Math::round((std::rand() / (float)RAND_MAX) * (total - 1));
	for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++)
	{
		if ((*it)->isGameSystem())
		{
			if (target > 0)
			{
				target--;
			}
			else
			{
				return (*it);
			}
		}
	}

	// if we end up here, there is no valid system
	return NULL;
}

FileData* SystemData::getRandomGame()
{
	std::vector<FileData*> list = mRootFolder->getFilesRecursive(GAME, true);
	unsigned int total = (int)list.size();
	if (total == 0)
		return NULL;

	int target = Randomizer::random(total);
	//target = (int)Math::round((std::rand() / (float)RAND_MAX) * (total - 1));
	return list.at(target);
}

GameCountInfo* SystemData::getGameCountInfo()
{
	if (mGameCountInfo != nullptr)
		return mGameCountInfo;	

	std::vector<FileData*> games = mRootFolder->getFilesRecursive(GAME, true);

	int realTotal = games.size();
	if (mFilterIndex != nullptr)
	{
		auto savedFilter = mFilterIndex;
		mFilterIndex = nullptr;
		realTotal = mRootFolder->getFilesRecursive(GAME, true).size();
		mFilterIndex = savedFilter;
	}


	mGameCountInfo = new GameCountInfo();
	mGameCountInfo->visibleGames = games.size();
	mGameCountInfo->totalGames = realTotal;
	mGameCountInfo->favoriteCount = 0;
	mGameCountInfo->hiddenCount = 0;
	mGameCountInfo->playCount = 0;
	mGameCountInfo->gamesPlayed = 0;
	mGameCountInfo->playTime = 0;
	
	int mostPlayCount = 0;
	long gameTime = 0;
	std::string mostCountPlayed;

	for (auto game : games)
	{
		if (game->getFavorite())
			mGameCountInfo->favoriteCount++;

		if (game->getHidden())
			mGameCountInfo->hiddenCount++;

		int playCount = Utils::String::toInteger(game->getMetadata(MetaDataId::PlayCount));
		if (playCount > 0)
		{
			mGameCountInfo->gamesPlayed++;
			mGameCountInfo->playCount += playCount;

			if (playCount > mostPlayCount)
			{
				mostCountPlayed = game->getName();
				mostPlayCount = playCount;
			}
		}

		long seconds = atol(game->getMetadata(MetaDataId::GameTime).c_str());
		if (seconds > 0)
			{
			mGameCountInfo->playTime += seconds;
if (seconds > gameTime)
			{
				mGameCountInfo->mostPlayed = game->getName();
				gameTime = seconds;
			}
		}
		auto lastPlayed = game->getMetadata(MetaDataId::LastPlayed);
		if (!lastPlayed.empty() && lastPlayed > mGameCountInfo->lastPlayedDate)
			mGameCountInfo->lastPlayedDate = lastPlayed;
	}

	if (mGameCountInfo->mostPlayed.empty() && !mostCountPlayed.empty())
		mGameCountInfo->mostPlayed = mostCountPlayed;

	return mGameCountInfo;
	/*
	if (this == CollectionSystemManager::get()->getCustomCollectionsBundle())
		mGameCount = mRootFolder->getChildren().size();
	else
		mGameCount = mRootFolder->getFilesRecursive(GAME, true).size();

	return mGameCount;*/
}

void SystemData::updateDisplayedGameCount()
{
	if (mGameCountInfo != nullptr)
		delete mGameCountInfo;

	mGameCountInfo = nullptr;
}

void SystemData::loadTheme()
{
	mTheme = std::make_shared<ThemeData>();

	std::string path = getThemePath();

	if(!Utils::FileSystem::exists(path)) // no theme available for this platform
		return;

	try
	{
		// build map with system variables for theme to use,
		std::map<std::string, std::string> sysData;

		// Global variables
		sysData["global.help"] = Settings::getInstance()->getBool("ShowHelpPrompts") ? "true" : "false";
		sysData["global.clock"] = Settings::DrawClock() ? "true" : "false";
		sysData["global.architecture"] = Utils::Platform::getArchString();

		sysData["global.cheevos"] = SystemConf::getInstance()->getBool("global.retroachievements") ? "true" : "false";
		sysData["global.cheevos.username"] = SystemConf::getInstance()->getBool("global.retroachievements") ? SystemConf::getInstance()->get("global.retroachievements.username") : "";

		sysData["global.netplay"] = SystemConf::getInstance()->getBool("global.netplay") ? "true" : "false";
		sysData["global.netplay.username"] = SystemConf::getInstance()->getBool("global.netplay") ? SystemConf::getInstance()->get("global.netplay.nickname") : "";

		// Screen
		sysData["screen.width"] = std::to_string(Renderer::getScreenWidth());
		sysData["screen.height"] = std::to_string(Renderer::getScreenHeight());
		sysData["screen.ratio"] = Renderer::getAspectRatio();
		sysData["screen.vertical"] = Renderer::isVerticalScreen() ? "true" : "false";

		// Retrocompatibility
		sysData["cheevos.username"] = SystemConf::getInstance()->getBool("global.retroachievements") ? SystemConf::getInstance()->get("global.retroachievements.username") : "";

		// System variables
		sysData["system.cheevos"] = SystemConf::getInstance()->getBool("global.retroachievements") && (isCheevosSupported() || isCollection()) ? "true" : "false";
		sysData["system.netplay"] = SystemConf::getInstance()->getBool("global.netplay") && isNetplaySupported() ? "true" : "false";
		sysData["system.savestates"] = isCurrentFeatureSupported(EmulatorFeatures::autosave) ? "true" : "false";

		if (Settings::getInstance()->getString("SortSystems") == "hardware")
			sysData["system.sortedBy"] = Utils::String::proper(getSystemMetadata().hardwareType);
		else
			sysData["system.sortedBy"] = getSystemMetadata().manufacturer;

		auto showSystemName = (!isGameSystem() || isCollection()) && Settings::getInstance()->getBool("CollectionShowSystemInfo");
		if (showSystemName && !isGameSystem() && getFolderViewMode() != "never")
			showSystemName = false;

		sysData["system.showSystemName"] = showSystemName ? "true" : "false";

		if (getSystemMetadata().releaseYear > 0)
		{
			sysData["system.releaseYearOrNull"] = std::to_string(getSystemMetadata().releaseYear);
			sysData["system.releaseYear"] = std::to_string(getSystemMetadata().releaseYear);
		}
		else
			sysData["system.releaseYear"] = _("Unknown");
		
		for (auto property : properties)
		{
			auto name = "system." + property.first;
			if (sysData.find(name) == sysData.cend())
				sysData.insert(std::pair<std::string, std::string>("system." + property.first, property.second(this).toString()));
		}

		// Variables 
		/*
		global.architecture
		global.help					( bool )
		global.clock				( bool )
		global.cheevos				( bool )
		global.cheevos.username
		global.netplay				( bool )
		global.netplay.username
		global.language
		screen.width				( float )
		screen.height				( float ) 
		screen.ratio
		screen.vertical             ( bool )
		system.cheevos				( bool )
		system.netplay				( bool )
		system.savestates			( bool )
		system.fullName
		system.group
		system.hardwareType
		system.manufacturer
		system.name
		system.releaseYear
		system.releaseYearOrNull
		system.sortedBy
		system.theme
		system.command

		lang					
		cheevos.username		-> retrocompat
		*/

		mTheme->loadFile(getThemeFolder(), sysData, path);
	}
	catch(ThemeException& e)
	{
		LOG(LogError) << e.what();
		mTheme = std::make_shared<ThemeData>(); // reset to empty
	}
}

void SystemData::setSortId(const unsigned int sortId)
{
	mSortId = sortId;
	Settings::getInstance()->setInt(getName() + ".sort", mSortId);
}

bool SystemData::setSystemViewMode(std::string newViewMode, Vector2f gridSizeOverride, bool setChanged)
{
	if (newViewMode == "automatic")
		newViewMode = "";

	if (mViewMode == newViewMode && gridSizeOverride == mGridSizeOverride)
		return false;

	mGridSizeOverride = gridSizeOverride;
	mViewMode = newViewMode;

	if (setChanged)
	{
		Settings::getInstance()->setString(getName() + ".defaultView", mViewMode == "automatic" ? "" : mViewMode);
		Settings::getInstance()->setString(getName() + ".gridSize", Utils::String::replace(Utils::String::replace(mGridSizeOverride.toString(), ".000000", ""), "0 0", ""));
	}

	return true;
}

Vector2f SystemData::getGridSizeOverride()
{
	return mGridSizeOverride;
}

bool SystemData::isNetplaySupported()
{
	for (auto emul : mEmulators)
		for (auto core : emul.cores)
			if (core.netplay)
				return true;

	if (!isGameSystem())
		return false;

	if (!CustomFeatures::FeaturesLoaded)
		return getSystemEnvData() != nullptr && getSystemEnvData()->mLaunchCommand.find("%NETPLAY%") != std::string::npos;

	return false;
}

std::string SystemData::getCompatibleCoreNames(EmulatorFeatures::Features feature)
{
	std::string ret;

	for (auto emul : mEmulators)
	{
		if ((emul.features & feature) == feature)
			ret += ret.empty() ? emul.name : ", " + emul.name;
		else
		{
			for (auto core : emul.cores)
			{
				if ((core.features & feature) == feature)
				{
					std::string name = emul.name == core.name ? core.name : emul.name + "/" + core.name;
					ret += ret.empty() ? name : ", " + name;
				}
			}
		}
	}

	return ret;
}


bool SystemData::isCheevosSupported()
{
	if (isCollection())
		return false;

	if (mIsCheevosSupported < 0)
	{
		mIsCheevosSupported = 0;
		
		if (isGroupSystem())
		{
			auto groupName = getName();

			for (auto sys : SystemData::sSystemVector)
			{
				if (sys == this || sys->mEnvData == nullptr || sys->mEnvData->mGroup != groupName)
					continue;

				if (sys->isCheevosSupported())
				{
					mIsCheevosSupported = 1;
					return true;
				}
			}

			return false;
		}

		if (!CustomFeatures::FeaturesLoaded)
		{
			const std::set<std::string> cheevosSystems = {
				"megadrive", "n64", "snes", "gb", "gba", "gbc", "nes", "fds", "pcengine", "segacd", "sega32x", "mastersystem",
				"atarilynx", "lynx", "ngp", "gamegear", "pokemini", "atari2600", "fbneo", "fbn", "virtualboy", "pcfx", "tg16", "famicom", "msx1",
				"psx", "sg-1000", "sg1000", "coleco", "colecovision", "atari7800", "wonderswan", "pc88", "saturn", "3do", "apple2", "neogeo",
				"arcade", "mame", "nds", "arcade", "megadrive-japan", "pcenginecd", "supergrafx", "supervision", "snes-msu1", "amstradcpc",
				"dreamcast", "psp", "jaguar", "intellivision", "vectrex", "megaduck", "arduboy", "wasm4", "ps2", "gamecube", "wii", "channelf",
				"o2em", "uzebox" };

			if (cheevosSystems.find(getName()) != cheevosSystems.cend())
				mIsCheevosSupported = 1;

			return mIsCheevosSupported != 0;
		}
		
		for (auto emul : mEmulators)
		{
			for (auto core : emul.cores)
			{
				if ((core.features & EmulatorFeatures::cheevos) == EmulatorFeatures::cheevos)
				{
					mIsCheevosSupported = 1;
					return true;
				}
			}
		}
	}

	return mIsCheevosSupported != 0;
}

bool SystemData::isNetplayActivated()
{
	return sSystemVector.any([](auto sys) { return sys->isNetplaySupported(); });
}

bool SystemData::isGroupChildSystem() 
{ 
	if (mEnvData != nullptr && !mEnvData->mGroup.empty())
		return !Settings::getInstance()->getBool(mEnvData->mGroup + ".ungroup") && 
			   !Settings::getInstance()->getBool(getName() + ".ungroup");

	return false;
}

std::unordered_set<std::string> SystemData::getAllGroupNames()
{
	auto hiddenSystems = Utils::String::split(Settings::HiddenSystems(), ';');

	std::unordered_set<std::string> names;
	
	for (auto sys : SystemData::sSystemVector)
	{
		std::string name;
		if (sys->isGroupSystem())
			name = sys->getName();
		else if (sys->mEnvData != nullptr && !sys->mEnvData->mGroup.empty())
			name = sys->mEnvData->mGroup;

		if (!name.empty() && std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), name) == hiddenSystems.cend())
			names.insert(name);
	}

	return names;
}

std::unordered_set<std::string> SystemData::getGroupChildSystemNames(const std::string groupName)
{
	std::unordered_set<std::string> names;

	for (auto sys : SystemData::sSystemVector)
		if (sys->mEnvData != nullptr && sys->mEnvData->mGroup == groupName)
			names.insert(sys->getName());
		
	return names;
}

SystemData* SystemData::getParentGroupSystem()
{
	if (!isGroupChildSystem() || isGroupSystem())
		return this;

	for (auto sys : SystemData::sSystemVector)
		if (sys->isGroupSystem() && sys->getName() == mEnvData->mGroup)
			return sys;

	return this;
}


std::string SystemData::getEmulator(bool resolveDefault)
{
#if WIN32 && !_DEBUG
	std::string emulator = Settings::getInstance()->getString(getName() + ".emulator");
#else
	std::string emulator = SystemConf::getInstance()->get(getName() + ".emulator");
#endif

	for (auto emul : mEmulators)
		if (emul.name == emulator)
			return emulator;

	if (resolveDefault)
		return getDefaultEmulator();

	return "";
}

std::string SystemData::getCore(bool resolveDefault)
{
#if WIN32 && !_DEBUG
	std::string core = Settings::getInstance()->getString(getName() + ".core");
#else
	std::string core = SystemConf::getInstance()->get(getName() + ".core");
#endif

	if (!core.empty() && core != "auto")
	{
		auto emul = getEmulator(true);

		for (auto memul : mEmulators)
			if (memul.name == emul)
				for (auto mcore : memul.cores)
					if (mcore.name == core)
						return core;
	}

	if (!getEmulator(false).empty())
		return getDefaultCore(getEmulator(false));

	if (resolveDefault)
		return getDefaultCore(getEmulator(true));

	return "";
}


std::string SystemData::getDefaultEmulator()
{
	// Seeking default="true" attribute
	for (auto emul : mEmulators)
		for (auto core : emul.cores)
			if (core.isDefault)
				return emul.name;
		
	auto emulators = getEmulators();
	if (emulators.size() > 0)
		return emulators.begin()->name;

	return "";
}

std::string SystemData::getDefaultCore(const std::string emulatorName)
{
	std::string emul = emulatorName;
	if (emul.empty() || emul == "auto")
		emul = getDefaultEmulator();

	if (emul.empty())
		return "";
	
	for (auto it : mEmulators)
	{
		if (it.name == emul)
		{
			for (auto core : it.cores)
				if (core.isDefault)
					return core.name;

			if (it.cores.size() > 0)
				return it.cores.begin()->name;
		}
	}	

	return "";
}

std::string SystemData::getLaunchCommand(const std::string emulatorName, const std::string coreName)
{
	for (auto emulator : mEmulators)
	{
		if (emulator.name == emulatorName)
		{
			for (auto& core : emulator.cores)
				if (coreName == core.name && !core.customCommandLine.empty())
					return core.customCommandLine;

			if (!emulator.customCommandLine.empty())
				return emulator.customCommandLine;
		}
	}

	return getSystemEnvData()->mLaunchCommand;
}

std::vector<std::string> SystemData::getCoreNames(std::string emulatorName)
{
	std::vector<std::string> list;

	for (auto& emulator : mEmulators)
		if (emulatorName == emulator.name)
			for(auto& core : emulator.cores)
				list.push_back(core.name);

	return list;
}

bool SystemData::hasEmulatorSelection()
{
	if (isCollection() || hasPlatformId(PlatformIds::PLATFORM_IGNORE))
		return false;

	int ec = 0;
	int cc = 0;

	for (auto& emulator : mEmulators)
	{
		ec++;
		for (auto& core : emulator.cores)
			cc++;
	}

	return ec > 1 || cc > 1;
}

SystemData* SystemData::getSystem(const std::string name)
{	
	for (auto sys : SystemData::sSystemVector)
		if (Utils::String::compareIgnoreCase(sys->getName(), name) == 0)
			return sys;

	return nullptr;
}

SystemData* SystemData::getFirstVisibleSystem()
{
	for (auto sys : SystemData::sSystemVector)
		if (sys->mTheme != nullptr && sys->isVisible())
			return sys;

	return nullptr;
}

std::string SystemData::getKeyboardMappingFilePath()
{		
	return Paths::getUserKeyboardMappingsPath() + "/" + getName() + ".keys";
}

bool SystemData::hasKeyboardMapping()
{
	if (isCollection())
		return false;

	return Utils::FileSystem::exists(getKeyboardMappingFilePath());
}

KeyMappingFile SystemData::getKeyboardMapping()
{
	KeyMappingFile ret;

	if (Utils::FileSystem::exists(getKeyboardMappingFilePath()))
		ret = KeyMappingFile::load(getKeyboardMappingFilePath());
	else
	{
		std::string path = Paths::getKeyboardMappingsPath();
		if (!path.empty())
			ret = KeyMappingFile::load(path + "/" + getName() + ".keys");
	}

	ret.path = getKeyboardMappingFilePath();
	return ret;
}

bool SystemData::shouldExtractHashesFromArchives()
{
	return
		!hasPlatformId(PlatformIds::ARCADE) &&
		!hasPlatformId(PlatformIds::NEOGEO) &&
		!hasPlatformId(PlatformIds::DAPHNE) &&
		!hasPlatformId(PlatformIds::LUTRO) &&
		!hasPlatformId(PlatformIds::SEGA_DREAMCAST) &&
		!hasPlatformId(PlatformIds::ATOMISWAVE) &&
		!hasPlatformId(PlatformIds::NAOMI);
}

void SystemData::resetSettings()
{
	for(auto sys : sSystemVector)
		sys->mShowFilenames.reset();
}

SaveStateRepository* SystemData::getSaveStateRepository()
{
	if (mSaveRepository == nullptr)
		mSaveRepository = new SaveStateRepository(this);

	return mSaveRepository;
}

bool SystemData::getShowFilenames()
{
	if (mShowFilenames == nullptr)
	{
		auto group = getParentGroupSystem();
		std::string name = group ? group->getName() : getName();

		auto curFn = Settings::getInstance()->getString(name + ".ShowFilenames");
		if (curFn.empty())
			mShowFilenames = std::make_shared<bool>(Settings::getInstance()->getBool("ShowFilenames"));
		else
			mShowFilenames = std::make_shared<bool>(curFn == "1");
	}

	return *mShowFilenames;
}

bool SystemData::getBoolSetting(const std::string& settingName)
{
	bool show = Settings::getInstance()->getBool(settingName);

	auto spf = Settings::getInstance()->getString(getName() + "." + settingName);
	if (spf == "1")
		return true;
	else if (spf == "0")
		return false;

	return show;
}

bool SystemData::getShowParentFolder()
{
	return getBoolSetting("ShowParentFolder");
}

std::string SystemData::getFolderViewMode()
{
	if (this == CollectionSystemManager::get()->getCustomCollectionsBundle())
		return "always";

	std::string showFoldersMode = Settings::getInstance()->getString("FolderViewMode");

	auto fvm = Settings::getInstance()->getString(getName() + ".FolderViewMode");
	if (!fvm.empty() && fvm != "auto") 
		showFoldersMode = fvm;

	if ((fvm.empty() || fvm == "auto") && this == CollectionSystemManager::get()->getCustomCollectionsBundle())
		showFoldersMode = "always";
	else if (getName() == "windows_installers")
		showFoldersMode = "always";

	return showFoldersMode;
}

bool SystemData::getShowFavoritesFirst()
{
	if (!getShowFavoritesIcon())
		return false;

	return getBoolSetting("FavoritesFirst");
}

bool SystemData::getShowFavoritesIcon()
{
	return getName() != "favorites" && getName() != "recent";
}

bool SystemData::getShowCheevosIcon()
{
	if (getName() != "retroachievements" && SystemConf::getInstance()->getBool("global.retroachievements"))
		return isCollection() || isCheevosSupported();

	return false;
}

int SystemData::getShowFlags()
{
	if (!getShowFavoritesIcon())
		return false;

	if (hasPlatformId(PlatformIds::IMAGEVIEWER) || hasPlatformId(PlatformIds::PLATFORM_IGNORE))
		return false;

	int show = Utils::String::toInteger(Settings::getInstance()->getString("ShowFlags"));

	auto spf = Settings::getInstance()->getString(getName() + ".ShowFlags");
	if (spf == "" || spf == "auto")
		return show;
	
	return Utils::String::toInteger(spf);
}

BindableRandom::BindableRandom(SystemData* system)
{
	mSystem = system;
}

BindableProperty BindableRandom::getProperty(const std::string& name)
{
	if (mRandom.empty())
	{
		SystemRandomPlaylist::PlaylistType type = SystemRandomPlaylist::IMAGE;

		if (name == "thumbnail")
			type = SystemRandomPlaylist::THUMBNAIL;
		else if (name == "marquee")
			type = SystemRandomPlaylist::MARQUEE;
		else if (name == "fanart")
			type = SystemRandomPlaylist::FANART;
		else if (name == "titleshot")
			type = SystemRandomPlaylist::TITLESHOT;
		else if (name == "video")
			type = SystemRandomPlaylist::VIDEO;		

		SystemRandomPlaylist rand(mSystem, type);

		mRandom = rand.getNextItem();
		if (mRandom.empty())
			mRandom = "--empty--";
	}

	if (mRandom == "--empty--")
		return std::string();

	return mRandom;
}

BindableProperty SystemData::getProperty(const std::string& name)
{
	auto it = properties.find(name);
	if (it != properties.cend())
		return it->second(this);

	if (name == "ascollection" || name == "asCollection")
		return isCollection() ? BindableProperty(this) : BindableProperty::Null;

	if (name == "random")
	{
		if (mBindableRandom == nullptr)
			mBindableRandom = new BindableRandom(this);

		return BindableProperty(mBindableRandom);		
	}

	if (name == "image" || name == "logo")
	{
		if (mTheme != nullptr)
		{
			const ThemeData::ThemeElement* logoElem = mTheme->getElement("system", "logo", "image");
			if (logoElem && logoElem->has("path"))
				return BindableProperty(logoElem->get<std::string>("path"), BindablePropertyType::Path);

			return BindableProperty("", BindablePropertyType::Path);
		}
	}

	if (name == "subSystems")
	{
		if (this == CollectionSystemManager::get()->getCustomCollectionsBundle())
			return (int)getRootFolder()->getChildren().size();

		return Math::max(1, getGroupChildSystemNames(getName()).size());
	}

	// Statistics
	GameCountInfo* info = getGameCountInfo();
	if (info == nullptr)
		return BindableProperty::Null;

	if (name == "total")
	{
		if (info->totalGames != info->visibleGames)
			return std::to_string(info->visibleGames) + " / " + std::to_string(info->totalGames);

		return info->totalGames;
	}

	if (name == "played")
		return info->playCount;

	if (name == "favorites")
		return info->favoriteCount;

	if (name == "hidden")
		return info->hiddenCount;

	if (name == "gamesPlayed")
		return info->gamesPlayed;

	if (name == "mostPlayed")
		return info->mostPlayed;

	if (name == "gameTime" && info->playTime != 0)
	{
		auto seconds = info->playTime;

		int d = 0, h = 0, m = 0, s = 0;
		d = seconds / 86400;
		h = (seconds / 3600) % 24;
		m = (seconds / 60) % 60;
		s = seconds % 60;

		std::string timeText;
		if (d > 0)
			timeText = Utils::String::format("%02d:%02d:%02d:%02d", d, h, m, s);
		else if (h > 0)
			timeText = Utils::String::format("%02d:%02d:%02d", h, m, s);
		else
			timeText = Utils::String::format("%02d:%02d", m, s);

		return timeText;
	}

	if (name == "lastPlayedDate")
	{
		Utils::Time::DateTime dt = info->lastPlayedDate;
		if (dt.getTime() != 0)
		{
			time_t     clockNow = dt.getTime();
			struct tm  clockTstruct = *localtime(&clockNow);

			char       clockBuf[256];
			strftime(clockBuf, sizeof(clockBuf), "%x", &clockTstruct);
			
			return BindableProperty(clockBuf, BindablePropertyType::String);
		}

		return BindableProperty::EmptyString;
	}

	return BindableProperty::Null;
}

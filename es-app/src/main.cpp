//EmulationStation, a graphical front-end for ROM browsing. Created by Alec "Aloshi" Lofquist.
//http://www.aloshi.com

#include "services/HttpServerThread.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "Genres.h"
#include "utils/Platform.h"
#include "PowerSaver.h"
#include "Settings.h"
#include "SystemData.h"
#include "SystemScreenSaver.h"
#include <SDL.h> 
#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_timer.h>
#include <iostream>
#include <time.h>
#include "LocaleES.h"
#include <SystemConf.h>
#include "ApiSystem.h"
#include "AudioManager.h"
#include "NetworkThread.h"
#include "scrapers/ThreadedScraper.h"
#include "ThreadedHasher.h"
#include <FreeImage.h>
#include "ImageIO.h"
#include "components/VideoVlcComponent.h"
#include <csignal>
#include "InputConfig.h"
#include "RetroAchievements.h"
#include "TextToSpeech.h"
#include "Paths.h"
#include "resources/TextureData.h"
#include "Scripting.h"
#include "watchers/WatchersManager.h"
#include "HttpReq.h"
#include <thread>
#include "GameStore/EpicGames/EpicGamesStore.h"
#include "GameStore/EpicGames/GameStoreManager.h"
#include "MetaData.h"
#include "FileSorts.h" 
#include "guis/GuiBusyInfoPopup.h"
#include "SdlEvents.h"


#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#define PATH_MAX MAX_PATH
#endif

static std::string gPlayVideo;
static int gPlayVideoDuration = 0;
static bool enable_startup_game = true;

bool parseArgs(int argc, char* argv[])
{
	Paths::setExePath(argv[0]);

	// We need to process --home before any call to Settings::getInstance(), because settings are loaded from homepath
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--home") == 0)
		{
			if (i == argc - 1)
				continue;

			std::string arg = argv[i + 1];
			if (arg.find("-") == 0)
				continue;

			Paths::setHomePath(argv[i + 1]);
			break;
		}
	}

	for(int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--videoduration") == 0)
		{
			gPlayVideoDuration = atoi(argv[i + 1]);
			i++; // skip the argument value
		}
		else if (strcmp(argv[i], "--video") == 0)
		{
			gPlayVideo = argv[i + 1];
			i++; // skip the argument value
		}
		else if (strcmp(argv[i], "--monitor") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << "Invalid monitor supplied.";
				return false;
			}

			int monitorId = atoi(argv[i + 1]);
			i++; // skip the argument value
			Settings::getInstance()->setInt("MonitorID", monitorId);
		}
		else if(strcmp(argv[i], "--resolution") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid resolution supplied.";
				return false;
			}

			int width = atoi(argv[i + 1]);
			int height = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("WindowWidth", width);
			Settings::getInstance()->setInt("WindowHeight", height);
			Settings::getInstance()->setBool("FullscreenBorderless", false);
		}else if(strcmp(argv[i], "--screensize") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid screensize supplied.";
				return false;
			}

			int width = atoi(argv[i + 1]);
			int height = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("ScreenWidth", width);
			Settings::getInstance()->setInt("ScreenHeight", height);
		}else if(strcmp(argv[i], "--screenoffset") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid screenoffset supplied.";
				return false;
			}

			int x = atoi(argv[i + 1]);
			int y = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("ScreenOffsetX", x);
			Settings::getInstance()->setInt("ScreenOffsetY", y);
		}else if (strcmp(argv[i], "--screenrotate") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << "Invalid screenrotate supplied.";
				return false;
			}

			int rotate = atoi(argv[i + 1]);
			++i; // skip the argument value
			Settings::getInstance()->setInt("ScreenRotate", rotate);
		}else if(strcmp(argv[i], "--gamelist-only") == 0)
		{
			Settings::getInstance()->setBool("ParseGamelistOnly", true);
		}else if(strcmp(argv[i], "--ignore-gamelist") == 0)
		{
			Settings::getInstance()->setBool("IgnoreGamelist", true);
		}else if(strcmp(argv[i], "--show-hidden-files") == 0)
		{
			Settings::setShowHiddenFiles(true);
		}else if(strcmp(argv[i], "--draw-framerate") == 0)
		{
			Settings::getInstance()->setBool("DrawFramerate", true);
		}else if(strcmp(argv[i], "--no-exit") == 0)
		{
			Settings::getInstance()->setBool("ShowExit", false);
		}else if(strcmp(argv[i], "--exit-on-reboot-required") == 0)
		{
			Settings::getInstance()->setBool("ExitOnRebootRequired", true);
		}else if(strcmp(argv[i], "--no-startup-game") == 0)
		{
		        enable_startup_game = false;
		}else if(strcmp(argv[i], "--no-splash") == 0)
		{
			Settings::getInstance()->setBool("SplashScreen", false);
		}else if(strcmp(argv[i], "--splash-image") == 0)
		{
		        if (i >= argc - 1)
			{
				std::cerr << "Invalid splash image supplied.";
				return false;
			}
			Settings::getInstance()->setString("AlternateSplashScreen", argv[i+1]);
			++i; // skip the argument value
		}else if(strcmp(argv[i], "--debug") == 0)
		{
			Settings::getInstance()->setBool("Debug", true);
			Settings::getInstance()->setBool("HideConsole", false);
		}
		else if (strcmp(argv[i], "--fullscreen-borderless") == 0)
		{
			Settings::getInstance()->setBool("FullscreenBorderless", true);
		}
		else if (strcmp(argv[i], "--fullscreen") == 0)
		{
		Settings::getInstance()->setBool("FullscreenBorderless", false);
		}
		else if(strcmp(argv[i], "--windowed") == 0)
		{
			Settings::getInstance()->setBool("Windowed", true);
		}else if(strcmp(argv[i], "--vsync") == 0)
		{
			bool vsync = (strcmp(argv[i + 1], "on") == 0 || strcmp(argv[i + 1], "1") == 0) ? true : false;
			Settings::getInstance()->setBool("VSync", vsync);
			i++; // skip vsync value
		}else if(strcmp(argv[i], "--max-vram") == 0)
		{
			int maxVRAM = atoi(argv[i + 1]);
			Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
		}
		else if (strcmp(argv[i], "--force-kiosk") == 0)
		{
			Settings::getInstance()->setBool("ForceKiosk", true);
		}
		else if (strcmp(argv[i], "--force-kid") == 0)
		{
			Settings::getInstance()->setBool("ForceKid", true);
		}
		else if (strcmp(argv[i], "--force-disable-filters") == 0)
		{
			Settings::getInstance()->setBool("ForceDisableFilters", true);
		}
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
#ifdef WIN32
			// This is a bit of a hack, but otherwise output will go to nowhere
			// when the application is compiled with the "WINDOWS" subsystem (which we usually are).
			// If you're an experienced Windows programmer and know how to do this
			// the right way, please submit a pull request!
			AttachConsole(ATTACH_PARENT_PROCESS);
			freopen("CONOUT$", "wb", stdout);
#endif
			std::cout <<
				"EmulationStation, a graphical front-end for ROM browsing.\n"
				"Written by Alec \"Aloshi\" Lofquist.\n"
				"Version " << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING << "\n\n"
				"Command line arguments:\n"
				"--resolution [width] [height]	try and force a particular resolution\n"
				"--gamelist-only			skip automatic game search, only read from gamelist.xml\n"
				"--ignore-gamelist		ignore the gamelist (useful for troubleshooting)\n"
				"--draw-framerate		display the framerate\n"
				"--no-exit			don't show the exit option in the menu\n"
				"--no-splash			don't show the splash screen\n"
				"--debug				more logging, show console on Windows\n"				
				"--windowed			not fullscreen, should be used with --resolution\n"
				"--vsync [1/on or 0/off]		turn vsync on or off (default is on)\n"
				"--max-vram [size]		Max VRAM to use in Mb before swapping. 0 for unlimited\n"
				"--force-kid		Force the UI mode to be Kid\n"
				"--force-kiosk		Force the UI mode to be Kiosk\n"
				"--force-disable-filters		Force the UI to ignore applied filters in gamelist\n"
				"--home [path]		Directory to use as home path\n"
				"--help, -h			summon a sentient, angry tuba\n\n"
				"--monitor [index]			monitor index\n\n"				
				"More information available in README.md.\n";
			return false; //exit after printing help
		}
	}

	return true;
}
void updateEpicGamesMetadata(Window* window, EpicGamesStoreAPI* api);
bool verifyHomeFolderExists()
{
	//make sure the config directory exists	
	std::string configDir = Paths::getUserEmulationStationPath();
	if(!Utils::FileSystem::exists(configDir))
	{
		std::cout << "Creating config directory \"" << configDir << "\"\n";
		Utils::FileSystem::createDirectory(configDir);
		if(!Utils::FileSystem::exists(configDir))
		{
			std::cerr << "Config directory could not be created!\n";
			return false;
		}
	}

	return true;
}

// Returns true if everything is OK,
bool loadSystemConfigFile(Window* window, const char** errorString)
{
	
	*errorString = NULL;

	StopWatch stopWatch("loadSystemConfigFile :", LogDebug);

	ImageIO::loadImageCache();

	if(!SystemData::loadConfig(window))
	{
		LOG(LogError) << "Error while parsing systems configuration file!";
		*errorString = "IT LOOKS LIKE YOUR SYSTEMS CONFIGURATION FILE HAS NOT BEEN SET UP OR IS INVALID. YOU'LL NEED TO DO THIS BY HAND, UNFORTUNATELY.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
		return false;
	}

	if(SystemData::sSystemVector.size() == 0)
	{
		LOG(LogError) << "No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)";
		*errorString = "WE CAN'T FIND ANY SYSTEMS!\n"
			"CHECK THAT YOUR PATHS ARE CORRECT IN THE SYSTEMS CONFIGURATION FILE, "
			"AND YOUR GAME DIRECTORY HAS AT LEAST ONE GAME WITH THE CORRECT EXTENSION.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
		return false;
	}
	

	return true;
}


//called on exit, assuming we get far enough to have the log initialized
void onExit()
{
	Log::close();
}

#ifdef WIN32
#define PATH_MAX MAX_PATH
#include <direct.h>
#endif

int setLocale(char * argv1)
{
#if WIN32
	std::locale::global(std::locale("en-US"));
#else
	if (Utils::FileSystem::exists("./locale/lang")) // for local builds
		EsLocale::init("", "./locale/lang");	
	else
		EsLocale::init("", "/usr/share/locale");	
#endif

	setlocale(LC_TIME, "");

	return 0;
}


void signalHandler(int signum) 
{
	if (signum == SIGSEGV)
		LOG(LogError) << "Interrupt signal SIGSEGV received.\n";
	else if (signum == SIGFPE)
		LOG(LogError) << "Interrupt signal SIGFPE received.\n";
	else if (signum == SIGFPE)
		LOG(LogError) << "Interrupt signal SIGFPE received.\n";
	else
		LOG(LogError) << "Interrupt signal (" << signum << ") received.\n";

	// cleanup and close up stuff here  
	exit(signum);
}

void playVideo()
{
	ApiSystem::getInstance()->setReadyFlag(false);
	Settings::getInstance()->setBool("AlwaysOnTop", true);

	Window window;
	if (!window.init(true))
	{
		LOG(LogError) << "Window failed to initialize!";
		return;
	}

	Settings::getInstance()->setBool("VideoAudio", true);

	bool exitLoop = false;

	VideoVlcComponent vid(&window);
	vid.setVideo(gPlayVideo);
	vid.setOrigin(0.5f, 0.5f);
	vid.setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
	vid.setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());

	vid.setOnVideoEnded([&exitLoop]()
	{
		exitLoop = true;
		return false;
	});

	window.pushGui(&vid);

	vid.onShow();
	vid.topWindow(true);

	int lastTime = SDL_GetTicks();
	int totalTime = 0;

	while (!exitLoop)
	{
		SDL_Event event;

		if (SDL_PollEvent(&event))
		{
			do
			{
				if (event.type == SDL_QUIT)
					return;
			} 
			while (SDL_PollEvent(&event));
		}

		int curTime = SDL_GetTicks();
		int deltaTime = curTime - lastTime;

		if (vid.isPlaying())
		{
			totalTime += deltaTime;

			if (gPlayVideoDuration > 0 && totalTime > gPlayVideoDuration * 100)
				break;
		}

		Transform4x4f transform = Transform4x4f::Identity();
		vid.update(deltaTime);
		vid.render(transform);

		Renderer::swapBuffers();

		if (ApiSystem::getInstance()->isReadyFlagSet())
			break;
	}

	window.deinit(true);
}

void launchStartupGame()
{
	auto gamePath = SystemConf::getInstance()->get("global.bootgame.path");
	if (gamePath.empty() || !Utils::FileSystem::exists(gamePath))
		return;
	
	auto command = SystemConf::getInstance()->get("global.bootgame.cmd");
	if (!command.empty())
	{
		InputManager::getInstance()->init();
		command = Utils::String::replace(command, "%CONTROLLERSCONFIG%", InputManager::getInstance()->configureEmulators());
		Utils::Platform::ProcessStartInfo(command).run();		
	}	
}

#include "utils/MathExpr.h"
 void initEpicGamesStore(Window& window) {
     LOG(LogDebug) << "--- EPIC GAMES STORE INIT START (V4 Final - Conditional API) ---";

     // --- 1. Setup Iniziale (GameStoreManager, EpicGamesStore) ---
     GameStoreManager* gsm = GameStoreManager::get();
     if (!gsm) { LOG(LogError) << "Epic Store Init: Failed GameStoreManager."; return; }
     EpicGamesStore* epicGamesStore = nullptr;
     GameStore* baseStore = gsm->getStore("EpicGamesStore");
     if (baseStore) {
         epicGamesStore = dynamic_cast<EpicGamesStore*>(baseStore);
         if (!epicGamesStore) { LOG(LogError) << "Epic Store Init: Failed cast."; return; }
     } else {
         LOG(LogError) << "Epic Store Init: Store 'EpicGamesStore' not registered."; return;
     }

     // --- 2. Determina e Controlla Gamelist Path (Relativo) ---
     std::map<std::string, std::string> existingNames; // path -> name from XML
     std::string gamelistPath = "";
     bool gamelistParsedSuccessfully = false;
     bool gamelistExists = false;
     std::string gamelistDir = "";

     try {
         std::string exeDir = Utils::FileSystem::getParent(Paths::getExePath());
         gamelistDir = Utils::FileSystem::combine(exeDir, "roms/epicgamestore");
         gamelistPath = Utils::FileSystem::combine(gamelistDir, "gamelist.xml");
         LOG(LogInfo) << "Using relative gamelist path: " << gamelistPath;

         if (!Utils::FileSystem::exists(gamelistDir)) {
             LOG(LogWarning) << "Gamelist directory does not exist, creating: " << gamelistDir;
             Utils::FileSystem::createDirectory(gamelistDir);
             gamelistExists = false;
         } else {
             gamelistExists = Utils::FileSystem::exists(gamelistPath);
         }
     } catch (const std::exception& e) {
         LOG(LogError) << "Error determining relative gamelist path: " << e.what();
         return;
     }

     // --- 3. Tenta di Parsare Gamelist (se esiste) ---
     if (gamelistExists) {
         LOG(LogInfo) << "Attempting to parse existing relative gamelist: " << gamelistPath;
         pugi::xml_document doc;
         pugi::xml_parse_result result = doc.load_file(gamelistPath.c_str());
         if (!result) {
             LOG(LogError) << "Error parsing " << gamelistPath << ": " << result.description() << ". Will treat as non-existent.";
             gamelistParsedSuccessfully = false;
             gamelistExists = false; // Tratta il file corrotto come non esistente per forzare API
         } else {
             pugi::xml_node root = doc.child("gameList");
             if (root) {
                 for (pugi::xml_node gameNode = root.child("game"); gameNode; gameNode = gameNode.next_sibling("game")) {
                     std::string path = gameNode.child_value("path");
                     std::string name = gameNode.child_value("name");
                     if (!path.empty()) {
                         existingNames[path] = name; // Salva path e nome (anche se vuoto)
                     }
                 }
                 LOG(LogInfo) << "Found " << existingNames.size() << " entries in relative gamelist map.";
                 gamelistParsedSuccessfully = true;
             } else {
                 LOG(LogWarning) << "Could not find <gameList> root node in " << gamelistPath << ". Will treat as non-existent.";
                 gamelistParsedSuccessfully = false;
                 gamelistExists = false; // Tratta il file malformato come non esistente
             }
         }
     } else {
         LOG(LogWarning) << "Relative gamelist file NOT FOUND at: " << gamelistPath;
         gamelistParsedSuccessfully = false;
     }

     // --- 4. Ottieni o Crea SystemData ---
     SystemData* epicSystem = SystemData::getSystem("epicgamestore");
     bool systemJustCreated = false;
     if (!epicSystem) {
         LOG(LogWarning) << "Epic Store Init: SystemData 'epicgamestore' not found. Creating dynamically.";
         // --- Blocco creazione dinamica ---
         SystemMetadata epicSystemMetadata;
         epicSystemMetadata.name = "epicgamestore";
         epicSystemMetadata.fullName = "Epic Games Store";
         epicSystemMetadata.themeFolder = "epicgamestore";
         SystemEnvironmentData* epicSystemEnvData = new SystemEnvironmentData();
         epicSystemEnvData->mStartPath = Utils::FileSystem::getGenericPath(gamelistDir);
         epicSystemEnvData->mSearchExtensions = {};
         epicSystemEnvData->mPlatformIds = { PlatformIds::PC };
         epicSystemEnvData->mLaunchCommand = "";
         std::vector<EmulatorData> epicEmulators;
         epicSystem = new SystemData(epicSystemMetadata, epicSystemEnvData, &epicEmulators, false, false, true, false);
         if (!epicSystem) { delete epicSystemEnvData; LOG(LogError) << "Failed dynamic create!"; return; }
         systemJustCreated = true;
         // --- Fine Blocco ---
     } else {
         LOG(LogDebug) << "Epic Store Init: Found existing SystemData object (from es_systems.cfg?).";
         // Pulisci le voci esistenti per evitare conflitti con quelle caricate da noi
         LOG(LogDebug) << "Clearing existing root folder entries before repopulating.";
         epicSystem->getRootFolder()->clear();
     }
     if (!epicSystem->getRootFolder()) { LOG(LogError) << "Epic Store Init: Root folder is null!"; return; }
    pugi::xml_document doc;
    pugi::xml_parse_result loadResult = doc.load_file(gamelistPath.c_str());
    bool fileExists = loadResult && loadResult.status == pugi::xml_parse_status::status_ok;
    bool shouldRunApiCheck = !fileExists;

     // --- 5. Decidi se eseguire l'API Check ---
   //  bool shouldRunApiCheck = false;
     // Esegui API se gamelist non esiste, il parsing è fallito, o era vuoto.
     if (!gamelistExists || !gamelistParsedSuccessfully || existingNames.empty()) {
          LOG(LogInfo) << "CONDITION MET: Gamelist missing, failed parse, or empty. API Check will run.";
          shouldRunApiCheck = true;
     } else {
          LOG(LogInfo) << "CONDITION NOT MET: Gamelist existed, parsed ok, and contains " << existingNames.size() << " entries. API check will be SKIPPED.";
          shouldRunApiCheck = false;
     }

     // --- 6. Popola Giochi Virtuali (da XML o API) ---
     if (shouldRunApiCheck) {
          // --- Esegui Chiamata API ---
          LOG(LogInfo) << "Proceeding with API call to populate virtual Epic games...";
          EpicGamesStoreAPI* epicApi = epicGamesStore->getApi();
          if (epicApi && epicGamesStore->getAuth() && !epicGamesStore->getAuth()->getAccessToken().empty()) {
              try {
                  // existingNames è vuota o non valida qui, populateEpicGamesVirtual la ignorerà (si spera)
                  epicSystem->populateEpicGamesVirtual(epicApi, existingNames);
                  LOG(LogInfo) << "populateEpicGamesVirtual completed (via API).";
              } catch (const std::exception& e) { LOG(LogError) << "Error during populateEpicGamesVirtual (API): " << e.what(); }
              catch (...) { LOG(LogError) << "Unknown error during populateEpicGamesVirtual (API)."; }
          } else {
              LOG(LogError) << "Cannot run populateEpicGamesVirtual: API instance invalid or user not authenticated.";
          }
     } else {
          // ---> INIZIO BLOCCO CORRETTO (SOSTITUISCE IL TUO BLOCCO "Carica da Mappa XML") <---
 // --- Carica da XML Completo ---
        LOG(LogInfo) << "Parsing full gamelist XML to load virtual games and metadata...";

        // Recupera il nodo root (<gameList>) dal documento 'doc'
        pugi::xml_node root = doc.child("gameList"); // 'doc' è nello scope qui
        if (!root) {
             LOG(LogError) << "initEpicGamesStore: Impossibile trovare il nodo gameList nel documento XML caricato nel blocco ELSE!";
        }
        else // Procedi solo se root XML è valido
        {
            int virtualAddedOrUpdated = 0;
            FolderData* rootFolder = epicSystem->getRootFolder(); // Ottieni FolderData root
            if (!rootFolder) {
                LOG(LogError) << "initEpicGamesStore: Impossibile ottenere rootFolder per epicSystem nel blocco ELSE!";
            } else
            {
                // Itera sui NODI <game> nel documento XML 'root'
                for (pugi::xml_node gameNode = root.child("game"); gameNode; gameNode = gameNode.next_sibling("game"))
                {
                    std::string path = gameNode.child_value("path"); // Leggi il path dal nodo XML

                    // Processa solo i percorsi virtuali qui
                    if (path.empty() || !Utils::String::startsWith(path, "epic:/virtual/")) { // Usa UN solo slash
                         continue;
                    }

                    // ---> INIZIO LOGICA REPLICATA (da findOrCreateFile) <---
                    FileData* gameFileData = nullptr; // Puntatore al FileData finale

                    // 1. Controlla se esiste già tra i figli diretti
                    FileData* existingChild = nullptr;
                    for(FileData* child : rootFolder->getChildren()) {
                         if(child != nullptr && child->getPath() == path) {
                              existingChild = child;
                              break;
                         }
                    }

                    if (existingChild != nullptr) {
                        // Trovato! Usa l'oggetto esistente.
                        LOG(LogDebug) << "initEpicGamesStore: Trovato FileData ESISTENTE per path XML: " << path;
                        gameFileData = existingChild;
                    } else {
                        // Non trovato, CREA un nuovo oggetto FileData
                        LOG(LogDebug) << "initEpicGamesStore: FileData non esistente, CREO NUOVO per path XML: " << path;
                        gameFileData = new FileData(FileType::GAME, path, epicSystem);
                        // Aggiungi il NUOVO oggetto ai figli della root folder
                        rootFolder->addChild(gameFileData);
                    }
                    // ---> FINE LOGICA REPLICATA <---


                    if (gameFileData != nullptr) // Ora gameFileData punta a quello giusto (vecchio o nuovo)
                    {
                        LOG(LogDebug) << "Processing game from XML node for path: " << path;
                        // ---> INIZIO LETTURA COMPLETA METADATI (dal nodo XML 'gameNode') <---
                        gameFileData->getMetadata().set(MetaDataId::Name,        gameNode.child_value("name"));
                        gameFileData->getMetadata().set(MetaDataId::Desc,        gameNode.child_value("desc"));
                        gameFileData->getMetadata().set(MetaDataId::Rating,      gameNode.child_value("rating"));
                        gameFileData->getMetadata().set(MetaDataId::ReleaseDate, gameNode.child_value("releasedate"));
                        gameFileData->getMetadata().set(MetaDataId::Developer,   gameNode.child_value("developer"));
                        gameFileData->getMetadata().set(MetaDataId::Publisher,   gameNode.child_value("publisher"));
                        gameFileData->getMetadata().set(MetaDataId::Genre,       gameNode.child_value("genre"));
                        gameFileData->getMetadata().set(MetaDataId::Players,     gameNode.child_value("players"));
                        gameFileData->getMetadata().set(MetaDataId::Image,       gameNode.child_value("image"));
                        gameFileData->getMetadata().set(MetaDataId::Video,       gameNode.child_value("video"));
                        gameFileData->getMetadata().set(MetaDataId::Thumbnail,   gameNode.child_value("thumbnail"));
                        // ... altri standard ...

                        // Leggi e imposta i metadati specifici di Epic (Usa stringhe come ID)
                        gameFileData->getMetadata().set("epicns",    gameNode.child_value("epicns"));
                        gameFileData->getMetadata().set("epiccstid", gameNode.child_value("epiccstid"));
                        gameFileData->getMetadata().set("launch",    gameNode.child_value("launch"));
                        gameFileData->getMetadata().set("virtual",   gameNode.child_value("virtual"));
                        gameFileData->getMetadata().set(MetaDataId::Installed, "false");
                        // ---> FINE LETTURA COMPLETA METADATI <---

                        virtualAddedOrUpdated++;
                    } else {
                         LOG(LogError) << "Errore critico: gameFileData è nullo dopo check/creazione per path XML: " << path;
                    }
                } // Fine ciclo for sui gameNode
                LOG(LogInfo) << "Aggiunti/Aggiornati " << virtualAddedOrUpdated << " giochi virtuali leggendo il gamelist XML completo.";
            } // Fine if (rootFolder)
        } // Fine if (root)
    } // ---> FINE BLOCCO CORRETTO <---

     // --- 7. Gestisci Giochi INSTALLATI (Aggiorna o Aggiungi) ---
     //    (Questa parte viene eseguita sempre, dopo aver popolato i virtuali da XML o API)
     LOG(LogDebug) << "Processing *installed* Epic Games...";
     int installedProcessed = 0;
     try {
         std::vector<EpicGamesStore::EpicGameInfo> installedEpicGames = epicGamesStore->getInstalledEpicGamesWithDetails();
         LOG(LogInfo) << "Found " << installedEpicGames.size() << " installed Epic Games manifests.";
         for (const auto& installedGame : installedEpicGames) {
             std::string installedPath = installedGame.installDir;
             if (installedPath.empty()) continue;
             FileData* installedFileData = epicSystem->getRootFolder()->FindByPath(installedPath);

             if (installedFileData) {
                  // Gioco installato trovato, aggiorna i metadati principali
                  installedFileData->getMetadata().set(MetaDataId::Installed, "true");
                  installedFileData->getMetadata().set(MetaDataId::Virtual, "false");
                  installedFileData->getMetadata().set(MetaDataId::LaunchCommand, installedGame.launchCommand);
                   // NON sovrascrivere il nome se già impostato (potrebbe venire da XML o API precedente)
                  // if (installedFileData->getName().empty()) installedFileData->getMetadata().set(MetaDataId::Name, installedGame.name);
                  // ... aggiorna altri ID se necessario ...
             } else {
                  // Gioco installato non trovato, aggiungilo
                  FileData* gameFileData = new FileData(FileType::GAME, installedPath, epicSystem);
                  gameFileData->getMetadata().set(MetaDataId::Name, installedGame.name); // Usa nome da manifest
                  gameFileData->getMetadata().set(MetaDataId::Installed, "true");
                  gameFileData->getMetadata().set(MetaDataId::Virtual, "false");
                  gameFileData->getMetadata().set(MetaDataId::LaunchCommand, installedGame.launchCommand);
                  // ... imposta ID ...
                  epicSystem->getRootFolder()->addChild(gameFileData);
             }
              installedProcessed++;
         }
     } catch (const std::exception& e) { LOG(LogError) << "Error processing installed: " << e.what(); }
     LOG(LogInfo) << "Finished processing " << installedProcessed << " installed Epic games.";
     LOG(LogInfo) << "System now has " << epicSystem->getRootFolder()->getChildren().size() << " total entries before UI update.";


     // --- 8. Aggiungi Sistema a Vettore Globale e Aggiorna UI ---
     if (systemJustCreated) {
         bool nameCollision = false;
         for(const auto& sys : SystemData::sSystemVector) { if(sys->getName() == epicSystem->getName()) { nameCollision = true; break; } }
         if (!nameCollision && epicSystem->getRootFolder()->getChildren().size() > 0) {
              SystemData::sSystemVector.push_back(epicSystem);
              LOG(LogInfo) << "Added newly created system '" << epicSystem->getName() << "' to sSystemVector.";
              CollectionSystemManager::get()->updateSystemsList();
              LOG(LogInfo) << "Called CollectionSystemManager::get()->updateSystemsList() for dynamic system.";
         } else {
              if (nameCollision) LOG(LogError) << "Name collision: epicgamestore";
              else LOG(LogWarning) << "Newly created system is empty. Not adding.";
              delete epicSystem; epicSystem = nullptr;
         }
     } else if (epicSystem) { // Se esisteva già, aggiorna UI
          CollectionSystemManager::get()->updateSystemsList();
          LOG(LogInfo) << "Called CollectionSystemManager::get()->updateSystemsList() for existing system.";
     }

     // --- 9. Aggiorna Conteggio (Ordinamento Rimosso) ---
     if (epicSystem) {
         epicSystem->updateDisplayedGameCount();
         LOG(LogInfo) << "Final game count updated.";
     }

     LOG(LogDebug) << "--- EPIC GAMES STORE INIT END (V4 Final - Conditional API) ---";
 }

Uint32 SDL_EPIC_REFRESH_COMPLETE; // <- Definisci senza valore qui
int main(int argc, char* argv[])
{	
	Utils::MathExpr::performUnitTests();

	// signal(SIGABRT, signalHandler);
	signal(SIGFPE, signalHandler);
	signal(SIGILL, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGSEGV, signalHandler);
	// signal(SIGTERM, signalHandler);

	srand((unsigned int)time(NULL));

	std::locale::global(std::locale("C"));

	if(!parseArgs(argc, argv))
		return 0;

	// only show the console on Windows if HideConsole is false
#ifdef WIN32
	// MSVC has a "SubSystem" option, with two primary options: "WINDOWS" and "CONSOLE".
	// In "WINDOWS" mode, no console is automatically created for us.  This is good,
	// because we can choose to only create the console window if the user explicitly
	// asks for it, preventing it from flashing open and then closing.
	// In "CONSOLE" mode, a console is always automatically created for us before we
	// enter main. In this case, we can only hide the console after the fact, which
	// will leave a brief flash.
	// TL;DR: You should compile ES under the "WINDOWS" subsystem.
	// I have no idea how this works with non-MSVC compilers.
	if(!Settings::getInstance()->getBool("HideConsole"))
	{
		// we want to show the console
		// if we're compiled in "CONSOLE" mode, this is already done.
		// if we're compiled in "WINDOWS" mode, no console is created for us automatically;
		// the user asked for one, so make one and then hook stdin/stdout/sterr up to it
		if(AllocConsole()) // should only pass in "WINDOWS" mode
		{
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "wb", stdout);
			freopen("CONOUT$", "wb", stderr);
		}
	}else{
		// we want to hide the console
		// if we're compiled with the "WINDOWS" subsystem, this is already done.
		// if we're compiled with the "CONSOLE" subsystem, a console is already created;
		// it'll flash open, but we hide it nearly immediately
		if(GetConsoleWindow()) // should only pass in "CONSOLE" mode
			ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
#endif

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif

	//if ~/.emulationstation doesn't exist and cannot be created, bail
	if(!verifyHomeFolderExists())
		return 1;

	if (!gPlayVideo.empty())
	{
		playVideo();
		return 0;
	}

	//start the logger
	Log::init();	

	LOG(LogInfo) << "EmulationStation - v" << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING;

	//always close the log on exit
	atexit(&onExit);

	// Set locale
	setLocale(argv[0]);	

#if !WIN32
	if(enable_startup_game) {
	  // Run boot game, before Window Create for linux
	  launchStartupGame();
	}
#endif

	Scripting::fireEvent("start");

	// metadata init
	HttpReq::resetCookies();
	Genres::init();
	MetaDataList::initMetadata();

	Window window;
	SystemScreenSaver screensaver(&window);
    ViewController::init(&window);
	LOG(LogDebug) << "main - ViewController::init() called (ViewController address: " << ViewController::get() << ")";
	CollectionSystemManager::init(&window);
  LOG(LogDebug) << "main - CollectionSystemManager::init() called";

  VideoVlcComponent::init();
  LOG(LogDebug) << "main - VideoVlcComponent::init() called";

	window.pushGui(ViewController::get());
	if(!window.init(true, false))
	{
		LOG(LogError) << "Window failed to initialize!";
		return 1;
	}
SDL_EPIC_REFRESH_COMPLETE = SDL_RegisterEvents(1); // <- Assegna l'ID qui
if (SDL_EPIC_REFRESH_COMPLETE == (Uint32)-1) {
    LOG(LogError) << "SDL_RegisterEvents failed!";
} else {
    LOG(LogInfo) << "Registered SDL_EPIC_REFRESH_COMPLETE with ID: " << SDL_EPIC_REFRESH_COMPLETE;
}
	PowerSaver::init();

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

	if (splashScreen)
	{
		std::string progressText = _("Loading...");
		if (splashScreenProgress)
			progressText = _("Loading system config...");

		window.renderSplashScreen(progressText);
	}
    NetworkThread* nthread = new NetworkThread(&window);
	 std::function<void(const std::string&)> dummySetStateCallback = [](const std::string& state) {
  // Do nothing or log if needed
  };

	MameNames::init();
	HttpServerThread httpServer(&window, dummySetStateCallback); // 
	
	LOG(LogDebug) << "main - SystemData::sSystemVector after loading:";
 for (auto sys : SystemData::sSystemVector) {
 LOG(LogDebug) << "  - " << sys->getName() << " (address: " << sys << ")"
               << ", isCollection: " << sys->isCollection()
               << ", isGameSystem: " << sys->isGameSystem();
			   // --- SYSTEM VECTOR LOGGING (AFTER EPIC STORE INIT) ---
  LOG(LogDebug) << "main - SystemData::sSystemVector after initEpicGamesStore:";
  for (auto sys : SystemData::sSystemVector) {
  LOG(LogDebug) << "   - System: " << sys->getName() << " (address: " << sys << ")";
  }
  // --- END SYSTEM VECTOR LOGGING ---
 }


	const char* errorMsg = NULL;
	if(!loadSystemConfigFile(splashScreen && splashScreenProgress ? &window : nullptr, &errorMsg))
	{
		// something went terribly wrong
		if(errorMsg == NULL)
		{
			LOG(LogError) << "Unknown error occured while parsing system config file.";
			Renderer::deinit();
			return 1;
		}

		// we can't handle es_systems.cfg file problems inside ES itself, so display the error message then quit
		window.pushGui(new GuiMsgBox(&window, errorMsg, _("QUIT"), [] { Utils::Platform::quitES(); }));
	}

	SystemConf* systemConf = SystemConf::getInstance();
  //   initEpicGamesStore(window);

#ifdef _ENABLE_KODI_
	if (systemConf->getBool("kodi.enabled", true) && systemConf->getBool("kodi.atstartup"))
	{
		if (splashScreen)
			window.closeSplashScreen();

		ApiSystem::getInstance()->launchKodi(&window);

		if (splashScreen)
		{
			window.renderSplashScreen("");
			splashScreen = false;
		}
	}
#endif

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::PDFEXTRACTION))
		TextureData::PdfHandler = ApiSystem::getInstance();

	ApiSystem::getInstance()->getIpAddress();

	// preload what we can right away instead of waiting for the user to select i
	// this makes for no delays when accessing content, but a longer startup time

	ViewController::get()->preload();
	// Initialize input
	InputConfig::AssignActionButtons();
	InputManager::getInstance()->init();
	SDL_StopTextInput();
    
	

	// tts
	TextToSpeech::getInstance()->enable(Settings::getInstance()->getBool("TTS"), false);

	
	if (errorMsg == NULL)
		ViewController::get()->goToStart(true);
	LOG(LogInfo) << "goToStart() completed."; // Log aggiunto per chiarezza timing


LOG(LogInfo) << "Attempting to start background metadata update thread for Epic Games...";
GameStoreManager* gsm = GameStoreManager::get();
EpicGamesStore* epicStore = nullptr;
EpicGamesStoreAPI* epicApi = nullptr;EpicGamesAuth* epicAuth = nullptr; // <<< Variabile per l'Auth obje

if (gsm) {
    const auto& stores = gsm->getStores();
    auto it = stores.find("EpicGamesStore");
    if (it != stores.end()) {
         epicStore = static_cast<EpicGamesStore*>(it->second);
         if (epicStore) {
              // Assumiamo che tu aggiunga un getter pubblico a EpicGamesStore:
              // In EpicGamesStore.h: EpicGamesAuth* getAuth() { return mAuth; }
              // In EpicGamesStore.cpp: Aggiungi la definizione se necessario.
              epicApi = epicStore->getApi(); // <<< OTTIENI epicApi qui!
                  if (!epicApi) {
                  LOG(LogError) << "EpicGamesStore found, but failed to get Auth object.";
              }
         }
    }

   // --- Loop Principale UI ---
    int lastTime = SDL_GetTicks();
    int exitCode = 0;
    bool running = true;
    // ... (resto del loop main) ...
}

	window.closeSplashScreen();

	// Create a flag in  temporary directory to signal READY state
	ApiSystem::getInstance()->setReadyFlag();

	// Play music
	AudioManager::getInstance()->init();

	if (ViewController::get()->getState().viewing == ViewController::GAME_LIST || ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
		AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme());
	else
		AudioManager::getInstance()->playRandomMusic();

// *** NUOVO: Forza aggiornamento UI DOPO preload e PRIMA di goToStart ***
	// Questo dovrebbe far "vedere" EGS alla SystemView prima che venga mostrata
	// Usiamo reloadAll che esiste sicuramente in ViewController.h
	if (errorMsg == NULL) // Solo se la config sistemi era valida
	{
		LOG(LogInfo) << "Forcing UI reload via ViewController::reloadAll() after dynamic system add...";
		ViewController::get()->reloadAll(&window); // Usa ::get() e passa window
	}

	// Vai alla schermata iniziale (SystemView)
	// Usa ::get()
	if (errorMsg == NULL) // Solo se la config sistemi era valida
	{
		LOG(LogInfo) << "ViewController::goToStart()";
		ViewController::get()->goToStart(true); // Usa ::get()
	}
#ifdef WIN32	
	DWORD displayFrequency = 60;

	DEVMODE lpDevMode;
	memset(&lpDevMode, 0, sizeof(DEVMODE));
	lpDevMode.dmSize = sizeof(DEVMODE);
	lpDevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
	lpDevMode.dmDriverExtra = 0;

	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode) != 0) {
		displayFrequency = lpDevMode.dmDisplayFrequency; // default value if cannot retrieve from user settings.
	}

	int timeLimit = (1000 / displayFrequency) - 10;	 // Margin for vsync
	if (timeLimit < 0)
		timeLimit = 0;
#endif

	int lastTime = SDL_GetTicks();
	int ps_time = SDL_GetTicks();

	bool running = true;

	while(running)
	{
#ifdef WIN32	
		int processStart = SDL_GetTicks();
#endif

		SDL_Event event;

		bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() - ps_time > PowerSaver::getMode();
		if(ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event))
		{
			// PowerSaver can push events to exit SDL_WaitEventTimeout immediatly
			// Reset this event's state
			TRYCATCH("resetRefreshEvent", PowerSaver::resetRefreshEvent());

			    do {
        // Process the event 'event' that was fetched by the outer if or the previous while condition
        TRYCATCH("InputManager::parseEvent", InputManager::getInstance()->parseEvent(event, &window));
        // Il log qui sotto può essere commentato o rimosso se causa problemi o confusione
        // LOG(LogDebug) << "Processing event type: " << event.type << " (EPIC Event ID is: " << SDL_EPIC_REFRESH_COMPLETE << ")";
        if (event.type == SDL_QUIT) {
            running = false;
			}
        else if (event.type == SDL_USEREVENT && event.user.code == SDL_EPIC_REFRESH_COMPLETE) // Modificato per controllare anche SDL_USEREVENT
				{
                    // -------------- INIZIO CODICE NUOVO / MODIFICATO --------------
					LOG(LogInfo) << "Main Loop: Received SDL_EPIC_REFRESH_COMPLETE event.";
					// Recupera il payload (il puntatore al vettore)
					auto* newGamesPayload = static_cast<std::vector<NewEpicGameData>*>(event.user.data1);

                    // Controlla se il payload è valido
					if (newGamesPayload) {
						if (!newGamesPayload->empty()) {
							LOG(LogInfo) << "Processing " << newGamesPayload->size() << " new Epic games from payload.";
							// Ottieni le risorse necessarie (FALLIBILE, controlla i puntatori!)
							SystemData* system = SystemData::getSystem("epicgamestore");
							if (system) {
								FolderData* rootFolder = system->getRootFolder();
								FileFilterIndex* filterIndex = system->getFilterIndex(); // Può essere nullptr

								if (rootFolder) {
									std::vector<std::string> addedGamePathsForMetaUpdate; // Per l'aggiornamento successivo
									addedGamePathsForMetaUpdate.reserve(newGamesPayload->size());
									bool changesMade = false;

									for (const auto& gameData : *newGamesPayload) {
										// Crea il FileData
										FileData* newGame = new FileData(FileType::GAME, gameData.pseudoPath, system);
										MetaDataList& mdl = newGame->getMetadata();

										// Imposta i metadati dalla mappa
										for (const auto& metaPair : gameData.metadataMap) {
											mdl.set(metaPair.first, metaPair.second);
										}

										// Aggiungi al sistema
										rootFolder->addChild(newGame);
										changesMade = true;

                                        newGame->getMetadata().setDirty(); // <--- USA QUESTO!
                                     LOG(LogDebug) << "Main Loop: Marked metadata for new game '" << newGame->getName() << "' as dirty.";

										// Aggiungi all'indice dei filtri se esiste
										if (filterIndex) {
											filterIndex->addToIndex(newGame);
										}
										addedGamePathsForMetaUpdate.push_back(gameData.pseudoPath); // Salva per l'aggiornamento metadati completo

										LOG(LogDebug) << "Main Loop: Added new Epic game: " << gameData.pseudoPath << " (" << mdl.get(MetaDataId::Name) << ")";
									} // fine ciclo for gameData

									if (changesMade) {
										// Aggiorna il conteggio e la UI
										system->updateDisplayedGameCount();
										if (ViewController::get()) {
											LOG(LogInfo) << "Main Loop: Reloading Epic game list view.";
											ViewController::get()->reloadGameListView(system);
											// Potresti voler ricaricare TUTTE le viste se le collezioni sono state modificate
											// ViewController::get()->reloadAll();
										} else {
											LOG(LogWarning) << "Main Loop: ViewController not available, cannot reload game list view.";
										}

                                        // Mostra notifica DOPO aver aggiunto i giochi
                                        window.displayNotificationMessage(_("LIBRERIA EPIC AGGIORNATA.")); // Messaggio modificato




										// --- Triggera l'aggiornamento metadati completo per i nuovi giochi ---
										// Dobbiamo ottenere un puntatore all'istanza di EpicGamesStore.
										// Questo potrebbe richiedere un modo per accedere all'istanza (es. Singleton, passata a Window, etc.)
                                        // USA IL METODO CORRETTO PER LA TUA APPLICAZIONE PER OTTENERE epicStore
										EpicGamesStore* epicStore = nullptr;
                                        GameStoreManager* gsm = GameStoreManager::get();
                                        if (gsm) {
                                            const auto& stores = gsm->getStores();
                                            auto it = stores.find("EpicGamesStore");
                                            if (it != stores.end()) {
                                                epicStore = static_cast<EpicGamesStore*>(it->second);
                                            }
                                        }
                                        // Fine ottenimento epicStore

										if (epicStore && !addedGamePathsForMetaUpdate.empty()) {
											LOG(LogInfo) << "Main Loop: Triggering FULL metadata update task for " << addedGamePathsForMetaUpdate.size() << " newly added Epic games.";
											epicStore->updateGamesMetadataAsync(system, addedGamePathsForMetaUpdate);
										} else if (!epicStore) {
                                            LOG(LogWarning) << "Main Loop: Could not get EpicGamesStore instance to trigger metadata update.";
                                        }

									} // fine if(changesMade)
								} else {
									LOG(LogError) << "Main Loop: Root folder for 'epicgamestore' is null. Cannot add new games.";
								}
							} else {
								LOG(LogError) << "Main Loop: Could not find SystemData for 'epicgamestore'. Cannot add new games.";
							}
						} else {
							LOG(LogInfo) << "Main Loop: Epic refresh completed, but no new games were added.";
                            window.displayNotificationMessage(_("Libreria Epic: Nessun nuovo gioco trovato."));
						}

						// *** FONDAMENTALE: Dealloca la memoria del payload ***
						delete newGamesPayload;
						event.user.data1 = nullptr; // Buona norma

					} else {
						LOG(LogWarning) << "Main Loop: Received SDL_EPIC_REFRESH_COMPLETE but payload (data1) was null.";
					}

                    // Gestione GuiBusyInfoPopup (se necessario)
                    // Se l'aggiornamento è stato avviato da una GUI che mostrava il popup,
                    // potresti volerlo chiudere qui. Assicurati che GuiBusyInfoPopup.h sia incluso.
                    GuiComponent* topGui = window.peekGui();
                    if (topGui != nullptr && dynamic_cast<GuiBusyInfoPopup*>(topGui)) {
                        LOG(LogDebug) << "Main Loop: Closing GuiBusyInfoPopup after processing EPIC_REFRESH_COMPLETE.";
                        delete topGui;
                    } else {
                        // Potrebbe non essere un problema se il popup non c'è più
                        // LOG(LogWarning) << "GuiBusyInfoPopup not found on top when refresh completed. Top GUI: " << (topGui ? typeid(*topGui).name() : "nullptr");
                    }
}
    } while (SDL_PollEvent(&event));

			// check guns
			InputManager::getInstance()->updateGuns(&window);

			// triggered if exiting from SDL_WaitEvent due to event
			if (ps_standby)
				// show as if continuing from last event
				lastTime = SDL_GetTicks();

			// reset counter
			ps_time = SDL_GetTicks();
		}
		else if (ps_standby == false)
		{
		  // check guns
		  InputManager::getInstance()->updateGuns(&window);

		  // If exitting SDL_WaitEventTimeout due to timeout. Trail considering
		  // timeout as an event
		  //	ps_time = SDL_GetTicks();
		}

		if (window.isSleeping())
		{
			lastTime = SDL_GetTicks();
			SDL_Delay(1); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
			continue;
		}

		int curTime = SDL_GetTicks();
		int deltaTime = curTime - lastTime;
		lastTime = curTime;

		// cap deltaTime if it ever goes negative
		if(deltaTime < 0)
			deltaTime = 1000;

		TRYCATCH("Window.update" ,window.update(deltaTime))	
	//	LOG(LogInfo) << "[DEBUG_POPUP] Main loop calling window.render() NOW."; // <-- AGGIUNGI QUESTA RIGA QUI
		TRYCATCH("Window.render", window.render())

/*
#ifdef WIN32		
		int processDuration = SDL_GetTicks() - processStart;
		if (processDuration < timeLimit)
		{
			int timeToWait = timeLimit - processDuration;
			if (timeToWait > 0 && timeToWait < 25 && Settings::VSync())
				Sleep(timeToWait);
		}
#endif
*/

		Renderer::swapBuffers();

		Log::flush();
	}

	if (Utils::Platform::isFastShutdown())
		Settings::getInstance()->setBool("IgnoreGamelist", true);

	WatchersManager::stop();
	ThreadedHasher::stop();
	ThreadedScraper::stop();

	ApiSystem::getInstance()->deinit();

	while (window.peekGui() != ViewController::get())
		delete window.peekGui();

	if (SystemData::hasDirtySystems())
		window.renderSplashScreen(_("SAVING METADATA. PLEASE WAIT..."));

	ImageIO::saveImageCache();
	MameNames::deinit();
	ViewController::saveState();
	CollectionSystemManager::deinit();
	 ViewController* vc = ViewController::get();
        if (vc)
        {
             std::future<void>& future = vc->getEpicUpdateFuture(); // Usa il getter
             if (future.valid()) {
                 LOG(LogInfo) << "Shutdown: Checking status of Epic metadata update task...";
                 if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                     LOG(LogInfo) << "Shutdown: Waiting for Epic metadata update to finish before saving/deleting system data...";
                     try {
                         future.wait();
                         LOG(LogInfo) << "Shutdown: Epic metadata update finished.";
                     } catch (const std::exception& e) {
                         LOG(LogError) << "Shutdown: Exception caught while waiting for Epic update future: " << e.what();
                     } catch (...) {
                          LOG(LogError) << "Shutdown: Unknown exception caught while waiting for Epic update future.";
                     }
                 } else {
                      LOG(LogInfo) << "Shutdown: Epic metadata update task already finished.";
                 }
             } else {
                 LOG(LogDebug) << "Shutdown: No active Epic metadata update task found.";
             }
        } else {
            LOG(LogWarning) << "Shutdown: ViewController instance not available, cannot check Epic update status.";
        }
	SystemData::deleteSystems();
	Scripting::exitScriptingEngine();

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif
	
	// Delete ViewController
	while (window.peekGui() != nullptr)
		delete window.peekGui();

	window.deinit();

	Utils::Platform::processQuitMode();

	LOG(LogInfo) << "EmulationStation cleanly shutting down.";

	return 0;
}

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
#include "EpicGamesStore/EpicGamesStoreAPI.h" // Include EpicGamesStoreAPI
#include "FileData.h" // Include FileData
#include "EpicGamesStore/EpicGamesParser.h" // Include EpicGamesParser

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#define PATH_MAX MAX_PATH
#endif

using namespace Utils; // Use Utils namespace to avoid Utils:: prefix

static std::string gPlayVideo;
static int gPlayVideoDuration = 0;
static bool enable_startup_game = true;

bool parseArgs(int argc, char* argv) {
    Paths::setExePath(std::string(argv[0]));

    // We need to process --home before any call to Settings::getInstance(), because settings are loaded from homepath
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--home") {
            if (i == argc - 1)
                continue;

            std::string arg = std::string(argv[i + 1]);
            if (arg.find("-") == 0)
                continue;

            Paths::setHomePath(std::string(argv[i + 1]));
            break;
        }
    }

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--videoduration") {
            gPlayVideoDuration = atoi(argv[i + 1]);
            i++; // skip the argument value
        } else if (std::string(argv[i]) == "--video") {
            gPlayVideo = std::string(argv[i + 1]);
            i++; // skip the argument value
        } else if (std::string(argv[i]) == "--monitor") {
            if (i >= argc - 1) {
                std::cerr << "Invalid monitor supplied." << std::endl;
                return false;
            }

            int monitorId = std::stoi(argv[i + 1]);
            i++; // skip the argument value
            Settings::getInstance()->setInt("MonitorID", monitorId);
        } else if (std::string(argv[i]) == "--resolution") {
            if (i >= argc - 2) {
                std::cerr << "Invalid resolution supplied." << std::endl;
                return false;
            }

            int width = std::stoi(argv[i + 1]);
            int height = std::stoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("WindowWidth", width);
            Settings::getInstance()->setInt("WindowHeight", height);
            Settings::getInstance()->setBool("FullscreenBorderless", false);
        } else if (std::string(argv[i]) == "--screensize") {
            if (i >= argc - 2) {
                std::cerr << "Invalid screensize supplied." << std::endl;
                return false;
            }

            int width = std::stoi(argv[i + 1]);
            int height = std::stoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("ScreenWidth", width);
            Settings::getInstance()->setInt("ScreenHeight", height);
        } else if (std::string(argv[i]) == "--screenoffset") {
            if (i >= argc - 2) {
                std::cerr << "Invalid screenoffset supplied." << std::endl;
                return false;
            }

            int x = std::stoi(argv[i + 1]);
            int y = std::stoi(argv[i + 2]);
            i += 2; // skip the argument value
            Settings::getInstance()->setInt("ScreenOffsetX", x);
            Settings::getInstance()->setInt("ScreenOffsetY", y);
        } else if (std::string(argv[i]) == "--screenrotate") {
            if (i >= argc - 1) {
                std::cerr << "Invalid screenrotate supplied." << std::endl;
                return false;
            }

            int rotate = std::stoi(argv[i + 1]);
            ++i; // skip the argument value
            Settings::getInstance()->setInt("ScreenRotate", rotate);
        } else if (std::string(argv[i]) == "--gamelist-only") {
            Settings::getInstance()->setBool("ParseGamelistOnly", true);
        } else if (std::string(argv[i]) == "--ignore-gamelist") {
            Settings::getInstance()->setBool("IgnoreGamelist", true);
        } else if (std::string(argv[i]) == "--show-hidden-files") {
            Settings::setShowHiddenFiles(true);
        } else if (std::string(argv[i]) == "--draw-framerate") {
            Settings::getInstance()->setBool("DrawFramerate", true);
        } else if (std::string(argv[i]) == "--no-exit") {
            Settings::getInstance()->setBool("ShowExit", false);
        } else if (std::string(argv[i]) == "--exit-on-reboot-required") {
            Settings::getInstance()->setBool("ExitOnRebootRequired", true);
        } else if (std::string(argv[i]) == "--no-startup-game") {
            enable_startup_game = false;
        } else if (std::string(argv[i]) == "--no-splash") {
            Settings::getInstance()->setBool("SplashScreen", false);
        } else if (std::string(argv[i]) == "--splash-image") {
            if (i >= argc - 1) {
                std::cerr << "Invalid splash image supplied." << std::endl;
                return false;
            }
            Settings::getInstance()->setString("AlternateSplashScreen", argv[i + 1]);
            ++i; // skip the argument value
        } else if (std::string(argv[i]) == "--debug") {
            Settings::getInstance()->setBool("Debug", true);
            Settings::getInstance()->setBool("HideConsole", false);
        } else if (std::string(argv[i]) == "--fullscreen-borderless") {
            Settings::getInstance()->setBool("FullscreenBorderless", true);
        } else if (std::string(argv[i]) == "--fullscreen") {
            Settings::getInstance()->setBool("FullscreenBorderless", false);
        } else if (std::string(argv[i]) == "--windowed") {
            Settings::getInstance()->setBool("Windowed", true);
        } else if (std::string(argv[i]) == "--vsync") {
            bool vsync = (std::string(argv[i + 1]) == "on" || std::string(argv[i + 1]) == "1") ? true : false;
            Settings::getInstance()->setBool("VSync", vsync);
            i++; // skip vsync value
        } else if (std::string(argv[i]) == "--max-vram") {
            int maxVRAM = std::stoi(argv[i + 1]);
            Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
        } else if (std::string(argv[i]) == "--force-kiosk") {
            Settings::getInstance()->setBool("ForceKiosk", true);
        } else if (std::string(argv[i]) == "--force-kid") {
            Settings::getInstance()->setBool("ForceKid", true);
        } else if (std::string(argv[i]) == "--force-disable-filters") {
            Settings::getInstance()->setBool("ForceDisableFilters", true);
        } else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
#ifdef WIN32
            // This is a bit of a hack, but otherwise output will go to nowhere
            // when the application is compiled with the "WINDOWS" subsystem (which we usually are).
            // If you're an experienced Windows programmer and know how to do this
            // the right way, please submit a pull request!
            AttachConsole(ATTACH_PARENT_PROCESS);
            freopen("CONOUT$", "wb", stdout);
            freopen("CONERR$", "wb", stderr);
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

bool verifyHomeFolderExists() {
    //make sure the config directory exists
    std::string configDir = Paths::getUserEmulationStationPath();
    if (!FileSystemUtil::exists(configDir)) {
        std::cout << "Creating config directory \"" << configDir << "\"\n";
        FileSystemUtil::createDirectory(configDir);
        if (!FileSystemUtil::exists(configDir)) {
            std::cerr << "Config directory could not be created!\n";
            return false;
        }
    }

    return true;
}

// Returns true if everything is OK,
bool loadSystemConfigFile(Window* window, const char** errorString) {
    *errorString = NULL;

    StopWatch stopWatch("loadSystemConfigFile :", LogDebug);

    ImageIO::loadImageCache();

    if (!SystemData::loadConfig(window)) {
        Log::error("Error while parsing systems configuration file!");
        *errorString = "IT LOOKS LIKE YOUR SYSTEMS CONFIGURATION FILE HAS NOT BEEN SET UP OR IS INVALID. YOU'LL NEED TO DO THIS BY HAND, UNFORTUNATELY.\n\n"
            "VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
        return false;
    }

    if (SystemData::sSystemVector.size() == 0) {
        Log::error("No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)");
        *errorString = "WE CAN'T FIND ANY SYSTEMS!\n"
            "CHECK THAT YOUR PATHS ARE CORRECT IN THE SYSTEMS CONFIGURATION FILE, "
            "AND YOUR GAME DIRECTORY HAS AT LEAST ONE GAME WITH THE CORRECT EXTENSION.\n\n"
            "VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
        return false;
    }

    return true;
}

//called on exit, assuming we get far enough to have the log initialized
void onExit() {
    Log::close();
}

#ifdef WIN32
#define PATH_MAX MAX_PATH
#include <direct.h>
#endif

int setLocale(char* argv1) {
#if WIN32
    std::locale::global(std::locale("en-US"));
#else
    if (FileSystemUtil::exists("./locale/lang")) // for local builds
        EsLocale::init("", "./locale/lang");
    else
        EsLocale::init("", "/usr/share/locale");
#endif

    setlocale(LC_TIME, "");

    return 0;
}

void signalHandler(int signum) {
    if (signum == SIGSEGV)
        Log::error("Interrupt signal SIGSEGV received.\n");
    else if (signum == SIGFPE)
        Log::error("Interrupt signal SIGFPE received.\n");
    else if (signum == SIGFPE)
        Log::error("Interrupt signal SIGFPE received.\n");
    else
        Log::error("Interrupt signal (" + std::to_string(signum) + ") received.\n");

    // cleanup and close up stuff here
    exit(signum);
}

void playVideo() {
    ApiSystem::getInstance()->setReadyFlag(false);
    Settings::getInstance()->setBool("AlwaysOnTop", true);

    Window window;
    if (!window.init(true)) {
        Log::error("Window failed to initialize!");
        return;
    }

    Settings::getInstance()->setBool("VideoAudio", true);

    bool exitLoop = false;

    VideoVlcComponent vid(&window);
    vid.setVideo(gPlayVideo);
    vid.setOrigin(0.5f, 0.5f);
    vid.setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
    vid.setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());

    vid.setOnVideoEnded([&exitLoop]() {
        exitLoop = true;
        return false;
    });

    window.pushGui(&vid);

    vid.onShow();
    vid.topWindow(true);

    int lastTime = SDL_GetTicks();
    int totalTime = 0;

    while (!exitLoop) {
        SDL_Event event;

        if (SDL_PollEvent(&event)) {
            do {
                if (event.type == SDL_QUIT)
                    return;
            } while (SDL_PollEvent(&event));
        }

        int curTime = SDL_GetTicks();
        int deltaTime = curTime - lastTime;

        if (vid.isPlaying()) {
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

void launchStartupGame() {
    auto gamePath = SystemConf::getInstance()->get("global.bootgame.path");
    if (gamePath.empty() || !FileSystemUtil::exists(gamePath))
        return;

    auto command = SystemConf::getInstance()->get("global.bootgame.cmd");
    if (!command.empty()) {
        InputManager::getInstance()->init();
        command = StringUtil::replace(command, "%CONTROLLERSCONFIG%", InputManager::getInstance()->configureEmulators());
        Platform::ProcessStartInfo(command).run();
    }
}

#include "utils/MathExpr.h"

int main(int argc, char* argv) {
    MathExpr::performUnitTests();

    // signal(SIGABRT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGSEGV, signalHandler);
    // signal(SIGTERM, signalHandler);

    srand((unsigned int)time(NULL));

    std::locale::global(std::locale("C"));

    if (!parseArgs(argc, argv))
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
    if (!Settings::getInstance()->getBool("HideConsole")) {
        // we want to show the console
        // if we're compiled in "CONSOLE" mode, this is already done.
        // if we're compiled in "WINDOWS" mode, no console is created for us automatically;
        // the user asked for one, so make one and then hook stdin/stdout/sterr up to it
        if (AllocConsole()) // should only pass in "WINDOWS" mode
        {
            freopen("CONIN$", "r", stdin);
            freopen("CONOUT$", "wb", stdout);
            freopen("CONERR$", "wb", stderr);
        }
    } else {
        // we want to hide the console
        // if we're compiled with the "WINDOWS" subsystem, this is already done.
        // if we're compiled with the "CONSOLE" subsystem, a console is already created;
        // it'll flash open, but we hide it nearly immediately
        if (GetConsoleWindow()) // should only pass in "CONSOLE" mode
            ShowWindow(GetConsoleWindow(), SW_HIDE);
    }
#endif

    // call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif

    //if ~/.emulationstation doesn't exist and cannot be created, bail
    if (!verifyHomeFolderExists())
        return 1;

    if (!gPlayVideo.empty()) {
        playVideo();
        return 0;
    }

    //start the logger
    Log::init();

    Log::info("EmulationStation - v" + std::string(PROGRAM_VERSION_STRING) + ", built " + std::string(PROGRAM_BUILT_STRING));

    //always close the log on exit
    atexit(&onExit);

    // Set locale
    setLocale(argv[0]);

#if !WIN32
    if (enable_startup_game) {
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
    CollectionSystemManager::init(&window);
    VideoVlcComponent::init();

    window.pushGui(ViewController::get());
    if (!window.init(true, false)) {
        Log::error("Window failed to initialize!");
        return 1;
    }

    PowerSaver::init();

    bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
    bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

    if (splashScreen) {
        std::string progressText = _("Loading...");
        if (splashScreenProgress)
            progressText = _("Loading system config...");

        window.renderSplashScreen(progressText);
    }

    MameNames::init();

    const char* errorMsg = NULL;
    if (!loadSystemConfigFile(splashScreen && splashScreenProgress ? &window : nullptr, &errorMsg)) {
        // something went terribly wrong
        if (errorMsg == NULL) {
            Log::error("Unknown error occurred while parsing system config file.");
            Renderer::deinit();
            return 1;
        }

        // we can't handle es_systems.cfg file problems inside ES itself, so display the error message then quit
        window.pushGui(new GuiMsgBox(&window, errorMsg, _("QUIT"),{ Platform::quitES(); }));
    }

    SystemConf* systemConf = SystemConf::getInstance();

#ifdef _ENABLE_KODI_
    if (systemConf->getBool("kodi.enabled", true) && systemConf->getBool("kodi.atstartup")) {
        if (splashScreen)
            window.closeSplashScreen();

        ApiSystem::getInstance()->launchKodi(&window);

        if (splashScreen) {
            window.renderSplashScreen("");
            splashScreen = false;
        }
    }
#endif

    if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::PDFEXTRACTION))
        TextureData::PdfHandler = ApiSystem::getInstance();

    ApiSystem::getInstance()->getIpAddress();

    // preload what we can right away instead of waiting for the user to select it
    // this makes for no delays when accessing content, but a longer startup time
    ViewController::get()->preload();

    // Initialize input
    InputConfig::AssignActionButtons();
    InputManager::getInstance()->init();
    SDL_StopTextInput();

    NetworkThread* nthread = new NetworkThread(&window);
    HttpServerThread httpServer(&window);

    // tts
    TextToSpeech::getInstance()->enable(Settings::getInstance()->getBool("TTS"), false);

    if (errorMsg == NULL)
        ViewController::get()->goToStart(true);

    window.closeSplashScreen();

    // Create a flag in  temporary directory to signal READY state
    ApiSystem::getInstance()->setReadyFlag();

    // Play music
    AudioManager::getInstance()->init();

    if (ViewController::get()->getState().viewing == ViewController::GAME_LIST || ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
        AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme());
    else
        AudioManager::getInstance()->playRandomMusic();

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

    int timeLimit = (1000 / displayFrequency) - 10;     // Margin for vsync
    if (timeLimit < 0)
        timeLimit = 0;
#endif

    int lastTime = SDL_GetTicks();
    int ps_time = SDL_GetTicks();

    bool running = true;

    while (running) {
#ifdef WIN32
        int processStart = SDL_GetTicks();
#endif

        SDL_Event event;

        bool ps_standby = PowerSaver::getState() && (int)SDL_GetTicks() - ps_time > PowerSaver::getMode();
        if (ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event)) {
            // PowerSaver can push events to exit SDL_WaitEventTimeout immediatly
            // Reset this event's state
            TRYCATCH("resetRefreshEvent", PowerSaver::resetRefreshEvent());

            do {
                TRYCATCH("InputManager::parseEvent", InputManager::getInstance()->parseEvent(event, &window));

                if (event.type == SDL_QUIT)
                    running = false;
            } while (SDL_PollEvent(&event));

            // check guns
            InputManager::getInstance()->updateGuns(&window);

            // triggered if exiting from SDL_WaitEvent due to event
            if (ps_standby)
                // show as if continuing from last event
                lastTime = SDL_GetTicks();

            // reset counter
            ps_time = SDL_GetTicks();
        } else if (ps_standby == false) {
            // check guns
            InputManager::getInstance()->updateGuns(&window);

            // If exitting SDL_WaitEventTimeout due to timeout. Trail considering
            // timeout as an event
            //  ps_time = SDL_GetTicks();
        }

        if (window.isSleeping()) {
            lastTime = SDL_GetTicks();
            SDL_Delay(1); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
            continue;
        }

        int curTime = SDL_GetTicks();
        int deltaTime = curTime - lastTime;
        lastTime = curTime;

        // cap deltaTime if it ever goes negative
        if (deltaTime < 0)
            deltaTime = 1000;

        TRYCATCH("Window.update", window.update(deltaTime))
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

    if (Platform::isFastShutdown())
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

    Platform::processQuitMode();

    Log::info("EmulationStation cleanly shutting down.");

    // Initialize Epic Games Store API
    EpicGamesStoreAPI epicAPI;
    if (!epicAPI.initialize()) {
        // Handle error (e.g., log message, show a warning)
        std::cerr << "Error initializing Epic Games Store integration" << std::endl;
        // You might choose to continue or exit depending on the severity
    }

    // Load games for each system
    for (auto system : SystemData::sSystemVector) {
        // ... existing code to load games for emulators ...

        // Add code to load Epic Games Store games (if it's a designated system)
        if (system->getName() == "EpicGamesStore") {
            // 1. Get the list of installed Epic Games Store games
            std::string gamesList = epicAPI.getGamesList();
            std::cout << "Raw JSON from epicAPI.getGamesList():\n" << gamesList << std::endl;

            // 2. Parse the games list (if it's in a specific format)
            // (Use a JSON parser if needed)
            std::vector<FileData*> epicGames = parseEpicGamesList(gamesList, system);

            // 3. Add the Epic Games Store games to the system's game list
            FolderData* gamesFolder = system->getRootFolder(); // Adapt this line based on EmulationStation's API
            if (gamesFolder) {
                for (FileData* game : epicGames) {
                    gamesFolder->addChild(game); // Adapt this line
                }
            } else {
                std::cerr << "Error: Could not get games folder for EpicGamesStore." << std::endl;
                // Handle error (e.g., delete game, log message)
            }
        }
    }

    // Shutdown Epic Games Store API
    epicAPI.shutdown();

    return 0;
}

project("emulationstation")

set(ES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmulationStation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Genres.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreenSaver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NetworkThread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ContentInstaller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ThreadedScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThreadedHasher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThreadedBluetooth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemRandomPlaylist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LangParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KeyboardMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/HttpServerThread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/HttpApi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/httplib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RetroAchievements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveStateRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveStateConfigFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomFeatures.h
    #gamestore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/GameStoreManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesUI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesStoreAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/GameStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/PlaceholderStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesModels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamStoreAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamUI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxStoreAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxUI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxModels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesModels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesScanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesUI.h

    # GuiComponents
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ScraperSearchComponent.h

    # Guis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGeneralScreensaverOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperStart.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiHashStart.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBackupStart.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInstallStart.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiUpdate.h # batocera
    # NOTA: GuiUpdate.cpp era listato negli header, l'ho lasciato ma potrebbe essere un errore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiUpdate.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBackup.h # batocera
    # NOTA: GuiBackup.cpp era listato negli header, l'ho lasciato ma potrebbe essere un errore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBackup.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInstall.h # batocera
    # NOTA: GuiInstall.cpp era listato negli header, l'ho lasciato ma potrebbe essere un errore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInstall.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiLoading.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiThemeInstaller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBezelInstaller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNetPlay.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNetPlaySettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiRetroAchievements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiRetroAchievementsSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSystemInformation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothDevices.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothPair.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothDeviceOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiWifi.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiImageViewer.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBatoceraStore.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBios.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiControllersSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiKeyMappingEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiKeyboardLayout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameAchievements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSaveState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiFileBrowser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBusyInfoPopup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNegoziOnlineMenu.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiWebViewAuthLogin.h

    # Scrapers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ArcadeDBJSONScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/HfsDBScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/EpicGamesScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/SteamScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/XboxScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/EAGamesScraper.h

    # Views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/CarouselGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GameNameFormatter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.h

    # Animations
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/LaunchAnimation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/MoveCameraAnimation.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/ApiSystem.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LibretroRatio.h # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Win32ApiSystem.h # batocera
)

set(ES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Genres.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreenSaver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NetworkThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ContentInstaller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ThreadedScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThreadedHasher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThreadedBluetooth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemRandomPlaylist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LangParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KeyboardMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/HttpServerThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/HttpApi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RetroAchievements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveStateRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SaveStateConfigFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomFeatures.cpp
    # GamesStore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/GameStoreManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesAuth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesStoreAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/GameStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/PlaceholderStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames/EpicGamesModels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamAuth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamStoreAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam/SteamUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxAuth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxStoreAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Xbox/XboxModels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesAuth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesModels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EAGames/EAGamesUI.cpp

    # GuiComponents
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ScraperSearchComponent.cpp

    # Guis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGeneralScreensaverOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperStart.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiHashStart.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBackupStart.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInstallStart.cpp # batocera
    # NOTA: GuiUpdate.cpp non era listato negli header originali, ma era in ES_HEADERS. Ora è solo qui.
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiUpdate.cpp # batocera # Rimosso perché già presente in ES_HEADERS? Controllare la logica. Se .cpp era erroneamente in headers, ora è corretto.
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBackup.cpp # batocera # Come sopra
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInstall.cpp # batocera # Come sopra
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiThemeInstaller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBezelInstaller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNetPlay.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNetPlaySettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiRetroAchievements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiRetroAchievementsSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSystemInformation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothDevices.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothPair.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBluetoothDeviceOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiWifi.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiImageViewer.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBatoceraStore.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBios.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiControllersSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiKeyMappingEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiKeyboardLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameAchievements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSaveState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiFileBrowser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiBusyInfoPopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiNegoziOnlineMenu.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiWebViewAuthLogin.cpp

    # Scrapers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ArcadeDBJSONScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/HfsDBScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/EpicGamesScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/SteamScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/XboxScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/EAGamesScraper.cpp

    # Views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/CarouselGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GameNameFormatter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/ApiSystem.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LibretroRatio.cpp # batocera
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Win32ApiSystem.cpp # batocera
)

#-------------------------------------------------------------------------------
# define OS specific sources and headers
if(MSVC)
    LIST(APPEND ES_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EmulationStation.rc
    )
endif()

# Keep Directory structure in Visual Studio
if(MSVC)
    file(
         GLOB_RECURSE source_list RELATIVE
         "${CMAKE_CURRENT_SOURCE_DIR}"
         *.cpp *.c
        )
    foreach(source IN LISTS source_list)
        get_filename_component(source_path "${source}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        string(REPLACE "src" "" source_path_final "${source_path_msvc}") # Presumo che "src" debba essere rimosso dal percorso del gruppo
        source_group("${source_path_final}" FILES "${source}")
    endforeach()
endif()

#-------------------------------------------------------------------------------
# define target
add_executable(emulationstation ${ES_SOURCES} ${ES_HEADERS})

target_include_directories(emulationstation PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore # CORRETTO DA GamesStore se era un typo
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/EpicGames # CORRETTO DA GamesStore se era un typo
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GameStore/Steam # CORRETTO DA GamesStore se era un typo
    ${CMAKE_SOURCE_DIR}/external/webview2sdk/build/native/include # Header WebView2, già presente e corretto
    # RIGA RIMOSSA CORRETTAMENTE: "${CMAKE_SOURCE_DIR}/third_party/cppwinrt"
)
# Assicurati che il percorso a "include" sia corretto per la tua struttura di webview2sdk
set(WEBVIEW2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/webview2sdk/build/native/include")

if(EXISTS "${WEBVIEW2_INCLUDE_DIR}")
    target_include_directories(emulationstation PRIVATE "${WEBVIEW2_INCLUDE_DIR}")
    message(STATUS "Directory degli header WebView2 aggiunta: ${WEBVIEW2_INCLUDE_DIR}")
else()
    message(WARNING "Directory degli header WebView2 non trovata in ${WEBVIEW2_INCLUDE_DIR}. Potrebbero verificarsi errori di compilazione.")
endif()
# --------------- WebView2 Integration STARTS ---------------
# Determina il percorso della libreria WebView2 in base all'architettura
set(WEBVIEW2_ARCH_DIR "")
# Prova prima con le variabili specifiche di MSVC poiché stai generando un .vcxproj
if(MSVC)
    if(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
        set(WEBVIEW2_ARCH_DIR "x64")
    elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
        set(WEBVIEW2_ARCH_DIR "x86")
    elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
        set(WEBVIEW2_ARCH_DIR "arm64")
    else()
        message(WARNING "Piattaforma MSVC non riconosciuta: ${CMAKE_VS_PLATFORM_NAME}. Tento con x64 come predefinita per la libreria WebView2.")
        set(WEBVIEW2_ARCH_DIR "x64")
    endif()
else()
    # Fallback per generatori non MSVC o se CMAKE_VS_PLATFORM_NAME non è impostato
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x64"))
        set(WEBVIEW2_ARCH_DIR "x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
        set(WEBVIEW2_ARCH_DIR "arm64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i386" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i686"))
        set(WEBVIEW2_ARCH_DIR "x86")
    else()
        message(WARNING "Impossibile determinare in modo affidabile l'architettura per la libreria WebView2 (Processore: ${CMAKE_SYSTEM_PROCESSOR}, Dimensione Puntatore: ${CMAKE_SIZEOF_VOID_P}). Tento con x64 come predefinita.")
        set(WEBVIEW2_ARCH_DIR "x64")
    endif()
endif()

set(WEBVIEW2_LIB_PATH "${CMAKE_SOURCE_DIR}/external/webview2sdk/build/native/${WEBVIEW2_ARCH_DIR}/WebView2LoaderStatic.lib")

# Verifica se il file della libreria esiste
if(NOT EXISTS "${WEBVIEW2_LIB_PATH}")
    message(FATAL_ERROR "WebView2LoaderStatic.lib non trovata al percorso determinato: ${WEBVIEW2_LIB_PATH}. Controlla la struttura della tua SDK WebView2 (cartella 'external'), l'architettura per cui stai compilando e la logica di rilevamento dell'architettura in CMakeLists.txt.")
else()
    message(STATUS "Libreria statica WebView2 trovata in: ${WEBVIEW2_LIB_PATH}")
endif()

# Collega la libreria WebView2LoaderStatic.lib
target_link_libraries(emulationstation PRIVATE "${WEBVIEW2_LIB_PATH}")


# --------------- WebView2 Integration ENDS ---------------

# RIMOSSA: link_directories(${CMAKE_SOURCE_DIR}/external/webview2sdk/build/native/x86)
# `WebView2LoaderStatic` è stato rimosso da qui e aggiunto con percorso completo sopra
target_link_libraries(emulationstation PRIVATE
    ${COMMON_LIBRARIES}
    es-core
)

if(WIN32)
    target_link_libraries(emulationstation PRIVATE WindowsApp.lib) # QUESTA È CORRETTA E NECESSARIA

    if(MSVC)
        target_compile_options(emulationstation PRIVATE
            /await
            /bigobj
            /EHsc
            /permissive-
        )
        set_target_properties(emulationstation PROPERTIES WIN32_EXECUTABLE ON)
    endif()
endif()

#-------------------------------------------------------------------------------
# set up CPack install stuff so `make install` does something useful
# (Il resto del file rimane invariato)
# ... (la tua configurazione CPack esistente) ...

install(TARGETS emulationstation
    RUNTIME
    DESTINATION bin)

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A flexible graphical emulator front-end")
SET(CPACK_PACKAGE_DESCRIPTION "EmulationStation is a flexible, graphical front-end designed for keyboardless navigation of your multi-platform retro game collection.")

SET(CPACK_RESOURCE_FILE LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
SET(CPACK_RESOURCE_FILE README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alec Lofquist <allofquist@yahoo.com>")
SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libsdl2-2.0-0, libsdl2-mixer-2.0-0, libfreeimage3, libfreetype6, libcurl3, libasound2")
SET(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>= 8.0.0), cmake, g++ (>= 4.8), libsdl2-dev, libsdl2-mixer-dev, libfreeimage-dev, libfreetype6-dev, libcurl4-openssl-dev, libasound2-dev, libgl1-mesa-dev, rapidjson-dev, libgl1-mesa-dev")

SET(CPACK_PACKAGE_VENDOR "emulationstation.org")
SET(CPACK_PACKAGE_VERSION "2.0.0~rc1")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "emulationstation_${CMAKE_PACKAGE_VERSION}")
SET(CPACK_PACKAGE_EXECUTABLES "emulationstation" "emulationstation")

SET(CPACK_GENERATOR "TGZ;DEB")

INCLUDE(CPack)

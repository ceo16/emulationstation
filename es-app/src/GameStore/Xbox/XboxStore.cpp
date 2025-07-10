#define NOMINMAX
#include <winrt/base.h> // Messo qui
#include "GameStore/Xbox/XboxStore.h"
#include "GameStore/Xbox/XboxUI.h"
#include "Log.h"
#include "SystemData.h"
#include "FileData.h"
#include "MetaData.h"
#include "Window.h"
#include "Settings.h"
#include "SdlEvents.h"
#include "utils/StringUtil.h"
#include "utils/Platform.h"
#include "utils/TimeUtil.h"
#include "PlatformId.h"
#include "guis/GuiMsgBox.h"
#include "LocaleES.h"
#include "views/ViewController.h"
#include <string>       // Necessario per std::wstring
#include <combaseapi.h>
#include <locale>


#include <set>
#include <algorithm>
#include <future>

#ifdef _WIN32
#include <ShObjIdl.h>
#include <appmodel.h>
#include <PathCch.h>
#pragma comment(lib, "Pathcch.lib")

std::string ConvertWideToUtf8_XboxStore_Unique(const WCHAR* wideString) {
    if (wideString == nullptr) return "";
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) return "";
    std::string utf8String(bufferSize - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideString, -1, &utf8String[0], bufferSize, NULL, NULL);
    return utf8String;
}

const std::set<std::wstring> pfnExclusionListGlobal = {
    L"Microsoft.WindowsCalculator_8wekyb3d8bbwe", L"microsoft.windowscommunicationsapps_8wekyb3d8bbwe",
    L"Microsoft.Windows.Photos_8wekyb3d8bbwe", L"Microsoft.MicrosoftEdge_8wekyb3d8bbwe",
    L"Microsoft.MicrosoftEdge.Stable_8wekyb3d8bbwe", L"Microsoft.WindowsStore_8wekyb3d8bbwe",
    L"Microsoft.StorePurchaseApp_8wekyb3d8bbwe", L"Microsoft.XboxApp_8wekyb3d8bbwe",
    L"Microsoft.GamingApp_8wekyb3d8bbwe",
    L"Microsoft.XboxGamingOverlay_8wekyb3d8bbwe",
    L"Microsoft.XboxSpeechToTextOverlay_8wekyb3d8bbwe",
    L"Microsoft.XboxIdentityProvider_8wekyb3d8bbwe",
    L"Microsoft.YourPhone_8wekyb3d8bbwe",
    L"Microsoft.WindowsTerminal_8wekyb3d8bbwe",
    L"Microsoft.ScreenSketch_8wekyb3d8bbwe",
    L"Microsoft.WindowsSoundRecorder_8wekyb3d8bbwe",
    L"Microsoft.ZuneVideo_8wekyb3d8bbwe",
    L"Microsoft.ZuneMusic_8wekyb3d8bbwe",
    L"Microsoft.Office.OneNote_8wekyb3d8bbwe",
    L"Microsoft.People_8wekyb3d8bbwe",
    L"Microsoft.Wallet_8wekyb3d8bbwe",
    L"Microsoft.GetHelp_8wekyb3d8bbwe",
    L"Microsoft.Getstarted_8wekyb3d8bbwe",
    L"Microsoft.WindowsMaps_8wekyb3d8bbwe",
    L"Microsoft.549981C3F5F10_8wekyb3d8bbwe",
    L"Microsoft.WindowsAlarms_8wekyb3d8bbwe",
    L"Microsoft.WindowsCamera_8wekyb3d8bbwe",
    L"Microsoft.SkypeApp_kzf8qxf38zg5c",
    L"Microsoft.MSPaint_8wekyb3d8bbwe",
    L"Microsoft.VP9VideoExtensions_8wekyb3d8bbwe",
    L"Microsoft.WebMediaExtensions_8wekyb3d8bbwe",
    L"Microsoft.HEIFImageExtension_8wekyb3d8bbwe",
    L"Microsoft.OutlookForWindows_8wekyb3d8bbwe",
    L"windows.immersivecontrolpanel_cw5n1h2txyewy",
    L"BytedancePte.Ltd.TikTok_6yccndn6064se",
    L"Disney.37853FC22B2CE_6rarf9sa4v8jt",
    L"NVIDIACorp.NVIDIAControlPanel_56jybvy8sckqj",
    L"Microsoft.SecHealthUI_8wekyb3d8bbwe",
    L"RealtekSemiconductorCorp.RealtekAudioControl_dt26b99r8h8gj",
    L"Microsoft.BingNews_8wekyb3d8bbwe",
    L"AppUp.IntelGraphicsExperience_8j3eq9eme6ctt",
    L"Microsoft.XboxDevices_8wekyb3d8bbwe",
    L"FACEBOOK.317180B0BB486_8xx8rvfyw5nnt",
    L"Microsoft.BingWeather_8wekyb3d8bbwe",
    L"Microsoft.PowerAutomateDesktop_8wekyb3d8bbwe",
    L"Clipchamp.Clipchamp_yxz26nhyzhsrt",
    L"MicrosoftCorporationII.QuickAssist_8wekyb3d8bbwe",
    L"Microsoft.MicrosoftStickyNotes_8wekyb3d8bbwe",
    L"9426MICRO-STARINTERNATION.MSICenter_kzh8wxbdkxb8p",
    L"MSTeams_8wekyb3d8bbwe",
    L"Microsoft.WindowsFeedbackHub_8wekyb3d8bbwe",
    L"AmazonVideo.PrimeVideo_pwbj9vvecjh7j",
    L"Microsoft.WindowsNotepad_8wekyb3d8bbwe",
    L"MicrosoftWindows.Client.CBS_cw5n1h2txyewy",
    L"MicrosoftWindows.Client.CoreAI_cw5n1h2txyewy",
    L"SpotifyAB.SpotifyMusic_zpdnekdrzrea0",
    L"Microsoft.Todos_8wekyb3d8bbwe",
    L"Microsoft.MicrosoftOfficeHub_8wekyb3d8bbwe",
    L"Microsoft.Paint_8wekyb3d8bbwe"
};
#endif

XboxStore::XboxStore(XboxAuth* auth, Window* window_param)
    : GameStore(), mAuth(auth), mAPI(nullptr), mInstanceWindow(window_param), _initialized(false)
#ifdef _WIN32
    , mComInitialized(false)
#endif
{
#ifdef _WIN32
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        mComInitialized = true;
    } catch (const winrt::hresult_error& e) {
        LOG(LogError) << "!!! FAILED TO INITIALIZE COM IN XBOXSTORE CONSTRUCTOR: " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str());
        mComInitialized = false;
    }
#endif
    if (mAuth) {
        mAPI = new XboxStoreAPI(mAuth);
    } else { LOG(LogError) << "XboxStore created with null XboxAuth pointer!"; }
    LOG(LogDebug) << "XboxStore constructor finished.";
}

XboxStore::~XboxStore() {
#ifdef _WIN32
    if (mComInitialized) { winrt::uninit_apartment(); mComInitialized = false; }
#endif
    delete mAPI; mAPI = nullptr;
    LOG(LogDebug) << "XboxStore destructor finished.";
}

bool XboxStore::init(Window* window_param_from_manager) {
    if (!this->mInstanceWindow && window_param_from_manager) this->mInstanceWindow = window_param_from_manager;
    else if (window_param_from_manager && this->mInstanceWindow != window_param_from_manager) LOG(LogWarning) << "XboxStore::init - Window mismatch.";
    if (!mAuth || !mAPI) { LOG(LogError) << "XboxStore::init - Auth or API module missing."; _initialized = false; return false; }
    if (!this->mInstanceWindow) { LOG(LogError) << "XboxStore::init - Window reference null."; _initialized = false; return false; }
    LOG(LogInfo) << "XboxStore initialized successfully."; _initialized = true; return true;
}

void XboxStore::shutdown() { LOG(LogInfo) << "XboxStore shutting down."; _initialized = false; }

void XboxStore::showStoreUI(Window* window_context_param) {
    Window* targetWindow = this->mInstanceWindow ? this->mInstanceWindow : window_context_param;
    if (!targetWindow) { LOG(LogError) << "XboxStore::showStoreUI - Window context null."; return; }
    if (!_initialized) {
        LOG(LogError) << "XboxStore::showStoreUI - Store not initialized.";
        targetWindow->pushGui(new GuiMsgBox(targetWindow, _("XBOX ERROR") + std::string("\n") + _("Xbox store not properly initialized."), _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
        return;
    }
    targetWindow->pushGui(new XboxUI(targetWindow, this));
}

// MODIFICA 1: Restituisce il nome corretto in minuscolo
std::string XboxStore::getStoreName() const { return "xboxstore"; }

std::string XboxStore::getGameLaunchCommand(const std::string& aumid) {
    if (aumid.empty()) return "";
    return "explorer.exe shell:AppsFolder\\" + aumid;
}

bool XboxStore::launchGame(const std::string& gameId_AUMID) {
    LOG(LogInfo) << "XboxStore::launchGame (virtual) called with AUMID: " << gameId_AUMID;
    return launchGameByAumid(gameId_AUMID);
}

bool XboxStore::launchGameByAumid(const std::string& aumid) {
#ifndef _WIN32
    LOG(LogError) << "XboxStore: Game launching only on Windows."; return false;
#else
    if (!mComInitialized) { LOG(LogError) << "XboxStore: COM not initialized for launch."; return false; }
    if (aumid.empty()) { LOG(LogError) << "XboxStore: AUMID empty for launch."; return false; }
    LOG(LogInfo) << "XboxStore: Attempting to launch AUMID: " << aumid;
    try {
        std::wstring wideAumid = Utils::String::convertToWideString(aumid);
        winrt::com_ptr<IApplicationActivationManager> activationManager;
        HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(activationManager.put()));
        if (SUCCEEDED(hr)) {
            DWORD processId = 0;
            hr = activationManager->ActivateApplication(wideAumid.c_str(), nullptr, AO_NONE, &processId);
            if (SUCCEEDED(hr)) {
                LOG(LogInfo) << "XboxStore: Game launched successfully via ActivateApplication. PID: " << processId;
                return true;
            }
            else { LOG(LogError) << "XboxStore: ActivateApplication failed. AUMID: " << aumid << ", HRESULT: 0x" << std::hex << hr; }
        } else { LOG(LogError) << "XboxStore: CoCreateInstance for IApplicationActivationManager failed. HRESULT: 0x" << std::hex << hr; }
    } catch (const winrt::hresult_error& e) {
        LOG(LogError) << "XboxStore: WinRT exception during launch for AUMID " << aumid << ": " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str()) << " HRESULT: 0x" << std::hex << e.code();
    } catch (const std::exception& ex) {
        LOG(LogError) << "XboxStore: Standard exception during launch for AUMID " << aumid << ": " << ex.what();
    } catch (...) {
        LOG(LogError) << "XboxStore: Unknown exception during launch for AUMID " << aumid;
    }
    return false;
#endif
}



std::vector<Xbox::InstalledXboxGameInfo> XboxStore::findInstalledXboxGames() {
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    if (FAILED(hr)) {
        LOG(LogError) << "XboxStore: CRITICO - Fallita inizializzazione COM. HRESULT: 0x" << std::hex << hr;
        return {};
    }

    LOG(LogInfo) << "[XboxStore] Avvio scansione puramente locale del sistema per i giochi installati...";
    
    // Chiama la funzione helper che ora è autonoma e non si connette a internet.
    std::vector<Xbox::InstalledXboxGameInfo> games = findInstalledGames_PackageManagerHelper();

    CoUninitialize();
    return games;

#else
    LOG(LogWarning) << "XboxStore::findInstalledXboxGames() - Solo su Windows.";
    return {};
#endif
}

// 2. QUESTA FUNZIONE ORA HA UN FILTRO LOCALE PIÙ INTELLIGENTE
std::vector<Xbox::InstalledXboxGameInfo> XboxStore::findInstalledGames_PackageManagerHelper() {
    std::vector<Xbox::InstalledXboxGameInfo> installedGames;
#ifdef _WIN32
    std::locale loc;
    
    // Lista di stringhe che, se trovate nel nome del publisher, identificano un gioco.
    const std::vector<std::wstring> knownGamePublishers = {
        L"annapurna", L"sega", L"bandai namco", L"ubisoft", L"electronic arts", 
        L"bethesda", L"square enix", L"capcom", L"riot games", L"blizzard",
        L"team17", L"devolver", L"paradox", L"slitherine", L"tinybuild", L"dotemu"
    };

    LOG(LogInfo) << "[XboxStore-PMHelper] Avvio scansione completa con filtro locale a più livelli...";

    try {
        winrt::Windows::Management::Deployment::PackageManager packageManager;
        auto allUserPackages = packageManager.FindPackagesForUser(L"");

        for (const auto& package : allUserPackages) {
            if (package.IsFramework() || package.IsResourcePackage() || package.IsBundle() ||
                package.Status().NotAvailable() || package.Status().DeploymentInProgress() ||
                pfnExclusionListGlobal.count(package.Id().FamilyName().c_str())) {
                continue; 
            }

            try {
                auto appEntries = package.GetAppListEntriesAsync().get();
                if (!appEntries.First().HasCurrent()) {
                    continue;
                }

                bool isGame = false;

                // CRITERIO 1: Dipendenza da Xbox Live (il più affidabile)
                for (const auto& dep : package.Dependencies()) {
                    std::wstring dependencyName(dep.Id().Name().c_str());
                    if (dependencyName.find(L"Microsoft.Xbox.Live") != std::wstring::npos) {
                        isGame = true;
                        LOG(LogDebug) << "Identificato via dipendenza Xbox Live: " << ConvertWideToUtf8_XboxStore_Unique(package.DisplayName().c_str());
                        break;
                    }
                }
                
                // CRITERIO 2: Publisher conosciuto (per giochi come Flock)
                if (!isGame) {
                    std::wstring publisherNameLower = package.PublisherDisplayName().c_str();
                    std::transform(publisherNameLower.begin(), publisherNameLower.end(), publisherNameLower.begin(),
                        [&loc](wchar_t c) { return std::tolower(c, loc); });
                    
                    for (const auto& publisher : knownGamePublishers) {
                        if (publisherNameLower.find(publisher) != std::wstring::npos) {
                            isGame = true;
                            LOG(LogDebug) << "Identificato via publisher '" << ConvertWideToUtf8_XboxStore_Unique(publisher.c_str()) << "': " << ConvertWideToUtf8_XboxStore_Unique(package.DisplayName().c_str());
                            break;
                        }
                    }
                }

                // CRITERIO 3: Nome che contiene "game" (per casi come "Solitaire & Casual Games")
                if (!isGame) {
                    std::wstring displayNameLower = package.DisplayName().c_str();
                    std::transform(displayNameLower.begin(), displayNameLower.end(), displayNameLower.begin(),
                        [&loc](wchar_t c) { return std::tolower(c, loc); });

                    if (displayNameLower.find(L"game") != std::wstring::npos) {
                        isGame = true;
                        LOG(LogDebug) << "Identificato via nome 'game': " << ConvertWideToUtf8_XboxStore_Unique(package.DisplayName().c_str());
                    }
                }

                if (isGame) {
                    auto mainAppEntry = appEntries.First().Current();
                    
                    Xbox::InstalledXboxGameInfo gameInfo;
                    gameInfo.displayName = ConvertWideToUtf8_XboxStore_Unique(mainAppEntry.DisplayInfo().DisplayName().c_str());
                    gameInfo.aumid = ConvertWideToUtf8_XboxStore_Unique(mainAppEntry.AppUserModelId().c_str());
                    gameInfo.pfn = ConvertWideToUtf8_XboxStore_Unique(package.Id().FamilyName().c_str());
                    gameInfo.packageFullName = ConvertWideToUtf8_XboxStore_Unique(package.Id().FullName().c_str());
                    gameInfo.isInstalled = true;
                    winrt::Windows::Storage::StorageFolder instLoc = package.InstalledLocation();
                    gameInfo.installLocation = instLoc ? ConvertWideToUtf8_XboxStore_Unique(instLoc.Path().c_str()) : "";

                    std::string tempAppId = "App";
                    size_t bang_pos = gameInfo.aumid.find('!');
                    if (bang_pos != std::string::npos && bang_pos + 1 < gameInfo.aumid.length()) {
                        tempAppId = gameInfo.aumid.substr(bang_pos + 1);
                    }
                    gameInfo.applicationId = tempAppId;

                    LOG(LogInfo) << "[XboxStore-PMHelper] Gioco Identificato: \"" << gameInfo.displayName << "\"";
                    installedGames.push_back(gameInfo);
                }

            } catch (const winrt::hresult_error&) {
                // Ignora errori
            }
        }
    } catch (const winrt::hresult_error& e) {
        LOG(LogError) << "[XboxStore-PMHelper] Eccezione Critica: " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str());
    }

    LOG(LogInfo) << "[XboxStore-PMHelper] Scansione terminata. Trovati " << installedGames.size() << " giochi.";
#endif
    return installedGames;
}


std::vector<FileData*> XboxStore::getGamesList() {
    LOG(LogDebug) << "XboxStore::getGamesList() called.";
    std::vector<FileData*> gameFiles;
    // MODIFICA 2: Usa il nome corretto del sistema
    SystemData* system = SystemData::getSystem("xboxstore");
    if (!system) { LOG(LogError) << "XboxStore::getGamesList - System 'xboxstore' not found!"; return gameFiles; }
    if (!_initialized || !mAuth || !mAPI) { LOG(LogError) << "XboxStore::getGamesList - Store not initialized or Auth/API missing!"; return gameFiles; }

    std::map<std::string, FileData*> existingGamesMap;
    FolderData* root = system->getRootFolder();
    if (root) {
        for (auto* fd_base : root->getChildren()) {
            FileData* fd = dynamic_cast<FileData*>(fd_base);
            if (fd && fd->getType() == GAME) {
                std::string key = fd->getMetadata().get(MetaDataId::XboxAumid);
                if (key.empty() || fd->getMetadata().get(MetaDataId::Virtual) == "true") {
                    key = fd->getPath();
                }
                if (!key.empty()) {
                    existingGamesMap[key] = fd;
                }
            }
        }
    }
    LOG(LogDebug) << "XboxStore::getGamesList - Found " << existingGamesMap.size() << " existing FileData entries (indexed by AUMID or PseudoPath).";

    std::vector<Xbox::InstalledXboxGameInfo> installedApps = findInstalledXboxGames();
    LOG(LogInfo) << "XboxStore::getGamesList - Detected " << installedApps.size() << " UWP apps from findInstalledXboxGames.";

    for (const auto& installedApp : installedApps) {
        if (installedApp.aumid.empty()) {
            LOG(LogWarning) << "[XboxStore GetList] Skipping installed app with empty AUMID. Name: " << installedApp.displayName;
            continue;
        }
        auto it = existingGamesMap.find(installedApp.aumid);
        if (it != existingGamesMap.end()) {
            FileData* fd = it->second;
            LOG(LogDebug) << "XboxStore: Updating existing game in FileData: " << installedApp.displayName << " (AUMID: " << installedApp.aumid << ")";
            bool changed = false;
            if (fd->getMetadata().get(MetaDataId::Name) != installedApp.displayName) { fd->getMetadata().set(MetaDataId::Name, installedApp.displayName); changed = true; }
            if (fd->getMetadata().get(MetaDataId::Installed) != "true") { fd->getMetadata().set(MetaDataId::Installed, "true"); changed = true; }
            if (fd->getMetadata().get(MetaDataId::Virtual) != "false") { fd->getMetadata().set(MetaDataId::Virtual, "false"); changed = true; }
            if (fd->getMetadata().get(MetaDataId::XboxPfn) != installedApp.pfn) { fd->getMetadata().set(MetaDataId::XboxPfn, installedApp.pfn); changed = true; }
            if (fd->getMetadata().get(MetaDataId::XboxAumid) != installedApp.aumid) { fd->getMetadata().set(MetaDataId::XboxAumid, installedApp.aumid); changed = true; }
            
            if (fd->getPath() != installedApp.aumid) {
                 LOG(LogWarning) << "[XboxStore GetList] FileData for " << installedApp.displayName << " has path " << fd->getPath() << " but its canonical AUMID is " << installedApp.aumid << ". Consider Gamelist cleanup.";
            }
            if (fd->getMetadata().get(MetaDataId::LaunchCommand) != installedApp.aumid) { fd->getMetadata().set(MetaDataId::LaunchCommand, installedApp.aumid); changed = true; }
            
            if (changed) fd->getMetadata().setDirty();
            gameFiles.push_back(fd);
            existingGamesMap.erase(it);
        } else {
            LOG(LogInfo) << "XboxStore: Adding new installed game to FileData list: " << installedApp.displayName << " (AUMID: " << installedApp.aumid << ")";
            FileData* newGame = new FileData(FileType::GAME, installedApp.aumid, system); // Path = AUMID
            newGame->getMetadata().set(MetaDataId::Name, installedApp.displayName);
            newGame->getMetadata().set(MetaDataId::XboxPfn, installedApp.pfn);
            newGame->getMetadata().set(MetaDataId::XboxAumid, installedApp.aumid);
            newGame->getMetadata().set(MetaDataId::Installed, "true");
            newGame->getMetadata().set(MetaDataId::Virtual, "false");
            newGame->getMetadata().set(MetaDataId::LaunchCommand, installedApp.aumid); // LaunchCommand = AUMID puro
            newGame->getMetadata().setDirty();
            gameFiles.push_back(newGame);
        }
    }

    if (mAuth && mAuth->isAuthenticated()) {
        LOG(LogDebug) << "XboxStore: Fetching online library titles for virtual games...";
        std::vector<Xbox::OnlineTitleInfo> onlineTitles = mAPI->GetLibraryTitles();
        LOG(LogDebug) << "XboxStore: Fetched " << onlineTitles.size() << " titles from online library.";
        for (const auto& title : onlineTitles) {
            bool isPCGame = false;
            for (const std::string& device : title.devices) if (device == "PC") { isPCGame = true; break; }
            
            if (!isPCGame || (title.pfn.empty() && title.detail.productId.empty())) {
                LOG(LogDebug) << "[XboxStore GetList] Skipping online title '" << title.name
                    << "' (Not PC, or missing PFN and ProductID for virtual entry).";
                continue;
            }

            if (!title.pfn.empty()) {
                bool alreadyProcessedAsInstalled = false;
                for(const auto& gf : gameFiles) {
                    if(gf->getMetadata().get(MetaDataId::XboxPfn) == title.pfn && gf->getMetadata().get(MetaDataId::Installed) == "true") {
                        alreadyProcessedAsInstalled = true;
                        LOG(LogDebug) << "[XboxStore GetList] Online title '" << title.name << "' (PFN: " << title.pfn << ") already processed as installed. Skipping virtual entry.";
                        break;
                    }
                }
                if(alreadyProcessedAsInstalled) continue;
            }
            
            std::string pseudoPathForOnline;
            std::string storeLink;

            if (!title.detail.productId.empty()) {
                pseudoPathForOnline = "xbox_online_prodid:/" + title.detail.productId;
                storeLink = "ms-windows-store://pdp/?ProductId=" + title.detail.productId;
            } else if (!title.pfn.empty()) {
                pseudoPathForOnline = "xbox_online_pfn:/" + title.pfn;
                storeLink = "ms-windows-store://pdp/?PFN=" + title.pfn;
            } else {
                 LOG(LogWarning) << "[XboxStore GetList] Could not create a valid pseudoPath/storeLink for online game: " << title.name;
                 continue;
            }
            LOG(LogDebug) << "[XboxStore GetList] Processing online title '" << title.name << "' with pseudoPath: '" << pseudoPathForOnline << "'";

            auto it_virtual = existingGamesMap.find(pseudoPathForOnline);
            if (it_virtual != existingGamesMap.end()) {
                FileData* fd = it_virtual->second;
                LOG(LogDebug) << "XboxStore: Updating existing virtual game in FileData: " << title.name;
                bool changed = false;
                if (fd->getMetadata().get(MetaDataId::Installed) != "false") { fd->getMetadata().set(MetaDataId::Installed, "false"); changed = true; }
                if (fd->getMetadata().get(MetaDataId::Virtual) != "true") { fd->getMetadata().set(MetaDataId::Virtual, "true"); changed = true; }
                if (fd->getMetadata().get(MetaDataId::Name) != title.name && !title.name.empty()) { fd->getMetadata().set(MetaDataId::Name, title.name); changed = true; }
                if (!title.pfn.empty() && fd->getMetadata().get(MetaDataId::XboxPfn) != title.pfn) { fd->getMetadata().set(MetaDataId::XboxPfn, title.pfn); changed = true; }
                if (!title.detail.productId.empty() && fd->getMetadata().get(MetaDataId::XboxProductId) != title.detail.productId) { fd->getMetadata().set(MetaDataId::XboxProductId, title.detail.productId); changed = true; }
                if (!title.titleId.empty() && fd->getMetadata().get(MetaDataId::XboxTitleId) != title.titleId) { fd->getMetadata().set(MetaDataId::XboxTitleId, title.titleId); changed = true; }
                if (!storeLink.empty() && fd->getMetadata().get(MetaDataId::LaunchCommand) != storeLink) { fd->getMetadata().set(MetaDataId::LaunchCommand, storeLink); changed = true; }

                if (changed) fd->getMetadata().setDirty();
                if (std::find(gameFiles.begin(), gameFiles.end(), fd) == gameFiles.end()) gameFiles.push_back(fd);
                existingGamesMap.erase(it_virtual);
            } else {
                LOG(LogInfo) << "XboxStore: Adding new (virtual) library game: " << title.name << " with path " << pseudoPathForOnline;
                FileData* newGame = new FileData(FileType::GAME, pseudoPathForOnline, system);
                newGame->getMetadata().set(MetaDataId::Name, title.name.empty() ? (title.pfn.empty() ? title.detail.productId : title.pfn) : title.name);
                if (!title.pfn.empty()) newGame->getMetadata().set(MetaDataId::XboxPfn, title.pfn);
                if (!title.titleId.empty()) newGame->getMetadata().set(MetaDataId::XboxTitleId, title.titleId);
                if (!title.detail.productId.empty()) newGame->getMetadata().set(MetaDataId::XboxProductId, title.detail.productId);
                newGame->getMetadata().set(MetaDataId::Installed, "false");
                newGame->getMetadata().set(MetaDataId::Virtual, "true");
                if (!storeLink.empty()) newGame->getMetadata().set(MetaDataId::LaunchCommand, storeLink);
                else { LOG(LogWarning) << "XboxStore: No store link for new virtual game: " << title.name; }

                newGame->getMetadata().setDirty();
                gameFiles.push_back(newGame);
            }
        }
    } else { LOG(LogWarning) << "XboxStore::getGamesList - Not authenticated, skipping online library."; }

    for (const auto& pair : existingGamesMap) {
        FileData* fd = pair.second;
        LOG(LogDebug) << "XboxStore: Including game from gamelist.xml not matched with installed/online: " << fd->getName() << " (Path: " << fd->getPath() << ")";
        if (std::find(gameFiles.begin(), gameFiles.end(), fd) == gameFiles.end()) {
            bool changed = false;
            if (fd->getMetadata().get(MetaDataId::Installed) != "false") { fd->getMetadata().set(MetaDataId::Installed, "false"); changed = true; }
            if (fd->getMetadata().get(MetaDataId::Virtual) != "true") { fd->getMetadata().set(MetaDataId::Virtual, "true"); changed = true; }
            if (changed) fd->getMetadata().setDirty();
            gameFiles.push_back(fd);
        }
    }
    LOG(LogInfo) << "XboxStore::getGamesList - Returning " << gameFiles.size() << " FileData entries.";
    return gameFiles;
}

std::future<void> XboxStore::refreshGamesListAsync() {
    return std::async(std::launch::async, [this]() {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        LOG(LogInfo) << "Xbox Store Refresh BG: Starting refreshGamesListAsync...";
        // MODIFICA 3: Usa il nome corretto del sistema
        SystemData* xboxSystem = SystemData::getSystem("xboxstore");

        if (!_initialized || !mAuth || !mAPI) {
            LOG(LogError) << "Xbox Store Refresh BG: Store not ready (uninitialized, no auth, or no API).";
            if (xboxSystem) {
                auto emptyPayload = new std::vector<NewXboxGameData>();
                SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
                event.user.code = SDL_XBOX_REFRESH_COMPLETE;
                event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
                SDL_PushEvent(&event);
            }
            winrt::uninit_apartment(); return;
        }
        if (!mAuth->isAuthenticated()) {
            LOG(LogWarning) << "Xbox Store Refresh BG: Not authenticated. Aborting refresh.";
            if (xboxSystem) {
                auto emptyPayload = new std::vector<NewXboxGameData>();
                SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
                event.user.code = SDL_XBOX_REFRESH_COMPLETE;
                event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
                SDL_PushEvent(&event);
            }
             winrt::uninit_apartment(); return;
        }

        if (!xboxSystem || !xboxSystem->getRootFolder()) {
            LOG(LogError) << "Xbox Store Refresh BG: Cannot find Xbox system or its root folder.";
             winrt::uninit_apartment(); return;
        }

        std::set<std::string> existingAumidsInSystem_refresh;
        std::map<std::string, FileData*> fileDataMapByAumidOrPseudoPath_refresh;

        try {
             std::vector<FileData*> currentSystemGames = xboxSystem->getRootFolder()->getFilesRecursive(GAME, true);
             for (FileData* fd : currentSystemGames) {
                 if (fd) {
                     std::string key = fd->getMetadata().get(MetaDataId::XboxAumid);
                     if (key.empty() || fd->getMetadata().get(MetaDataId::Virtual) == "true") {
                         key = fd->getPath();
                     }

                     if (!key.empty()) {
                         fileDataMapByAumidOrPseudoPath_refresh[key] = fd;
                         if (!fd->getMetadata().get(MetaDataId::XboxAumid).empty() && fd->getMetadata().get(MetaDataId::Installed) == "true") {
                             existingAumidsInSystem_refresh.insert(fd->getMetadata().get(MetaDataId::XboxAumid));
                         }
                     }
                 }
             }
             LOG(LogDebug) << "Xbox Store Refresh BG: Found " << fileDataMapByAumidOrPseudoPath_refresh.size() << " existing FileData entries and " << existingAumidsInSystem_refresh.size() << " existing installed AUMIDs in system '" << xboxSystem->getName() << "'.";
        } catch (const std::exception& e) {
            LOG(LogError) << "Xbox Store Refresh BG: Exception collecting existing FileData: " << e.what();
            auto emptyPayload = new std::vector<NewXboxGameData>();
            SDL_Event event; SDL_zero(event); event.type = SDL_USEREVENT;
            event.user.code = SDL_XBOX_REFRESH_COMPLETE;
            event.user.data1 = emptyPayload; event.user.data2 = xboxSystem;
            SDL_PushEvent(&event);
            winrt::uninit_apartment(); return;
        }

        auto newGamesPayload = new std::vector<NewXboxGameData>();
        bool metadataPotentiallyChanged = false;
        std::set<std::string> processedPfnsForVirtual_refresh;

        std::vector<Xbox::InstalledXboxGameInfo> installedGames = findInstalledXboxGames();
        LOG(LogInfo) << "Xbox Store Refresh BG: Found " << installedGames.size() << " installed UWP apps via findInstalledXboxGames.";
        for (const auto& installedGame : installedGames) {
            if (installedGame.aumid.empty()) {
                 LOG(LogWarning) << "Xbox Store Refresh BG: Skipping installed game with empty AUMID: " << installedGame.displayName;
                continue;
            }
            processedPfnsForVirtual_refresh.insert(installedGame.pfn);

            auto it = fileDataMapByAumidOrPseudoPath_refresh.find(installedGame.aumid);
            if (it == fileDataMapByAumidOrPseudoPath_refresh.end()) {
                NewXboxGameData data;
                data.pfn = installedGame.pfn;
                data.pseudoPath = installedGame.aumid;
                data.metadataMap[MetaDataId::Name] = installedGame.displayName;
                data.metadataMap[MetaDataId::XboxPfn] = installedGame.pfn;
                data.metadataMap[MetaDataId::XboxAumid] = installedGame.aumid;
                data.metadataMap[MetaDataId::Installed] = "true";
                data.metadataMap[MetaDataId::Virtual] = "false";
                data.metadataMap[MetaDataId::LaunchCommand] = installedGame.aumid;
                data.metadataMap[MetaDataId::Path] = installedGame.aumid;
                newGamesPayload->push_back(data);
                LOG(LogDebug) << "  Payload Add (Installed New): " << installedGame.displayName << " (AUMID: " << installedGame.aumid << ")";
            } else {
                FileData* fd = it->second;
                bool changed = false;
                if (fd->getMetadata().get(MetaDataId::Installed) != "true") { fd->getMetadata().set(MetaDataId::Installed, "true"); changed = true; }
                if (fd->getMetadata().get(MetaDataId::Virtual) != "false") { fd->getMetadata().set(MetaDataId::Virtual, "false"); changed = true; }
                if (fd->getMetadata().get(MetaDataId::LaunchCommand) != installedGame.aumid) { fd->getMetadata().set(MetaDataId::LaunchCommand, installedGame.aumid); changed = true; }
                if (fd->getMetadata().get(MetaDataId::XboxAumid) != installedGame.aumid) { fd->getMetadata().set(MetaDataId::XboxAumid, installedGame.aumid); changed = true; }
                if (fd->getMetadata().get(MetaDataId::XboxPfn) != installedGame.pfn) { fd->getMetadata().set(MetaDataId::XboxPfn, installedGame.pfn); changed = true; }
                if (changed) {
                    fd->getMetadata().setDirty();
                    metadataPotentiallyChanged = true;
                    LOG(LogDebug) << "  Marked existing game as Installed & updated metadata: " << installedGame.displayName;
                }
            }
        }

        std::vector<Xbox::OnlineTitleInfo> onlineTitles = mAPI->GetLibraryTitles();
        LOG(LogInfo) << "Xbox Store Refresh BG: Fetched " << onlineTitles.size() << " titles from online library for virtual entries.";
        for (const auto& onlineGame : onlineTitles) {
            bool isPC = false;
            for(const auto& dev : onlineGame.devices) if(dev == "PC") isPC = true;
            
            if (!isPC || (onlineGame.pfn.empty() && onlineGame.detail.productId.empty())) continue;
            if (!onlineGame.pfn.empty() && processedPfnsForVirtual_refresh.count(onlineGame.pfn)) continue;

            std::string pseudoPathForOnline;
            std::string storeLink;
            if (!onlineGame.detail.productId.empty()) {
                pseudoPathForOnline = "xbox_online_prodid://" + onlineGame.detail.productId;
                storeLink = "ms-windows-store://pdp/?ProductId=" + onlineGame.detail.productId;
            } else if (!onlineGame.pfn.empty()){
                pseudoPathForOnline = "xbox_online_pfn://" + onlineGame.pfn;
                storeLink = "ms-windows-store://pdp/?PFN=" + onlineGame.pfn;
            } else {
                continue;
            }

            if(!onlineGame.pfn.empty()) processedPfnsForVirtual_refresh.insert(onlineGame.pfn);

            auto it = fileDataMapByAumidOrPseudoPath_refresh.find(pseudoPathForOnline);
            if (it == fileDataMapByAumidOrPseudoPath_refresh.end()) {
                NewXboxGameData data;
                data.pfn = onlineGame.pfn;
                data.pseudoPath = pseudoPathForOnline;
                data.metadataMap[MetaDataId::Name] = onlineGame.name.empty() ? (onlineGame.pfn.empty() ? onlineGame.detail.productId : onlineGame.pfn) : onlineGame.name;
                if(!onlineGame.pfn.empty()) data.metadataMap[MetaDataId::XboxPfn] = onlineGame.pfn;
                data.metadataMap[MetaDataId::XboxTitleId] = onlineGame.titleId;
                if (!onlineGame.detail.productId.empty()) data.metadataMap[MetaDataId::XboxProductId] = onlineGame.detail.productId;
                data.metadataMap[MetaDataId::Installed] = "false";
                data.metadataMap[MetaDataId::Virtual] = "true";
                if(!storeLink.empty()) data.metadataMap[MetaDataId::LaunchCommand] = storeLink;
                data.metadataMap[MetaDataId::Path] = pseudoPathForOnline;
                if (!onlineGame.detail.developerName.empty()) data.metadataMap[MetaDataId::Developer] = onlineGame.detail.developerName;
                if (!onlineGame.detail.publisherName.empty()) data.metadataMap[MetaDataId::Publisher] = onlineGame.detail.publisherName;
                 if (!onlineGame.detail.releaseDate.empty()) {
                      time_t release_t = Utils::Time::iso8601ToTime(onlineGame.detail.releaseDate);
                      if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                          data.metadataMap[MetaDataId::ReleaseDate] = Utils::Time::timeToMetaDataString(release_t);
                      }
                 }
                if (!onlineGame.detail.description.empty()) data.metadataMap[MetaDataId::Desc] = onlineGame.detail.description;
                if (!onlineGame.mediaItemType.empty()) data.metadataMap[MetaDataId::XboxMediaType] = onlineGame.mediaItemType;
                else if (!onlineGame.type.empty()) data.metadataMap[MetaDataId::XboxMediaType] = onlineGame.type;
                if (!onlineGame.devices.empty()) {
                    std::string devicesStr;
                    for (size_t i = 0; i < onlineGame.devices.size(); ++i) {
                        devicesStr += onlineGame.devices[i] + (i < onlineGame.devices.size() - 1 ? ", " : "");
                    }
                    data.metadataMap[MetaDataId::XboxDevices] = devicesStr;
                }
                newGamesPayload->push_back(data);
                LOG(LogDebug) << "  Payload Add (Online New Virtual): " << data.metadataMap[MetaDataId::Name];
            } else {
                FileData* fd = it->second;
                bool changed = false;
                if (fd->getMetadata().get(MetaDataId::Installed) != "false") { fd->getMetadata().set(MetaDataId::Installed, "false"); changed = true; }
                if (fd->getMetadata().get(MetaDataId::Virtual) != "true") { fd->getMetadata().set(MetaDataId::Virtual, "true"); changed = true; }
                if (!storeLink.empty() && fd->getMetadata().get(MetaDataId::LaunchCommand) != storeLink) { fd->getMetadata().set(MetaDataId::LaunchCommand, storeLink); changed = true; }
                if (fd->getPath() != pseudoPathForOnline) {
                     LOG(LogWarning) << "[Xbox Store Refresh BG] Virtual game " << fd->getName() << " has path " << fd->getPath() << " but its online pseudoPath is " << pseudoPathForOnline << ". Consider Gamelist cleanup or path update strategy.";
                }
                if (changed) {
                    fd->getMetadata().setDirty();
                    metadataPotentiallyChanged = true;
                }
            }
        }

        for (const std::string& aumid_in_es : existingAumidsInSystem_refresh) {
            bool stillInstalled = false;
            for(const auto& installed : installedGames) if(installed.aumid == aumid_in_es) { stillInstalled = true; break; }

            if (!stillInstalled) {
                auto it = fileDataMapByAumidOrPseudoPath_refresh.find(aumid_in_es);
                if (it != fileDataMapByAumidOrPseudoPath_refresh.end()) {
                    FileData* fd = it->second;
                    LOG(LogInfo) << "Xbox Store Refresh BG: Game " << fd->getName() << " (AUMID: " << aumid_in_es << ") is no longer installed.";
                    fd->getMetadata().set(MetaDataId::Installed, "false");
                    
                    bool isInOnlineLib = false;
                    std::string pfnForOnlineCheck = fd->getMetadata().get(MetaDataId::XboxPfn);
                    if (!pfnForOnlineCheck.empty()) {
                        for(const auto& onlineGame : onlineTitles) {
                            if(onlineGame.pfn == pfnForOnlineCheck) {
                                isInOnlineLib = true;
                                fd->getMetadata().set(MetaDataId::Virtual, "true");
                                std::string newStoreLink;
                                if (!onlineGame.detail.productId.empty()) {
                                    newStoreLink = "ms-windows-store://pdp/?ProductId=" + onlineGame.detail.productId;
                                } else {
                                    newStoreLink = "ms-windows-store://pdp/?PFN=" + onlineGame.pfn;
                                }
                                fd->getMetadata().set(MetaDataId::LaunchCommand, newStoreLink);
                                LOG(LogInfo) << "Marked " << fd->getName() << " (Path/AUMID: " << fd->getPath() << ") as virtual (still in online library). LaunchCommand set to store.";
                                break;
                            }
                        }
                    }
                    if (!isInOnlineLib) {
                        LOG(LogInfo) << fd->getName() << " is not in online library either. Marked as non-installed and virtual.";
                        fd->getMetadata().set(MetaDataId::Virtual, "true");
                        fd->getMetadata().set(MetaDataId::LaunchCommand, "");
                    }
                    fd->getMetadata().setDirty();
                    metadataPotentiallyChanged = true;
                }
            }
        }

        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_USEREVENT;
        event.user.code = SDL_XBOX_REFRESH_COMPLETE;
        event.user.data1 = newGamesPayload;
        event.user.data2 = xboxSystem;
        SDL_PushEvent(&event);

        if (newGamesPayload->empty() && metadataPotentiallyChanged && ViewController::get()) {
             LOG(LogInfo) << "Xbox Store Refresh BG: Only metadata changed for existing games. Requesting UI reload.";
             SDL_Event meta_event;
             SDL_zero(meta_event);
             meta_event.type = SDL_USEREVENT;
             meta_event.user.code = SDL_GAMELIST_UPDATED;
             meta_event.user.data1 = xboxSystem;
             meta_event.user.data2 = nullptr;
             SDL_PushEvent(&meta_event);
        }

        LOG(LogInfo) << "Xbox Store Refresh BG: Finished. Pushed " << newGamesPayload->size() << " new games. Metadata changed: " << metadataPotentiallyChanged;
        winrt::uninit_apartment();
    });
}

std::future<void> XboxStore::updateGamesMetadataAsync(SystemData* system, const std::vector<std::string>& gamePfnsToUpdate) {
     return std::async(std::launch::async, [this, system, gamePfnsToUpdate]() {
         winrt::init_apartment(winrt::apartment_type::multi_threaded);
         LOG(LogInfo) << "Xbox Store MetaUpdate BG: Starting for " << gamePfnsToUpdate.size() << " PFNs.";
         if (!_initialized || !mAuth || !mAPI || !system) {
             LOG(LogError) << "Xbox Store MetaUpdate BG: Store not ready or system invalid.";
             winrt::uninit_apartment(); return;
         }
         if (!mAuth->isAuthenticated()) {
             LOG(LogWarning) << "Xbox Store MetaUpdate BG: Not authenticated.";
             winrt::uninit_apartment(); return;
         }

         bool anyMetadataChanged = false;
         int successCount = 0;

         for (const std::string& pfn : gamePfnsToUpdate) {
             if (pfn.empty()) continue;

             FileData* gameFile = nullptr;
             auto allGamesInSystem = system->getRootFolder()->getFilesRecursive(GAME, true);
             for(auto* fd : allGamesInSystem) {
                 if (fd && fd->getMetadata().get(MetaDataId::XboxPfn) == pfn) {
                     gameFile = fd;
                     LOG(LogDebug) << "Xbox Store MetaUpdate BG: Found FileData by metadata PFN: " << pfn << " with path " << fd->getPath();
                     break;
                 }
             }
             
             if (!gameFile) {
                 LOG(LogWarning) << "Xbox Store MetaUpdate BG: FileData still not found for PFN: " << pfn << " after all checks.";
                 continue;
             }

             LOG(LogDebug) << "Xbox Store MetaUpdate BG: Fetching details for PFN: " << pfn << " (Game: " << gameFile->getName() << ")";
             Xbox::OnlineTitleInfo details = mAPI->GetTitleInfo(pfn);

             if (details.pfn.empty() && details.name.empty() && details.detail.productId.empty()) {
                 LOG(LogWarning) << "Xbox Store MetaUpdate BG: No details returned from API for PFN: " << pfn;
                 continue;
             }

             MetaDataList& mdl = gameFile->getMetadata();
             bool gameChanged = false;

             std::string apiName = details.name.empty() ? (details.pfn.empty() ? pfn : details.pfn) : details.name;
             if (mdl.get(MetaDataId::Name) != apiName) {
                 mdl.set(MetaDataId::Name, apiName); gameChanged = true;
             }
             if (!details.detail.description.empty() && mdl.get(MetaDataId::Desc) != details.detail.description) {
                 mdl.set(MetaDataId::Desc, details.detail.description); gameChanged = true;
             }
             if (!details.detail.developerName.empty() && mdl.get(MetaDataId::Developer) != details.detail.developerName) {
                 mdl.set(MetaDataId::Developer, details.detail.developerName); gameChanged = true;
             }
             if (!details.detail.publisherName.empty() && mdl.get(MetaDataId::Publisher) != details.detail.publisherName) {
                 mdl.set(MetaDataId::Publisher, details.detail.publisherName); gameChanged = true;
             }
             if (!details.detail.releaseDate.empty()) {
                 time_t release_t = Utils::Time::iso8601ToTime(details.detail.releaseDate);
                 if (release_t != Utils::Time::NOT_A_DATE_TIME) {
                     std::string esDate = Utils::Time::timeToMetaDataString(release_t);
                     if (!esDate.empty() && mdl.get(MetaDataId::ReleaseDate) != esDate) {
                         mdl.set(MetaDataId::ReleaseDate, esDate); gameChanged = true;
                     }
                 } else { LOG(LogWarning) << "Xbox Meta BG: Could not parse release date: " << details.detail.releaseDate; }
             }
             std::string apiMediaType = details.mediaItemType.empty() ? details.type : details.mediaItemType;
             if (!apiMediaType.empty() && mdl.get(MetaDataId::XboxMediaType) != apiMediaType) {
                 mdl.set(MetaDataId::XboxMediaType, apiMediaType); gameChanged = true;
             }
             if (!details.devices.empty()) {
                 std::string devicesStr;
                 for (size_t i = 0; i < details.devices.size(); ++i) {
                     devicesStr += details.devices[i] + (i < details.devices.size() - 1 ? ", " : "");
                 }
                 if (mdl.get(MetaDataId::XboxDevices) != devicesStr) {
                     mdl.set(MetaDataId::XboxDevices, devicesStr); gameChanged = true;
                 }
             }
             if (!details.detail.productId.empty() && mdl.get(MetaDataId::XboxProductId) != details.detail.productId) {
                 mdl.set(MetaDataId::XboxProductId, details.detail.productId); gameChanged = true;
             }
             if (!details.titleId.empty() && mdl.get(MetaDataId::XboxTitleId) != details.titleId) {
                 mdl.set(MetaDataId::XboxTitleId, details.titleId); gameChanged = true;
             }

             if (gameChanged) {
                 LOG(LogInfo) << "Xbox Store MetaUpdate BG: Updated metadata for " << apiName;
                 anyMetadataChanged = true;
                 mdl.setDirty();
                 successCount++;
             }
         }

         LOG(LogInfo) << "Xbox Store MetaUpdate BG: Finished. Successfully updated metadata for " << successCount << " PFN(s).";

         if (anyMetadataChanged && ViewController::get()) {
             LOG(LogInfo) << "Xbox Store MetaUpdate BG: Metadata changed, requesting UI reload for system " << system->getName();
             SDL_Event event;
             SDL_zero(event);
             event.type = SDL_USEREVENT;
             event.user.code = SDL_GAMELIST_UPDATED;
             event.user.data1 = system;
             event.user.data2 = nullptr;
             SDL_PushEvent(&event);
         }
         winrt::uninit_apartment();
      });
}

bool XboxStore::installGame(const std::string& idToInstall) {
    LOG(LogDebug) << "XboxStore::installGame called for ID: " << idToInstall;
    std::string storeUrl;
    bool isLikelyPfn = idToInstall.find('_') != std::string::npos && idToInstall.length() > 20;
    bool isLikelyProductId = idToInstall.length() == 12 && std::all_of(idToInstall.begin(), idToInstall.end(), ::isalnum);

    if (isLikelyPfn) {
        storeUrl = "ms-windows-store://pdp/?PFN=" + idToInstall;
    } else if (isLikelyProductId) {
        storeUrl = "ms-windows-store://pdp/?ProductId=" + idToInstall;
    } else {
        LOG(LogWarning) << "XboxStore::installGame - ID format not clearly PFN or ProductID: " << idToInstall << ". Assuming ProductID as fallback.";
        storeUrl = "ms-windows-store://pdp/?ProductId=" + idToInstall;
    }
    LOG(LogInfo) << "Attempting to open Microsoft Store page: " << storeUrl;
    Utils::Platform::openUrl(storeUrl);
    return true;
}
bool XboxStore::uninstallGame(const std::string& pfnOrAumidOrProductId) {
    LOG(LogDebug) << "XboxStore::uninstallGame called for ID: " << pfnOrAumidOrProductId;
    LOG(LogInfo) << "Opening Apps & Features settings page for manual uninstallation. User should find game associated with ID: " << pfnOrAumidOrProductId;
    Utils::Platform::openUrl("ms-settings:appsfeatures");
    return true;
}
bool XboxStore::updateGame(const std::string& idToUpdate) {
    LOG(LogDebug) << "XboxStore::updateGame called for ID: " << idToUpdate;
    return installGame(idToUpdate);
}
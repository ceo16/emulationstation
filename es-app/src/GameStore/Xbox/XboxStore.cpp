#define NOMINMAX
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

// Lista di esclusione PFN (invariata)
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
    : GameStore(), mAuth(auth), mAPI(nullptr), mInstanceWindow(window_param), _initialized(false), mIsXboxAppInstalled(false)
#ifdef _WIN32
    , mComInitialized(false)
#endif
{
#ifdef _WIN32
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        mComInitialized = true;
    } catch (const winrt::hresult_error& e) {
        LOG(LogError) << "XboxStore: Failed to initialize COM apartment: " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str());
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

std::string XboxStore::getStoreName() const { return "XboxStore"; }

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

// --- MODIFICA ---
// Aggiunte le funzioni isXboxAppInstalled e openStoreClient
bool XboxStore::isXboxAppInstalled() {
#ifndef _WIN32
    return false;
#else
    LOG(LogInfo) << "--- XBOX APP INSTALL CHECK (HYPER-DEBUG V2) ---";
    
    try {
        // Inizializziamo COM esplicitamente per questa chiamata
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        struct ComUninitializer { ~ComUninitializer() { winrt::uninit_apartment(); } } comUninitializer;

        LOG(LogDebug) << "[Xbox-HyperDebug] COM Initialized. Creating PackageManager...";
        winrt::Windows::Management::Deployment::PackageManager packageManager;
        const wchar_t* PFN = L"Microsoft.GamingApp_8wekyb3d8bbwe";
        LOG(LogDebug) << "[Xbox-HyperDebug] Attempting to find package with PFN: Microsoft.GamingApp_8wekyb3d8bbwe";

        auto package = packageManager.FindPackageForUser(L"", PFN);

        if (package) {
            LOG(LogInfo) << "[Xbox-HyperDebug] SUCCESSO: Trovato un oggetto 'package' per il PFN.";
            
            auto status = package.Status();
            
            // --- MODIFICA ---
            // Sostituiamo il controllo 'IsOk()' con la verifica delle singole proprietà negative.
            // Un pacchetto è "OK" se non è un framework e nessuno degli stati negativi è attivo.
            bool isOk = !package.IsFramework() && 
                        !status.Disabled() && 
                        !status.NotAvailable() && 
                        !status.NeedsRemediation();

            // Log Dettagliato dello stato per il debug
            LOG(LogInfo) << "[Xbox-HyperDebug] Package.IsFramework: " << (package.IsFramework() ? "Si" : "No");
            LOG(LogInfo) << "[Xbox-HyperDebug] Status.Disabled: " << (status.Disabled() ? "Si" : "No");
            LOG(LogInfo) << "[Xbox-HyperDebug] Status.NotAvailable: " << (status.NotAvailable() ? "Si" : "No");
            LOG(LogInfo) << "[Xbox-HyperDebug] Status.NeedsRemediation: " << (status.NeedsRemediation() ? "Si" : "No");
            LOG(LogInfo) << "[Xbox-HyperDebug] >>> Calcolato IsOk: " << (isOk ? "Si" : "No");

            if (isOk) {
                LOG(LogInfo) << "[Xbox-HyperDebug] DECISIONE: Lo stato del pacchetto è valido. App considerata INSTALLATA. Ritorno true.";
                return true;
            } else {
                LOG(LogWarning) << "[Xbox-HyperDebug] DECISIONE: Pacchetto trovato, ma il suo stato non è valido. App considerata NON INSTALLATA. Ritorno false.";
                return false;
            }
        } else {
            LOG(LogWarning) << "[Xbox-HyperDebug] DECISIONE: FindPackageForUser ha restituito un oggetto nullo. App considerata NON INSTALLATA. Ritorno false.";
            return false;
        }
    } catch (const winrt::hresult_error& e) {
        if (e.code() == static_cast<int32_t>(0x80073D54)) { // Package not found
             LOG(LogInfo) << "[Xbox-HyperDebug] ECCEZIONE: FindPackageForUser ha lanciato 'Package not found'. Normale se l'app non è installata. Ritorno false.";
        } else {
             LOG(LogError) << "[Xbox-HyperDebug] ECCEZIONE: Errore COM/WinRT imprevisto. HRESULT: " << std::hex << e.code() << ", Messaggio: " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str()) << ". Ritorno false.";
        }
        return false;
    } catch (...) {
        LOG(LogError) << "[Xbox-HyperDebug] Eccezione sconosciuta durante il controllo. Ritorno false.";
        return false;
    }
#endif
}
// --- FINE MODIFICA ---

std::vector<Xbox::InstalledXboxGameInfo> XboxStore::findInstalledGames_PackageManagerHelper(
    const std::map<std::wstring, std::wstring>* pfnAndAppIdMap) {
    
    std::vector<Xbox::InstalledXboxGameInfo> installedGames;
#ifdef _WIN32
    if (!mComInitialized) { 
        LOG(LogError) << "XboxStore: COM not initialized (PM Helper)."; 
        return installedGames; 
    }
    LOG(LogDebug) << "XboxStore: PM Helper searching UWP games (" 
                  << (pfnAndAppIdMap && !pfnAndAppIdMap->empty() ? "PFN/AppID Whitelist Mode (Iterative)" : "Global ExclusionList Mode (Iterative)") 
                  << ")...";

    try {
        winrt::Windows::Management::Deployment::PackageManager packageManager;
        
        auto allUserPackages = packageManager.FindPackagesForUser(L"");
        uint32_t totalUserPackagesCount = 0;
        if (allUserPackages) {
            for (const auto& p_hldr : allUserPackages) { 
                (void)p_hldr; 
                totalUserPackagesCount++;
            }
        }
        LOG(LogDebug) << "[XboxStore-PMHelper] Fetched " << totalUserPackagesCount << " total packages for the current user (counted via iteration).";

        std::map<winrt::hstring, winrt::Windows::ApplicationModel::Package> userPackageMap;
        if (totalUserPackagesCount > 0 && allUserPackages) { 
            for (const auto& pkg : allUserPackages) {
                userPackageMap.insert({pkg.Id().FamilyName(), pkg});
            }
        }
        
        if (pfnAndAppIdMap && !pfnAndAppIdMap->empty()) { 
            LOG(LogInfo) << "[XboxStore-PMHelper] Whitelist Mode: Checking " << pfnAndAppIdMap->size() << " PFNs from online library against " << totalUserPackagesCount << " installed user packages.";
            for (const auto& pair : *pfnAndAppIdMap) {
                const std::wstring& pfn_w = pair.first;
                const std::wstring& appIdFromGameList_w = pair.second; 
                winrt::hstring currentPfn_h(pfn_w);
                std::string currentPfn_s = ConvertWideToUtf8_XboxStore_Unique(currentPfn_h.c_str());

                LOG(LogDebug) << "[XboxStore-PMHelper-Whitelist] Processing PFN from GameList: " << currentPfn_s;

                winrt::Windows::ApplicationModel::Package package{ nullptr };

                auto it = userPackageMap.find(currentPfn_h);
                if (it != userPackageMap.end()) {
                    package = it->second;
                    LOG(LogInfo) << "[XboxStore-PMHelper] Found PFN " << currentPfn_s << " in user's pre-fetched installed packages list.";
                } else {
                    LOG(LogDebug) << "[XboxStore-PMHelper] PFN " << currentPfn_s << " NOT found in user's pre-fetched installed packages list.";
                }

                if (!package) {
                    continue;
                }
                
                if (package.IsFramework() || package.IsResourcePackage() || package.IsBundle() || 
                    package.Status().NotAvailable() || 
                    package.Status().Disabled() || 
                    package.Status().NeedsRemediation() ||
                    package.Status().DeploymentInProgress() ) {
                    LOG(LogInfo) << "[XboxStore-PMHelper] Skipping PFN " << currentPfn_s << " due to package type or status.";
                    continue;
                }

                try {
                    auto appEntries = package.GetAppListEntriesAsync().get();
                    uint32_t numAppEntries_Whitelist = 0;
                    if (appEntries) {
                        for (const auto& e_hldr_wl : appEntries) { 
                            (void)e_hldr_wl; 
                            numAppEntries_Whitelist++;
                        }
                    }

                    if (numAppEntries_Whitelist > 0) {
                        winrt::Windows::ApplicationModel::Core::AppListEntry targetAppEntry{ nullptr };
                        
                        if (!appIdFromGameList_w.empty()) {
                            winrt::hstring targetAumid_h = currentPfn_h + L"!" + appIdFromGameList_w;
                            for (const auto& entry : appEntries) { 
                                if (entry.AppUserModelId() == targetAumid_h) {
                                    targetAppEntry = entry;
                                    LOG(LogDebug) << "[XboxStore-PMHelper] Matched AppListEntry via provided AppID: " << ConvertWideToUtf8_XboxStore_Unique(targetAumid_h.c_str());
                                    break;
                                }
                            }
                        }
                        
                        if (!targetAppEntry && appEntries && numAppEntries_Whitelist > 0) { 
                           targetAppEntry = appEntries.First().Current();
                           LOG(LogDebug) << "[XboxStore-PMHelper] No specific AppID match or AppID was empty, using first AppListEntry: " << ConvertWideToUtf8_XboxStore_Unique(targetAppEntry.AppUserModelId().c_str());
                        }

                        if (targetAppEntry) {
                            Xbox::InstalledXboxGameInfo gameInfo; 
                            gameInfo.displayName = ConvertWideToUtf8_XboxStore_Unique(targetAppEntry.DisplayInfo().DisplayName().c_str());
                            gameInfo.applicationDisplayName = gameInfo.displayName;
                            gameInfo.aumid = ConvertWideToUtf8_XboxStore_Unique(targetAppEntry.AppUserModelId().c_str());
                            gameInfo.pfn = currentPfn_s;
                            gameInfo.packageFullName = ConvertWideToUtf8_XboxStore_Unique(package.Id().FullName().c_str());
                            
                            std::string extractedAppId = "App"; 
                            if (!gameInfo.aumid.empty()) {
                                size_t bang_pos = gameInfo.aumid.find('!');
                                if (bang_pos != std::string::npos && bang_pos + 1 < gameInfo.aumid.length()) {
                                    extractedAppId = gameInfo.aumid.substr(bang_pos + 1);
                                }
                            }
                            
                            if (!appIdFromGameList_w.empty()) {
                                gameInfo.applicationId = ConvertWideToUtf8_XboxStore_Unique(appIdFromGameList_w.c_str());
                                std::string constructedAumidWithOnlineAppId = gameInfo.pfn + "!" + gameInfo.applicationId;
                                if (constructedAumidWithOnlineAppId != gameInfo.aumid) {
                                     LOG(LogWarning) << "[XboxStore-PMHelper] AppID from online library (" << gameInfo.applicationId 
                                                     << ") for PFN " << gameInfo.pfn 
                                                     << " results in AUMID " << constructedAumidWithOnlineAppId 
                                                     << " which differs from manifest AUMID " << gameInfo.aumid
                                                     << ". Preferring manifest AppID: " << extractedAppId;
                                     gameInfo.applicationId = extractedAppId;
                                }
                            } else {
                                gameInfo.applicationId = extractedAppId;
                            }
                            
                            gameInfo.isInstalled = true;
                            winrt::Windows::Storage::StorageFolder instLoc = package.InstalledLocation();
                            if (instLoc) gameInfo.installLocation = ConvertWideToUtf8_XboxStore_Unique(instLoc.Path().c_str()); else gameInfo.installLocation = "";
                            
                            LOG(LogInfo) << "[XboxStore-PMHelper] Whitelist - Identified Installed Game: Name=[" << gameInfo.displayName << "], AUMID=[" << gameInfo.aumid << "], AppID=[" << gameInfo.applicationId <<"], PFN=[" << gameInfo.pfn << "]";
                            if (!gameInfo.aumid.empty()) installedGames.push_back(gameInfo);
                        } else {
                            LOG(LogWarning) << "[XboxStore-PMHelper] No valid AppListEntry could be targeted for PFN: " << currentPfn_s;
                        }
                    } else {
                        LOG(LogWarning) << "[XboxStore-PMHelper] No AppListEntries found for package with PFN: " << currentPfn_s;
                    }
                } catch (const winrt::hresult_error& ae_err) { 
                    LOG(LogWarning) << "[XboxStore-PMHelper] Exception getting AppListEntries for PFN " << currentPfn_s << ": " << ConvertWideToUtf8_XboxStore_Unique(ae_err.message().c_str()) << " HRESULT: 0x" << std::hex << ae_err.code();
                }
            }
        } else { 
            LOG(LogInfo) << "[XboxStore-PMHelper] Global ExclusionList Mode: Processing " << userPackageMap.size() << " total user packages.";
            for (const auto& packagePair : userPackageMap) { 
                const auto& package = packagePair.second; 
                winrt::hstring currentPfn_h_global = package.Id().FamilyName(); 
                std::string currentPfn_s_global = ConvertWideToUtf8_XboxStore_Unique(currentPfn_h_global.c_str());

                if (package.IsFramework() || package.IsResourcePackage() || package.IsBundle() ||
                    package.Status().NotAvailable() || package.Status().Servicing() || 
                    package.Status().DeploymentInProgress() || package.Status().Disabled() ||
                    package.Status().NeedsRemediation()) {
                    continue;
                }
                
                if (pfnExclusionListGlobal.count(currentPfn_h_global.c_str())) { 
                    LOG(LogDebug) << "[XboxStoreFilter-PMHelper-Exclusion] Skipping PFN via exclusion list: " << currentPfn_s_global;
                    continue;
                }
                
                try {
                    auto appEntries = package.GetAppListEntriesAsync().get();
                    uint32_t numAppEntries_Exclusion = 0;
                    if (appEntries) {
                        for (const auto& e_hldr_ex : appEntries) { 
                            (void)e_hldr_ex; 
                             numAppEntries_Exclusion++;
                        }
                    }

                    if (numAppEntries_Exclusion > 0) {
                        winrt::Windows::ApplicationModel::Core::AppListEntry appEntryToUse = appEntries.First().Current();
                        if (appEntryToUse) { 
                            Xbox::InstalledXboxGameInfo gameInfo;
                            gameInfo.displayName = ConvertWideToUtf8_XboxStore_Unique(appEntryToUse.DisplayInfo().DisplayName().c_str());
                            gameInfo.applicationDisplayName = gameInfo.displayName;
                            gameInfo.aumid = ConvertWideToUtf8_XboxStore_Unique(appEntryToUse.AppUserModelId().c_str());
                            gameInfo.pfn = currentPfn_s_global; 
                            gameInfo.packageFullName = ConvertWideToUtf8_XboxStore_Unique(package.Id().FullName().c_str());
                            
                            std::string tempAppId = "App";
                            if (!gameInfo.aumid.empty()) {
                                size_t bang_pos_temp = gameInfo.aumid.find('!');
                                if (bang_pos_temp != std::string::npos && bang_pos_temp + 1 < gameInfo.aumid.length()) {
                                    tempAppId = gameInfo.aumid.substr(bang_pos_temp + 1);
                                } else {
                                    LOG(LogDebug) << "[XboxStore-PMHelper] ExclusionMode - No '!' in AUMID to extract AppId, or AppId is empty: " << gameInfo.aumid;
                                }
                            }
                            gameInfo.applicationId = tempAppId;
                            gameInfo.isInstalled = true;
                            winrt::Windows::Storage::StorageFolder instLoc = package.InstalledLocation();
                            if(instLoc) gameInfo.installLocation = ConvertWideToUtf8_XboxStore_Unique(instLoc.Path().c_str()); else gameInfo.installLocation = "";
                            
                            LOG(LogInfo) << "[XboxStore-PMHelper] ExclusionMode - Identified App: Name=[" << gameInfo.displayName << "], AUMID=[" << gameInfo.aumid << "]";
                            if(!gameInfo.aumid.empty()) installedGames.push_back(gameInfo);
                        }
                    }
                } catch (const winrt::hresult_error& ae_err) { 
                    LOG(LogWarning) << "[XboxStore-PMHelper] ExclusionMode - Exception getting AppListEntries for PFN " 
                                    << currentPfn_s_global << ": " 
                                    << ConvertWideToUtf8_XboxStore_Unique(ae_err.message().c_str()) << " HRESULT: 0x" << std::hex << ae_err.code();
                }
            }
        }
    } catch (const winrt::hresult_error& e) {
        LOG(LogError) << "[XboxStore-PMHelper] Outer Exception: Error finding UWP packages: " << ConvertWideToUtf8_XboxStore_Unique(e.message().c_str()) << " HRESULT: 0x" << std::hex << e.code();
    } catch (const std::exception& e_std) {
        LOG(LogError) << "[XboxStore-PMHelper] Outer Std::Exception: " << e_std.what();
    } catch (...) {
        LOG(LogError) << "[XboxStore-PMHelper] Outer Unknown Exception while finding packages.";
    }

    LOG(LogInfo) << "[XboxStore-PMHelper] Search finished. Found " << installedGames.size() << " UWP apps using iterative approach (" 
                 << (pfnAndAppIdMap && !pfnAndAppIdMap->empty() ? "Whitelist" : "ExclusionList") 
                 << ").";
#endif
    return installedGames;
}

std::vector<Xbox::InstalledXboxGameInfo> XboxStore::findInstalledXboxGames() {
#ifndef _WIN32
    LOG(LogWarning) << "XboxStore::findInstalledXboxGames() - Only on Windows.";
    return {};
#else
    if (!mComInitialized) {
        LOG(LogError) << "XboxStore: COM not initialized (findInstalledXboxGames).";
        return {};
    }

    if (!mAPI || !mAuth || !mAuth->isAuthenticated()) {
        LOG(LogWarning) << "[XboxStore findInstalledXboxGames] Not authenticated or API object missing. Cannot fetch user's Xbox library titles to check installations.";
        return {};
    }

    LOG(LogInfo) << "[XboxStore findInstalledXboxGames] Fetching user's online library titles to identify installed PC games...";
    std::vector<Xbox::OnlineTitleInfo> onlineTitles = mAPI->GetLibraryTitles();

    if (onlineTitles.empty()) {
        LOG(LogInfo) << "[XboxStore findInstalledXboxGames] User's online Xbox library is empty or could not be fetched. No specific games to check for installation.";
        return {};
    }

    std::map<std::wstring, std::wstring> pfnPcGamesFromLibraryMap;
    LOG(LogDebug) << "[XboxStore findInstalledXboxGames] Processing " << onlineTitles.size() << " online titles to build PFN map for targeted PackageManager query.";

    for (const auto& title : onlineTitles) {
        bool isPCGame = false;
        for (const std::string& device : title.devices) {
            if (device == "PC") {
                isPCGame = true;
                break;
            }
        }

        if (isPCGame && !title.pfn.empty()) {
            std::wstring pfn_w = Utils::String::convertToWideString(title.pfn); 
            std::wstring appId_w = L""; 
            LOG(LogDebug) << "[XboxStore findInstalledXboxGames] Adding to PFN query map: PFN=" << title.pfn
                          << ", Name=" << title.name << ", AppID Hint (from online list - typically empty/unused):" << Utils::String::convertFromWideString(appId_w);
            pfnPcGamesFromLibraryMap.insert({pfn_w, appId_w});
        } else {
            if (!isPCGame) {
                LOG(LogDebug) << "[XboxStore findInstalledXboxGames] Skipping online title (not a PC game): " << title.name << " (PFN: " << title.pfn << ")";
            } else if (title.pfn.empty()) {
                LOG(LogDebug) << "[XboxStore findInstalledXboxGames] Skipping online PC title (PFN is empty): " << title.name;
            }
        }
    }

    if (pfnPcGamesFromLibraryMap.empty()) {
        LOG(LogInfo) << "[XboxStore findInstalledXboxGames] No PC game PFNs with valid PFNs derived from the user's online library to check for installation.";
        return {};
    }

    LOG(LogInfo) << "[XboxStore findInstalledXboxGames] Querying PackageManager for " << pfnPcGamesFromLibraryMap.size()
                 << " specific PFNs from user's library.";
    std::vector<Xbox::InstalledXboxGameInfo> installedGamesFromLibrary =
        findInstalledGames_PackageManagerHelper(&pfnPcGamesFromLibraryMap);

    LOG(LogInfo) << "[XboxStore findInstalledXboxGames] Found " << installedGamesFromLibrary.size()
                 << " installed games by checking user's library PFNs with PackageManager.";
    return installedGamesFromLibrary;
#endif
}

std::vector<FileData*> XboxStore::getGamesList() {
    LOG(LogDebug) << "XboxStore::getGamesList() called.";
    std::vector<FileData*> gameFiles;
    SystemData* system = SystemData::getSystem("xbox");
    if (!system) { LOG(LogError) << "XboxStore::getGamesList - System 'xbox' not found!"; return gameFiles; }
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
            FileData* newGame = new FileData(FileType::GAME, installedApp.aumid, system);
            newGame->getMetadata().set(MetaDataId::Name, installedApp.displayName);
            newGame->getMetadata().set(MetaDataId::XboxPfn, installedApp.pfn); 
            newGame->getMetadata().set(MetaDataId::XboxAumid, installedApp.aumid);
            newGame->getMetadata().set(MetaDataId::Installed, "true");
            newGame->getMetadata().set(MetaDataId::Virtual, "false");
            newGame->getMetadata().set(MetaDataId::LaunchCommand, installedApp.aumid);
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
            
            // --- MODIFICA ---
            // Sostituiamo la creazione del link diretto con il nostro comando personalizzato
            std::string pseudoPathForOnline;
            std::string launchCommand;

            if (!title.detail.productId.empty()) {
                pseudoPathForOnline = "xbox_online_prodid:/" + title.detail.productId;
                launchCommand = "xbox:install:" + title.detail.productId;
            } else if (!title.pfn.empty()) { 
                pseudoPathForOnline = "xbox_online_pfn:/" + title.pfn;
                launchCommand = ""; 
            } else {
                continue;
            }
            LOG(LogDebug) << "[XboxStore GetList] Processing online title '" << title.name << "' with pseudoPath: '" << pseudoPathForOnline << "'";
            // --- FINE MODIFICA ---

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
                
                // --- MODIFICA ---
                // Applichiamo il comando personalizzato
                if (!launchCommand.empty() && fd->getMetadata().get(MetaDataId::LaunchCommand) != launchCommand) { 
                    fd->getMetadata().set(MetaDataId::LaunchCommand, launchCommand); 
                    changed = true; 
                }
                // --- FINE MODIFICA ---

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
                
                // --- MODIFICA ---
                // Applichiamo il comando personalizzato
                if (!launchCommand.empty()) {
                    newGame->getMetadata().set(MetaDataId::LaunchCommand, launchCommand);
                } else { 
                    LOG(LogWarning) << "XboxStore: No launch command for new virtual game: " << title.name; 
                }
                // --- FINE MODIFICA ---

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
		 // --- BLOCCO DA AGGIUNGERE ALL'INIZIO ---
        // Controlliamo se l'app Xbox è installata all'inizio della sincronizzazione
        // e salviamo il risultato nella nostra variabile membro.
        LOG(LogInfo) << "Xbox Store Refresh BG: Checking for Xbox App installation status...";
        this->mIsXboxAppInstalled = this->isXboxAppInstalled();
        LOG(LogInfo) << "Xbox Store Refresh BG: Xbox App installed status: " << (this->mIsXboxAppInstalled ? "Yes" : "No");
        // --- FINE BLOCCO DA AGGIUNGERE ---
		
        SystemData* xboxSystem = SystemData::getSystem("xbox"); 

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

            // --- MODIFICA ---
            std::string pseudoPathForOnline;
            std::string launchCommand;
            if (!onlineGame.detail.productId.empty()) {
                pseudoPathForOnline = "xbox_online_prodid://" + onlineGame.detail.productId;
                launchCommand = "xbox:install:" + onlineGame.detail.productId;
            } else if (!onlineGame.pfn.empty()){
                pseudoPathForOnline = "xbox_online_pfn://" + onlineGame.pfn;
                launchCommand = "";
            } else {
                continue;
            }
            // --- FINE MODIFICA ---
            
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
                if(!launchCommand.empty()) data.metadataMap[MetaDataId::LaunchCommand] = launchCommand;
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
                if (!launchCommand.empty() && fd->getMetadata().get(MetaDataId::LaunchCommand) != launchCommand) { fd->getMetadata().set(MetaDataId::LaunchCommand, launchCommand); changed = true; }
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
                                
                                // --- MODIFICA ---
                                // Aggiorna il LaunchCommand al nostro comando personalizzato
                                std::string newLaunchCommand;
                                if (!onlineGame.detail.productId.empty()) {
                                    newLaunchCommand = "xbox:install:" + onlineGame.detail.productId;
                                }
                                if (!newLaunchCommand.empty()) {
                                    fd->getMetadata().set(MetaDataId::LaunchCommand, newLaunchCommand);
                                } else {
                                    // Se non c'è ProductID, rimuoviamo il vecchio comando di avvio (AUMID)
                                    fd->getMetadata().set(MetaDataId::LaunchCommand, "");
                                }
                                // --- FINE MODIFICA ---

                                LOG(LogInfo) << "Marked " << fd->getName() << " (Path/AUMID: " << fd->getPath() << ") as virtual (still in online library). LaunchCommand set to install command.";
                                break;
                            }
                        }
                    }
                    if (!isInOnlineLib) {
                        LOG(LogInfo) << fd->getName() << " is not in online library either. Marked as non-installed and virtual.";
                        fd->getMetadata().set(MetaDataId::Virtual, "true"); 
                        fd->getMetadata().set(MetaDataId::LaunchCommand, ""); // Rimuovi launch command
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

// --- MODIFICA ---
// Modificata la funzione installGame per usare il link corretto in base alla presenza dell'App Xbox
bool XboxStore::installGame(const std::string& idToInstall) { 
    LOG(LogDebug) << "XboxStore::installGame called for ID: " << idToInstall;
    std::string storeUrl;
    std::string productId;
    
    // La logica per estrarre il productId rimane la stessa
    bool isLikelyPfn = idToInstall.find('_') != std::string::npos && idToInstall.length() > 20;
    bool isLikelyProductId = idToInstall.length() == 12 && std::all_of(idToInstall.begin(), idToInstall.end(), ::isalnum);

    if (isLikelyProductId) {
        productId = idToInstall;
    } else if (!isLikelyPfn) {
        productId = idToInstall;
    }

    // --- NUOVA LOGICA SEMPLIFICATA ---
    // Controlliamo la nostra variabile membro, non eseguiamo più la funzione di controllo qui.
    if (this->mIsXboxAppInstalled && !productId.empty()) {
        storeUrl = "ms-xbox-app://navigate?productId=" + productId;
        LOG(LogInfo) << "Xbox App status is 'Installed'. Using deep link: " << storeUrl;
    } else {
        // Se l'app non è installata (o non è stata ancora fatta una sincronizzazione), usiamo il fallback.
        if (!productId.empty()) {
             storeUrl = "ms-windows-store://pdp/?ProductId=" + productId;
        } else {
             storeUrl = "ms-windows-store://pdp/?PFN=" + idToInstall;
        }
        LOG(LogInfo) << "Xbox App status is 'Not Installed' or unknown. Using fallback Microsoft Store link: " << storeUrl;
    }

    Utils::Platform::openUrl(storeUrl);
    return true;
}
// --- FINE MODIFICA ---


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
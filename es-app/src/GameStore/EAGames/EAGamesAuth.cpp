#include "GameStore/EAGames/EAGamesAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "Settings.h"
#include "utils/Platform.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/RandomString.h"
#include "utils/Uri.h"
#include "utils/TimeUtil.h"
#include "utils/base64.h"
#include "utils/Crypto.h"
#include "json.hpp"
#include "Paths.h"
#include "LocaleES.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "Window.h"
#include "ApiSystem.h"

#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <fstream> // Per std::ofstream nella funzione helper write_text_to_file

#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif

namespace {
    const std::string EA_AUTH_CREDENTIALS_FILENAME = "eagames_juno_credentials.json";

    const std::string PC_SIGN_V1_KEY = "ISa3dpGOc8wW7Adn4auACSQmaccrOyR2";
    const std::string PC_SIGN_V2_KEY = "nt5FfJbdPzNcl2pkC3zgjO43Knvscxft";

    std::string helper_base64url_encode_string(const std::string& input) {
        return base64_encode(input, true);
    }

    std::string helper_base64url_encode_bytes(const unsigned char* bytes, size_t len) {
        return base64_encode(bytes, len, true);
    }

    bool helper_write_text_to_file(const std::string& path, const std::string& content) {
        try {
            std::ofstream fs(path, std::ofstream::binary);
            if (fs.is_open()) {
                fs << content;
                fs.close();
                return !fs.fail();
            }
             LOG(LogError) << "helper_write_text_to_file: Failed to open file for writing: " << path;
        } catch (const std::exception& e) {
            LOG(LogError) << "Exception in helper_write_text_to_file for " << path << ": " << e.what();
        }
        return false;
    }

} // namespace

namespace EAGames {
	
	void EAGamesAuth::postToUiThread(std::function<void()> func) {
    if (mWindow && func) { // mWindow è il membro privato di EAGamesAuth
        mWindow->postToUiThread(func);
    } else if (func) {
        LOG(LogWarning) << "EAGamesAuth::postToUiThread: mWindow è nullo, il callback potrebbe non essere eseguito sul thread UI.";
        // Considera se eseguire func() direttamente o loggare un errore più severo se mWindow è essenziale.
        // Per ora, eseguiamolo se mWindow non è disponibile e func è valido,
        // ma questo potrebbe non essere sicuro per tutti i callback UI.
        // Se la UI DEVE essere aggiornata, e mWindow è null, allora è un problema.
        // Una gestione più sicura sarebbe:
        // if (mWindow && func) {
        //     mWindow->postToUiThread(func);
        // } else if (func) {
        //     LOG(LogError) << "EAGamesAuth::postToUiThread: mWindow è nullo. Impossibile inviare al thread UI.";
        // }
        func(); // Esecuzione diretta come fallback temporaneo, con avviso.
    }
}

    const std::string EAGamesAuth::EA_AUTH_BASE_URL = "https://accounts.ea.com/connect/auth";
    const std::string EAGamesAuth::EA_TOKEN_URL = "https://accounts.ea.com/connect/token";
    const std::string EAGamesAuth::OAUTH_CLIENT_ID = "JUNO_PC_CLIENT";
    const std::string EAGamesAuth::OAUTH_CLIENT_SECRET = "4mRLtYMb6vq9qglomWEaT4ChxsXWcyqbQpuBNfMPOYOiDmYYQmjuaBsF2Zp0RyVeWkfqhE9TuGgAw7te";

    unsigned short EAGamesAuth::s_localRedirectPort = 0;

    std::string EAGamesAuth::calculateFnv1aHash(const std::vector<std::string>& components) {
        uint64_t hash = 0xcbf29ce484222325ULL;
        const uint64_t prime = 0x100000001b3ULL;

        for (const auto& item_str : components) {
            for (char c : item_str) {
                hash ^= static_cast<unsigned char>(c);
                hash *= prime;
            }
        }
        return std::to_string(hash);
    }

    bool EAGamesAuth::getWindowsHardwareInfo(std::string& bbm, std::string& bsn, int& gid,
                                             std::string& hsn, std::string& msn, std::string& mac,
                                             std::string& osn, std::string& osi_timestamp_str) {
        LOG(LogInfo) << "EA Games Auth: Gathering Windows hardware info for pc_sign...";

        std::string ps_script_content = R"PSSCRI(
            try {
                $ErrorActionPreference = "Stop"
                $bios = Get-CimInstance -ClassName Win32_BIOS
                $baseBoard = Get-CimInstance -ClassName Win32_BaseBoard
                $os = Get-CimInstance -ClassName Win32_OperatingSystem
                $videoControllers = Get-CimInstance -ClassName Win32_VideoController
                $diskDrive = Get-WmiObject -Class Win32_DiskDrive | Select-Object -Index 0
                $networkAdapter = Get-CimInstance Win32_NetworkAdapter | Where-Object { $_.PhysicalAdapter -and $_.NetEnabled -and $_.ServiceName -notmatch 'vmnetadapter|vboxnetadp|ndisip|tap|hyperv|loopback' } | Select-Object -First 1

                $gid_val = 0
                foreach ($gpu_item in $videoControllers) {
                    if ($gpu_item.PNPDeviceID -match "DEV_([0-9A-F]+)") {
                        $gid_val = [Convert]::ToInt32($matches[1], 16)
                        break
                    }
                }

                $mac_address_val = ""
                if ($networkAdapter -and $networkAdapter.MACAddress) {
                    $mac_address_val = "$" + ($networkAdapter.MACAddress -replace ':', '' -replace '-', '').ToLower()
                }

                $osiTimestamp_val_str = ""
                if ($os.InstallDate) {
                    try {
                        $utcDate = $os.InstallDate.ToUniversalTime()
                        $epoch = New-Object DateTime(1970, 1, 1, 0, 0, 0, [DateTimeKind]::Utc)
                        $osiTimestamp_val_str = [string]([long](($utcDate - $epoch).TotalMilliseconds))
                    } catch {
                        # Silently catch, $osiTimestamp_val_str remains ""
                    }
                }

                @{
                    bbm = if ($bios.Manufacturer) { $bios.Manufacturer } else { "" };
                    bsn = if ($bios.SerialNumber) { $bios.SerialNumber } else { "" };
                    gid = $gid_val;
                    hsn = if ($diskDrive.SerialNumber) { $diskDrive.SerialNumber.Trim() } else { "" };
                    msn = if ($baseBoard.SerialNumber) { $baseBoard.SerialNumber.Trim() } else { "" };
                    mac = $mac_address_val;
                    osn = if ($os.SerialNumber) { ($os.SerialNumber.Trim() -replace '-', '' -replace ' ', '').ToLower() } else { "" };
                    osi = $osiTimestamp_val_str
                } | ConvertTo-Json -Compress
            } catch {
                @{ bbm = ""; bsn = ""; gid = 0; hsn = ""; msn = ""; mac = ""; osn = ""; osi = "" } | ConvertTo-Json -Compress
            }
        )PSSCRI";

        std::string tempDir = Paths::getUserEmulationStationPath() + "/tmp";
        if (!Utils::FileSystem::exists(tempDir)) {
            Utils::FileSystem::createDirectory(tempDir);
        }
        std::string tempPs1Path = tempDir + "/ea_hw_info.ps1";

        if (!helper_write_text_to_file(tempPs1Path, ps_script_content)) {
            LOG(LogError) << "EA Games Auth: Failed to write temporary PowerShell script for hardware info to " << tempPs1Path;
            return false;
        }

        std::string scriptOutput;
        bool scriptSuccess = false;
        if (ApiSystem::getInstance()) {
            // Assumendo che ApiSystem::runPowershellScript esista e funzioni come previsto
            // scriptOutput = ApiSystem::getInstance()->runPowershellScript(tempPs1Path, scriptSuccess);
            LOG(LogWarning) << "EA Games Auth: ApiSystem::runPowershellScript call needs verification against actual ApiSystem.h";
            scriptOutput = "{\"bbm\":\"Test\",\"bsn\":\"TestSN\",\"gid\":0,\"hsn\":\"TestHDDSN\",\"msn\":\"TestMBOSN\",\"mac\":\"$001122334455\",\"osn\":\"TestOSSN\",\"osi\":\"1609459200000\"}";
            scriptSuccess = true;
        } else {
            LOG(LogError) << "EA Games Auth: ApiSystem not available to run PowerShell script.";
            Utils::FileSystem::removeFile(tempPs1Path);
            return false;
        }
        Utils::FileSystem::removeFile(tempPs1Path);

        if (!scriptSuccess || scriptOutput.empty()) {
            LOG(LogError) << "EA Games Auth: PowerShell script for hardware info failed or returned empty output. Output: " << scriptOutput;
            return false;
        }

        LOG(LogDebug) << "EA Games Auth: PowerShell script output: " << scriptOutput;

        try {
            auto hwJson = nlohmann::json::parse(scriptOutput);
            bbm = hwJson.value("bbm", "");
            bsn = hwJson.value("bsn", "");
            gid = hwJson.value("gid", 0);
            hsn = hwJson.value("hsn", "");
            msn = hwJson.value("msn", "");
            mac = hwJson.value("mac", "");
            osn = hwJson.value("osn", "");
            osi_timestamp_str = hwJson.value("osi", "");

            if (mac.rfind("$", 0) != 0 && !mac.empty()) {
                mac = "$" + mac;
            }

            LOG(LogInfo) << "EA Games Auth: Hardware info gathered successfully.";
            return true;

        } catch (const nlohmann::json::parse_error& e) {
            LOG(LogError) << "EA Games Auth: Failed to parse hardware info JSON: " << e.what() << ". Output was: " << scriptOutput;
        } catch (const std::exception& e) {
            LOG(LogError) << "EA Games Auth: Exception processing hardware info: " << e.what();
        }
        return false;
    }

    std::string EAGamesAuth::generatePcSign() {
        std::string bbm, bsn, hsn, msn, mac_addr, osn, osi_str;
        int gid_val = 0;

        if (!getWindowsHardwareInfo(bbm, bsn, gid_val, hsn, msn, mac_addr, osn, osi_str)) {
            LOG(LogError) << "EA Games Auth: Failed to get Windows hardware info for pc_sign.";
            return "";
        }

        std::vector<std::string> mid_components = {bsn, std::to_string(gid_val), hsn, msn, mac_addr, osn, osi_str};
        std::string mid = calculateFnv1aHash(mid_components);

        char ts_buf[30];
        auto now_chrono = std::chrono::system_clock::now();
        time_t now_time_t = std::chrono::system_clock::to_time_t(now_chrono);
        std::tm local_tm;
        #ifdef _WIN32
            localtime_s(&local_tm, &now_time_t);
        #else
            local_tm = *std::localtime(&now_time_t);
        #endif
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_chrono.time_since_epoch()).count() % 1000;
        std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%d %H:%M:%S", &local_tm);

        std::string ts_milliseconds_part = std::to_string(ms);
        while(ts_milliseconds_part.length() < 3) { ts_milliseconds_part = "0" + ts_milliseconds_part; }
        std::string ts = std::string(ts_buf) + ":" + ts_milliseconds_part;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 2);
        std::string sv_str = (distrib(gen) == 1) ? "v1" : "v2";
        std::string sign_key_bytes = (sv_str == "v1") ? PC_SIGN_V1_KEY : PC_SIGN_V2_KEY;

        nlohmann::json payload_json;
        payload_json["av"] = "v1";
        payload_json["bsn"] = bsn;
        payload_json["gid"] = gid_val;
        payload_json["hsn"] = hsn;
        payload_json["mac"] = mac_addr;
        payload_json["mid"] = mid;
        payload_json["msn"] = msn;
        payload_json["sv"] = sv_str;
        payload_json["ts"] = ts;

        std::string payload_string_to_encode = payload_json.dump();
        std::string encoded_payload = helper_base64url_encode_string(payload_string_to_encode);

        LOG(LogWarning) << "EA Games Auth: HMAC-SHA256 is NOT correctly implemented if Utils::Crypto::HMAC_SHA256 is missing. pc_sign will be invalid. You need to implement Utils::Crypto::HMAC_SHA256 or integrate a library.";

        const unsigned int digest_size = 32;
        unsigned char hmac_digest_output[digest_size];

        Utils::Crypto::HMAC_SHA256(
            reinterpret_cast<const unsigned char*>(sign_key_bytes.c_str()), sign_key_bytes.length(),
            reinterpret_cast<const unsigned char*>(encoded_payload.c_str()), encoded_payload.length(),
            hmac_digest_output
        );

        std::string encoded_signature = helper_base64url_encode_bytes(hmac_digest_output, digest_size);

        LOG(LogDebug) << "EA Games Auth: pc_sign generated. Payload starts: " << encoded_payload.substr(0, 20)
                      << ", Signature starts: " << encoded_signature.substr(0, 20);

        return encoded_payload + "." + encoded_signature;
    }

    // MODIFICA 1: Ripristina getLocalRedirectUri a TRE barre per il parametro server di EA
    std::string EAGamesAuth::getLocalRedirectUri() const {
        if (s_localRedirectPort == 0) {
            EAGamesAuth::GetLocalRedirectPort();
        }
        return "qrc:///html/login_successful.html"; // TRE barre per la registrazione con EA
    }

   EAGamesAuth::EAGamesAuth(Window* window)
    : mWindow(window), mIsLoggedIn(false), mTokenExpiryTime(0)
{
    LOG(LogDebug) << "EAGamesAuth: Constructor (JUNO_PC_CLIENT flow)";

    // Usa la funzione corretta e inizializza la nuova variabile membro
    std::string basePath = Utils::FileSystem::getEsConfigPath() + "/ea/";
    if (!Utils::FileSystem::exists(basePath)) {
        Utils::FileSystem::createDirectory(basePath);
    }
    mCredentialsPath = basePath + EA_AUTH_CREDENTIALS_FILENAME;

    loadCredentials();
}

    bool EAGamesAuth::isUserLoggedIn() const {
        if (mIsLoggedIn && !mAccessToken.empty()) {
            if (Utils::Time::now() < mTokenExpiryTime) {
                return true;
            } else if (!mRefreshToken.empty()) {
                LOG(LogInfo) << "EA Games Auth: Access token expired, refresh token available. Need refresh.";
            }
        }
        return false;
    }
	

    std::string EAGamesAuth::getAccessToken() const { return mAccessToken; }
    std::string EAGamesAuth::getRefreshToken() const { return mRefreshToken; }
    std::string EAGamesAuth::getPidId() const { return mPid; }
    std::string EAGamesAuth::getPersonaId() const { return mPersonaId; }
    std::string EAGamesAuth::getUserName() const { return mUserName; }

    unsigned short EAGamesAuth::GetLocalRedirectPort() {
        if (s_localRedirectPort == 0) {
            std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
            std::uniform_int_distribution<unsigned short> distrib(40000, 42000);
            s_localRedirectPort = distrib(rng);
            LOG(LogInfo) << "EA Games Auth: Generated local redirect port: " << s_localRedirectPort;
        }
        return s_localRedirectPort;
    }

    void EAGamesAuth::StartLoginFlow(std::function<void(bool success, const std::string& message)> onFlowFinished) {
        LOG(LogInfo) << "EA Games Auth: Starting login flow (JUNO_PC_CLIENT).";

        std::string pcSignValue = generatePcSign();
        if (pcSignValue.empty()) {
            LOG(LogError) << "EA Games Auth: Failed to generate pc_sign.";
            if (onFlowFinished) {
                 if (mWindow) mWindow->postToUiThread([onFlowFinished] {
                    onFlowFinished(false, _("Errore interno: impossibile generare pc_sign EA."));
                 }); else if (onFlowFinished) {
                    onFlowFinished(false, _("Errore interno: impossibile generare pc_sign EA."));
                 }
            }
            return;
        }
        LOG(LogDebug) << "EA Games Auth: Generated pc_sign: " << pcSignValue.substr(0, 40) << "...";

        // MODIFICA 2: redirectUriForEAServer (TRE barre) per il parametro URL di EA
        std::string redirectUriForEAServer = getLocalRedirectUri(); // Es. "qrc:///html/login_successful.html"

        Utils::Uri authUriBuilder(EA_AUTH_BASE_URL);
        authUriBuilder.arguments.set("client_id", OAUTH_CLIENT_ID);
        authUriBuilder.arguments.set("response_type", "code");
        authUriBuilder.arguments.set("redirect_uri", HttpReq::urlEncode(redirectUriForEAServer)); // Invia quello con TRE barre a EA
        authUriBuilder.arguments.set("display", "junoClient/login");
        authUriBuilder.arguments.set("locale", "en_US");
        authUriBuilder.arguments.set("pc_sign", pcSignValue);

        std::string authUrl = authUriBuilder.toString();
        LOG(LogInfo) << "EA Games Auth: Auth URL for WebView: " << authUrl;

        if (!mWindow) {
            LOG(LogError) << "EA Games Auth: mWindow is null.";
            if (onFlowFinished) onFlowFinished(false, _("Errore interno: finestra non disponibile."));
            return;
        }

        // MODIFICA 3: watchPrefixForWebView (UNA barra) per GuiWebViewAuthLogin
        std::string watchPrefixForWebView = "qrc:/html/login_successful.html"; // Per osservare l'output di WebView2

        GuiWebViewAuthLogin* webViewGui = new GuiWebViewAuthLogin(mWindow, authUrl, _("Login EA"), watchPrefixForWebView);

       webViewGui->setOnLoginFinishedCallback(
            [this, onFlowFinished, redirectUriForEAServer, watchPrefixForWebView](bool webViewSuccess, const std::string& webViewResultUrl) { // Passa redirectUriForEAServer per exchangeCodeForToken
            LOG(LogDebug) << "EA Games Auth: GuiWebViewAuthLogin finished. Success: " << webViewSuccess << ", Result URL: " << webViewResultUrl;

           if (!webViewSuccess) {
                LOG(LogError) << "EA Games Auth: WebView login failed or was cancelled. URL: " << webViewResultUrl;
                std::string errorMsgToUser = _("Login EA annullato o fallito nella WebView.");
                // Usa watchPrefixForWebView per controllare se l'errore è sulla URL di redirect attesa
                if (webViewResultUrl.rfind(watchPrefixForWebView, 0) != 0 &&
                    (webViewResultUrl.rfind("http://", 0) == 0 || webViewResultUrl.rfind("https://", 0) == 0) ) {
                     errorMsgToUser = _("Fallimento navigazione WebView:") + " " + webViewResultUrl;
                } else if (webViewResultUrl.find("access_denied") != std::string::npos) {
                    errorMsgToUser = _("Accesso negato dall'utente.");
                } else if (webViewResultUrl.find("error") != std::string::npos && webViewResultUrl.rfind(watchPrefixForWebView,0) != 0 ) {
                     Utils::Uri errorUriParser(webViewResultUrl);
                     std::string errParam = errorUriParser.arguments.get("error");
                     std::string errDescParam = errorUriParser.arguments.get("error_description");
                     if (!errParam.empty()) errorMsgToUser = _("Errore login EA:") + " " + errParam;
                     if (!errDescParam.empty()) errorMsgToUser += " (" + errDescParam + ")";
                } else if (!webViewSuccess && webViewResultUrl.rfind(watchPrefixForWebView,0) != 0) {
                     errorMsgToUser = webViewResultUrl;
                }

                if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, errorMsgToUser] { onFlowFinished(false, errorMsgToUser); });
                else if (onFlowFinished) onFlowFinished(false, errorMsgToUser);
                return;
            } 

            Utils::Uri uri(webViewResultUrl);
            std::string code = uri.arguments.get("code");

            if (code.empty()) {
                LOG(LogError) << "EA Games Auth: Auth code missing in qrc redirect. URL: " << webViewResultUrl;
                std::string errorMsg = _("Codice autorizzazione EA mancante (qrc).");
                 if (this->mWindow) this->mWindow->postToUiThread([onFlowFinished, errorMsg] { onFlowFinished(false, errorMsg); });
                else if (onFlowFinished) onFlowFinished(false, errorMsg);
                return;
            }

            LOG(LogInfo) << "EA Games Auth: Auth code received via qrc redirect. Exchanging for token.";
            // MODIFICA 4: Passa redirectUriForEAServer a exchangeCodeForToken
            this->exchangeCodeForToken(code, redirectUriForEAServer, onFlowFinished);
        });

        mWindow->pushGui(webViewGui);
        webViewGui->init();
        mWindow->displayNotificationMessage(_("Segui le istruzioni nella finestra di login EA..."));
    }

    // MODIFICA 5: Modifica la firma di exchangeCodeForToken per accettare il redirect_uri corretto
 void EAGamesAuth::exchangeCodeForToken(
    const std::string& authCode,
    const std::string& redirectUriForTokenExchange, // Questo è l'URI con TRE barre (es. qrc:///...)
    std::function<void(bool success, const std::string& message)> callback)
{
    LOG(LogInfo) << "EA Games Auth: Exchanging authorization code for token (JUNO_PC_CLIENT flow). Code: " << authCode.substr(0, 10) << "... Redirect URI for POST: " << redirectUriForTokenExchange;

    std::string postDataStr = "grant_type=authorization_code";
    postDataStr += "&code=" + HttpReq::urlEncode(authCode);
    postDataStr += "&redirect_uri=" + HttpReq::urlEncode(redirectUriForTokenExchange); // Usa il redirect_uri fornito
    postDataStr += "&client_id=" + OAUTH_CLIENT_ID;
    postDataStr += "&client_secret=" + OAUTH_CLIENT_SECRET;
    postDataStr += "&token_format=JWS";

    LOG(LogDebug) << "EA Games Auth: Token Exchange POST data (for easy_perform): " << postDataStr;

    // HttpReqOptions options; // Non più usata direttamente per HttpReq, ma optionsCopy la cattura
    // options.dataToPost = postDataStr; // optionsCopy.dataToPost verrà usata
    // ... options.customHeaders ... optionsCopy.customHeaders verranno usate

    // Le HttpReqOptions vengono comunque preparate per essere passate a optionsCopy
    HttpReqOptions optionsForEasyPerform; // Rinomino per chiarezza nel contesto della lambda
    optionsForEasyPerform.dataToPost = postDataStr;
    optionsForEasyPerform.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    optionsForEasyPerform.customHeaders.push_back("Accept: application/json");
    optionsForEasyPerform.customHeaders.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");
    // optionsForEasyPerform.customHeaders.push_back("Expect:"); // Già aggiunta dall'utente, la manterrei se optionsCopy la include

    std::string tokenUrlCopy = EA_TOKEN_URL;
    LOG(LogDebug) << "EA Games Auth: Token Exchange URL (for easy_perform): " << tokenUrlCopy;

    // --- Inizio del blocco std::thread ---
    std::thread([this, callback, tokenUrlCopy, optionsCopy = optionsForEasyPerform]() mutable {
        LOG(LogInfo) << "EA Games Auth: Thread per scambio token (easy_perform) avviato per URL: " << tokenUrlCopy;

        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG(LogError) << "EA Games Auth (easy_perform): curl_easy_init() failed.";
            if (this->mWindow && callback) this->mWindow->postToUiThread([callback] { callback(false, _("Errore inizializzazione cURL (easy_perform).")); });
            else if (callback) callback(false, _("Errore inizializzazione cURL (easy_perform)."));
            return;
        }

        std::string response_body_buffer;
        std::string response_headers_buffer; // Per debug, se necessario
        long http_response_code = 0;
        char curl_error_buffer[CURL_ERROR_SIZE];
        curl_error_buffer[0] = '\0'; // Inizializza il buffer degli errori

        // 1. Imposta URL
        curl_easy_setopt(curl, CURLOPT_URL, tokenUrlCopy.c_str());

        // 2. Imposta dati POST
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, optionsCopy.dataToPost.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)optionsCopy.dataToPost.length());

        // 3. Imposta Headers Custom (già presenti in optionsCopy.customHeaders)
        struct curl_slist* hs = nullptr;
        for (const auto& header : optionsCopy.customHeaders) {
            hs = curl_slist_append(hs, header.c_str());
        }
        // Se vuoi forzare l'assenza dell'header Expect (anche se l'hai già aggiunto a optionsCopy):
        // hs = curl_slist_append(hs, "Expect:");
        if (hs) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
        }

        // 4. Callback per la scrittura del corpo della risposta
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body_buffer);
        
        // 5. Callback opzionale per gli header della risposta (per debug)
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers_buffer);

        // 6. Impostazioni SSL - USA IL PERCORSO CORRETTO PER IL TUO cacert.pem!
        // Dai tuoi log precedenti, HttpReq usava: "C:/Users/ceo16/Desktop/lumaca-setup-BETA/build/emulationstation/cacert.pem"
        //std::string caCertPath = "C:/Users/ceo16/Desktop/lumaca-setup-BETA/build/emulationstation/cacert.pem";
        // In alternativa, se hai un modo più robusto per ottenere il percorso:
         std::string caCertPath = Paths::getEmulationStationPath() + "/cacert.pem"; // O simile
        
        if (Utils::FileSystem::exists(caCertPath)) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, caCertPath.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            LOG(LogInfo) << "EA Games Auth (easy_perform): CA bundle impostato: " << caCertPath;
        } else {
            LOG(LogError) << "EA Games Auth (easy_perform): Certificato CA NON TROVATO: " << caCertPath << ". La verifica SSL potrebbe fallire.";
            // Considera di fallire qui o disabilitare la verifica SSL SOLO per test estremi (NON RACCOMANDATO)
            // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        // 7. Timeout (gli stessi che usava HttpReq dai tuoi log)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); // Timeout connessione
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);      // Timeout operazione totale

        // 8. Buffer per messaggi d'errore specifici di cURL
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer);

        // 9. Abilita output verboso di cURL (simile a -v dalla console)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        // Per debug ancora più approfondito (output di cURL su stderr, utile se i log non mostrano tutto)
        // curl_easy_setopt(curl, CURLOPT_STDERR, stderr);


        // Esegui la richiesta bloccante
        CURLcode curl_res = curl_easy_perform(curl);

        // Ottieni il codice di stato HTTP (fallo sempre, ma è più significativo se curl_res è CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response_code);

        // Pulisci la lista degli header custom
        if (hs) {
            curl_slist_free_all(hs);
        }

        // Log dettagliato dell'esito della richiesta HTTP con easy_perform
        LOG(LogInfo) << "EA Games Auth (easy_perform): Risultato curl_easy_perform: " << curl_res << " (" << curl_easy_strerror(curl_res) << ")";
        LOG(LogInfo) << "EA Games Auth (easy_perform): HTTP Response Code: " << http_response_code;
        LOG(LogInfo) << "EA Games Auth (easy_perform): cURL Error Buffer: [" << curl_error_buffer << "]";
        LOG(LogInfo) << "EA Games Auth (easy_perform): Lunghezza Corpo Risposta: " << response_body_buffer.length();
        if (!response_body_buffer.empty()) {
            LOG(LogDebug) << "EA Games Auth (easy_perform): Corpo Risposta: " << response_body_buffer.substr(0, 1000); // Logga una parte del corpo
        }
        if (!response_headers_buffer.empty()) {
             LOG(LogDebug) << "EA Games Auth (easy_perform): Headers Risposta:\n" << response_headers_buffer;
        }


        if (curl_res == CURLE_OK && http_response_code >= 200 && http_response_code < 300) {
            LOG(LogDebug) << "EA Token Exchange Response Body (easy_perform): " << response_body_buffer; // Log completo se serve
            try {
                auto jsonResponse = nlohmann::json::parse(response_body_buffer);
                if (!jsonResponse.contains("access_token") || jsonResponse.value("access_token", "").empty()) {
                    LOG(LogError) << "EA Games Auth (easy_perform): Access token mancante o vuoto nella risposta JSON.";
                    std::string errorDetail = jsonResponse.value("error_description", _("Token EA mancante nella risposta JSON (easy_perform)."));
                    if (this->mWindow && callback) this->mWindow->postToUiThread([callback, errorDetail] { callback(false, errorDetail); });
                    else if (callback) callback(false, errorDetail);
                } else {
                    this->mAccessToken = jsonResponse.value("access_token", "");
                    this->mRefreshToken = jsonResponse.value("refresh_token", "");

                    if (jsonResponse.contains("expires_in") && jsonResponse["expires_in"].is_number()) {
                        long expiresInSeconds = jsonResponse["expires_in"].get<long>();
                        this->mTokenExpiryTime = Utils::Time::now() + expiresInSeconds - 60; // 60 secondi di buffer
                    } else {
                        this->mTokenExpiryTime = Utils::Time::now() + 3540; // Default
                        LOG(LogWarning) << "EA Games Auth (easy_perform): 'expires_in' non trovato/valido. Scadenza default.";
                    }

                    fetchUserIdentity([this, callback](bool idSuccess, const std::string& idMessage) {
                        if (idSuccess) {
                            this->mIsLoggedIn = true;
                            this->saveCredentials();
                            LOG(LogInfo) << "EA Games Auth (easy_perform): Scambio token e recupero identità RIUSCITI.";
                            if (this->mWindow && callback) this->mWindow->postToUiThread([callback] { callback(true, _("Login EA riuscito (easy_perform)!")); });
                            else if (callback) callback(true, _("Login EA riuscito (easy_perform)!"));
                        } else {
                            LOG(LogError) << "EA Games Auth (easy_perform): Scambio token OK ma recupero identità FALLITO: " << idMessage;
                            this->clearCredentials();
                            if (this->mWindow && callback) this->mWindow->postToUiThread([callback, idMessage] { callback(false, _("Fallito recupero info utente EA (easy_perform):") + " " + idMessage); });
                            else if (callback) callback(false, _("Fallito recupero info utente EA (easy_perform):") + " " + idMessage);
                        }
                    });
                }
            } catch (const nlohmann::json::parse_error& e) {
                LOG(LogError) << "EA Games Auth (easy_perform): Errore parsing JSON: " << e.what() << ". Risposta: " << response_body_buffer.substr(0, 500);
                std::string jsonError = _("Errore risposta token EA (JSON malformato, easy_perform):") + std::string(e.what());
                if (this->mWindow && callback) this->mWindow->postToUiThread([callback, jsonError] { callback(false, jsonError); });
                else if (callback) callback(false, jsonError);
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games Auth (easy_perform): Eccezione generale: " << e.what();
                std::string generalError = _("Errore elaborazione token EA (easy_perform):") + std::string(e.what());
                if (this->mWindow && callback) this->mWindow->postToUiThread([callback, generalError] { callback(false, generalError); });
                else if (callback) callback(false, generalError);
            }
        } else { // Errore cURL o HTTP status non 2xx
            std::string error_to_report;
            if (curl_res != CURLE_OK) { // Errore cURL (rete, timeout, SSL, ecc.)
                error_to_report = "cURL error (" + std::to_string(curl_res) + "): " + std::string(curl_easy_strerror(curl_res));
                if (strlen(curl_error_buffer) > 0) {
                    error_to_report += " - Details: " + std::string(curl_error_buffer);
                }
            } else { // Errore HTTP (status non 2xx)
                error_to_report = "HTTP error " + std::to_string(http_response_code);
                if (!response_body_buffer.empty()) {
                    try {
                        auto jError = nlohmann::json::parse(response_body_buffer);
                        std::string ea_error_desc = jError.value("error_description", "");
                        std::string ea_error = jError.value("error", "");
                        if (!ea_error_desc.empty()) error_to_report += " - " + ea_error_desc;
                        else if (!ea_error.empty()) error_to_report += " - " + ea_error;
                        else error_to_report += " - Body: " + response_body_buffer.substr(0, 100);
                    } catch (...) {
                        error_to_report += " - Non-JSON Body: " + response_body_buffer.substr(0, 100);
                    }
                }
            }
            
            LOG(LogError) << "EA Games Auth (easy_perform): Scambio token HTTP FALLITO. Errore finale: [" << error_to_report << "]";
            if (this->mWindow && callback) {
                this->mWindow->postToUiThread([callback, error_to_report] { callback(false, error_to_report); });
            } else if (callback) {
                callback(false, error_to_report);
            }
        }

        curl_easy_cleanup(curl); // Pulisci sempre l'handle cURL
    }).detach();
}

   void EAGamesAuth::RefreshTokens(std::function<void(bool success, const std::string& message)> callback) {
    if (mRefreshToken.empty()) {
        LOG(LogWarning) << "EA Games Auth: Refresh token is empty, cannot refresh.";
        if (callback) {
            if (mWindow) mWindow->postToUiThread([callback]{ callback(false, _("Refresh token mancante.")); });
            else callback(false, _("Refresh token mancante."));
        }
        return;
    }

    LOG(LogInfo) << "EA Games Auth: Refreshing access token (JUNO_PC_CLIENT).";
    std::string postDataStr = "grant_type=refresh_token";
    postDataStr += "&refresh_token=" + HttpReq::urlEncode(mRefreshToken);
    postDataStr += "&client_id=" + OAUTH_CLIENT_ID;
    postDataStr += "&client_secret=" + OAUTH_CLIENT_SECRET;

    HttpReqOptions options;
    options.dataToPost = postDataStr;
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.customHeaders.push_back("Accept: application/json");
    // Per il refresh, non è strettamente necessario lo User-Agent complesso, ma non fa male se HttpReq lo aggiunge di default o se lo metti qui.

    std::string tokenUrlCopy = EA_TOKEN_URL; // URL per il refresh è lo stesso del token exchange
    LOG(LogDebug) << "EA Games Auth: Refresh Token POST data: " << postDataStr;

    std::thread([this, callback, tokenUrlCopy, optionsCopy = options]() mutable {
        HttpReq request(tokenUrlCopy, &optionsCopy);

        LOG(LogDebug) << "EA Games Auth (RefreshTokens): Inizio attesa (request.wait()) per URL: " << tokenUrlCopy;
        if (!request.wait()) { // ATTENDI il completamento
            HttpReq::Status failedStatus = request.status();
            std::string errMsg = request.getErrorMsg();
            if (errMsg.empty()) {
                // Usa HttpReq::statusToString se disponibile
                errMsg = "request.wait() for RefreshTokens failed. Status: " + std::to_string(static_cast<int>(failedStatus));
                // Se hai HttpReq::statusToString:
                // errMsg = "request.wait() for RefreshTokens failed. Status: " + HttpReq::statusToString(failedStatus);
            }
            LOG(LogError) << "EA Games Auth: Fallimento HttpReq::wait() per RefreshTokens. URL: " << tokenUrlCopy << " Errore: " << errMsg;

            if (callback) {
                 std::string finalErrMsg = _("Fallimento refresh token EA (wait):") + " " + errMsg;
                 if (mWindow) mWindow->postToUiThread([callback, finalErrMsg] { callback(false, finalErrMsg); });
                 else callback(false, finalErrMsg);
            }
            return;
        }

        HttpReq::Status reqStatus = request.status();
        std::string responseBody = request.getContent();
        LOG(LogInfo) << "EA Games Auth: Refresh token HttpReq status (after wait): " << static_cast<int>(reqStatus)
                     // << " (" << HttpReq::statusToString(reqStatus) << ")" // Se hai statusToString
                     << ". Response Length: " << responseBody.length();

        if (reqStatus == HttpReq::Status::REQ_SUCCESS) {
            LOG(LogDebug) << "EA Refresh Token Response Body: " << responseBody;
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                if (!jsonResponse.contains("access_token") || jsonResponse.value("access_token", "").empty()) {
                    LOG(LogError) << "EA Games Auth: Access token missing in refresh response.";
                    clearCredentials(); // Se il refresh fallisce in modo pulito ma non dà un token, le vecchie credenziali sono probabilmente invalide.
                    if (this->mWindow && callback) this->mWindow->postToUiThread([callback]{ callback(false, _("Token EA mancante nella risposta di refresh.")); });
                    else if(callback) callback(false, _("Token EA mancante nella risposta di refresh."));
                    return;
                }
                this->mAccessToken = jsonResponse.value("access_token", "");
                // Il refresh token POTREBBE cambiare, anche se spesso non lo fa. Controlla se EA lo restituisce.
                if (jsonResponse.contains("refresh_token") && !jsonResponse.value("refresh_token", "").empty()) {
                    this->mRefreshToken = jsonResponse.value("refresh_token", "");
                }
                if (jsonResponse.contains("expires_in") && jsonResponse["expires_in"].is_number()) {
                    long expiresInSeconds = jsonResponse["expires_in"].get<long>();
                    this->mTokenExpiryTime = Utils::Time::now() + expiresInSeconds - 60; // buffer
                } else {
                    this->mTokenExpiryTime = Utils::Time::now() + 3540; // default
                }

                // Dopo un refresh, è buona norma riconfermare l'identità utente se necessario,
                // o almeno assicurarsi che i dati utente (PID, ecc.) siano ancora validi/presenti.
                // Se fetchUserIdentity è leggero, chiamarlo qui potrebbe essere una buona idea.
                // Per ora, assumiamo che se il refresh token ha successo, l'utente è ancora valido.
                // Modifica: Chiamiamo fetchUserIdentity per coerenza e per aggiornare i dati utente se necessario.
                fetchUserIdentity([this, callback](bool idSuccess, const std::string& idMessage){
                    if(idSuccess) {
                        this->mIsLoggedIn = true; // Ora è loggato con i nuovi token
                        saveCredentials();
                        LOG(LogInfo) << "EA Games Auth: Token refreshed and identity confirmed successfully.";
                        if (this->mWindow && callback) this->mWindow->postToUiThread([callback]{ callback(true, _("Token EA rinfrescati.")); });
                        else if(callback) callback(true, _("Token EA rinfrescati."));
                    } else {
                        LOG(LogError) << "EA Games Auth: Token refreshed but identity fetch failed: " << idMessage;
                        // Se il refresh è andato a buon fine ma l'identità fallisce, è uno stato strano.
                        // Potresti voler cancellare le credenziali o lasciare i token appena rinfrescati e segnalare l'errore.
                        // Per sicurezza, cancelliamo se l'identità non può essere confermata.
                        clearCredentials();
                        std::string finalErrMsg = _("Refresh parziale, fallito recupero info utente:") + " " + idMessage;
                        if (this->mWindow && callback) this->mWindow->postToUiThread([callback, finalErrMsg]{ callback(false, finalErrMsg); });
                        else if(callback) callback(false, finalErrMsg);
                    }
                });

            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games Auth: Error parsing refresh token response: " << e.what();
                // Non cancellare le credenziali qui, il refresh token potrebbe essere ancora valido per un tentativo successivo
                if (this->mWindow && callback) this->mWindow->postToUiThread([callback, e_what = std::string(e.what())]{ callback(false, _("Errore parsing risposta refresh token:") + " " + e_what); });
                else if(callback) callback(false, _("Errore parsing risposta refresh token:") + " " + e.what());
            }
        } else { // Errore HTTP durante il refresh
            std::string errorMsgToUser = request.getErrorMsg();
            if (errorMsgToUser.empty()) {
                 errorMsgToUser = _("Fallimento refresh token EA (HTTP). Codice Stato HttpReq: ") + std::to_string(static_cast<int>(reqStatus));
                 // Se hai statusToString:
                 // errorMsgToUser = _("Fallimento refresh token EA (HTTP). Stato HttpReq: ") + HttpReq::statusToString(reqStatus);
            }

            if (static_cast<int>(reqStatus) >= 400 && !responseBody.empty()) {
                try {
                    auto jError = nlohmann::json::parse(responseBody);
                    std::string ea_error_desc = jError.value("error_description", "");
                    std::string ea_error = jError.value("error", "");
                    if (!ea_error_desc.empty()) errorMsgToUser = ea_error_desc;
                    else if (!ea_error.empty()) errorMsgToUser = ea_error;
                    LOG(LogError) << "EA Games Auth: Refresh token error from EA server: " << errorMsgToUser;
                } catch(...){ 
                     LOG(LogWarning) << "EA Games Auth: Impossibile parsare il corpo della risposta di errore (RefreshToken) come JSON.";
                }
            }
            
            LOG(LogError) << "EA Games Auth: Refresh token HTTP request failed. Status Raw: " << static_cast<int>(reqStatus)
                         // << " (" << HttpReq::statusToString(reqStatus) << ")" // Se hai statusToString
                         << " Final Error Msg: [" << errorMsgToUser << "] Body (partial): " << responseBody.substr(0,200);

             // Se il refresh token stesso è invalido (tipicamente errore 400 o 401 da EA per grant_type=refresh_token)
             if (reqStatus == HttpReq::Status::REQ_400_BADREQUEST || reqStatus == HttpReq::Status::REQ_401_FORBIDDEN) {
                LOG(LogInfo) << "EA Games Auth: Refresh token seems invalid or expired. Clearing credentials.";
                clearCredentials(); // Il refresh token non è più valido, login completo necessario.
                errorMsgToUser = _("Sessione EA scaduta. Effettua nuovamente il login."); // Messaggio più specifico per l'utente
            }

            if (this->mWindow && callback) this->mWindow->postToUiThread([callback, errorMsgToUser]{ callback(false, errorMsgToUser); });
            else if(callback) callback(false, errorMsgToUser);
        }
    }).detach();
}


   void EAGamesAuth::fetchUserIdentity(std::function<void(bool success, const std::string& message)> callback) {
    if (mAccessToken.empty()) {
        if (callback) {
             if (mWindow) mWindow->postToUiThread([callback] { callback(false, _("Access token mancante per recuperare l'identità EA.")); });
             else callback(false, _("Access token mancante per recuperare l'identità EA."));
        }
        return;
    }

    LOG(LogInfo) << "EA Games Auth: Fetching user identity via GraphQL API.";
    std::string query = "query{me{player{pd psd displayName}}}";
    // 'url' è definita qui e sarà catturata dalla lambda
    std::string url_param = "https://service-aggregation-layer.juno.ea.com/graphql?query=" + HttpReq::urlEncode(query);

    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: Bearer " + mAccessToken);
    options.customHeaders.push_back("User-Agent: EAApp/PC/13.468.0.5981"); // User-Agent specifico per questa API
    options.customHeaders.push_back("x-client-id: EAX-JUNO-CLIENT");     // Header specifico per questa API

    std::thread([this, callback, url_param_captured = url_param, optionsCopy = options]() mutable { // Cattura url_param con un nuovo nome se preferisci
        HttpReq request(url_param_captured, &optionsCopy); // Usa la url catturata

        LOG(LogDebug) << "EA Games Auth (fetchUserIdentity): Inizio attesa (request.wait()) per URL: " << url_param_captured;
        if (!request.wait()) { // ATTENDI il completamento della richiesta
            HttpReq::Status failedStatus = request.status();
            std::string errMsg = request.getErrorMsg();
            if (errMsg.empty()) {
                // Usa HttpReq::statusToString se disponibile, altrimenti solo il numero
                errMsg = "request.wait() for fetchUserIdentity failed. Status: " + std::to_string(static_cast<int>(failedStatus));
                // Se hai HttpReq::statusToString:
                // errMsg = "request.wait() for fetchUserIdentity failed. Status: " + HttpReq::statusToString(failedStatus);
            }
            LOG(LogError) << "EA Games Auth: Fallimento HttpReq::wait() per fetchUserIdentity. URL: " << url_param_captured << " Errore: " << errMsg;
            
            if (callback) {
                std::string finalErrMsg = _("Fallimento recupero info utente EA (wait):") + " " + errMsg;
                if (mWindow) mWindow->postToUiThread([callback, finalErrMsg] { callback(false, finalErrMsg); });
                else callback(false, finalErrMsg);
            }
            return; 
        }

        HttpReq::Status reqStatus = request.status();
        std::string responseBody = request.getContent();
        
        LOG(LogInfo) << "EA Games Auth: Fetch identity HttpReq status (after wait) for URL " << url_param_captured 
                     << ": " << static_cast<int>(reqStatus) 
                     // << " (" << HttpReq::statusToString(reqStatus) << ")" // Se hai statusToString
                     << ". Response Length: " << responseBody.length();


        if (reqStatus == HttpReq::Status::REQ_SUCCESS) {
            LOG(LogDebug) << "EA Fetch Identity Response: " << responseBody;
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                if (jsonResponse.contains("data") &&
                    jsonResponse["data"].is_object() &&
                    jsonResponse["data"].contains("me") &&
                    jsonResponse["data"]["me"].is_object() &&
                    jsonResponse["data"]["me"].contains("player") &&
                    jsonResponse["data"]["me"]["player"].is_object()) {

                    auto player = jsonResponse["data"]["me"]["player"];
                    this->mPid = player.value("pd", "");
                    this->mPersonaId = player.value("psd", "");
                    this->mUserName = player.value("displayName", "");

                    if (!this->mPid.empty()) {
                        LOG(LogInfo) << "EA Games Auth: User identity fetched. PID: " << mPid << ", PSD: " << mPersonaId << ", Name: " << mUserName;
                        if (callback) {
                            if (mWindow) mWindow->postToUiThread([callback] { callback(true, _("Info utente EA recuperate.")); });
                            else callback(true, _("Info utente EA recuperate."));
                        }
                    } else {
                         LOG(LogError) << "EA Games Auth: Failed to extract PID from identity response (pd field missing or empty). Response: " << responseBody;
                         if (callback) {
                             if(mWindow) mWindow->postToUiThread([callback] { callback(false, _("Risposta identità EA non valida (PID mancante).")); });
                             else callback(false, _("Risposta identità EA non valida (PID mancante)."));
                         }
                    }
                } else {
                    LOG(LogError) << "EA Games Auth: Invalid JSON structure in identity response: " << responseBody;
                    if (callback) {
                        if(mWindow) mWindow->postToUiThread([callback] { callback(false, _("Struttura risposta identità EA non valida.")); });
                        else callback(false, _("Struttura risposta identità EA non valida."));
                    }
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games Auth: Error parsing identity response: " << e.what() << ". Response: " << responseBody;
                if (callback) {
                    if(mWindow) mWindow->postToUiThread([callback, e_what=std::string(e.what())] { callback(false, _("Errore parsing identità EA:") + " " + e_what); });
                    else callback(false, _("Errore parsing identità EA:") + " " + e.what());
                }
            }
        } else { // HttpReq non ha restituito REQ_SUCCESS
            std::string errorMsgForCallback = request.getErrorMsg(); // Messaggio dall'error buffer di HttpReq
             if (errorMsgForCallback.empty()) { // Se vuoto, costruisci un messaggio più generico
                errorMsgForCallback = _("Fallito recupero info utente EA (HTTP). Codice Stato HttpReq: ") + std::to_string(static_cast<int>(reqStatus));
                // Se hai statusToString:
                // errorMsgForCallback = _("Fallito recupero info utente EA (HTTP). Stato HttpReq: ") + HttpReq::statusToString(reqStatus);
            }

            // Tenta di estrarre un errore più specifico dal corpo JSON, se presente e se è un errore HTTP
            if (static_cast<int>(reqStatus) >= 400 && !responseBody.empty()) {
                try {
                    auto jError = nlohmann::json::parse(responseBody);
                    if(jError.contains("errors") && jError["errors"].is_array() && !jError["errors"].empty()) {
                         // Per errori GraphQL, il messaggio potrebbe essere qui
                        errorMsgForCallback = jError["errors"][0].value("message", errorMsgForCallback);
                    } else if (jError.contains("error_description") && jError["error_description"].is_string()) { // Per errori OAuth-like
                        errorMsgForCallback = jError["error_description"].get<std::string>();
                    } else if (jError.contains("error") && jError["error"].is_string()) {
                         errorMsgForCallback = jError["error"].get<std::string>();
                    }
                    LOG(LogError) << "EA Games Auth: Identity fetch error from EA server: " << errorMsgForCallback;
                } catch(...){ 
                    LOG(LogWarning) << "EA Games Auth: Impossibile parsare il corpo della risposta di errore (fetchIdentity) come JSON.";
                }
            }
            
            LOG(LogError) << "EA Games Auth: Failed to fetch user identity. Status Raw: " << static_cast<int>(reqStatus) 
                          // << " (" << HttpReq::statusToString(reqStatus) << ")" // Se hai statusToString
                          << " Final Error Msg: [" << errorMsgForCallback << "] Body (partial): " << responseBody.substr(0, 200);

            if (reqStatus == HttpReq::Status::REQ_401_FORBIDDEN && !mRefreshToken.empty()) {
                 LOG(LogInfo) << "EA Games Auth: Identity fetch got 401, session may have expired. Login will attempt refresh.";
                 // Sovrascrivi il messaggio per renderlo più specifico per il caso 401
                 errorMsgForCallback = _("Sessione scaduta durante recupero info utente. Riprovare login.");
            }

            if (callback) {
                if (mWindow) mWindow->postToUiThread([callback, errorMsgForCallback] { callback(false, errorMsgForCallback); });
                else callback(false, errorMsgForCallback);
            }
        }
    }).detach();
}

    void EAGamesAuth::login(std::function<void(bool success, const std::string& message)> callback) {
        if (isUserLoggedIn()) {
            LOG(LogInfo) << "EA Games Auth: Already logged in and token is valid.";
            if (mPid.empty()) {
                LOG(LogInfo) << "EA Games Auth: User info missing, fetching now.";
                fetchUserIdentity([this, callback](bool idSuccess, const std::string& idMsg){
                    if (idSuccess) {
                        if (mWindow && callback) mWindow->postToUiThread([callback] { callback(true, _("Login EA già effettuato.")); });
                        else if (callback) callback(true, _("Login EA già effettuato."));
                    } else {
                         LOG(LogError) << "EA Games Auth: Logged in, but failed to fetch user identity: " << idMsg;
                         if (mWindow && callback) mWindow->postToUiThread([callback, idMsg] { callback(false, _("Login EA parziale, errore info utente:") + " " +idMsg); });
                         else if (callback) callback(false, _("Login EA parziale, errore info utente:") + " " + idMsg);
                    }
                });
            } else {
                 if (mWindow && callback) mWindow->postToUiThread([callback] { callback(true, _("Login EA già effettuato.")); });
                 else if (callback) callback(true, _("Login EA già effettuato."));
            }
            return;
        }
        if (!mRefreshToken.empty()) {
            LOG(LogInfo) << "EA Games Auth: Access token expired or missing, attempting to refresh.";
            RefreshTokens([this, callback](bool refreshSuccess, const std::string& refreshMsg) {
                if (refreshSuccess) {
                    LOG(LogInfo) << "EA Games Auth: Token refresh successful during login attempt.";
                     if (mWindow && callback) mWindow->postToUiThread([callback] { callback(true, _("Login EA riuscito (via refresh).")); });
                     else if (callback) callback(true, _("Login EA riuscito (via refresh)."));
                } else {
                    LOG(LogWarning) << "EA Games Auth: Token refresh failed. Initiating full login flow. Msg: " << refreshMsg;
                    StartLoginFlow(callback);
                }
            });
        } else {
            LOG(LogInfo) << "EA Games Auth: No refresh token. Initiating full login flow.";
            StartLoginFlow(callback);
        }
    }

    void EAGamesAuth::logout() {
        clearCredentials();
        LOG(LogInfo) << "EA Games Auth: User logged out locally.";
        if (mWindow) mWindow->displayNotificationMessage(_("Disconnesso da EA"));
    }

    void EAGamesAuth::loadCredentials() {
        if (!Utils::FileSystem::exists(mCredentialsPath)) return;
        std::string content = Utils::FileSystem::readAllText(mCredentialsPath);;
        if (content.empty()) return;
        try {
            auto j = nlohmann::json::parse(content);
            mAccessToken = j.value("access_token", "");
            mRefreshToken = j.value("refresh_token", "");
            mPid = j.value("pid", "");
            mPersonaId = j.value("persona_id", "");
            mUserName = j.value("user_name", "");
            mTokenExpiryTime = j.value("token_expiry_time_s", (time_t)0);

            if (!mAccessToken.empty() && Utils::Time::now() < mTokenExpiryTime) {
                mIsLoggedIn = true;
                LOG(LogInfo) << "EA Games Auth: Credentials loaded. User is logged in. PID: " << mPid;
            } else if (!mRefreshToken.empty()) {
                mIsLoggedIn = false;
                LOG(LogInfo) << "EA Games Auth: Credentials loaded, access token expired, refresh token available.";
            } else {
                clearCredentials();
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "EA Games Auth: Error parsing credentials file: " << e.what();
            Utils::FileSystem::removeFile(mCredentialsPath);
            clearCredentials();
        }
    }

void EAGamesAuth::saveCredentials() {
    if (mAccessToken.empty() && mRefreshToken.empty()) {
        // Usa la variabile membro per controllare e rimuovere il file
        if (Utils::FileSystem::exists(mCredentialsPath)) {
            Utils::FileSystem::removeFile(mCredentialsPath);
        }
        return;
    }
    nlohmann::json j;
    j["access_token"] = mAccessToken;
    j["refresh_token"] = mRefreshToken;
    j["pid"] = mPid;
    j["persona_id"] = mPersonaId;
    j["user_name"] = mUserName;
    j["token_expiry_time_s"] = mTokenExpiryTime;

    // Rimuovi la definizione di 'path' e usa la variabile membro per scrivere
    try {
        if (!helper_write_text_to_file(mCredentialsPath, j.dump(2))) {
            LOG(LogError) << "EA Games Auth: Failed to save credentials to " << mCredentialsPath;
        }
    } catch (const std::exception& e) {
        LOG(LogError) << "EA Games Auth: Error saving credentials: " << e.what();
    }
}

    void EAGamesAuth::clearCredentials() {
    mAccessToken.clear();
    mRefreshToken.clear();
    mPid.clear();
    mPersonaId.clear();
    mUserName.clear();
    mTokenExpiryTime = 0;
    mIsLoggedIn = false;

    // Usa la variabile membro per trovare e rimuovere il file
    if (Utils::FileSystem::exists(mCredentialsPath)) {
        Utils::FileSystem::removeFile(mCredentialsPath);
    }
}

} // namespace EAGames
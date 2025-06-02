// HttpReq.cpp COMPLETO E MODIFICATO PER DEBUG ESTREMO

#include "HttpReq.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Paths.h" 
#include <thread>
#include <mutex>
#include <fstream>  
#include <sstream>  

#ifdef WIN32
#include <windows.h> 
#else
#include <unistd.h>  
#include <errno.h> // Per strerror con fwrite
#endif

std::string HttpReq::statusToString(HttpReq::Status s) {
    switch (s) {
        case REQ_IN_PROGRESS: return "IN_PROGRESS (0)";
        case REQ_SUCCESS: return "SUCCESS (1)"; // Assumendo che REQ_SUCCESS sia 1
        case REQ_IO_ERROR: return "IO_ERROR (2)"; // Assumendo che REQ_IO_ERROR sia 2
        case REQ_SERVER_ERROR: return "SERVER_ERROR (3)"; // Assumendo che REQ_SERVER_ERROR sia 3 (o il valore che ha nel tuo enum)
        case REQ_FILESTREAM_ERROR: return "FILESTREAM_ERROR (4)"; // Assumendo...
        default:
            // Se lo Status può contenere direttamente codici HTTP (es. 400, 401, 403, 404)
            if (s >= 400 && s < 600) { // Gestisce errori HTTP 4xx e 5xx se usati direttamente come status
                return "HTTP_ERROR (" + std::to_string(s) + ")";
            }
            return "UNKNOWN_STATUS (" + std::to_string(s) + ")";
    }
}

static std::mutex s_curlMultiMutex; // Rinomina per chiarezza, era mMutex ma è statica
CURLM* HttpReq::s_multi_handle = nullptr; // Inizializza a nullptr, verrà creato da initCurlMulti
std::map<CURL*, HttpReq*> HttpReq::s_requests;


// --- Funzione di inizializzazione globale per curl e multi handle ---
// DEVE essere chiamata una volta all'avvio dell'applicazione (es. in main.cpp)
// static bool s_curl_initialized = false; // Per assicurare che init sia chiamato una volta
bool HttpReq::initializeGlobal() {
    LOG(LogInfo) << "HttpReq::initializeGlobal() - Inizializzazione globale di libcurl.";
    CURLcode global_init_res = curl_global_init(CURL_GLOBAL_ALL);
    if (global_init_res != CURLE_OK) {
        LOG(LogError) << "HttpReq::initializeGlobal() - curl_global_init() FALLITO: " << curl_easy_strerror(global_init_res);
        return false;
    }
    LOG(LogInfo) << "HttpReq::initializeGlobal() - curl_global_init() OK.";

    if (s_multi_handle == nullptr) {
        s_multi_handle = curl_multi_init();
        if (s_multi_handle == nullptr) {
            LOG(LogError) << "HttpReq::initializeGlobal() - curl_multi_init() FALLITO!";
            curl_global_cleanup();
            return false;
        }
        LOG(LogInfo) << "HttpReq::initializeGlobal() - curl_multi_init() OK.";
    }
    return true;
}

int HttpReq::curl_debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
    HttpReq* req = static_cast<HttpReq*>(userptr);
    std::string url_short = (req && !req->mUrl.empty()) ? req->mUrl.substr(0, 50) + "..." : "URL_SCONOSCIUTO"; // Per brevità nel log

    // Converti i dati binari in esadecimale per una visualizzazione sicura se necessario,
    // ma per header e text, va bene così. Evita di loggare dati binari troppo lunghi.
    std::string text;
    bool is_binary_data = false;

    switch (type) {
        case CURLINFO_TEXT:
            text = std::string(data, size);
            LOG(LogDebug) << "cURL_Debug Info [" << url_short << "]: " << Utils::String::trim(text);
            break;
        case CURLINFO_HEADER_IN:
            text = std::string(data, size);
            LOG(LogDebug) << "cURL_Debug HeaderIn [" << url_short << "]: " << Utils::String::trim(text);
            break;
        case CURLINFO_HEADER_OUT:
            text = std::string(data, size);
            LOG(LogDebug) << "cURL_Debug HeaderOut [" << url_short << "]: " << Utils::String::trim(text);
            break;
        case CURLINFO_DATA_IN:
            LOG(LogDebug) << "cURL_Debug DataIn [" << url_short << "]: " << size << " bytes.";
            // Non loggare il contenuto dei dati per evitare spam, a meno che non sia strettamente necessario
            // text = std::string(data, size); LOG(LogDebug) << text;
            break;
        case CURLINFO_DATA_OUT:
            LOG(LogDebug) << "cURL_Debug DataOut [" << url_short << "]: " << size << " bytes.";
            // Non loggare il contenuto dei dati
            // text = std::string(data, size); LOG(LogDebug) << text;
            break;
        case CURLINFO_SSL_DATA_IN:
            LOG(LogDebug) << "cURL_Debug SSLDataIn [" << url_short << "]: " << size << " bytes.";
            break;
        case CURLINFO_SSL_DATA_OUT:
            LOG(LogDebug) << "cURL_Debug SSLDataOut [" << url_short << "]: " << size << " bytes.";
            break;
        default: // CURLINFO_END non ha dati
            break;
    }
    return 0;
}

void HttpReq::cleanupGlobal() {
    LOG(LogInfo) << "HttpReq::cleanupGlobal() - Cleanup globale di libcurl.";
    if (s_multi_handle) {
        curl_multi_cleanup(s_multi_handle);
        s_multi_handle = nullptr;
        LOG(LogInfo) << "HttpReq::cleanupGlobal() - curl_multi_cleanup() OK.";
    }
    curl_global_cleanup();
    LOG(LogInfo) << "HttpReq::cleanupGlobal() - curl_global_cleanup() OK.";
}


// Funzioni helper come prima (urlEncode, isUrl, _regGetDWORD, _regGetString, getCookiesContainerPath, resetCookies)
std::string HttpReq::urlEncode(const std::string &s) {
    const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
    std::string escaped="";
    for(size_t i=0; i<s.length(); i++) {
        if (unreserved.find_first_of(s[i]) != std::string::npos) {
            escaped.push_back(s[i]);
        } else {
            escaped.append("%");
            char buf[3];
            sprintf(buf, "%.2X", (unsigned char)s[i]);
            escaped.append(buf);
        }
    }
    return escaped;
}

bool HttpReq::isUrl(const std::string& str) {
	return (!str.empty() && !Utils::FileSystem::exists(str) && 
		(str.find("http://") != std::string::npos || str.find("https://") != std::string::npos || str.find("www.") != std::string::npos));
}

#ifdef WIN32
static LONG _regGetDWORD(HKEY hKey, const std::string &strPath, const std::string &strValueName) {
	HKEY hSubKey;
	LONG nRet = ::RegOpenKeyExA(hKey, strPath.c_str(), 0L, KEY_QUERY_VALUE, &hSubKey);
	if (nRet == ERROR_SUCCESS) {
		DWORD dwBufferSize(sizeof(DWORD));
		DWORD nResult(0);
		nRet = ::RegQueryValueExA(hSubKey, strValueName.c_str(), 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
		::RegCloseKey(hSubKey);
		if (nRet == ERROR_SUCCESS) return nResult;
	}
	return 0;
}

static std::string _regGetString(HKEY hKey, const std::string &strPath, const std::string &strValueName) {
	std::string ret;
	HKEY hSubKey;
	LONG nRet = ::RegOpenKeyExA(hKey, strPath.c_str(), 0L, KEY_QUERY_VALUE, &hSubKey);
	if (nRet == ERROR_SUCCESS) {
		char szBuffer[1024];
		DWORD dwBufferSize = sizeof(szBuffer);
		nRet = ::RegQueryValueExA(hSubKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
		::RegCloseKey(hSubKey);
		if (nRet == ERROR_SUCCESS) ret = szBuffer;
	}
	return ret;
}
#endif

static std::string getCookiesContainerPath(bool createDirectory = true) {	
	std::string cookiesPath = Utils::FileSystem::getGenericPath(Paths::getUserEmulationStationPath() + std::string("/tmp"));
	if (createDirectory)
		Utils::FileSystem::createDirectory(cookiesPath);
	return Utils::FileSystem::getGenericPath(Utils::FileSystem::combine(cookiesPath, "cookies.txt"));
}

void HttpReq::resetCookies() {
	auto path = getCookiesContainerPath(false);
	Utils::FileSystem::removeFile(path);
}

// Costruttori
HttpReq::HttpReq(const std::string& url, const std::string& outputFilename)
    : mStatus(REQ_IN_PROGRESS), mHandle(nullptr), mFile(nullptr), mHttpStatusCode(0), mPosition(0), mPercent(-1) {
    mCurlErrorBuffer[0] = '\0';
    HttpReqOptions options;
    options.outputFilename = outputFilename;
    performRequest(url, &options);
}

HttpReq::HttpReq(const std::string& url, HttpReqOptions* options)
    : mStatus(REQ_IN_PROGRESS), mHandle(nullptr), mFile(nullptr), mHttpStatusCode(0), mPosition(0), mPercent(-1) {
    mCurlErrorBuffer[0] = '\0';
    performRequest(url, options);
}

// Metodo onError modificato
void HttpReq::onError(const char* msg_cstr) {
    std::string processed_msg;
    if (msg_cstr != nullptr && strlen(msg_cstr) > 0) {
        processed_msg = msg_cstr;
    } else {
        processed_msg = "HttpReq: Errore cURL sconosciuto o nessun messaggio di errore specifico fornito.";
    }

    // Controlla se mCurlErrorBuffer contiene informazioni aggiuntive (da setopt)
    if (mCurlErrorBuffer[0] != '\0') {
        if (!processed_msg.empty() && processed_msg.find(mCurlErrorBuffer) == std::string::npos) { // Evita duplicati se strerror è simile a errorbuffer
             processed_msg += " (Buffer Errore cURL: " + std::string(mCurlErrorBuffer) + ")";
        } else if (processed_msg.empty()) {
            processed_msg = "HttpReq: Errore cURL (Buffer: " + std::string(mCurlErrorBuffer) + ")";
        }
    }
    mErrorMsg = processed_msg; // Imposta mErrorMsg
    LOG(LogError) << "HttpReq::onError - URL: [" << mUrl << "] Stato Richiesta: [" << std::to_string(mStatus) << "] Messaggio Errore: [" << mErrorMsg << "]";
}

void HttpReq::performRequest(const std::string& url, HttpReqOptions* options) {
    mUrl = url;
    mErrorMsg.clear();
    mStatus = REQ_IN_PROGRESS;
    mHttpStatusCode = 0;
    mCurlErrorBuffer[0] = '\0';

    LOG(LogInfo) << "HttpReq::performRequest - INIZIO per URL: " << mUrl;

    if (s_multi_handle == nullptr) {
        LOG(LogError) << "HttpReq::performRequest - ERRORE FATALE: HttpReq::s_multi_handle non inizializzato! Chiamare HttpReq::initializeGlobal().";
        mStatus = REQ_IO_ERROR;
        onError("HttpReq: s_multi_handle non inizializzato.");
        return;
    }

    mHandle = curl_easy_init();
    if (mHandle == NULL) {
        mStatus = REQ_IO_ERROR;
        onError("curl_easy_init failed");
        return;
    }
    LOG(LogDebug) << "HttpReq::performRequest [" << mUrl << "] - curl_easy_init() OK.";

    struct curl_slist* custom_headers_slist = nullptr;
    bool headers_set_successfully_on_handle = false; 

    curl_easy_setopt(mHandle, CURLOPT_ERRORBUFFER, mCurlErrorBuffer);
    LOG(LogDebug) << "HttpReq::performRequest [" << mUrl << "] - CURLOPT_ERRORBUFFER impostato.";
    curl_easy_setopt(mHandle, CURLOPT_VERBOSE, 1L);
    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - CURLOPT_VERBOSE impostato a 1L.";
	curl_easy_setopt(mHandle, CURLOPT_DEBUGFUNCTION, HttpReq::curl_debug_callback);
    curl_easy_setopt(mHandle, CURLOPT_DEBUGDATA, this); // Passa 'this' come userptr
    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - CURLOPT_DEBUGFUNCTION e CURLOPT_DEBUGDATA impostati.";

    CURLcode err;
    auto log_setopt_error = [&](CURLcode opt_err, const char* opt_name, bool is_critical = true) {
        if (opt_err != CURLE_OK) {
            mStatus = REQ_IO_ERROR;
            std::string specific_error_msg = std::string(opt_name) + " failed: " +
                                             (mCurlErrorBuffer[0] != '\0' ? mCurlErrorBuffer : curl_easy_strerror(opt_err));
            onError(specific_error_msg.c_str());
            if (is_critical) {
                 if (mHandle) { curl_easy_cleanup(mHandle); mHandle = nullptr; }
                 if (custom_headers_slist && !headers_set_successfully_on_handle) { // Pulisci se non gestito da curl
                     curl_slist_free_all(custom_headers_slist);
                     custom_headers_slist = nullptr;
                 }
            }
        } else {
            LOG(LogDebug) << "HttpReq::performRequest [" << mUrl << "] - " << opt_name << " OK.";
        }
        return opt_err;
    };

    if (options != nullptr && !options->outputFilename.empty()) {
        mFilePath = options->outputFilename;
    } else {
        mFilePath.clear();
    }
    mPosition = -1; // Inizializza qui, dopo aver determinato mFilePath
    mPercent = -1;


    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str()), "CURLOPT_URL") != CURLE_OK) return;

    if (options != nullptr && !options->dataToPost.empty()) {
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_POST, 1L), "CURLOPT_POST") != CURLE_OK) return;
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_COPYPOSTFIELDS, options->dataToPost.c_str()), "CURLOPT_COPYPOSTFIELDS") != CURLE_OK) return;
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_POSTFIELDSIZE, (long)options->dataToPost.length()), "CURLOPT_POSTFIELDSIZE") != CURLE_OK) return;
    }

    if (options != nullptr && !options->customHeaders.empty()) {
        LOG(LogDebug) << "HttpReq::performRequest [" << mUrl << "] - Adding Custom Headers...";
        for (const auto& header : options->customHeaders) {
            // LOG(LogDebug) << "  Header: " << header; // Può essere troppo verboso
            custom_headers_slist = curl_slist_append(custom_headers_slist, header.c_str());
        }
        err = curl_easy_setopt(mHandle, CURLOPT_HTTPHEADER, custom_headers_slist);
        if (log_setopt_error(err, "CURLOPT_HTTPHEADER") != CURLE_OK) {
            // log_setopt_error pulisce mHandle, ma custom_headers_slist (se non nullo)
            // deve essere liberato qui perché non è stato passato con successo a curl.
            // La lambda log_setopt_error ora gestisce la pulizia di custom_headers_slist se critica e mHandle diventa null.
            return;
        }
        headers_set_successfully_on_handle = true;
    }

    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_FOLLOWLOCATION, 1L), "CURLOPT_FOLLOWLOCATION") != CURLE_OK) return;
    
    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Configurazione SSL...";
    std::string caBundlePath = Paths::getEmulationStationPath() + "/cacert.pem";
    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Percorso tentato per CA bundle: " << caBundlePath;

    if (Utils::FileSystem::exists(caBundlePath)) {
        LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Trovato CA bundle: " << caBundlePath << ". Abilitazione verifica SSL.";
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_SSL_VERIFYPEER, 1L), "CURLOPT_SSL_VERIFYPEER=1L") != CURLE_OK) return;
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_SSL_VERIFYHOST, 2L), "CURLOPT_SSL_VERIFYHOST=2L") != CURLE_OK) return;
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_CAINFO, caBundlePath.c_str()), "CURLOPT_CAINFO") != CURLE_OK) return;
    } else {
        LOG(LogError) << "HttpReq::performRequest [" << mUrl << "] - ATTENZIONE: File CA bundle (" << caBundlePath << ") NON TROVATO!";
        LOG(LogError) << "HttpReq::performRequest [" << mUrl << "] - Si procede con la verifica SSL DISABILITATA (CURLOPT_SSL_VERIFYPEER=0L). NON SICURO!";
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_SSL_VERIFYPEER, 0L), "CURLOPT_SSL_VERIFYPEER=0L (fallback)") != CURLE_OK) return;
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_SSL_VERIFYHOST, 0L), "CURLOPT_SSL_VERIFYHOST=0L (fallback)") != CURLE_OK) return;
    }

    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_CONNECTTIMEOUT, 30L), "CURLOPT_CONNECTTIMEOUT=30L") != CURLE_OK) return;
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_TIMEOUT, 60L), "CURLOPT_TIMEOUT=60L") != CURLE_OK) return;
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_MAXREDIRS, 2L), "CURLOPT_MAXREDIRS") != CURLE_OK) return;

#if WIN32
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), "CURLOPT_REDIR_PROTOCOLS") != CURLE_OK) return;
#else
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS_STR, "http,https"), "CURLOPT_REDIR_PROTOCOLS_STR") != CURLE_OK) return;
#endif

    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, &HttpReq::write_content), "CURLOPT_WRITEFUNCTION") != CURLE_OK) return;
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, this), "CURLOPT_WRITEDATA") != CURLE_OK) return;

    std::string userAgentToSet = (options && !options->userAgent.empty()) ? options->userAgent : HTTP_REQ_USERAGENT;
    if (!userAgentToSet.empty()) {
        if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_USERAGENT, userAgentToSet.c_str()), "CURLOPT_USERAGENT") != CURLE_OK) return;
    }

    if (options == nullptr || options->useCookieManager) {
        std::string cookiesFile = getCookiesContainerPath();
        LOG(LogDebug) << "HttpReq::performRequest [" << mUrl << "] - Usando cookie file: " << cookiesFile;
        log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_COOKIEFILE, cookiesFile.c_str()), "CURLOPT_COOKIEFILE", false); 
        log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_COOKIEJAR, cookiesFile.c_str()), "CURLOPT_COOKIEJAR", false);  
    }

    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_HEADERFUNCTION, &HttpReq::header_callback), "CURLOPT_HEADERFUNCTION") != CURLE_OK) return;
    if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_HEADERDATA, this), "CURLOPT_HEADERDATA") != CURLE_OK) return;

#ifdef WIN32
    if (_regGetDWORD(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyEnable")) {
        auto proxyServerReg = _regGetString(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyServer");
        if (!proxyServerReg.empty()) {
            std::string proxyToUse = proxyServerReg; 
            LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Usando proxy di sistema: " << proxyToUse;
            if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_PROXY, proxyToUse.c_str()), "CURLOPT_PROXY") != CURLE_OK) return;
            if (log_setopt_error(curl_easy_setopt(mHandle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP), "CURLOPT_PROXYTYPE") != CURLE_OK) return;
        }
    }
#endif

    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Tutte le opzioni cURL impostate. Pronto per aggiungere all'handle multi.";

    std::unique_lock<std::mutex> lock(s_curlMultiMutex); 

    if (!mFilePath.empty()) {
        mTempStreamPath = mFilePath + ".tmp"; 
        Utils::FileSystem::removeFile(mTempStreamPath);
#if defined(_WIN32)
        mFile = _wfopen(Utils::String::convertToWideString(mTempStreamPath).c_str(), L"wb");
#else
        mFile = fopen(mTempStreamPath.c_str(), "wb");
#endif
        if (mFile == nullptr) {
            mStatus = REQ_IO_ERROR;
            onError("IO Error (file di output temporaneo non scrivibile)"); 
            if (mHandle) { 
                if (custom_headers_slist && !headers_set_successfully_on_handle) curl_slist_free_all(custom_headers_slist);
                curl_easy_cleanup(mHandle); mHandle = nullptr; 
            } else if (custom_headers_slist && !headers_set_successfully_on_handle) { // mHandle già pulito da log_setopt_error
                 curl_slist_free_all(custom_headers_slist);
            }
            return;
        }
        mPosition = 0;
        if (Utils::FileSystem::exists(mFilePath)) { 
             Utils::FileSystem::removeFile(mFilePath); 
        }
    }

    CURLMcode merr = curl_multi_add_handle(s_multi_handle, mHandle);
    if (merr != CURLM_OK) {
        closeStream(); 
        mStatus = REQ_IO_ERROR;
        const char* multi_err_str = curl_multi_strerror(merr);
        std::string detailed_merr = "curl_multi_add_handle() failed. Code: " + std::to_string(merr) +
                                    " (" + (multi_err_str ? multi_err_str : "Unknown curl_multi error") + ")";
        onError(detailed_merr.c_str());
        if (mHandle) { 
            if (custom_headers_slist && !headers_set_successfully_on_handle) curl_slist_free_all(custom_headers_slist);
            curl_easy_cleanup(mHandle); mHandle = nullptr; 
        } else if (custom_headers_slist && !headers_set_successfully_on_handle) {
             curl_slist_free_all(custom_headers_slist);
        }
        return;
    }
    LOG(LogInfo) << "HttpReq::performRequest [" << mUrl << "] - Handle aggiunto con successo a s_multi_handle.";
    s_requests[mHandle] = this;
    // Se l'handle è stato aggiunto con successo e CURLOPT_HTTPHEADER è stato chiamato con successo (headers_set_successfully_on_handle = true),
    // libcurl è responsabile di custom_headers_slist. Non liberarlo qui.
    // Se l'aggiunta fallisce, la pulizia di custom_headers_slist è gestita sopra.
}


HttpReq::Status HttpReq::status()
{
    if (mStatus == REQ_IN_PROGRESS)
    {
        // Usa il mutex specifico per le operazioni multi handle se è quello corretto.
        // Il tuo codice originale usava 'mMutex' che era statico e non qualificato.
        // Se 's_curlMultiMutex' è il mutex corretto per proteggere s_multi_handle e s_requests, usalo.
        std::unique_lock<std::mutex> lock(s_curlMultiMutex); // Assumendo s_curlMultiMutex sia il mutex corretto

        if (!s_multi_handle) { // Controllo aggiunto per sicurezza
            LOG(LogError) << "HttpReq::status() - ERRORE: s_multi_handle è nullo! Impossibile eseguire curl_multi_perform.";
            mStatus = REQ_IO_ERROR;
            onError("HttpReq internal error: s_multi_handle non inizializzato all'interno di status().");
            return mStatus;
        }

        int handle_count;
        CURLMcode merr = curl_multi_perform(s_multi_handle, &handle_count);
        if (merr != CURLM_OK && merr != CURLM_CALL_MULTI_PERFORM)
        {
            closeStream(); // Chiudi il file stream se aperto per questa istanza
            mStatus = REQ_IO_ERROR;
            const char* multi_err_str = curl_multi_strerror(merr);
            std::string detailed_merr = "HttpReq::status() - curl_multi_perform() failed. Code: " + std::to_string(merr) +
                                        " (" + (multi_err_str ? multi_err_str : "Unknown curl_multi error") + ")";
            // Imposta l'errore per TUTTE le richieste in corso? No, questo è un errore del multi_handle.
            // Questo errore è più globale, difficile da attribuire a una singola HttpReq se non tramite log.
            LOG(LogError) << detailed_merr; // Logga l'errore del multi_perform
            // Potresti voler iterare su tutte le s_requests e impostare il loro stato su errore qui,
            // ma è complesso. Per ora, questo HttpReq (se è l'unico attivo) rifletterà questo errore.
            // Se altre richieste sono attive, potrebbero non aggiornare il loro stato correttamente qui.
            // La logica originale onError(detailed_merr.c_str()); è per l'istanza corrente,
            // ma l'errore di curl_multi_perform è più generale.
            // Per ora, impostiamo l'errore sull'istanza corrente se è l'unica o se questo ha senso nel flusso.
            onError(detailed_merr.c_str()); 
            return mStatus;
        }

        int msgs_left;
        CURLMsg* msg;
        while ((msg = curl_multi_info_read(s_multi_handle, &msgs_left)) != nullptr)
        {
            if (msg->msg == CURLMSG_DONE)
            {
                HttpReq* req = nullptr;
                auto it = s_requests.find(msg->easy_handle);
                if (it != s_requests.end()) {
                    req = it->second;
                }

                if (req == nullptr || req->mHandle != msg->easy_handle) { // Aggiunto controllo req->mHandle
                    LOG(LogError) << "HttpReq::status() - ERRORE CRITICO: Impossibile trovare easy handle (" 
                                  << msg->easy_handle << ") o req non corrispondente nella mappa s_requests!";
                    // Non tentare di accedere a req->mUrl se req è nullo o non corrisponde.
                    // Rimuovi l'handle dal multi se è lì, ma non pulire l'easy_handle perché non abbiamo un proprietario HttpReq valido.
                    if (msg->easy_handle) {
                         LOG(LogWarning) << "HttpReq::status() - Tentativo di rimuovere handle orfano " << msg->easy_handle << " dal multi_handle.";
                         curl_multi_remove_handle(s_multi_handle, msg->easy_handle);
                         // Non chiamare curl_easy_cleanup qui perché non abbiamo il proprietario HttpReq
                    }
                    continue;
                }
                
                LOG(LogDebug) << "HttpReq::status() - CURLMSG_DONE per URL: " << req->mUrl 
                              << ", Handle: " << msg->easy_handle 
                              << ", Risultato cURL: " << msg->data.result 
                              << " (" << curl_easy_strerror(msg->data.result) << ")";

                req->closeStream();
                
                if (req->mStatus == REQ_FILESTREAM_ERROR) 
                {
                    // onError è già stato chiamato da write_content, mErrorMsg dovrebbe essere impostato.
                    LOG(LogError) << "HttpReq::status() - Errore Filestream rilevato precedentemente per URL: " << req->mUrl << ". Errore memorizzato: [" << req->mErrorMsg << "]";
                }
                else if (msg->data.result == CURLE_OK) 
                {
                    long http_status_code_long = 0;
                    // Usa l'handle specifico del messaggio per getinfo
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_status_code_long);
                    req->mHttpStatusCode = static_cast<int>(http_status_code_long); 

                    if (req->mHttpStatusCode >= 200 && req->mHttpStatusCode <= 299) 
                    {
                        if (!req->mFilePath.empty())
                        {
                            bool renamed = Utils::FileSystem::renameFile(req->mTempStreamPath.c_str(), req->mFilePath.c_str());
#if WIN32
                            if (renamed)
                            {
                                auto wfn = Utils::String::convertToWideString(req->mFilePath);
                                HANDLE hFile = CreateFileW(wfn.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                                if (hFile != INVALID_HANDLE_VALUE)
                                {
                                    SYSTEMTIME st; GetSystemTime(&st);
                                    FILETIME ft; SystemTimeToFileTime(&st, &ft);
                                    SetFileTime(hFile, &ft, &ft, &ft);
                                    CloseHandle(hFile);
                                }
                            }
#endif
                            if (!renamed)
                            {
                                if (Utils::FileSystem::copyFile(req->mTempStreamPath, req->mFilePath))
                                    renamed = true;
                            }
                            if (renamed) req->mStatus = REQ_SUCCESS;
                            else 
                            {
                                req->mStatus = REQ_IO_ERROR;
                                req->onError("file rename/copy failed");
                            }
                        }
                        else
                        {
                            req->mStatus = REQ_SUCCESS;
                        }
                        LOG(LogInfo) << "HttpReq::status() - Richiesta COMPLETATA con SUCCESSO per URL: " << req->mUrl << " (HTTP " << req->mHttpStatusCode << ")";
                    }
                    else // Errore HTTP (es. 4xx, 5xx)
                    {
                        std::string err_content_body;
                        if (req->mFilePath.empty()) { 
                            err_content_body = req->getContent(); // Questo chiama mContent.str()
                        } else { 
                            if (Utils::FileSystem::exists(req->mTempStreamPath)) {
                                std::ifstream ifs(req->mTempStreamPath, std::ios_base::in | std::ios_base::binary);
                                if (ifs.is_open()) {
                                    std::stringstream temp_stream; temp_stream << ifs.rdbuf();
                                    err_content_body = temp_stream.str();
                                } else {
                                     LOG(LogWarning) << "HttpReq::status() - Impossibile aprire il file temporaneo " << req->mTempStreamPath << " per leggere il corpo dell'errore.";
                                }
                            }
                        }
                        
                        if (!err_content_body.empty() && err_content_body.find("<body") != std::string::npos){
                            auto body_parsed = Utils::String::extractString(err_content_body, "<body", "</body>", true);
                            body_parsed = Utils::String::replace(body_parsed, "\r", ""); body_parsed = Utils::String::replace(body_parsed, "\n", "");
                            body_parsed = Utils::String::replace(body_parsed, "</p>", "\r\n"); body_parsed = Utils::String::replace(body_parsed, "<br>", "\r\n");
                            body_parsed = Utils::String::replace(body_parsed, "<hr>", "\r\n"); body_parsed = Utils::String::removeHtmlTags(body_parsed);
                            if (!body_parsed.empty()) err_content_body = body_parsed;
                        }

                        std::string http_err_msg = "Errore HTTP " + std::to_string(req->mHttpStatusCode);
                        if (!err_content_body.empty()) {
                            http_err_msg += ": " + (err_content_body.length() > 256 ? err_content_body.substr(0, 256) + "..." : err_content_body);
                        }
                        
                        // Usa REQ_SERVER_ERROR se definito in HttpReq.h, altrimenti REQ_IO_ERROR per 5xx
                        req->mStatus = (req->mHttpStatusCode >= 500) ? REQ_SERVER_ERROR : static_cast<Status>(req->mHttpStatusCode);
                        req->onError(http_err_msg.c_str());
                        LOG(LogError) << "HttpReq::status() - Richiesta COMPLETATA con ERRORE HTTP per URL: " << req->mUrl << ". Codice HTTP: " << req->mHttpStatusCode << ". Dettagli: " << req->mErrorMsg;
                    }
                }
                else // msg->data.result != CURLE_OK -> Errore di libcurl per questo handle
                {
                    req->mStatus = REQ_IO_ERROR;
                    const char* curl_err_str = curl_easy_strerror(msg->data.result);
                    std::string error_message_from_strerror = (curl_err_str ? curl_err_str : "Stringa errore cURL non disponibile/nulla");
                    
                    // mCurlErrorBuffer è un membro di req, impostato da CURLOPT_ERRORBUFFER in performRequest
                    std::string error_from_handle_buffer = (req->mCurlErrorBuffer[0] != '\0') ? req->mCurlErrorBuffer : "Buffer errore handle vuoto";

                    std::string final_detailed_error = "Codice errore cURL: " + std::to_string(msg->data.result) +
                                                       " (" + error_message_from_strerror + ").";
                    // Aggiungi il contenuto di mCurlErrorBuffer solo se fornisce informazioni diverse/aggiuntive
                    if (error_from_handle_buffer != "Buffer errore handle vuoto" && 
                        (error_message_from_strerror == "Stringa errore cURL non disponibile/nulla" || error_from_handle_buffer.find(error_message_from_strerror) == std::string::npos) ) 
                    {
                        final_detailed_error += " Dettaglio da buffer errore handle: [" + error_from_handle_buffer + "]";
                    }
                    
                    req->onError(final_detailed_error.c_str());

                    LOG(LogError) << "HttpReq::status() - Trasferimento FALLITO (CURLMSG_DONE) per URL: " << req->mUrl;
                    LOG(LogError) << "  Raw cURL result code: " << msg->data.result; // Codice numerico
                    LOG(LogError) << "  HttpReq mErrorMsg finale impostato a: [" << req->mErrorMsg << "]";
                }
            }
        }
    }
    return mStatus;
}
// Distruttore
HttpReq::~HttpReq() {
    std::unique_lock<std::mutex> lock(s_curlMultiMutex);
    closeStream();
    if (!mTempStreamPath.empty()) {
        Utils::FileSystem::removeFile(mTempStreamPath);
    }
    if (mHandle) {
        s_requests.erase(mHandle);
        curl_multi_remove_handle(s_multi_handle, mHandle);
        curl_easy_cleanup(mHandle);
        mHandle = nullptr;
    }
}

// Metodo getContent
std::string HttpReq::getContent() {
    if (mStatus == REQ_IN_PROGRESS && mFilePath.empty()) {
        // Non è sicuro leggere mContent se la richiesta è ancora in corso e scrive su di essa.
        // Tuttavia, la logica attuale di solito chiama getContent dopo che status() non è più REQ_IN_PROGRESS.
        // Se chiamata prematuramente, potrebbe restituire dati parziali.
    }
    if (mFilePath.empty()) { // Risposta in memoria
        return mContent.str();
    }
    // Risposta su file
    std::string path_to_read = (mStatus == REQ_SUCCESS) ? mFilePath : mTempStreamPath;
    if (!Utils::FileSystem::exists(path_to_read)) {
        LOG(LogWarning) << "HttpReq::getContent - File non trovato per leggere: " << path_to_read;
        return "";
    }
    std::ifstream ifs(path_to_read, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        LOG(LogError) << "HttpReq::getContent - Impossibile aprire il file: " << path_to_read;
        return "";
    }
    std::stringstream content_stream;
    content_stream << ifs.rdbuf();
    return content_stream.str();
}

// Metodo getErrorMsg
std::string HttpReq::getErrorMsg() {
    return mErrorMsg;
}

// Metodo getResponseHeader
std::string HttpReq::getResponseHeader(const std::string& header) {
    auto it = mResponseHeaders.find(header);
    if (it != mResponseHeaders.end())
        return it->second;
    return "";
}

// header_callback
size_t HttpReq::header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    HttpReq* request = static_cast<HttpReq*>(userdata);
    if (!request) return 0;
    std::string header_line(buffer, size * nitems);
    header_line = Utils::String::trim(header_line);
    if (!header_line.empty() && !Utils::String::startsWith(header_line, "HTTP/")) {
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = Utils::String::trim(header_line.substr(0, colon_pos));
            std::string value = Utils::String::trim(header_line.substr(colon_pos + 1));
            request->mResponseHeaders[key] = value;
        } else if (!header_line.empty()) {
            request->mResponseHeaders[header_line] = "";
        }
    }
    return nitems * size;
}

// write_content
size_t HttpReq::write_content(void* buff, size_t size, size_t nmemb, void* req_ptr) {
    HttpReq* request = static_cast<HttpReq*>(req_ptr);
    if (!request) return 0;
    size_t total_bytes = size * nmemb;

    if (request->mFilePath.empty()) {
        request->mContent.write(static_cast<char*>(buff), total_bytes);
        if (request->mContent.fail()) {
            LOG(LogError) << "HttpReq::write_content - Errore scrittura su mContent stringstream per URL: " << request->mUrl;
            request->mStatus = REQ_IO_ERROR; 
            request->onError("Errore scrittura su stringstream in memoria");
            return 0; // Segnala errore a libcurl per interrompere il trasferimento
        }
    } else {
        if (request->mFile == nullptr) {
            LOG(LogError) << "HttpReq::write_content - Tentativo di scrivere su mFile nullo per URL: " << request->mUrl;
            // Non impostare mStatus o chiamare onError qui, perché non abbiamo modo di segnalare
            // l'errore a libcurl tranne che restituendo 0. Lo stato verrà gestito in status().
            return 0;
        }
        size_t written_elements = fwrite(buff, size, nmemb, request->mFile);
        if (written_elements < nmemb) {
            std::string ferr_msg = "Errore sconosciuto";
            if (ferror(request->mFile)) {
                ferr_msg = strerror(errno); 
            }
            LOG(LogError) << "HttpReq::write_content - Errore fwrite per URL: " << request->mUrl 
                          << ". Previsti " << nmemb << " elementi di size " << size << ", scritti " << written_elements 
                          << ". Errore C: " << ferr_msg;
            request->closeStream(); // Chiudi il file immediatamente
            request->mStatus = REQ_FILESTREAM_ERROR; // Imposta stato per essere controllato in status()
            request->onError(("Errore scrittura su file: " + ferr_msg).c_str());
            return 0; // Indica errore a libcurl
        }
    }

    if (request->mHandle) {
        curl_off_t cl = 0;
        if (curl_easy_getinfo(request->mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl) == CURLE_OK) {
            request->mPosition += total_bytes;
            if (cl <= 0) {
                request->mPercent = -1;
            } else {
                request->mPercent = static_cast<int>((request->mPosition * 100LL) / cl);
            }
        }
    }
    return total_bytes;
}
void HttpReq::closeStream()
{
    if (mFile) // mFile è il puntatore FILE* per l'output su file
    {
        fflush(mFile);
        fclose(mFile);
        mFile = nullptr; // Imposta a nullptr dopo aver chiuso
    }
}

// wait
bool HttpReq::wait() {
    while (status() == HttpReq::REQ_IN_PROGRESS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return (mStatus == REQ_SUCCESS);
}
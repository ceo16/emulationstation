#include "GameStore/Xbox/XboxAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "Settings.h" // Per ottenere il percorso di configurazione
#include "Paths.h"    // Per Paths::getEsConfigPath()
#include "Window.h"   // Se devi mostrare UI per il codice
#include "utils/StringUtil.h" // Per trim, ecc.
#include "utils/TimeUtil.h"   // Per la gestione del tempo di scadenza
#include "json.hpp"  // Per il parsing JSON
#include <string>
#include "utils/FileSystemUtil.h"
#include <thread> 
#include "guis/GuiWebViewAuthLogin.h" // Includi il tuo header GuiWebViewAuthLogin
#include "guis/GuiMsgBox.h"           // Per mostrare messaggi all'utente
#include "LocaleES.h" 

#include <fstream>      // Per std::ifstream, std::ofstream
#include <iomanip>      // Per std::put_time, std::get_time (per salvare/caricare expiry)

namespace nj = nlohmann;

// --- Definizione costanti statiche ---
const std::string XboxAuth::CLIENT_ID = "38cd2fa8-66fd-4760-afb2-405eb65d5b0c"; // Playnite's client_id
const std::string XboxAuth::REDIRECT_URI = "https://login.live.com/oauth20_desktop.srf";
const std::string XboxAuth::SCOPE = "XboxLive.signin XboxLive.offline_access";
const std::string XboxAuth::LIVE_AUTHORIZE_URL = "https://login.live.com/oauth20_authorize.srf";
const std::string XboxAuth::LIVE_TOKEN_URL = "https://login.live.com/oauth20_token.srf";
const std::string XboxAuth::XBOX_USER_AUTHENTICATE_URL = "https://user.auth.xboxlive.com/user/authenticate";
const std::string XboxAuth::XBOX_XSTS_AUTHORIZE_URL = "https://xsts.auth.xboxlive.com/xsts/authorize";

const std::string XboxAuth::LIVE_TOKENS_FILENAME_DEF = "xbox_live_tokens.json";
const std::string XboxAuth::XSTS_TOKENS_FILENAME_DEF = "xbox_xsts_tokens.json";
const std::string XboxAuth::USER_INFO_FILENAME_DEF = "xbox_user_info.json"; // File per XUID, UHS

// Helper per leggere/scrivere file (puoi usare quelli che hai già se sono globali)
static std::string readFileToStringLocal(const std::filesystem::path& p) {
    std::ifstream file(p, std::ios::binary); // Apri in binario per consistenza
    std::string content;
    if (file) {
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());
    } else {
        LOG(LogDebug) << "XboxAuth: Failed to open file for reading or file empty: " << p.string();
    }
    return content;
}

static bool writeStringToFileLocal(const std::filesystem::path& p, const std::string& content) {
    std::ofstream file(p, std::ios::binary); // Apri in binario
    if (file) {
        file.write(content.c_str(), content.length());
        return true;
    }
    LOG(LogError) << "XboxAuth: Failed to open file for writing: " << p.string();
    return false;
}


XboxAuth::XboxAuth(std::function<void(const std::string&)> setStateCallback)
    : mSetStateCallback(setStateCallback),
      mLiveTokenExpiry(std::chrono::system_clock::now()), // Inizializza a "ora" (quindi potenzialmente scaduto)
      mXstsTokenExpiry(std::chrono::system_clock::now()),   // Inizializza a "ora"
      mHasTriedAutoLogin(false)
{
    mTokenStoragePath = Utils::FileSystem::getEsConfigPath() + "/xbox/";
    if (!Utils::FileSystem::exists(mTokenStoragePath)) {
        Utils::FileSystem::createDirectory(mTokenStoragePath);
    }

    loadTokenData(); // Carica i token esistenti

    // Dopo aver caricato, controlla se siamo "autenticati" nel senso stretto 
    // (token XSTS presente e non immediatamente scaduto).
    // Se non lo siamo, o se i token Live sono scaduti, e abbiamo un refresh token,
    // e non abbiamo già provato un auto-login/refresh in questa sessione, tentiamo il refresh.
    bool needsRefresh = false;
    if (mXstsToken.empty() || mXstsTokenExpiry <= std::chrono::system_clock::now() + std::chrono::minutes(5)) {
        needsRefresh = true; // XSTS mancante o scaduto/in scadenza
    }
    if (!mLiveRefreshToken.empty() && (mLiveAccessToken.empty() || mLiveTokenExpiry <= std::chrono::system_clock::now() + std::chrono::minutes(5))) {
        needsRefresh = true; // Live Access Token mancante o scaduto/in scadenza, ma abbiamo un refresh token
    }

    if (needsRefresh && !mLiveRefreshToken.empty() && !mHasTriedAutoLogin) {
        LOG(LogInfo) << "XboxAuth: Tokens loaded require refresh, or XSTS is missing. Attempting automatic token refresh.";
        if (refreshTokens()) { // refreshTokens() gestirà l'aggiornamento di mHasTriedAutoLogin e il salvataggio
            LOG(LogInfo) << "XboxAuth: Automatic token refresh successful on startup.";
        } else {
            LOG(LogWarning) << "XboxAuth: Automatic token refresh failed on startup. User may need to log in manually.";
            // Non cancellare i token qui, refreshTokens() potrebbe averlo già fatto se il refresh token era invalido.
        }
    } else if (!needsRefresh && !mXstsToken.empty()) {
         LOG(LogInfo) << "XboxAuth: Tokens loaded and appear valid. XUID: " << mUserXUID;
    } else {
        // Se non serve un refresh (es. token validi) o non possiamo fare refresh (no refresh_token),
        // o abbiamo già provato.
        mHasTriedAutoLogin = true; // Segna comunque come tentato se non c'erano le condizioni per un refresh.
        LOG(LogDebug) << "XboxAuth: No immediate refresh action taken on startup. mHasTriedAutoLogin set to true.";
    }
}


bool XboxAuth::isAuthenticated() const {
    if (mXstsToken.empty()) {
        return false;
    }
    // Controlla se XSTS token è scaduto (o scadrà molto presto)
    // Aggiungiamo un buffer di, ad esempio, 1 minuto per sicurezza (o anche 0 se refresh è gestito bene)
    auto now = std::chrono::system_clock::now();
    if (mXstsTokenExpiry <= (now + std::chrono::minutes(1))) { // Se scade entro 1 minuto o è già scaduto
        LOG(LogDebug) << "XboxAuth::isAuthenticated (const) - XSTS token is empty or considered expired/nearing expiry. XUID: " << mUserXUID;
        return false;
    }
    LOG(LogDebug) << "XboxAuth::isAuthenticated (const) - XSTS token present and valid. XUID: " << mUserXUID;
    return true;
}

std::string XboxAuth::getAuthorizationUrl(std::string& state_out) {
    // Per il flusso manuale, non usiamo lo state come per Epic, ma l'URL è fisso
    // Potresti voler generare uno state se il tuo HttpReq ne avesse bisogno per qualche motivo
    // state_out = generateRandomState(); // Se necessario
    std::string authUrl = LIVE_AUTHORIZE_URL +
                          "?client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                          "&response_type=code" +
                          "&approval_prompt=auto" + // O "force" se vuoi sempre il prompt
                          "&scope=" + HttpReq::urlEncode(SCOPE) +
                          "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI);
    LOG(LogDebug) << "XboxAuth: Generated Authorization URL: " << authUrl;
    return authUrl;
}

void XboxAuth::authenticateWithWebView(Window* window)
{
    // Costruisci l'URL di autorizzazione Xbox Live.
    // Usiamo le costanti già definite nella tua classe XboxAuth.
    std::string authUrl = LIVE_AUTHORIZE_URL +
                          "?client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                          "&response_type=code" +
                          "&approval_prompt=auto" + // "auto" non forza il prompt se l'utente è già loggato
                          "&scope=" + HttpReq::urlEncode(SCOPE) +
                          "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI);

    LOG(LogInfo) << "[XboxAuth] Avvio login WebView per Xbox Live. URL: " << authUrl;

    // Crea e mostra la GuiWebViewAuthLogin.
    // Passiamo l'URL iniziale, il nome dello store, e il prefisso di reindirizzamento atteso.
    auto webViewGui = new GuiWebViewAuthLogin(
        window,
        authUrl,
        "Xbox Live",             // mStoreNameForLogging: Nome per i log e la UI
        REDIRECT_URI,            // mWatchRedirectPrefix: Il prefisso che la WebView monitorerà
        GuiWebViewAuthLogin::AuthMode::DEFAULT // Utilizziamo la modalità DEFAULT
    );

    // Imposta la callback che verrà chiamata quando la WebView rileva il reindirizzamento.
    // Questa callback è garantita essere eseguita sul thread UI.
    webViewGui->setOnLoginFinishedCallback(
        [this, window, webViewGui](bool success, const std::string& dataOrErrorUrl)
    {
            // La GuiWebViewAuthLogin si auto-gestisce la chiusura
            // (removeGui e delete this) dopo che la callback è stata invocata e completata.

            if (success)
            {
                LOG(LogInfo) << "[XboxAuth] WebView login completato per Xbox. URL di reindirizzamento ricevuto.";

                // Il codice di autorizzazione è già stato estratto e memorizzato da GuiWebViewAuthLogin
                // e passed as part of the dataOrErrorUrl if success is true.
                std::string authCode = Utils::String::getUrlParam(dataOrErrorUrl, "code");

                if (!authCode.empty())
                {
                    LOG(LogInfo) << "[XboxAuth] Codice di autorizzazione Xbox Live ottenuto (primi 10 caratteri): " << authCode.substr(0, 10) << "...";

                    // Esegui lo scambio del codice per i token in un thread separato
                    // per evitare di bloccare l'interfaccia utente.
                    std::thread([this, authCode, window]() {
                        LOG(LogInfo) << "[XboxAuth Thread] Avvio scambio token Xbox in background.";
                        bool exchangeAndXstsSuccess = false;
                        try {
                            // Chiama il metodo esistente che scambia il codice per i token Live
                            // e poi procede all'autenticazione XSTS.
                            exchangeAndXstsSuccess = exchangeAuthCodeForTokens(authCode);

                            // Dopo un successo, i token sono già salvati e i membri di XboxAuth aggiornati.
                        } catch (const std::exception& e) {
                            LOG(LogError) << "[XboxAuth Thread] Eccezione durante lo scambio token/XSTS di Xbox: " << e.what();
                            exchangeAndXstsSuccess = false;
                        }

                        // Torna al thread UI per mostrare il risultato finale.
                        window->postToUiThread([this, window, exchangeAndXstsSuccess]() {
                            if (exchangeAndXstsSuccess)
                            {
                                LOG(LogInfo) << "[XboxAuth UI] Login Xbox Live COMPLETATO con successo! XUID: " << mUserXUID;
                                window->pushGui(new GuiMsgBox(window, _("Login Xbox Live riuscito! Benvenuto,") + " " + mUserXUID + "!"));

                                // L'utente è autenticato, ora puoi scatenare la scansione dei giochi.
                                // La logica verrà gestita da XboxUI::optionRefreshGamesList()
                                // o direttamente qui se il flusso lo richiede.
                                // Se vuoi avviare un refresh automatico subito dopo il login:
                                // XboxStore::getInstance()->refreshGamesListAsync();
                            }
                            else
                            {
                                LOG(LogError) << "[XboxAuth UI] Login Xbox Live fallito dopo lo scambio token o XSTS.";
                                window->pushGui(new GuiMsgBox(window, _("Accesso Xbox Live fallito. Riprova più tardi.")));
                                // Pulisci i dati dei token per assicurare uno stato coerente dopo il fallimento.
                                clearAllTokenData();
                            }
                        });
                    }).detach(); // Dettach il thread per farlo eseguire in background.
                }
                else // Codice di autorizzazione non trovato nell'URL
                {
                    LOG(LogError) << "[XboxAuth] Login Xbox Live fallito: Codice di autorizzazione non trovato nell'URL di reindirizzamento.";
                    window->pushGui(new GuiMsgBox(window, _("Accesso Xbox Live fallito: Codice non ricevuto nell'URL.")));
                }
            }
            else // La WebView ha segnalato un fallimento o l'utente ha annullato
            {
                LOG(LogError) << "[XboxAuth] Login WebView per Xbox Live annullato o fallito: " << dataOrErrorUrl;
                window->pushGui(new GuiMsgBox(window, _("Accesso Xbox Live annullato o fallito.")));
            }
        });

    window->pushGui(webViewGui); // Aggiungi la WebView alla pila delle GUI per visualizzarla
}

bool XboxAuth::exchangeAuthCodeForTokens(const std::string& authCode) {
    LOG(LogInfo) << "XboxAuth: Exchanging authorization code for Live tokens...";
    if (authCode.empty()) { /* ... log e return false ... */ return false; }
    mHasTriedAutoLogin = true; // Un login manuale conta

    HttpReqOptions options;
    std::string postData = "grant_type=authorization_code"
                           "&code=" + HttpReq::urlEncode(authCode) +
                           "&scope=" + HttpReq::urlEncode(SCOPE) +
                           "&client_id=" + HttpReq::urlEncode(CLIENT_ID) +
                           "&redirect_uri=" + HttpReq::urlEncode(REDIRECT_URI);
    options.dataToPost = postData;
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.customHeaders.push_back("Accept: application/json");

    HttpReq request(LIVE_TOKEN_URL, &options); // Non unique_ptr se la vuoi solo nello scope
    if (!request.wait()) { /* ... log e return false ... */ return false; }

    HttpReq::Status liveStatus = request.status();
    if (liveStatus == HttpReq::REQ_SUCCESS) {
        try {
            nj::json responseJson = nj::json::parse(request.getContent());
            if (responseJson.contains("error")) { /* ... log errore da JSON e return false ... */ return false; }

            mLiveAccessToken = responseJson.value("access_token", "");
            mLiveRefreshToken = responseJson.value("refresh_token", "");
            int expiresIn = responseJson.value("expires_in", 0);
            mLiveTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn - 60);
            mUserXUID = responseJson.value("user_id", mUserXUID); // Mantieni XUID precedente se non fornito

            if (mLiveAccessToken.empty() || expiresIn <= 0 || mLiveRefreshToken.empty()) { /* ... log e return false ... */ return false; }
            
            LOG(LogInfo) << "XboxAuth: Successfully obtained Live tokens from auth code.";
            // Non salvare ancora, prima ottieni XSTS
            if (authenticateXSTS()) { // authenticateXSTS aggiorna i membri XSTS e User
                LOG(LogInfo) << "XboxAuth: Successfully obtained XSTS token. Authentication complete.";
                saveTokenData(); // Salva TUTTO qui
                Settings::getInstance()->setBool("XboxLoggedIn", true);
                if (mSetStateCallback) mSetStateCallback("Autenticazione Xbox completata. XUID: " + mUserXUID);
                return true;
            } else {
                LOG(LogError) << "XboxAuth: Failed to obtain XSTS token after Live auth from code.";
                // Qui potresti voler cancellare i token Live appena ottenuti se XSTS fallisce per non lasciare uno stato a metà
                // clearAllTokenData(); // O solo i token Live
                return false;
            }
        } catch (const nj::json::parse_error& e) { /* ... log e return false ... */ return false; }
    } else { /* ... log errore HTTP e return false ... */ return false; }
    return false; // Dovrebbe essere già gestito
}


bool XboxAuth::authenticateXSTS() {
    if (mLiveAccessToken.empty()) {
        LOG(LogError) << "XboxAuth: Cannot authenticate XSTS without a Live access token.";
        return false;
    }
    LOG(LogInfo) << "XboxAuth: Authenticating for XSTS token...";

    // Step 1: User Authentication
    std::string firstXstsToken;
    {
        HttpReqOptions optionsStep1;
        nj::json requestBodyStep1 = {
            {"RelyingParty", "http://auth.xboxlive.com"},
            {"TokenType", "JWT"},
            {"Properties", {
                {"AuthMethod", "RPS"},
                {"SiteName", "user.auth.xboxlive.com"},
                {"RpsTicket", "d=" + mLiveAccessToken}
            }}
        };
        optionsStep1.dataToPost = requestBodyStep1.dump();
        optionsStep1.customHeaders.push_back("Content-Type: application/json");
        optionsStep1.customHeaders.push_back("Accept: application/json");
        optionsStep1.customHeaders.push_back("x-xbl-contract-version: 1");

        HttpReq requestStep1(XBOX_USER_AUTHENTICATE_URL, &optionsStep1);
        if (!requestStep1.wait()) { LOG(LogError) << "XboxAuth: XSTS Step 1 request failed (wait): " << requestStep1.getErrorMsg(); return false; }
        HttpReq::Status statusStep1 = requestStep1.status();
        if (statusStep1 != HttpReq::REQ_SUCCESS) { LOG(LogError) << "XboxAuth: XSTS Step 1 failed. Status: " << static_cast<int>(statusStep1) << " Body: " << requestStep1.getContent(); return false; }
        try {
            nj::json responseJson = nj::json::parse(requestStep1.getContent());
            if (responseJson.contains("Token")) { firstXstsToken = responseJson["Token"].get<std::string>(); } 
            else { LOG(LogError) << "XboxAuth: XSTS Step 1 response missing 'Token'."; return false; }
        } catch (const nj::json::parse_error& e) { LOG(LogError) << "XboxAuth: JSON parse error in XSTS Step 1: " << e.what(); return false; }
    }

    // Step 2: XSTS Authorization
    {
        HttpReqOptions optionsStep2;
        nj::json requestBodyStep2 = {
            {"RelyingParty", "http://xboxlive.com"},
            {"TokenType", "JWT"},
            {"Properties", {
                {"SandboxId", "RETAIL"},
                {"UserTokens", {firstXstsToken}}
            }}
        };
        optionsStep2.dataToPost = requestBodyStep2.dump();
        optionsStep2.customHeaders.push_back("Content-Type: application/json");
        optionsStep2.customHeaders.push_back("Accept: application/json");
        optionsStep2.customHeaders.push_back("x-xbl-contract-version: 1");

        HttpReq requestStep2(XBOX_XSTS_AUTHORIZE_URL, &optionsStep2);
        if (!requestStep2.wait()) { LOG(LogError) << "XboxAuth: XSTS Step 2 request failed (wait): " << requestStep2.getErrorMsg(); return false; }
        HttpReq::Status statusStep2 = requestStep2.status();
        if (statusStep2 != HttpReq::REQ_SUCCESS) { /* ... (gestione errore come nel vostro codice, ma NON chiamare refreshTokens() da qui per evitare ricorsione) ... */ 
            LOG(LogError) << "XboxAuth: XSTS Step 2 failed. Status: " << static_cast<int>(statusStep2) << " Body: " << requestStep2.getContent();
            // Non chiamare refreshTokens() qui per evitare loop. La funzione chiamante (refreshTokens) gestirà il fallimento.
            return false; 
        }
        try {
            nj::json responseJson = nj::json::parse(requestStep2.getContent());
            mXstsToken = responseJson.value("Token", "");
            if (responseJson.contains("DisplayClaims") && responseJson["DisplayClaims"].is_object() &&
                responseJson["DisplayClaims"].contains("xui") && responseJson["DisplayClaims"]["xui"].is_array() &&
                !responseJson["DisplayClaims"]["xui"].empty()) {
                const auto& xuiClaims = responseJson["DisplayClaims"]["xui"][0];
                if (xuiClaims.is_object()) {
                   mUserXUID = xuiClaims.value("xid", "");
                   mUserHash = xuiClaims.value("uhs", "");
                } else { /* ... log errore ... */ return false;}
            } else { /* ... log errore ... */ return false;}

            if (mLiveTokenExpiry > std::chrono::system_clock::now()) {
                 mXstsTokenExpiry = mLiveTokenExpiry;
            } else {
                 mXstsTokenExpiry = std::chrono::system_clock::now() + std::chrono::hours(8);
            }
            if (mXstsToken.empty() || mUserXUID.empty() || mUserHash.empty()) { /* ... log errore ... */ return false; }
            LOG(LogInfo) << "XboxAuth: Successfully obtained XSTS token parts. XUID: " << mUserXUID;
            // NON chiamare saveTokenData() qui.
            if (mSetStateCallback) mSetStateCallback("Authenticated XSTS (sub-step complete)");
            return true;
        } catch (const nj::json::parse_error& e) { /* ... log errore ... */ return false; }
    }
    return false;
}

bool XboxAuth::refreshTokens()
{
    // Utilizzare mHasTriedAutoLogin per evitare loop se chiamato più volte in caso di fallimento
    // Ma attenzione: se l'utente forza un refresh dalla UI, questo flag potrebbe impedirlo.
    // Per ora, lo gestiamo come un flag di "tentativo di refresh automatico all'avvio".
    // Potrebbe essere necessario un modo per resettarlo o bypassarlo per refresh manuali.
    if (mHasTriedAutoLogin && !Settings::getInstance()->getBool("ForceXboxTokenRefreshDebug")) {
         LOG(LogInfo) << "XboxAuth: refreshTokens() called, but auto-refresh attempt already made this session. Skipping unless forced.";
         return isAuthenticated(); // Restituisce lo stato attuale
    }
    mHasTriedAutoLogin = true; // Segna che un tentativo di refresh (automatico o manuale) è in corso o è stato fatto


    if (mLiveRefreshToken.empty()) {
        LOG(LogWarning) << "XboxAuth: No Live refresh token available. Cannot refresh.";
        return false;
    }

    LOG(LogInfo) << "XboxAuth: Attempting to refresh Live tokens...";

    // FASE 1: Refresh del token Live (usando mLiveRefreshToken)
    // Dovete implementare la chiamata HTTP POST a LIVE_TOKEN_URL con i parametri corretti:
    // client_id, refresh_token, grant_type="refresh_token", scope, redirect_uri
    // Esempio di chiamata HTTP (DA SOSTITUIRE CON LA VOSTRA IMPLEMENTAZIONE REALE CON HttpReq):
    nlohmann::json liveRefreshResponseJson;
    bool liveRefreshSuccess = false;

       // ========= INIZIO BLOCCO LOGICA HTTP CORRETTO =========
    std::string livePostFields = "client_id=" + CLIENT_ID +
                                 "&refresh_token=" + mLiveRefreshToken +
                                 "&grant_type=refresh_token" +
                                 "&scope=" + SCOPE +
                                 "&redirect_uri=" + REDIRECT_URI;

    HttpReqOptions options;
    options.dataToPost = livePostFields;
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
    options.customHeaders.push_back("Accept: application/json"); // Se la risposta attesa è JSON

    LOG(LogDebug) << "XboxAuth: Live Refresh POST to " << LIVE_TOKEN_URL;
    // Non loggare livePostFields in produzione se contiene dati sensibili

    auto liveReq = std::make_unique<HttpReq>(LIVE_TOKEN_URL, &options);

    // Attesa che la richiesta venga completata (HttpReq gestisce il thread internamente)
    // Il metodo wait() è più pulito di un ciclo con sleep se disponibile e fa ciò che serve.
    // Se HttpReq::wait() non esiste o non è bloccante fino al completamento,
    // il ciclo while(status() == REQ_IN_PROGRESS) è necessario.
    // Dal vostro HttpReq.h, wait() esiste: bool wait();
    
    // liveReq->wait(); // Questo dovrebbe bloccare finché la richiesta non è più IN_PROGRESS
    // OPPURE, se wait() non è sufficiente o per un controllo più esplicito:
    while (liveReq->status() == HttpReq::REQ_IN_PROGRESS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    HttpReq::Status reqStatus = liveReq->status(); // Ottieni lo stato finale

    if (reqStatus == HttpReq::REQ_SUCCESS) { // REQ_SUCCESS è 200
        try {
            liveRefreshResponseJson = nlohmann::json::parse(liveReq->getContent());
            if (liveRefreshResponseJson.contains("access_token") && liveRefreshResponseJson.contains("refresh_token")) {
                mLiveAccessToken = liveRefreshResponseJson["access_token"].get<std::string>();
                mLiveRefreshToken = liveRefreshResponseJson["refresh_token"].get<std::string>();
                int expiresIn = liveRefreshResponseJson.value("expires_in", 3600);
                mLiveTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn);
                if (liveRefreshResponseJson.count("user_id")) mUserXUID = liveRefreshResponseJson["user_id"].get<std::string>();

                LOG(LogInfo) << "XboxAuth: Live tokens refreshed successfully.";
                liveRefreshSuccess = true;
            } else {
                LOG(LogError) << "XboxAuth: Live token refresh response malformed. Response: " << liveReq->getContent();
            }
        } catch (const nlohmann::json::parse_error& e) {
            LOG(LogError) << "XboxAuth: Failed to parse Live token refresh response JSON: " << e.what() << ". Response: " << liveReq->getContent();
        }
    } else {
        // Gestisci altri stati di errore specifici da HttpReq::Status se necessario
        LOG(LogError) << "XboxAuth: Live token refresh HTTP request failed. Status enum: " << reqStatus
                      << " Error Msg: " << liveReq->getErrorMsg() << " Response: " << liveReq->getContent();
        
        // Controlla se lo status corrisponde a un codice di errore HTTP specifico che ci interessa
        if (reqStatus == HttpReq::REQ_400_BADREQUEST || reqStatus == HttpReq::REQ_401_FORBIDDEN || reqStatus == HttpReq::REQ_403_BADLOGIN) {
             LOG(LogError) << "XboxAuth: Live Refresh Token is likely invalid or revoked (HTTP " << static_cast<int>(reqStatus) << "). Clearing all tokens.";
             clearAllTokenData();
        }
    }
    // ========= FINE BLOCCO LOGICA HTTP CORRETTO =========

    if (!liveRefreshSuccess) {
        return false; // Il refresh del token Live è fallito
    }

    // FASE 2: Autenticazione XSTS con il nuovo token Live
    LOG(LogInfo) << "XboxAuth: Attempting to authenticate XSTS with new Live token...";
    if (authenticateXSTS()) { // authenticateXSTS dovrebbe usare mLiveAccessToken e aggiornare mXstsToken, mXstsTokenExpiry, mUserHash, mUserXUID
        LOG(LogInfo) << "XboxAuth: XSTS token obtained/refreshed successfully.";
        saveTokenData(); // Salva tutti i nuovi token (Live aggiornati e XSTS nuovo/aggiornato)
        Settings::getInstance()->setBool("XboxLoggedIn", true); // Assicura che il flag sia corretto
        return true;
    } else {
        LOG(LogError) << "XboxAuth: Failed to obtain/refresh XSTS token after successful Live token refresh.";
        // Qui potresti decidere se cancellare solo i token XSTS o lasciare che il prossimo isAuthenticated() fallisca.
        // Per ora, non cancelliamo nulla, ma l'autenticazione completa non è riuscita.
        // saveTokenData(); // Salva almeno i token Live rinfrescati, così l'utente non deve rifare il login completo subito
        return false;
    }
}


void XboxAuth::loadTokenData() {
    LOG(LogDebug) << "XboxAuth: Starting loadTokenData. Token storage path: " << mTokenStoragePath;
    mLiveAccessToken.clear(); mLiveRefreshToken.clear(); mXstsToken.clear(); mUserXUID.clear(); mUserHash.clear();
    mLiveTokenExpiry = std::chrono::system_clock::time_point();
    mXstsTokenExpiry = std::chrono::system_clock::time_point();

    std::filesystem::path liveTokenFilePath = std::filesystem::path(mTokenStoragePath) / LIVE_TOKENS_FILENAME_DEF;
    std::filesystem::path xstsTokenFilePath = std::filesystem::path(mTokenStoragePath) / XSTS_TOKENS_FILENAME_DEF;
    std::filesystem::path userInfoFilePath = std::filesystem::path(mTokenStoragePath) / USER_INFO_FILENAME_DEF;

    bool liveLoaded = false;
    bool xstsAndUserLoaded = false; // Tracciamo XSTS e UserInfo insieme per la validazione finale

    // Carica Live Tokens
    if (Utils::FileSystem::exists(liveTokenFilePath.string())) { // <--- CORREZIONE: .string()
        std::string liveTokenContent = readFileToStringLocal(liveTokenFilePath); // readFileToStringLocal prende già std::filesystem::path
        if (!liveTokenContent.empty()) {
            try {
                nj::json j = nj::json::parse(liveTokenContent);
                mLiveAccessToken = j.value("live_access_token", "");
                mLiveRefreshToken = j.value("live_refresh_token", "");
                long long expiry_t = j.value("live_token_expiry_epoch", 0LL);
                mLiveTokenExpiry = std::chrono::system_clock::from_time_t(static_cast<time_t>(expiry_t));
                if (!mLiveAccessToken.empty() && !mLiveRefreshToken.empty() && expiry_t > 0) {
                    liveLoaded = true;
                    LOG(LogInfo) << "XboxAuth: Live tokens loaded from " << liveTokenFilePath.string();
                } else {
                    LOG(LogError) << "XboxAuth: Incomplete Live tokens in " << liveTokenFilePath.string() << ". Removing file.";
                    Utils::FileSystem::removeFile(liveTokenFilePath.string()); // <--- CORREZIONE: .string()
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "XboxAuth: Error parsing Live tokens file: " << e.what() << ". Removing corrupt file: " << liveTokenFilePath.string();
                Utils::FileSystem::removeFile(liveTokenFilePath.string()); // <--- CORREZIONE: .string()
            }
        } else {
             LOG(LogWarning) << "XboxAuth: Live token file is empty, removing: " << liveTokenFilePath.string();
             Utils::FileSystem::removeFile(liveTokenFilePath.string()); // <--- CORREZIONE: .string()
        }
    } else {
        LOG(LogInfo) << "XboxAuth: Live token file not found: " << liveTokenFilePath.string();
    }

    // Carica XSTS Tokens
    bool xstsFileProcessed = false;
    if (Utils::FileSystem::exists(xstsTokenFilePath.string())) { // <--- CORREZIONE: .string()
        std::string xstsTokenContent = readFileToStringLocal(xstsTokenFilePath);
        if (!xstsTokenContent.empty()) {
            try {
                nj::json j = nj::json::parse(xstsTokenContent);
                mXstsToken = j.value("xsts_token", "");
                long long expiry_t = j.value("xsts_expiry_timestamp", 0LL);
                mXstsTokenExpiry = std::chrono::system_clock::from_time_t(static_cast<time_t>(expiry_t));
                 if (!mXstsToken.empty() && expiry_t > 0) {
                    LOG(LogInfo) << "XboxAuth: XSTS token loaded from " << xstsTokenFilePath.string();
                    xstsFileProcessed = true; // Abbiamo dati XSTS validi
                } else {
                    LOG(LogWarning) << "XboxAuth: Incomplete XSTS token in " << xstsTokenFilePath.string() << ". Removing file.";
                    Utils::FileSystem::removeFile(xstsTokenFilePath.string()); // <--- CORREZIONE: .string()
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "XboxAuth: Error parsing XSTS tokens file: " << e.what() << ". Removing corrupt file: " << xstsTokenFilePath.string();
                Utils::FileSystem::removeFile(xstsTokenFilePath.string()); // <--- CORREZIONE: .string()
            }
        } else {
            LOG(LogWarning) << "XboxAuth: XSTS token file is empty, removing: " << xstsTokenFilePath.string();
            Utils::FileSystem::removeFile(xstsTokenFilePath.string()); // <--- CORREZIONE: .string()
        }
    } else {
        LOG(LogInfo) << "XboxAuth: XSTS token file not found: " << xstsTokenFilePath.string();
    }

    // Carica User Info (XUID, UHS)
    bool userInfoFileProcessed = false;
    if (Utils::FileSystem::exists(userInfoFilePath.string())) { // <--- CORREZIONE: .string()
        std::string userInfoContent = readFileToStringLocal(userInfoFilePath);
        if (!userInfoContent.empty()) {
            try {
                nj::json j = nj::json::parse(userInfoContent);
                mUserXUID = j.value("user_xuid", "");
                mUserHash = j.value("user_hash", "");
                if (!mUserXUID.empty() && !mUserHash.empty()) {
                    LOG(LogInfo) << "XboxAuth: User info (XUID/UHS) loaded from " << userInfoFilePath.string() << ". XUID: " << mUserXUID;
                    userInfoFileProcessed = true; // Abbiamo dati utente validi
                } else {
                    LOG(LogError) << "XboxAuth: Incomplete User info in " << userInfoFilePath.string() << ". Removing file.";
                    Utils::FileSystem::removeFile(userInfoFilePath.string()); // <--- CORREZIONE: .string()
                }
            } catch (const std::exception& e) {
                LOG(LogError) << "XboxAuth: Error parsing User info file: " << e.what() << ". Removing corrupt file: " << userInfoFilePath.string();
                Utils::FileSystem::removeFile(userInfoFilePath.string()); // <--- CORREZIONE: .string()
            }
        } else {
            LOG(LogWarning) << "XboxAuth: User info file is empty, removing: " << userInfoFilePath.string();
            Utils::FileSystem::removeFile(userInfoFilePath.string()); // <--- CORREZIONE: .string()
        }
    } else {
        LOG(LogInfo) << "XboxAuth: User info file not found: " << userInfoFilePath.string();
    }

    // Valutazione finale del caricamento
    if (liveLoaded && xstsFileProcessed && userInfoFileProcessed) {
        LOG(LogInfo) << "XboxAuth: All essential token and user data loaded successfully.";
    } else if (liveLoaded && !mLiveRefreshToken.empty()) {
        LOG(LogWarning) << "XboxAuth: Live tokens loaded (with refresh token), but XSTS and/or User info might be missing or stale. Refresh will be attempted by constructor logic.";
    } else {
        LOG(LogWarning) << "XboxAuth: Not all critical token data could be loaded (especially Live Refresh Token or User Info). Refresh or new login may be required.";
        // Se mLiveRefreshToken è vuoto, il refresh automatico non sarà possibile.
        if (mLiveRefreshToken.empty()) {
             LOG(LogError) << "XboxAuth: Live Refresh Token is missing. Cannot perform automatic refresh.";
        }
    }
}

// --- saveTokenData() RIVISTA PER FILE SEPARATI ---
void XboxAuth::saveTokenData() {
    bool liveTokensSaved = false;
    bool xstsTokensSaved = false;
    bool userInfoSaved = false;

    // Salva Live Tokens
    if (!mLiveAccessToken.empty() && !mLiveRefreshToken.empty()) {
        nj::json liveTokensJson;
        liveTokensJson["live_access_token"] = mLiveAccessToken;
        liveTokensJson["live_refresh_token"] = mLiveRefreshToken;
        liveTokensJson["live_token_expiry_epoch"] = std::chrono::system_clock::to_time_t(mLiveTokenExpiry);
        
        std::filesystem::path liveTokenFilePath = std::filesystem::path(mTokenStoragePath) / LIVE_TOKENS_FILENAME_DEF;
        if (writeStringToFileLocal(liveTokenFilePath, liveTokensJson.dump(4))) {
            LOG(LogDebug) << "XboxAuth: Live tokens saved to " << liveTokenFilePath.string();
            liveTokensSaved = true;
        } else {
            LOG(LogError) << "XboxAuth: Failed to save Live tokens to " << liveTokenFilePath.string();
        }
    } else {
        LOG(LogWarning) << "XboxAuth: Live Access Token or Refresh Token is empty. Skipping save of Live tokens.";
    }

    // Salva XSTS Token
    if (!mXstsToken.empty()) {
        nj::json xstsTokensJson;
        xstsTokensJson["xsts_token"] = mXstsToken;
        xstsTokensJson["xsts_token_expiry_epoch"] = std::chrono::system_clock::to_time_t(mXstsTokenExpiry);
        // XUID e UHS sono tecnicamente parte della risposta XSTS, quindi potrebbero stare qui o in user_info
        // Per coerenza con il vostro load, li mettiamo in user_info. Se volete salvarli anche qui, aggiungeteli.
        // xstsTokensJson["xuid"] = mUserXUID;
        // xstsTokensJson["uhs"] = mUserHash;


        std::filesystem::path xstsTokenFilePath = std::filesystem::path(mTokenStoragePath) / XSTS_TOKENS_FILENAME_DEF;
        if (writeStringToFileLocal(xstsTokenFilePath, xstsTokensJson.dump(4))) {
            LOG(LogDebug) << "XboxAuth: XSTS token saved to " << xstsTokenFilePath.string();
            xstsTokensSaved = true; // Anche se XUID/UHS sono in un altro file, XSTS è stato salvato.
        } else {
            LOG(LogError) << "XboxAuth: Failed to save XSTS token to " << xstsTokenFilePath.string();
        }
    } else {
        LOG(LogWarning) << "XboxAuth: XSTS Token is empty. Skipping save of XSTS token.";
    }

    // Salva User Info (XUID, UHS)
    if (!mUserXUID.empty() && !mUserHash.empty()) {
        nj::json userInfoJson;
        userInfoJson["user_xuid"] = mUserXUID;
        userInfoJson["user_hash"] = mUserHash;

        std::filesystem::path userInfoFilePath = std::filesystem::path(mTokenStoragePath) / USER_INFO_FILENAME_DEF;
        if (writeStringToFileLocal(userInfoFilePath, userInfoJson.dump(4))) {
            LOG(LogDebug) << "XboxAuth: User info saved to " << userInfoFilePath.string();
            userInfoSaved = true;
        } else {
            LOG(LogError) << "XboxAuth: Failed to save User info to " << userInfoFilePath.string();
        }
    } else {
         LOG(LogWarning) << "XboxAuth: User XUID or Hash is empty. Skipping save of User Info.";
    }


    if (liveTokensSaved && xstsTokensSaved && userInfoSaved) { // O almeno live e user info se XSTS può essere derivato
        LOG(LogInfo) << "XboxAuth: All token and user data successfully saved.";
        Settings::getInstance()->setBool("XboxLoggedIn", true);
        Settings::getInstance()->setString("XboxUserXUID", mUserXUID);
    } else {
        LOG(LogError) << "XboxAuth: Failed to save one or more token/info files. Persistence might be incomplete.";
        // Non cancellare i token in memoria, sono ancora validi per la sessione
    }
}

// --- clearAllTokenData() RIVISTA PER FILE SEPARATI ---
void XboxAuth::clearAllTokenData() {
    mLiveAccessToken.clear();
    mLiveRefreshToken.clear();
    mLiveTokenExpiry = std::chrono::system_clock::time_point();

    mXstsToken.clear();
    mXstsTokenExpiry = std::chrono::system_clock::time_point();

    mUserXUID.clear();
    mUserHash.clear();

    mHasTriedAutoLogin = false; 

    std::filesystem::path liveTokenFilePath = std::filesystem::path(mTokenStoragePath) / LIVE_TOKENS_FILENAME_DEF;
    std::filesystem::path xstsTokenFilePath = std::filesystem::path(mTokenStoragePath) / XSTS_TOKENS_FILENAME_DEF;
    std::filesystem::path userInfoFilePath = std::filesystem::path(mTokenStoragePath) / USER_INFO_FILENAME_DEF;

    if (Utils::FileSystem::exists(liveTokenFilePath.string())) Utils::FileSystem::removeFile(liveTokenFilePath.string()); // <--- CORREZIONE
    if (Utils::FileSystem::exists(xstsTokenFilePath.string())) Utils::FileSystem::removeFile(xstsTokenFilePath.string()); // <--- CORREZIONE
    if (Utils::FileSystem::exists(userInfoFilePath.string())) Utils::FileSystem::removeFile(userInfoFilePath.string()); // <--- CORREZIONE

    LOG(LogInfo) << "XboxAuth: All token member variables cleared and token files removed if they existed.";

    Settings::getInstance()->setBool("XboxLoggedIn", false);
    Settings::getInstance()->setString("XboxUserXUID", "");
}


std::string XboxAuth::getLiveAccessToken() const { return mLiveAccessToken; }
std::string XboxAuth::getXstsToken() const { return mXstsToken; }
std::string XboxAuth::getXUID() const { return mUserXUID; }
std::string XboxAuth::getUserHash() const { return mUserHash; }
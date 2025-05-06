#include "GameStore/EpicGames/EpicGamesAuth.h"
#include "Log.h"
#include "utils/RandomString.h" // O la tua implementazione di generateRandomState
#include "utils/FileSystemUtil.h"
#include "Paths.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/base64.h" // Assicurati che questo sia corretto
#include "Settings.h"     // Se leggi ClientID/Secret da qui

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>      // Per ifstream, ofstream
#include <random>       // Per generateRandomState
#include <cstring>
#include <stdexcept>
#include <chrono>       // Per gestione tempo/scadenza
#include <ctime>        // Per time_t
#include <random>


using json = nlohmann::json;



// Definizione costanti nomi file (assicurati corrispondano a .h)
const std::string EpicGamesAuth::ACCESS_TOKEN_FILE = "epic_access_token.txt";
const std::string EpicGamesAuth::REFRESH_TOKEN_FILE = "epic_refresh_token.txt";
const std::string EpicGamesAuth::EXPIRY_FILE = "epic_expiry.txt";
const std::string EpicGamesAuth::ACCOUNT_ID_FILE = "epic_account_id.txt";
//const std::string EpicGamesAuth::STATE_FILE_NAME = "epic_auth_state.txt"; // Probabilmente non serve salvarlo

// Funzione Helper per leggere un file di testo semplice
std::string readFileToString(const std::string& path) {
    std::ifstream file(path);
    std::string content;
    if (file) {
        // Leggi l'intero contenuto in una volta (ok per file piccoli)
        content.assign((std::istreambuf_iterator<char>(file)),
                       (std::istreambuf_iterator<char>()));
    }
    return content;
}

// Funzione Helper per scrivere una stringa su file
bool writeStringToFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (file) {
        file << content;
        return true;
    }
    return false;
}


// Costruttore (Modificato)
EpicGamesAuth::EpicGamesAuth(std::function<void(const std::string&)> setStateCallback)
    : mSetStateCallback(setStateCallback), mAuthState(""), mHasValidTokenInfo(false) {
    LOG(LogDebug) << "EpicGamesAuth(callback) - Constructor";
    loadTokenData(); // Tenta di caricare
    if (mHasValidTokenInfo && !isAuthenticated()) {
        LOG(LogWarning) << "Loaded Epic token is expired. Clearing.";
        // Qui si potrebbe tentare il refresh automatico in futuro
        clearAllTokenData(); // Pulisci se caricato ma scaduto
    } else if (mHasValidTokenInfo) {
        LOG(LogInfo) << "EpicGamesAuth initialized with valid token.";
    } else {
        LOG(LogInfo) << "EpicGamesAuth initialized without saved token data.";
    }
}

// Costruttore Default (Modificato)
EpicGamesAuth::EpicGamesAuth()
    : mSetStateCallback(nullptr), mAuthState(""), mHasValidTokenInfo(false) {
    LOG(LogDebug) << "EpicGamesAuth() - Default Constructor";
    loadTokenData(); // Tenta di caricare
    if (mHasValidTokenInfo && !isAuthenticated()) {
        LOG(LogWarning) << "Loaded Epic token is expired. Clearing.";
        clearAllTokenData(); // Pulisci se caricato ma scaduto
    } else if (mHasValidTokenInfo) {
        LOG(LogInfo) << "EpicGamesAuth initialized with valid token.";
    } else {
        LOG(LogInfo) << "EpicGamesAuth initialized without saved token data.";
    }
}

EpicGamesAuth::~EpicGamesAuth() {
    LOG(LogDebug) << "EpicGamesAuth - Destructor";
    // Non serve fare nulla qui se non vogliamo salvare lo stato all'uscita
}

// Pulisce TUTTI i dati (memoria e file)
void EpicGamesAuth::clearAllTokenData() {
    LOG(LogDebug) << "EpicGamesAuth::clearAllTokenData() - Clearing all token info.";
    mAccessToken = "";
    mRefreshToken = "";
    mAccountId = "";
    mTokenExpiry = std::chrono::system_clock::now() - std::chrono::hours(1); // Imposta a un'ora fa
    mHasValidTokenInfo = false;

    // Cancella i file salvati
    std::string configDir = Utils::FileSystem::getEsConfigPath();
    Utils::FileSystem::removeFile(Utils::FileSystem::combine(configDir, ACCESS_TOKEN_FILE));
    Utils::FileSystem::removeFile(Utils::FileSystem::combine(configDir, REFRESH_TOKEN_FILE));
    Utils::FileSystem::removeFile(Utils::FileSystem::combine(configDir, EXPIRY_FILE));
    Utils::FileSystem::removeFile(Utils::FileSystem::combine(configDir, ACCOUNT_ID_FILE));
}

// Carica TUTTI i dati salvati
void EpicGamesAuth::loadTokenData() {
    std::string configDir = Utils::FileSystem::getEsConfigPath();
    bool success = true;
    time_t expiryTimestamp = 0;

    LOG(LogDebug) << "EpicGamesAuth::loadTokenData() - Loading saved token information.";

    mAccessToken = readFileToString(Utils::FileSystem::combine(configDir, ACCESS_TOKEN_FILE));
    if (mAccessToken.empty()) {
        LOG(LogDebug) << "Failed to load or empty file: " << ACCESS_TOKEN_FILE;
        success = false;
    }

    mRefreshToken = readFileToString(Utils::FileSystem::combine(configDir, REFRESH_TOKEN_FILE));
    if (mRefreshToken.empty() && success) { // Controlla solo se finora era ok
        LOG(LogDebug) << "Failed to load or empty file: " << REFRESH_TOKEN_FILE;
        // Potremmo considerare il refresh token opzionale per ora?
        // success = false;
    }

    mAccountId = readFileToString(Utils::FileSystem::combine(configDir, ACCOUNT_ID_FILE));
    if (mAccountId.empty() && success) {
        LOG(LogDebug) << "Failed to load or empty file: " << ACCOUNT_ID_FILE;
        success = false;
    }

    std::string expiryStr = readFileToString(Utils::FileSystem::combine(configDir, EXPIRY_FILE));
    if (expiryStr.empty() && success) {
        LOG(LogDebug) << "Failed to load or empty file: " << EXPIRY_FILE;
        success = false;
    } else if (success) {
        try {
            expiryTimestamp = std::stoll(expiryStr); // Converte stringa in time_t (long long)
            mTokenExpiry = std::chrono::system_clock::from_time_t(expiryTimestamp);
        } catch (const std::exception& e) {
            LOG(LogError) << "Error converting expiry timestamp '" << expiryStr << "': " << e.what();
            success = false;
        }
    }

    if (success) {
        mHasValidTokenInfo = true;
        LOG(LogInfo) << "Successfully loaded Epic token data from files.";
    } else {
        LOG(LogWarning) << "Failed to load complete Epic token data. Clearing all.";
        clearAllTokenData(); // Pulisce tutto se anche solo un dato essenziale manca o è corrotto
    }
}

// Salva TUTTI i dati correnti (chiamata da parseAndStoreTokenResponse)
void EpicGamesAuth::saveTokenData() {
    if (!mHasValidTokenInfo) {
        LOG(LogWarning) << "saveTokenData called but no valid token info present. Clearing files.";
        clearAllTokenData(); // Assicura che non rimangano file parziali
        return;
    }

    std::string configDir = Utils::FileSystem::getEsConfigPath();
    bool success = true;

    LOG(LogDebug) << "EpicGamesAuth::saveTokenData() - Saving token information.";

    if (!writeStringToFile(Utils::FileSystem::combine(configDir, ACCESS_TOKEN_FILE), mAccessToken)) {
        LOG(LogError) << "Failed to save file: " << ACCESS_TOKEN_FILE; success = false;
    }
    if (!writeStringToFile(Utils::FileSystem::combine(configDir, REFRESH_TOKEN_FILE), mRefreshToken)) {
        LOG(LogError) << "Failed to save file: " << REFRESH_TOKEN_FILE; success = false;
    }
    if (!writeStringToFile(Utils::FileSystem::combine(configDir, ACCOUNT_ID_FILE), mAccountId)) {
        LOG(LogError) << "Failed to save file: " << ACCOUNT_ID_FILE; success = false;
    }

    try {
        time_t expiryTimestamp = std::chrono::system_clock::to_time_t(mTokenExpiry);
        if (!writeStringToFile(Utils::FileSystem::combine(configDir, EXPIRY_FILE), std::to_string(expiryTimestamp))) {
            LOG(LogError) << "Failed to save file: " << EXPIRY_FILE; success = false;
        }
    } catch (const std::exception& e) {
         LOG(LogError) << "Error converting expiry time_point for saving: " << e.what();
         success = false;
    }

    if (success) {
        LOG(LogInfo) << "Epic token data successfully saved.";
    } else {
        LOG(LogError) << "Epic token data saving failed! Token might not persist.";
        // Non puliamo qui, altrimenti perdiamo i dati in memoria
    }
}


// Analizza la risposta JSON dal /token endpoint e salva i dati
void EpicGamesAuth::parseAndStoreTokenResponse(const nlohmann::json& response) {
    try {
        // Estrai i dati principali
        std::string newAccessToken = response.value("access_token", "");
        std::string newRefreshToken = response.value("refresh_token", "");
        std::string newAccountId = response.value("account_id", "");
        int expiresIn = response.value("expires_in", 0);

        // Verifica che i dati essenziali siano presenti
        if (!newAccessToken.empty() && !newAccountId.empty() && expiresIn > 0) {
            mAccessToken = newAccessToken;
            mRefreshToken = newRefreshToken; // Salva anche se vuoto
            mAccountId = newAccountId;

            // Calcola scadenza con margine di sicurezza (es. 60 secondi)
            int margin = 60;
            mTokenExpiry = std::chrono::system_clock::now() + std::chrono::seconds(expiresIn > margin ? expiresIn - margin : expiresIn);
            mHasValidTokenInfo = true;

            LOG(LogInfo) << "Epic token response parsed successfully. Account ID: " << mAccountId;

            // Salva i dati persistenti su file
            saveTokenData();

        } else {
            LOG(LogError) << "Access token, account ID, or expires_in missing/invalid in token response JSON.";
            clearAllTokenData(); // Pulisci tutto se i dati non sono validi
        }
    } catch (const json::exception& e) {
        LOG(LogError) << "JSON exception parsing token response: " << e.what();
        clearAllTokenData();
    } catch (const std::exception& e) {
        LOG(LogError) << "Generic exception parsing token response: " << e.what();
        clearAllTokenData();
    } catch (...) {
        LOG(LogError) << "Unknown exception parsing token response.";
        clearAllTokenData();
    }
}


// Metodo per ottenere il token (Modificato)
bool EpicGamesAuth::getAccessToken(const std::string& authCode, std::string& outAccessToken) {
    LOG(LogDebug) << "EpicGamesAuth::getAccessToken - Entering. Using Playnite Credentials.";

    std::string url = "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token";
    std::string clientId = "34a02cf8f4414e29b15921876da36f9a";
    std::string clientSecret = "daafbccc737745039dffe53d94fc76cf";
    std::string authString = clientId + ":" + clientSecret;
    std::string authEncoded = base64_encode(reinterpret_cast<const unsigned char*>(authString.c_str()), authString.length());
    std::string postData = "grant_type=authorization_code&code=" + authCode + "&token_type=eg1";

    HttpReqOptions options;
    options.dataToPost = postData;
    options.customHeaders.push_back("Authorization: Basic " + authEncoded);
    options.customHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

    HttpReq httpreq(url, &options);

    if (!httpreq.wait() || httpreq.status() != HttpReq::REQ_SUCCESS) {
        LOG(LogError) << "HTTP request failed to get access token! Status: " << httpreq.status() << ", Error: " << httpreq.getErrorMsg();
        LOG(LogError) << "Response Body: " << httpreq.getContent();
        clearAllTokenData(); // Pulisci tutto in caso di fallimento della richiesta
        outAccessToken = "";
        return false;
    }

    // Parsifica la risposta e salva i dati
    try {
        json response = json::parse(httpreq.getContent());
        parseAndStoreTokenResponse(response); // <<< Chiama il nuovo metodo di parsing e salvataggio
    } catch (const json::parse_error& e) {
        LOG(LogError) << "JSON parse error after getting token: " << e.what();
        clearAllTokenData();
        outAccessToken = "";
        return false;
    }

    // Restituisce lo stato basato sul successo del parsing/salvataggio
    if (mHasValidTokenInfo) {
        outAccessToken = mAccessToken; // Fornisci il token ottenuto all'esterno
        return true;
    } else {
        outAccessToken = "";
        return false;
    }
}

// Restituisce la stringa del token attuale (invariato)
std::string EpicGamesAuth::getAccessToken() const {
    return mAccessToken;
}

// Nuovo metodo per verificare l'autenticazione (Implementato)
bool EpicGamesAuth::isAuthenticated() const {
    if (!mHasValidTokenInfo || mAccessToken.empty()) {
        return false; // Non abbiamo dati validi o manca il token
    }
    // Controlla se la data/ora di scadenza è futura
    bool expired = std::chrono::system_clock::now() >= mTokenExpiry;
    if (expired) {
        LOG(LogWarning) << "Epic token check: Token is expired.";
    }
    return !expired; // Autenticato solo se non è scaduto
}

// --- Implementazione metodi helper rimanenti (getAuthorizationUrl, generateRandomState) ---
// Dovrebbero rimanere più o meno invariate rispetto al codice precedente,
// assicurati solo che generateRandomState funzioni come previsto.

std::string EpicGamesAuth::getAuthorizationUrl(std::string& state) {
    LOG(LogDebug) << "EpicGamesAuth::getAuthorizationUrl";
    state = generateRandomState();
    mAuthState = state; // Salva lo stato per la verifica nel callback (se usi callback)

    // Usa le credenziali Playnite per coerenza con getAccessToken
    std::string clientId = "34a02cf8f4414e29b15921876da36f9a";
    // L'URL authorize NON usa il client secret
    std::string redirectUri = "http://localhost:1234/epic_callback"; // O leggi da Settings

    std::string authUrl = "https://www.epicgames.com/id/authorize?"
                          "client_id=" + clientId + "&"
                          "response_type=code&"
                          // "redirect_uri=" + redirectUri + "&" // Rimosso per usare il login alternativo che mostra il codice
                          "scope=basic_profile%20offline_access&" // Aggiunto offline_access per refresh token
                          "prompt=login"; // Forziamo il login? Opzionale.
                          // "state=" + state; // Lo stato non serve più se non usi redirect_uri

    // -- URL per il flusso alternativo (mostra codice all'utente) --
     authUrl = "https://www.epicgames.com/id/api/redirect?clientId=" + clientId + "&responseType=code";

    LOG(LogDebug) << "Generated auth URL (manual code): " << authUrl;
    return authUrl;
}


std::string EpicGamesAuth::generateRandomState() {
    // Implementazione che usa <random>
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const int STATE_LENGTH = 32; // Lunghezza dello stato OAuth

    std::string stateStr;
    stateStr.reserve(STATE_LENGTH); // Pre-alloca la stringa per efficienza

    std::random_device rd;  // Generatore di numeri casuali non deterministico (se disponibile)
    std::mt19937 gen(rd()); // Motore Mersenne Twister inizializzato con rd
    std::uniform_int_distribution<> distrib(0, sizeof(alphanum) - 2); // Distribuzione uniforme sugli indici di alphanum

    for (int i = 0; i < STATE_LENGTH; ++i) {
        stateStr += alphanum[distrib(gen)];
    }

    LOG(LogDebug) << "Generated OAuth State: " << stateStr; // Log per debug
    return stateStr;
}

// Implementazione di clearAllTokenData (già definita sopra, ma per completezza)
// void EpicGamesAuth::clearAllTokenData() { ... implementazione già fornita ... }
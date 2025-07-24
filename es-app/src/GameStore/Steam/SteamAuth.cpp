#include "GameStore/Steam/SteamAuth.h"
#include "HttpReq.h"
#include "json.hpp"
#include "LocaleES.h"
#include "guis/GuiWebViewAuthLogin.h" // Per la WebView
#include "guis/GuiMsgBox.h"           // Per i messaggi UI
#include "Window.h"
#include <thread>

SteamAuth::SteamAuth()
    : mIsAuthenticated(false)
{
    LOG(LogInfo) << "SteamAuth: Inizializzazione modulo autenticazione Steam.";

    // Definisce e crea il percorso standardizzato
    std::string basePath = Utils::FileSystem::getEsConfigPath() + "/steam/";
    if (!Utils::FileSystem::exists(basePath)) {
        Utils::FileSystem::createDirectory(basePath);
    }
    mCredentialsPath = basePath + "steam_credentials.json";

    loadCredentials(); 

    // Se abbiamo una API Key, proviamo a ri-validare (per aggiornare il nome utente, ecc.)
    if (!mApiKey.empty()) {
        LOG(LogInfo) << "SteamAuth: Trovata API Key. Tentativo di validazione automatica...";
        validateAndSetAuthentication();
    }
    // Altrimenti, se non c'è API Key ma eravamo autenticati, ci fidiamo della sessione precedente.
    else if (mIsAuthenticated)
    {
        LOG(LogInfo) << "SteamAuth: Autenticato tramite sessione web precedente (nessuna API key per la ri-validazione).";
    }
}


SteamAuth::~SteamAuth()
{
    LOG(LogDebug) << "SteamAuth: Distruttore.";
}

void SteamAuth::loadCredentials()
{
    if (!Utils::FileSystem::exists(mCredentialsPath)) {
        return; // Nessun file da caricare
    }

    std::string content = Utils::FileSystem::readAllText(mCredentialsPath);
    if (content.empty()) {
        return;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(content);
        mApiKey = j.value("api_key", "");
        mSteamId = j.value("steam_id", "");
        mUserPersonaName = j.value("persona_name", "");
        mIsAuthenticated = j.value("is_authenticated", false);
    } catch (const std::exception& e) {
        LOG(LogError) << "SteamAuth: Errore nel parsing di steam_credentials.json: " << e.what();
        clearCredentials(); // Pulisce se il file è corrotto
    }
}

void SteamAuth::saveCredentials()
{
    nlohmann::json j;
    j["api_key"] = mApiKey;
    j["steam_id"] = mSteamId;
    j["persona_name"] = mUserPersonaName;
    j["is_authenticated"] = mIsAuthenticated;

    std::string content = j.dump(4);
    
    // Chiama la funzione direttamente, senza la condizione 'if'
    Utils::FileSystem::writeAllText(mCredentialsPath, content);
    
    // Logga semplicemente che l'operazione è stata eseguita
    LOG(LogInfo) << "SteamAuth: Credenziali salvate in " << mCredentialsPath;
}

bool SteamAuth::hasCredentials() const
{
    // Questa funzione ora significa "ha credenziali complete per una validazione API"
    return !mApiKey.empty() && !mSteamId.empty();
}

bool SteamAuth::validateAndSetAuthentication()
{
    if (!hasCredentials()) {
        LOG(LogWarning) << "SteamAuth: Impossibile validare con API, API Key o SteamID mancanti.";
        // Non impostiamo mIsAuthenticated a false qui, per non invalidare una sessione web
        return false;
    }

    LOG(LogInfo) << "SteamAuth: Tentativo di validazione credenziali API per SteamID: " << mSteamId;
    std::string url = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v0002/?key=" + mApiKey + "&steamids=" + mSteamId;

    HttpReq request(url);
    request.wait();
    HttpReq::Status status = request.status();

    bool wasAuthenticated = mIsAuthenticated;
    mIsAuthenticated = false;

    if (status == HttpReq::REQ_SUCCESS && !request.getContent().empty()) {
        try {
            nlohmann::json responseJson = nlohmann::json::parse(request.getContent());
            if (responseJson.contains("response") &&
                responseJson["response"].is_object() &&
                responseJson["response"].contains("players") &&
                responseJson["response"]["players"].is_array() &&
                !responseJson["response"]["players"].empty())
            {
                nlohmann::json playerJson = responseJson["response"]["players"][0];
                mUserPersonaName = playerJson.value("personaname", "");
                std::string steamIdFromJson = playerJson.value("steamid", "");

                if (!mUserPersonaName.empty() && steamIdFromJson == mSteamId) {
                    mIsAuthenticated = true;
                    LOG(LogInfo) << "SteamAuth: Validazione API Riuscita! Utente: " << mUserPersonaName;
                    saveCredentials(); // Salva i dati aggiornati
                } else {
                    LOG(LogError) << "SteamAuth: Validazione API Fallita. Dati utente non corrispondenti.";
                }
            } else {
                LOG(LogError) << "SteamAuth: Validazione API Fallita. Risposta API non valida.";
            }
        } catch (const nlohmann::json::exception& e) {
            LOG(LogError) << "SteamAuth: Validazione API Fallita. Errore parsing JSON: " << e.what();
        }
    } else {
        LOG(LogError) << "SteamAuth: Validazione API Fallita. Errore HTTP. Status: " << static_cast<int>(status);
    }

    if (!mIsAuthenticated) {
        mIsAuthenticated = wasAuthenticated; // Ripristina lo stato se la validazione fallisce
    }
    return mIsAuthenticated;
}

void SteamAuth::authenticateWithWebView(Window* window)
{
    const std::string STEAM_LOGIN_URL = "https://steamcommunity.com/login/home/?goto=login";

    LOG(LogInfo) << "[SteamAuth] Avvio login WebView per Steam. URL: " << STEAM_LOGIN_URL;

    auto webViewGui = new GuiWebViewAuthLogin(
        window,
        STEAM_LOGIN_URL,
        "Steam",
        "",      // Nessun prefisso di reindirizzamento per Steam cookie/script flow
        GuiWebViewAuthLogin::AuthMode::FETCH_STEAM_COOKIE
    );

    webViewGui->setSteamCookieDomain("steamcommunity.com");

    webViewGui->setOnLoginFinishedCallback(
        [this, window](bool success, const std::string& jsonDataOrError)
    {
        if (success)
        {
            LOG(LogInfo) << "[SteamAuth] WebView login per Steam completato. Dati JSON ricevuti.";
            try {
                nlohmann::json profileJson = nlohmann::json::parse(jsonDataOrError);
                std::string profileName = profileJson.value("strProfileName", "");
                std::string steamId = profileJson.value("strSteamId", "");

                if (!steamId.empty()) {
                    mSteamId = steamId;
                    mUserPersonaName = profileName;
                    mIsAuthenticated = true;
                    // L'API Key non è ottenuta tramite questo flusso, quindi mApiKey rimane invariato.
                    saveCredentials();

                    LOG(LogInfo) << "[SteamAuth UI] Login Steam riuscito! Utente: " << mUserPersonaName << " (" << mSteamId << ")";
                    window->pushGui(new GuiMsgBox(window, _("Login Steam riuscito! Benvenuto,") + " " + mUserPersonaName + "!"));

                    // NON CHIAMARE refreshGamesListAsync QUI.
                    // L'aggiornamento della lista giochi è un'azione separata.

                } else {
                    LOG(LogError) << "[SteamAuth] Login Steam fallito: SteamID non trovato nel JSON del profilo. Dati: " << jsonDataOrError;
                    window->pushGui(new GuiMsgBox(window, _("Accesso Steam fallito: Dati utente incompleti.")));
                    clearCredentials();
                }
            } catch (const nlohmann::json::parse_error& e) {
                LOG(LogError) << "[SteamAuth] Errore parsing JSON del profilo Steam (WebView): " << e.what() << ". Dati: " << jsonDataOrError;
                window->pushGui(new GuiMsgBox(window, _("Accesso Steam fallito: Errore dati profilo.")));
            } catch (const std::exception& e) {
                LOG(LogError) << "[SteamAuth] Errore generico nel callback SteamAuth WebView: " << e.what();
                window->pushGui(new GuiMsgBox(window, _("Accesso Steam fallito: Errore generico.")));
            }
        }
        else
        {
            LOG(LogError) << "[SteamAuth] Login WebView per Steam annullato o fallito: " << jsonDataOrError;
            window->pushGui(new GuiMsgBox(window, _("Accesso Steam annullato o fallito.")));
            clearCredentials();
        }
    });

    window->pushGui(webViewGui);
}

bool SteamAuth::isAuthenticated() const
{
    // Considera autenticato se mIsAuthenticated è true E abbiamo SteamID e PersonaName.
    return mIsAuthenticated && !mSteamId.empty() && !mUserPersonaName.empty();
}


std::string SteamAuth::getSteamId() const
{
    return mSteamId;
}

std::string SteamAuth::getApiKey() const
{
    return mApiKey;
}

std::string SteamAuth::getUserPersonaName() const
{
    if (mIsAuthenticated && !mUserPersonaName.empty()) {
        return mUserPersonaName;
    }
    return mSteamId.empty() ? _("NON AUTENTICATO") : (_("UTENTE (") + mSteamId + _(")"));
}

bool SteamAuth::isLoggedIn() const { return isAuthenticated(); }



void SteamAuth::setCredentials(const std::string& apiKey, const std::string& steamId)
{
    LOG(LogInfo) << "SteamAuth: Impostazione nuove credenziali Steam (API Key).";
    mApiKey = apiKey;
    mSteamId = steamId;
    mUserPersonaName = ""; // Reset del nome persona, verrà recuperato dalla validazione API o WebView
    mIsAuthenticated = false; // Reset dello stato, necessita di nuova validazione
    saveCredentials(); // Salva subito API Key e SteamID, il nome utente verrà aggiornato dopo validate.
}

void SteamAuth::clearCredentials()
{
    LOG(LogInfo) << "SteamAuth: Cancellazione di tutte le credenziali Steam.";
    mApiKey = "";
    mSteamId = "";
    mUserPersonaName = "";
    mIsAuthenticated = false;

    if (Utils::FileSystem::exists(mCredentialsPath)) {
        Utils::FileSystem::removeFile(mCredentialsPath);
    }
}

std::string SteamAuth::getCookie(const std::string& name) const
{
    if (name == "sessionid") {
        return mSessionIdCookie;
    }
    if (name == "steamLoginSecure") {
        return mSteamLoginSecureCookie;
    }
    return "";
}
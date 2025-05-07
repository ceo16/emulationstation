#include "GameStore/Steam/SteamAuth.h"
#include "HttpReq.h"
#include "json.hpp"           // Per mostrare messaggi all'utente
#include "LocaleES.h"  
// Non includere Window.h e GuiMsgBox.h qui, la UI dovrebbe gestire i messaggi.

SteamAuth::SteamAuth()
    : mIsAuthenticated(false)
{
    LOG(LogInfo) << "SteamAuth: Inizializzazione modulo autenticazione Steam.";
    loadCredentials();
}

SteamAuth::~SteamAuth()
{
    LOG(LogDebug) << "SteamAuth: Distruttore.";
}

void SteamAuth::loadCredentials()
{
    mApiKey = Settings::getInstance()->getString("SteamApiKey");
    mSteamId = Settings::getInstance()->getString("SteamUserId");
    mUserPersonaName = Settings::getInstance()->getString("SteamUserPersonaName");

    if (!mApiKey.empty() && !mSteamId.empty()) {
        LOG(LogInfo) << "SteamAuth: Credenziali Steam caricate (API Key presente, SteamID: " << mSteamId << ", Utente salvato: " << mUserPersonaName << ")";
    } else {
        LOG(LogInfo) << "SteamAuth: Nessuna API Key o SteamID trovati nelle impostazioni.";
        mApiKey = "";
        mSteamId = "";
        mUserPersonaName = "";
    }
    mIsAuthenticated = false;
}

void SteamAuth::saveCredentials()
{
    Settings::getInstance()->setString("SteamApiKey", mApiKey);
    Settings::getInstance()->setString("SteamUserId", mSteamId);
    Settings::getInstance()->setString("SteamUserPersonaName", mUserPersonaName);
    Settings::getInstance()->saveFile();
    LOG(LogInfo) << "SteamAuth: Credenziali Steam e nome utente salvati nelle impostazioni.";
}

bool SteamAuth::hasCredentials() const
{
    return !mApiKey.empty() && !mSteamId.empty();
}

bool SteamAuth::validateAndSetAuthentication()
{
    if (!hasCredentials()) {
        LOG(LogWarning) << "SteamAuth: Impossibile validare, API Key o SteamID mancanti.";
        mIsAuthenticated = false;
        mUserPersonaName = "";
        return false;
    }

    LOG(LogInfo) << "SteamAuth: Tentativo di validazione credenziali Steam per SteamID: " << mSteamId;
    std::string url = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v0002/?key=" + mApiKey + "&steamids=" + mSteamId;

    HttpReq request(url); // Non serve più std::unique_ptr qui se HttpReq è stack-based o gestisce la sua memoria
    request.wait(); // wait() è void
    HttpReq::Status status = request.status(); // Ottieni lo stato dopo wait()

    mIsAuthenticated = false;
    std::string oldPersonaName = mUserPersonaName;
    mUserPersonaName = "";

    if (status == HttpReq::REQ_SUCCESS && !request.getContent().empty()) { // CORRETTO
        try {
            nlohmann::json responseJson = nlohmann::json::parse(request.getContent());
            if (responseJson.contains("response") &&
                responseJson["response"].is_object() && // Aggiunto controllo is_object
                responseJson["response"].contains("players") &&
                responseJson["response"]["players"].is_array() &&
                !responseJson["response"]["players"].empty())
            {
                nlohmann::json playerJson = responseJson["response"]["players"][0];
                mUserPersonaName = playerJson.value("personaname", "");
                std::string steamIdFromJson = playerJson.value("steamid", "");

                if (!mUserPersonaName.empty() && steamIdFromJson == mSteamId) {
                    mIsAuthenticated = true;
                    LOG(LogInfo) << "SteamAuth: Validazione Riuscita! Utente: " << mUserPersonaName;
                    if (oldPersonaName != mUserPersonaName) {
                        saveCredentials();
                    }
                } else {
                    LOG(LogError) << "SteamAuth: Validazione Fallita. Dati utente non corrispondenti o mancanti nella risposta API.";
                    LOG(LogDebug) << "  Risposta: " << request.getContent();
                }
            } else {
                LOG(LogError) << "SteamAuth: Validazione Fallita. Risposta API non contiene i dati attesi.";
                LOG(LogDebug) << "  Risposta: " << request.getContent();
            }
        } catch (const nlohmann::json::exception& e) {
            LOG(LogError) << "SteamAuth: Validazione Fallita. Errore parsing JSON: " << e.what();
            LOG(LogDebug) << "  Risposta: " << request.getContent();
        }
    } else {
        LOG(LogError) << "SteamAuth: Validazione Fallita. Errore HTTP. Status: " << static_cast<int>(status) << " - " << request.getErrorMsg(); // CORRETTO
    }

    if (!mIsAuthenticated) {
        mUserPersonaName = oldPersonaName;
    }
    return mIsAuthenticated;
}

bool SteamAuth::isAuthenticated() const
{
    return mIsAuthenticated;
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

void SteamAuth::setCredentials(const std::string& apiKey, const std::string& steamId)
{
    LOG(LogInfo) << "SteamAuth: Impostazione nuove credenziali Steam. APIKey "
                 << (apiKey.empty() ? "vuota" : "fornita") << ", SteamID: " << steamId;
    mApiKey = apiKey;
    mSteamId = steamId;
    mUserPersonaName = "";
    mIsAuthenticated = false;
    saveCredentials();
}

void SteamAuth::clearCredentials()
{
    LOG(LogInfo) << "SteamAuth: Cancellazione delle credenziali Steam.";
    mApiKey = "";
    mSteamId = "";
    mUserPersonaName = "";
    mIsAuthenticated = false;
    saveCredentials();
}
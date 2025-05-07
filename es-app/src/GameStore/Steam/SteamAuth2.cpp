#include "SteamAuth.h"
#include "Settings.h" // Per salvare/caricare API key/SteamID
#include <fstream>
#include "json.hpp" // Se usi JSON per configurazioni, o un parser VDF

// TODO: Includere un parser VDF se necessario
// Ad esempio, potresti adattare la logica da Playnite per leggere loginusers.vdf
// #include "KeyValueParser.h" // Un ipotetico parser VDF

SteamAuth::SteamAuth() : mIsAuthenticated(false)
{
    LOG(LogDebug) << "SteamAuth: Constructor";
    // TODO: Carica API Key e SteamID da Settings o un file di configurazione sicuro
    // mApiKey = Settings::getInstance()->getString("SteamApiKey");
    // mSteamId = Settings::getInstance()->getString("SteamUserId");
    // if (!mApiKey.empty() && !mSteamId.empty()) {
    //     // TODO: Potresti voler validare le credenziali qui o fare un tentativo di login
    //     // mIsAuthenticated = true; // o dopo una validazione
    //     // mUserPersonaName = Settings::getInstance()->getString("SteamUserPersonaName");
    //     LOG(LogInfo) << "SteamAuth: Credenziali caricate per SteamID: " << mSteamId;
    // } else {
    //     LOG(LogInfo) << "SteamAuth: Nessuna credenziale Steam trovata.";
    // }
}

SteamAuth::~SteamAuth()
{
    LOG(LogDebug) << "SteamAuth: Destructor";
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
    // TODO: Recupera il nome utente se autenticato, altrimenti restituisci "Ospite" o vuoto
    // return mUserPersonaName.empty() ? "Steam User" : mUserPersonaName;
    return "Steam User (TODO)";
}

// TODO: Implementare gli altri metodi per login, caricamento/salvataggio credenziali,
//       e parsing di file di configurazione Steam (es. loginusers.vdf)
//       La logica di Playnite `GetSteamUsers()` in `SteamLibrary.cs` è un buon riferimento
//       per `loginusers.vdf`.

/* Esempio concettuale per loginusers.vdf (richiede un parser VDF)
std::string SteamAuth::getLoginUsersVdfPath() {
    // TODO: Trovare il percorso di installazione di Steam
    // Potrebbe essere dal registro su Windows o percorsi standard su Linux/macOS
    // std::string steamPath = Steam::getInstallationPath(); // Funzione helper da creare
    // if (steamPath.empty()) return "";
    // return Utils::FileSystem::resolvePath(steamPath + "/config/loginusers.vdf");
    return "C:/Program Files (x86)/Steam/config/loginusers.vdf"; // Placeholder
}

void SteamAuth::parseLoginUsersVdf(const std::string& path) {
    if (!Utils::FileSystem::exists(path)) {
        LOG(LogWarning) << "SteamAuth: loginusers.vdf non trovato in " << path;
        return;
    }
    // KeyValue vdfData; // Ipotetico oggetto risultato del parser VDF
    // if (vdfData.ReadFile(path)) { // Ipotetico metodo di parsing
    //     for (const auto& userNode : vdfData.getChildren("users")) { // o struttura simile
    //         std::string userId64 = userNode.getName(); // L'ID utente
    //         std::string accountName = userNode.getChildValue("AccountName");
    //         std::string personaName = userNode.getChildValue("PersonaName");
    //         bool mostRecent = userNode.getChildValue("mostrecent") == "1";
    //         // ... memorizza o usa questi dati ...
    //         if (mostRecent && mSteamId.empty()) { // Autoseleziona il più recente se non già configurato
    //              mSteamId = userId64;
    //              mUserPersonaName = personaName;
    //              LOG(LogInfo) << "SteamAuth: Utente Steam locale più recente selezionato: " << personaName << " (" << userId64 << ")";
    //         }
    //     }
    // } else {
    //     LOG(LogError) << "SteamAuth: Fallito il parsing di loginusers.vdf";
    // }
}
*/
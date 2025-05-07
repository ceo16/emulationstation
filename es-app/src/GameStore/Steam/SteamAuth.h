#pragma once
#ifndef ES_APP_GAMESTORE_STEAM_AUTH_H
#define ES_APP_GAMESTORE_STEAM_AUTH_H

#include <string>
#include "Log.h"      // Per il logging
#include "Settings.h" // Per salvare/recuperare API Key e SteamID dalla configurazione di ES

class SteamAuth
{
public:
    SteamAuth();
    ~SteamAuth();

    // Verifica se le credenziali (API Key, SteamID) sono state caricate/impostate
    bool hasCredentials() const;

    // Tenta di validare le credenziali caricate.
    // Internamente, può fare una chiamata API leggera per confermare che funzionino
    // e per recuperare/aggiornare il nome utente (PersonaName).
    // Restituisce true se la validazione ha successo, altrimenti false.
    // Dovrebbe essere chiamata dopo che l'utente ha inserito/modificato le credenziali,
    // o all'avvio se le credenziali sono già salvate.
    bool validateAndSetAuthentication();

    // Restituisce lo stato di autenticazione (true se validateAndSetAuthentication è andata a buon fine)
    bool isAuthenticated() const;

    std::string getSteamId() const;         // Restituisce lo SteamID64 dell'utente
    std::string getApiKey() const;          // Restituisce l'API Key dell'utente
    std::string getUserPersonaName() const; // Restituisce il nome utente visualizzato (recuperato da API)

    // Metodi per impostare le credenziali (tipicamente chiamati dalla UI dopo l'input dell'utente)
    void setCredentials(const std::string& apiKey, const std::string& steamId);

    // Metodo per cancellare le credenziali salvate
    void clearCredentials();

private:
    // Carica API Key e SteamID da Settings::getInstance()
    void loadCredentials();

    // Salva API Key, SteamID e PersonaName in Settings::getInstance()
    void saveCredentials();

    std::string mSteamId;         // Memorizza lo SteamID64
    std::string mApiKey;          // Memorizza l'API Key
    std::string mUserPersonaName; // Memorizza il nome utente recuperato
    bool mIsAuthenticated;        // Flag che indica se le credenziali sono state validate con successo
};

#endif // ES_APP_GAMESTORE_STEAM_AUTH_H
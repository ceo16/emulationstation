#pragma once
#ifndef ES_APP_GAMESTORE_STEAM_AUTH_H
#define ES_APP_GAMESTORE_STEAM_AUTH_H

#include <string>
#include "Log.h"      // Per il logging
#include "Settings.h" // Per salvare/recuperare API Key e SteamID dalla configurazione di ES
#include "Window.h"

class SteamAuth
{
public:
    SteamAuth();
    ~SteamAuth();

    // Verifica se le credenziali (API Key, SteamID) sono state caricate/impostate
    bool hasCredentials() const;

    void authenticateWithWebView(Window* window);
    bool validateAndSetAuthentication();

    // Restituisce lo stato di autenticazione (true se validateAndSetAuthentication è andata a buon fine)
    bool isAuthenticated() const;
	bool isLoggedIn() const;

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
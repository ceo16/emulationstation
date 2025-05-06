#pragma once
#ifndef ES_CORE_EPICGAMESAUTH_H
#define ES_CORE_EPICGAMESAUTH_H

#include <string>
#include <functional>
#include <chrono> // <-- Aggiunto
#include "json.hpp"  

// Forward declaration
using json = nlohmann::json;

class EpicGamesAuth {
public:
    // Costruttori e Distruttore
    EpicGamesAuth(std::function<void(const std::string&)> setStateCallback);
    EpicGamesAuth();
    ~EpicGamesAuth();

    // Metodi flusso OAuth
    std::string getAuthorizationUrl(std::string& state);
    bool getAccessToken(const std::string& authCode, std::string& outAccessToken); // Ottiene token con codice

    // Metodi per accedere ai dati (getAccessToken restituisce solo la stringa)
    std::string getAccessToken() const;
    std::string getAccountId() const { return mAccountId; } // Utile per UI/debug
    std::string getRefreshToken() const { return mRefreshToken; } // Potrebbe servire per refresh manuale

    // --- NUOVO Metodo di Controllo ---
    /**
     * @brief Controlla se esiste un token di accesso valido e non scaduto.
     * @return true se il token è considerato valido, false altrimenti.
     */
    bool isAuthenticated() const;

    // --- NUOVO Metodo per Logout/Pulizia Manuale ---
    /**
     * @brief Pulisce tutte le informazioni del token (memoria e file). Da chiamare per il logout.
     */
    void clearAllTokenData();


    // Metodo esistente per stato OAuth
    std::string getCurrentState() const { return mAuthState; }


private:
    // Metodi helper
    std::string generateRandomState();
    void loadTokenData(); // Carica TUTTI i dati salvati
    void saveTokenData(); // Salva TUTTI i dati correnti
    void parseAndStoreTokenResponse(const nlohmann::json& response); // Analizza risposta e salva

    // Membri
    std::function<void(const std::string&)> mSetStateCallback;
    std::string mAuthState; // Stato per il flusso OAuth

    // Dati del Token
    std::string mAccessToken;
    std::string mRefreshToken;
    std::string mAccountId;
    std::chrono::time_point<std::chrono::system_clock> mTokenExpiry;
    bool mHasValidTokenInfo; // Flag per sapere se abbiamo caricato/ottenuto dati token

    // Costanti per Nomi File
    static const std::string ACCESS_TOKEN_FILE;
    static const std::string REFRESH_TOKEN_FILE;
    static const std::string EXPIRY_FILE;
    static const std::string ACCOUNT_ID_FILE;
    // STATE_FILE_NAME non serve più se mAuthState non è persistente
    // static const std::string STATE_FILE_NAME;
};

#endif // ES_CORE_EPICGAMESAUTH_H
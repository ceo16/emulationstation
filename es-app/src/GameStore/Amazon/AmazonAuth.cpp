#include "AmazonAuth.h"
#include "GameStore/Amazon/AmazonGamesModels.h"
#include "GameStore/Amazon/AmazonGamesHelper.h"
#include "guis/GuiWebViewAuthLogin.h"
#include "HttpReq.h"
#include "Log.h"
#include "Paths.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"

#include <thread>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream> // Per stringstream

// --- FUNZIONE urlDecode MANCANTE, AGGIUNTA QUI ---
std::string urlDecode(const std::string& encoded) {
    std::string res;
    res.reserve(encoded.length());
    for (std::size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%') {
            if (i + 2 < encoded.length()) {
                int val;
                std::stringstream ss;
                ss << std::hex << encoded.substr(i + 1, 2);
                ss >> val;
                res += static_cast<char>(val);
                i += 2;
            }
        } else if (encoded[i] == '+') {
            res += ' ';
        } else {
            res += encoded[i];
        }
    }
    return res;
}

// Funzione helper che usa la nostra urlDecode
std::map<std::string, std::string> ParseQueryString(const std::string& query)
{
    std::map<std::string, std::string> result;
    std::string cleanQuery = query;
    if (query.length() > 0 && (query[0] == '?' || query[0] == '#'))
        cleanQuery = query.substr(1);

    auto pairs = Utils::String::split(cleanQuery, '&');
    for (const auto& pair : pairs)
    {
        auto keyValue = Utils::String::split(pair, '=');
        if (keyValue.size() == 2)
        {
            result[urlDecode(keyValue[0])] = urlDecode(keyValue[1]);
        }
    }
    return result;
}

AmazonAuth::AmazonAuth(Window* window) : mWindow(window) {
    std::string userPath = Paths::getUserEmulationStationPath();
    Utils::FileSystem::createDirectory(userPath + "/gamelists");
    Utils::FileSystem::createDirectory(userPath + "/gamelists/amazon");    
    mTokensPath = userPath + "/gamelists/amazon/amazon_tokens.json";
    loadTokens();
}

AmazonAuth::~AmazonAuth() {}

void AmazonAuth::startLoginFlow(std::function<void(bool success)> on_complete)
{
    const std::string loginUrl = "https://www.amazon.com/ap/signin?openid.ns=http://specs.openid.net/auth/2.0&openid.claimed_id=http://specs.openid.net/auth/2.0/identifier_select&openid.identity=http://specs.openid.net/auth/2.0/identifier_select&openid.mode=checkid_setup&openid.oa2.scope=device_auth_access&openid.ns.oa2=http://www.amazon.com/ap/ext/oauth/2&openid.oa2.response_type=token&openid.oa2.client_id=device:6330386435633439383366623032393938313066303133343139383335313266234132554D56484F58375550345637&language=en_US&marketPlaceId=ATVPDKIKX0DER&openid.return_to=https://www.amazon.com&openid.pape.max_auth_age=0&openid.assoc_handle=amzn_sonic_games_launcher&pageId=amzn_sonic_games_launcher";
    const std::string watchUrlPrefix = "https://www.amazon.com/";

    auto webViewGui = new GuiWebViewAuthLogin(mWindow, loginUrl, "Amazon Games", watchUrlPrefix, 
                                              GuiWebViewAuthLogin::AuthMode::AMAZON_OAUTH_FRAGMENT);

    webViewGui->setOnLoginFinishedCallback(
        [this, on_complete](bool success, const std::string& finalUrl) {
            if (success) {
                size_t queryPos = finalUrl.find('?');
                std::string queryString = (queryPos != std::string::npos) ? finalUrl.substr(queryPos) : "";
                
                auto params = ParseQueryString(queryString);
                std::string initialToken = params["openid.oa2.access_token"];

                if (!initialToken.empty()) {
                    LOG(LogInfo) << "Amazon Auth: Token estratto e decodificato con successo. Avvio scambio.";
                    std::thread(&AmazonAuth::exchangeInitialToken, this, initialToken, on_complete).detach();
                } else {
                    LOG(LogError) << "Amazon Auth: Login riuscito, ma token non trovato. URL: " << finalUrl;
                    if (on_complete) on_complete(false);
                }
            } else {
                LOG(LogError) << "Amazon Auth: Login WebView fallito o annullato.";
                if (on_complete) on_complete(false);
            }
        });

    mWindow->pushGui(webViewGui);
}

void AmazonAuth::exchangeInitialToken(const std::string& initialToken, std::function<void(bool success)> on_complete) {
    HttpReqOptions options;
    options.customHeaders.push_back("User-Agent: AGSLauncher/1.0.0");
    options.customHeaders.push_back("Content-Type: application/json");

    Amazon::DeviceRegistrationRequest reqData;
    reqData.auth_data.access_token = initialToken;
    reqData.registration_data.app_name = "AGSLauncher for Windows";
    reqData.registration_data.app_version = "1.0.0";
    reqData.registration_data.device_model = "Windows";
    reqData.registration_data.device_serial = Amazon::Helper::getMachineGuidNoHyphens();
    reqData.registration_data.device_type = "A2UMVHOX7UP4V7";
    reqData.registration_data.domain = "Device";
    reqData.registration_data.os_version = Amazon::Helper::getWindowsVersionString();
    reqData.requested_extensions = { "customer_info", "device_info" };
    reqData.requested_token_type = { "bearer", "mac_dms" };

    nlohmann::json jsonBody;
    Amazon::to_json(jsonBody, reqData);
    options.dataToPost = jsonBody.dump();

    HttpReq request("https://api.amazon.com/auth/register", &options);
    request.wait();

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            auto responseJson = nlohmann::json::parse(request.getContent());
            if (responseJson.contains("response") && responseJson["response"].contains("success")) {
                auto bearerToken = responseJson["response"]["success"]["tokens"]["bearer"];
                mAccessToken = bearerToken.value("access_token", "");
                mRefreshToken = bearerToken.value("refresh_token", "");
                saveTokens();
                LOG(LogInfo) << "Amazon Auth: VITTORIA! TOKEN SCAMBIATI E SALVATI CON SUCCESSO!";
                if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete(true); });
            } else {
                LOG(LogError) << "Amazon Auth: Risposta JSON non valida. Body: " << request.getContent();
                if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete(false); });
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Amazon Auth: Errore parsing risposta JSON: " << e.what();
            if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete(false); });
        }
    } else {
        LOG(LogError) << "Amazon Auth: Richiesta di registrazione fallita. Status: " << request.status() << " Body: " << request.getErrorMsg();
        if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete(false); });
    }
}

bool AmazonAuth::refreshTokens() {
    if (mRefreshToken.empty()) {
        LOG(LogError) << "Amazon Auth: Nessun refresh token disponibile per il refresh.";
        return false;
    }

    LOG(LogInfo) << "Amazon Auth: Access token scaduto. Tento il refresh...";

    HttpReqOptions options;
    options.customHeaders.push_back("User-Agent: com.amazon.agslauncher.win/1.1.133.2-9e2c3a3");
    options.customHeaders.push_back("Content-Type: application/json");
    options.customHeaders.push_back("Expect: 100-continue");

    Amazon::TokenRefreshRequest reqData;
    reqData.source_token = mRefreshToken;

    nlohmann::json jsonBody;
    Amazon::to_json(jsonBody, reqData);
    options.dataToPost = jsonBody.dump();

    HttpReq request("https://api.amazon.com/auth/token", &options);
    request.wait(); // Eseguiamo la richiesta in modo sincrono

    if (request.status() == HttpReq::Status::REQ_SUCCESS) {
        try {
            // La risposta qui è direttamente il token, non un oggetto complesso
            auto responseJson = nlohmann::json::parse(request.getContent());
            std::string newAccessToken = responseJson.value("access_token", "");
            
            if (!newAccessToken.empty()) {
                mAccessToken = newAccessToken;
                // Amazon potrebbe restituire un nuovo refresh token, ma di solito non lo fa.
                // Se lo facesse, dovremmo aggiornare anche mRefreshToken. Per ora, aggiorniamo solo l'access token.
                saveTokens(); // Salva i nuovi token (o almeno il nuovo access token)
                LOG(LogInfo) << "Amazon Auth: Token rinfrescato con successo.";
                return true;
            }
        } catch (const std::exception& e) {
            LOG(LogError) << "Amazon Auth: Errore parsing risposta refresh: " << e.what();
        }
    }
    
    LOG(LogError) << "Amazon Auth: Refresh del token fallito. Status: " << request.status() << " Body: " << request.getErrorMsg();
    // Se il refresh fallisce, probabilmente il refresh token è scaduto -> bisogna rifare il login
    logout();
    return false;
}

// ... (Il resto delle funzioni: logout, isAuthenticated, load/save/clearTokens rimangono invariate) ...

void AmazonAuth::logout() {
    clearTokens();
    LOG(LogInfo) << "Amazon Auth: Utente disconnesso.";
}

bool AmazonAuth::isAuthenticated() const {
    return !mAccessToken.empty();
}

std::string AmazonAuth::getAccessToken() const {
    return mAccessToken;
}

void AmazonAuth::loadTokens() {
    if (!Utils::FileSystem::exists(mTokensPath)) return;
    try {
        std::ifstream file(mTokensPath);
        nlohmann::json j;
        file >> j;
        mAccessToken = j.value("access_token", "");
        mRefreshToken = j.value("refresh_token", "");
        LOG(LogInfo) << "Amazon Auth: Token caricati da file.";
    } catch(...) {
        LOG(LogError) << "Amazon Auth: Errore nel caricamento dei token.";
        clearTokens();
    }
}

void AmazonAuth::saveTokens() {
    nlohmann::json j;
    j["access_token"] = mAccessToken;
    j["refresh_token"] = mRefreshToken;
    try {
        std::ofstream file(mTokensPath);
        file << j.dump(4);
    } catch(...) {
        LOG(LogError) << "Amazon Auth: Errore nel salvataggio dei token.";
    }
}

void AmazonAuth::clearTokens() {
    mAccessToken = "";
    mRefreshToken = "";
    if (Utils::FileSystem::exists(mTokensPath))
        Utils::FileSystem::removeFile(mTokensPath);
}
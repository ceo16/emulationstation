#include "GameStore/Xbox/XboxStoreAPI.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"

namespace nj = nlohmann;

XboxStoreAPI::XboxStoreAPI(XboxAuth* auth) : mAuth(auth) {
    if (!mAuth) {
        LOG(LogError) << "XboxStoreAPI initialized with null XboxAuth pointer!";
    }
}

std::vector<Xbox::OnlineTitleInfo> XboxStoreAPI::GetLibraryTitles() {
    std::vector<Xbox::OnlineTitleInfo> titles;
    if (!mAuth || !mAuth->isAuthenticated()) {
        LOG(LogWarning) << "XboxStoreAPI::GetLibraryTitles - Not authenticated.";
        return titles;
    }

    std::string xuid = mAuth->getXUID();
    std::string xstsToken = mAuth->getXstsToken();
    std::string userHash = mAuth->getUserHash();

    if (xuid.empty() || xstsToken.empty() || userHash.empty()) {
        LOG(LogError) << "XboxStoreAPI::GetLibraryTitles - Missing XUID, XSTS Token or UserHash.";
        return titles;
    }

    // L'API di Playnite usa "detail,image" per decoration. Verifichiamo se "image" è necessario o supportato qui.
    // Per ora usiamo solo "detail" come da documentazione più comune per titlehistory.
    std::string url = "https://titlehub.xboxlive.com/users/xuid(" + xuid + ")/titles/titlehistory/decoration/detail";
    // Se vuoi provare a ottenere anche le immagini (potrebbe non funzionare o richiedere product IDs diversi):
    // std::string url = "https://titlehub.xboxlive.com/users/xuid(" + xuid + ")/titles/titlehistory/decoration/detail,image";


    HttpReqOptions options;
    options.customHeaders.push_back("Authorization: XBL3.0 x=" + userHash + ";" + xstsToken);
    options.customHeaders.push_back("x-xbl-contract-version: 2"); // Playnite usa 2, alcune doc dicono 3 o 5
    options.customHeaders.push_back("Accept-Language: en-US"); // O la lingua dell'utente
    // options.customHeaders.push_back("Content-Type: application/json"); // Non necessario per GET

    LOG(LogDebug) << "XboxStoreAPI: Requesting Library Titles from: " << url;

    HttpReq request(url, &options);
    if (!request.wait()) {
        LOG(LogError) << "XboxStoreAPI::GetLibraryTitles - Request failed (wait): " << request.getErrorMsg();
        // Tenta un refresh se l'errore potrebbe essere dovuto a token scaduto
        if (request.status() == 401) { // Unauthorized
            LOG(LogInfo) << "XboxStoreAPI: Received 401, attempting token refresh.";
            if (mAuth->refreshTokens()) {
                return GetLibraryTitles(); // Riprova dopo il refresh
            }
        }
        return titles;
    }

    if (request.status() != 200) {
        LOG(LogError) << "XboxStoreAPI::GetLibraryTitles - API call failed. Status: " << request.status()
                      << " Body: " << request.getContent();
        return titles;
    }

    try {
        nj::json responseJson = nj::json::parse(request.getContent());
        Xbox::TitleHistoryResponse parsedResponse = Xbox::TitleHistoryResponse::fromJson(responseJson);
        titles = parsedResponse.titles;
        LOG(LogInfo) << "XboxStoreAPI::GetLibraryTitles - Successfully fetched " << titles.size() << " titles.";
    } catch (const nj::json::parse_error& e) {
        LOG(LogError) << "XboxStoreAPI::GetLibraryTitles - JSON parse error: " << e.what() << ". Response: " << request.getContent();
    } catch (const std::exception& e) {
        LOG(LogError) << "XboxStoreAPI::GetLibraryTitles - Exception: " << e.what();
    }

    return titles;
}

Xbox::OnlineTitleInfo XboxStoreAPI::GetTitleInfo(const std::string& pfn) {
    Xbox::OnlineTitleInfo titleInfo; // Ritorna un oggetto vuoto in caso di fallimento
    if (!mAuth || !mAuth->isAuthenticated() || pfn.empty()) {
        LOG(LogWarning) << "XboxStoreAPI::GetTitleInfo - Not authenticated or PFN is empty.";
        return titleInfo;
    }

    std::string xstsToken = mAuth->getXstsToken();
    std::string userHash = mAuth->getUserHash();

    if (xstsToken.empty() || userHash.empty()) {
        LOG(LogError) << "XboxStoreAPI::GetTitleInfo - Missing XSTS Token or UserHash.";
        return titleInfo;
    }

    std::string url = "https://titlehub.xboxlive.com/titles/batch/decoration/detail";

    nj::json requestBody;
    requestBody["pfns"] = nj::json::array({pfn});
    requestBody["windowsPhoneProductIds"] = nj::json::array(); // Vuoto come da esempio Playnite

    HttpReqOptions options;
    options.dataToPost = requestBody.dump();
    options.customHeaders.push_back("Authorization: XBL3.0 x=" + userHash + ";" + xstsToken);
    options.customHeaders.push_back("x-xbl-contract-version: 2");
    options.customHeaders.push_back("Accept-Language: en-US");
    options.customHeaders.push_back("Content-Type: application/json");

    LOG(LogDebug) << "XboxStoreAPI: Requesting Title Info for PFN: " << pfn << " from: " << url;

    HttpReq request(url, &options);
    if (!request.wait()) {
        LOG(LogError) << "XboxStoreAPI::GetTitleInfo - Request failed (wait): " << request.getErrorMsg();
         if (request.status() == 401) {
            if (mAuth->refreshTokens()) return GetTitleInfo(pfn);
        }
        return titleInfo;
    }

    if (request.status() != 200) {
        LOG(LogError) << "XboxStoreAPI::GetTitleInfo - API call failed. Status: " << request.status()
                      << " Body: " << request.getContent();
        return titleInfo;
    }

    try {
        nj::json responseJson = nj::json::parse(request.getContent());
        // La risposta è una TitleHistoryResponse, anche se chiediamo un solo PFN
        Xbox::TitleHistoryResponse parsedResponse = Xbox::TitleHistoryResponse::fromJson(responseJson);
        if (!parsedResponse.titles.empty()) {
            titleInfo = parsedResponse.titles[0]; // Prendiamo il primo (e unico atteso) risultato
            LOG(LogInfo) << "XboxStoreAPI::GetTitleInfo - Successfully fetched info for PFN: " << pfn << ", Name: " << titleInfo.name;
        } else {
            LOG(LogWarning) << "XboxStoreAPI::GetTitleInfo - No titles returned for PFN: " << pfn;
        }
    } catch (const nj::json::parse_error& e) {
        LOG(LogError) << "XboxStoreAPI::GetTitleInfo - JSON parse error: " << e.what() << ". Response: " << request.getContent();
    } catch (const std::exception& e) {
        LOG(LogError) << "XboxStoreAPI::GetTitleInfo - Exception: " << e.what();
    }

    return titleInfo;
}

// Implementa GetMinutesPlayed se necessario, seguendo lo schema di GetLibraryTitles
// con l'endpoint "https://userstats.xboxlive.com/batch" e il corpo JSON appropriato.
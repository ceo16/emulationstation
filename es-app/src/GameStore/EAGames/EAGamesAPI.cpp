// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesAPI.cpp
#include "EAGamesAPI.h"
#include "EAGamesAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "utils/StringUtil.h"
#include "utils/Uri.h"
#include "Settings.h"

#include <thread>

namespace EAGames
{
    const std::string GRAPHQL_API_URL = "https://service-aggregation-layer.juno.ea.com/graphql";

    EAGamesAPI::EAGamesAPI(EAGamesAuth* auth)
        : mAuth(auth) {}

    template<typename ResultType, typename ParserFunc>
    void EAGamesAPI::executeRequestThreaded(const std::string& url,
                                          const std::string& method,
                                          const std::string& postBody,
                                          const std::vector<std::string>& customHeaders,
                                          ParserFunc parser,
                                          std::function<void(ResultType result, bool success)> callback)
    {
        std::thread([url, method, postBody, customHeaders, parser, callback, this]() {
            HttpReqOptions options;
            options.customHeaders = customHeaders;
            if (method == "POST" && !postBody.empty()) {
                options.dataToPost = postBody;
            }

            HttpReq request(url, &options);

            while (request.status() == HttpReq::REQ_IN_PROGRESS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            if (request.status() == HttpReq::REQ_SUCCESS) {
                std::string responseBody = request.getContent();
                LOG(LogDebug) << "EA API Response for " << url << ": " << responseBody.substr(0, 500);
                try {
                    ResultType parsedResult = parser(responseBody);
                    if (callback) {
                        if (this->mAuth) {
                            this->mAuth->postToUiThread([callback, parsedResult] { callback(parsedResult, true); });
                        } else if (callback) {
                             callback(parsedResult, true);
                        }
                    }
                } catch (const std::exception& e) {
                    LOG(LogError) << "EA Games API: Failed to parse response for " << url << ": " << e.what();
                    if (callback) {
                        if (this->mAuth) {
                             this->mAuth->postToUiThread([callback] { callback(ResultType{}, false); });
                        } else if (callback) {
                            callback(ResultType{}, false);
                        }
                    }
                }
            } else {
                LOG(LogError) << "EA Games API: Request to " << url << " failed. Status: "
                                  << static_cast<int>(request.status())
                                  << " - Error: " << request.getErrorMsg();
                if (callback) {
                     if (this->mAuth) {
                         this->mAuth->postToUiThread([callback] { callback(ResultType{}, false); });
                     } else if (callback) {
                         callback(ResultType{}, false);
                     }
                }
            }
        }).detach();
    }

    void EAGamesAPI::getOwnedGames(std::function<void(std::vector<GameEntitlement>, bool success)> callback) {
    if (!mAuth || !mAuth->isUserLoggedIn() || mAuth->getPidId().empty()) {
        LOG(LogError) << "EA Games API: Not logged in for getOwnedGames.";
        if (callback) {
            if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
            else if (callback) callback({}, false);
        }
        return;
    }

    std::string graphql_query = R"graphql(
        query {
            me {
                ownedGameProducts(
                    locale: "DEFAULT",
                    entitlementEnabled: true,
                    storefronts: [EA],
                    type: [DIGITAL_FULL_GAME, PACKAGED_FULL_GAME, DIGITAL_EXTRA_CONTENT, PACKAGED_EXTRA_CONTENT],
                    platforms: [PC],
                    paging: { limit: 9999 }
                ) {
                    items {
                        originOfferId
                        product {
                            id
                            name
                            gameSlug
                            baseItem {
                                gameType
                            }
                            gameProductUser {
                                ownershipMethods
                                entitlementId
                            }
                        }
                    }
                }
            }
        }
    )graphql";

    std::string cleaned_graphql_query = graphql_query;
    cleaned_graphql_query.erase(std::remove(cleaned_graphql_query.begin(), cleaned_graphql_query.end(), '\n'), cleaned_graphql_query.end());
    cleaned_graphql_query.erase(std::remove(cleaned_graphql_query.begin(), cleaned_graphql_query.end(), '\r'), cleaned_graphql_query.end());

    std::string url = GRAPHQL_API_URL + "?query=" + HttpReq::urlEncode(cleaned_graphql_query);

    LOG(LogInfo) << "EA Games API: Fetching owned games from GraphQL URL: " << url;

    std::vector<std::string> headers;
    headers.push_back("Authorization: Bearer " + mAuth->getAccessToken());
    headers.push_back("Accept: application/json");
    headers.push_back("x-client-id: EAX-JUNO-CLIENT");
    headers.push_back("User-Agent: EAApp/PC/13.468.0.5981");

    executeRequestThreaded<AccountEntitlementsResponse>(url, "GET", "", headers,
        [](const std::string& responseBody) -> AccountEntitlementsResponse {
            if (responseBody.empty()) {
                LOG(LogWarning) << "EA Games API (getOwnedGames parser): Received empty response body from GraphQL ownedGameProducts.";
                return AccountEntitlementsResponse{};
            }
            LOG(LogDebug) << "EA Games API (getOwnedGames parser): Raw response for GraphQL ownedGameProducts: " << responseBody.substr(0, 2000);
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                return AccountEntitlementsResponse::fromJson(jsonResponse);
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games API (getOwnedGames parser): Exception parsing GraphQL ownedGameProducts JSON: " << e.what()
                                  << " - Body: " << responseBody.substr(0, 200);
                return AccountEntitlementsResponse{};
            }
        },
        [callback](AccountEntitlementsResponse result, bool success_param) {
            if (success_param && !result.entitlements.empty()) {
                LOG(LogInfo) << "EA Games API: Successfully fetched and parsed " << result.entitlements.size() << " game entitlements from GraphQL.";
                if (callback) callback(result.entitlements, true);
            } else if (success_param && result.entitlements.empty()) {
                LOG(LogInfo) << "EA Games API: Successfully fetched 0 game entitlements from GraphQL or parsing yielded empty result.";
                 if (callback) callback({}, true);
            }
            else {
                LOG(LogError) << "EA Games API: Failed to fetch/parse entitlements from GraphQL.";
                if (callback) callback({}, false);
            }
        }
    );
}

// Helper function per costruire la query GraphQL per i dettagli del Master Title
std::string buildJunoMasterTitleDetailsQuerySafe(const std::string& masterTitleId) {
    std::string query = Utils::String::format(
        "query {"
        "  masterTitles(masterTitleIds: [\"%s\"], locale: \"DEFAULT\") {"
        "    id,"
        "    title,"
        "    description,"
        "    developer,"
        "    publisher,"
        "    releaseDate,"
        "    genres,"
        "    offerImage: image(view: \"tile\", width: 1440, height: 810),"
        "    backgroundImage: image(view: \"superhero_standard\", width: 1920, height: 1080)"
        "  }"
        "}",
        masterTitleId.c_str()
    );
    return query;
}

std::string buildJunoOfferDetailsQuerySafe(const std::string& offerId) {
    std::string query = Utils::String::format(
        "query {"
        "  legacyOffers(offerIds: [\"%s\"], locale: \"DEFAULT\") {"
        "    offerId: id,"
        "    displayName,"
        "    contentId,"
        "    basePlatform,"
        "    primaryMasterTitleId,"
        "    displayType"
        "  },"
        "  gameProducts(offerIds: [\"%s\"], locale: \"DEFAULT\") {"
        "    items {"
        "      id,"
        "      name,"
        "      originOfferId,"
        "      gameSlug,"
        "      baseItem { title }"
        "    }"
        "  }"
        "}",
        offerId.c_str(), offerId.c_str()
    );
    return query;
}

void EAGamesAPI::getOfferStoreData(const std::string& offerId, const std::string& country, const std::string& locale,
                                   std::function<void(GameStoreData, bool success)> callback) {
     LOG(LogDebug) << "EAGamesAPI::getOfferStoreData - Entered function. OfferID: " << offerId;

    if (offerId.empty()) {
        LOG(LogError) << "EA Games API: Offer ID is empty for getOfferStoreData.";
        if (callback) {
            if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
            else if (callback) callback({}, false);
        }
        return;
    }
    LOG(LogDebug) << "EAGamesAPI::getOfferStoreData entered. mAuth pointer: " << static_cast<void*>(mAuth);
    if (!mAuth || mAuth->getAccessToken().empty()) {
        LOG(LogError) << "EA Games API: Not authenticated for getOfferStoreData. Access token is missing.";
         LOG(LogError) << "EAGamesAPI::getOfferStoreData - mAuth is NULL!";
        if (callback) {
             if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
             else if (callback) callback({}, false);
        }
        return;
    }

    std::string currentCountry = !country.empty() ? country : Settings::getInstance()->getString("ThemeRegion");
    if (currentCountry.empty()) currentCountry = "US";
    std::string currentLocaleForAcceptLang = !locale.empty() ? locale : Settings::getInstance()->getString("Language");
    if (currentLocaleForAcceptLang.empty()) currentLocaleForAcceptLang = "en_US";


    std::string graphqlQuery = EAGames::buildJunoOfferDetailsQuerySafe(offerId);
    std::string encodedQuery = HttpReq::urlEncode(graphqlQuery);
    std::string url = GRAPHQL_API_URL + "?query=" + encodedQuery;

    LOG(LogInfo) << "EA Games API: Fetching JUNO GraphQL for OfferID " << offerId << ". URL: " << url;
    LOG(LogDebug) << "EA Games API: Raw GQL Query: " << graphqlQuery;

    std::vector<std::string> headers;
    headers.push_back("Authorization: Bearer " + mAuth->getAccessToken());
    headers.push_back("x-client-id: EAX-JUNO-CLIENT");
    headers.push_back("User-Agent: EAApp/PC/13.468.0.5981");
    headers.push_back("Accept: application/json");
    headers.push_back("Accept-Language: " + currentLocaleForAcceptLang);

    executeRequestThreaded<GameStoreData>(url, "GET", "", headers,
        [offerIdCapture = offerId, cCountry = currentCountry, cLocale = currentLocaleForAcceptLang]
        (const std::string& responseBody) -> GameStoreData {
            LOG(LogDebug) << "EA Games API: Response body for OfferID " << offerIdCapture << " (JUNO GQL):\n" << responseBody.substr(0, 2000);
            if (responseBody.empty()) {
                LOG(LogError) << "EA Games API: Received empty response body for OfferID " << offerIdCapture << " (JUNO GQL).";
                return GameStoreData{};
            }
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                if (jsonResponse.contains("errors") && jsonResponse["errors"].is_array() && !jsonResponse["errors"].empty()) {
                    LOG(LogError) << "EA Games API: GraphQL errors for OfferID " << offerIdCapture << ". First error: " << jsonResponse["errors"][0].dump(2);
                    return GameStoreData{};
                }
                return GameStoreData::fromJson(jsonResponse); // Needs to be updated to handle legacyOffers response
            } catch (const nlohmann::json::parse_error& e) {
                LOG(LogError) << "EA Games API: JSON parse error for OfferID " << offerIdCapture << " (JUNO GQL): " << e.what() << ". Response: " << responseBody.substr(0, 500);
                return GameStoreData{};
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games API: General exception during parsing for OfferID " << offerIdCapture << " (JUNO GQL): " << e.what();
                return GameStoreData{};
            }
        },
        [callback, offerId](GameStoreData result, bool success_param) {
            if (success_param && (!result.title.empty() || !result.offerId.empty() || !result.masterTitleId.empty())) {
                LOG(LogInfo) << "EA Games API: Successfully processed JUNO GQL data for OfferID " << offerId << " -> Game Title: '" << result.title << "'";
            } else if (success_param) {
                 LOG(LogWarning) << "EA Games API: JUNO GQL call for OfferID " << offerId << " reported success, but parsed GameStoreData is empty or lacks essential info.";
            } else {
                LOG(LogError) << "EA Games API: Failed to fetch/process JUNO GQL data for OfferID " << offerId;
            }
            if (callback) callback(result, success_param);
        }
    );
}

void EAGamesAPI::getMasterTitleStoreData(const std::string& masterTitleId, const std::string& country, const std::string& locale,
                                         std::function<void(GameStoreData, bool success)> callback) {
    LOG(LogDebug) << "EAGamesAPI::getMasterTitleStoreData - Entered function. MasterTitleID: " << masterTitleId;

    if (masterTitleId.empty()) {
        LOG(LogError) << "EA Games API: MasterTitleID is empty for getMasterTitleStoreData.";
        if (callback) {
            if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
            else if (callback) callback({}, false);
        }
        return;
    }

    if (!mAuth || mAuth->getAccessToken().empty()) {
        LOG(LogError) << "EA Games API: Not authenticated for getMasterTitleStoreData. Access token is missing.";
        if (callback) {
            if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
            else if (callback) callback({}, false);
        }
        return;
    }

    std::string currentCountry = !country.empty() ? country : Settings::getInstance()->getString("ThemeRegion");
    if (currentCountry.empty()) currentCountry = "US";
    std::string currentLocaleForAcceptLang = !locale.empty() ? locale : Settings::getInstance()->getString("Language");
    if (currentLocaleForAcceptLang.empty()) currentLocaleForAcceptLang = "en_US";

    std::string graphqlQuery = EAGames::buildJunoMasterTitleDetailsQuerySafe(masterTitleId);
    std::string encodedQuery = HttpReq::urlEncode(graphqlQuery);
    std::string url = GRAPHQL_API_URL + "?query=" + encodedQuery;

    LOG(LogInfo) << "EA Games API: Fetching JUNO GraphQL for MasterTitleID " << masterTitleId << ". URL: " << url;
    LOG(LogDebug) << "EA Games API: Raw GQL Query (MasterTitle): " << graphqlQuery;

    std::vector<std::string> headers;
    headers.push_back("Authorization: Bearer " + mAuth->getAccessToken());
    headers.push_back("x-client-id: EAX-JUNO-CLIENT");
    headers.push_back("User-Agent: EAApp/PC/13.468.0.5981");
    headers.push_back("Accept: application/json");
    headers.push_back("Accept-Language: " + currentLocaleForAcceptLang);

    executeRequestThreaded<GameStoreData>(url, "GET", "", headers,
        [masterTitleIdCapture = masterTitleId, cCountry = currentCountry, cLocale = currentLocaleForAcceptLang]
        (const std::string& responseBody) -> GameStoreData {
            LOG(LogDebug) << "EA Games API: Response body for MasterTitleID " << masterTitleIdCapture << " (JUNO GQL):\n" << responseBody.substr(0, 2000);
            if (responseBody.empty()) {
                LOG(LogError) << "EA Games API: Received empty response body for MasterTitleID " << masterTitleIdCapture << " (JUNO GQL).";
                return GameStoreData{};
            }
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                if (jsonResponse.contains("errors") && jsonResponse["errors"].is_array() && !jsonResponse["errors"].empty()) {
                    LOG(LogError) << "EA Games API: GraphQL errors for MasterTitleID " << masterTitleIdCapture << ". First error: " << jsonResponse["errors"][0].dump(2);
                    return GameStoreData{};
                }
                // Now, GameStoreData::fromJson needs to parse the masterTitles response
                return GameStoreData::fromJson(jsonResponse); // Needs to be updated to handle masterTitles response
            } catch (const nlohmann::json::parse_error& e) {
                LOG(LogError) << "EA Games API: JSON parse error for MasterTitleID " << masterTitleIdCapture << " (JUNO GQL): " << e.what() << ". Response: " << responseBody.substr(0, 500);
                return GameStoreData{};
            } catch (const std::exception& e) {
                LOG(LogError) << "EA Games API: General exception during parsing for MasterTitleID " << masterTitleIdCapture << " (JUNO GQL): " << e.what();
                return GameStoreData{};
            }
        },
        [callback, masterTitleId](GameStoreData result, bool success_param) {
            if (success_param && (!result.title.empty() || !result.masterTitleId.empty())) {
                LOG(LogInfo) << "EA Games API: Successfully processed JUNO GQL data for MasterTitleID " << masterTitleId << " -> Game Title: '" << result.title << "'";
            } else if (success_param) {
                 LOG(LogWarning) << "EA Games API: JUNO GQL call for MasterTitleID " << masterTitleId << " reported success, but parsed GameStoreData is empty or lacks essential info.";
            } else {
                LOG(LogError) << "EA Games API: Failed to fetch/process JUNO GQL data for MasterTitleID " << masterTitleId;
            }
            if (callback) callback(result, success_param);
        }
    );
}

} // namespace EAGames
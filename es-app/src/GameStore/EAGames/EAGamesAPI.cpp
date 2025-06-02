// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesAPI.cpp
#include "EAGamesAPI.h"
#include "EAGamesAuth.h" 
#include "Log.h"
#include "HttpReq.h" 
#include "json.hpp"
#include "utils/StringUtil.h" // Inclusa nel tuo file originale
#include "utils/Uri.h"       // <--- INCLUSIONE CORRETTA E NECESSARIA!
#include "Settings.h"      // Inclusa nel tuo file originale
// #include "Window.h" // Probabilmente non serve qui se EAGamesAuth gestisce mWindow

#include <thread>

namespace EAGames
{
    // Definiamo la costante per l'URL di Origin deprecato.
    const std::string GRAPHQL_API_URL = "https://service-aggregation-layer.juno.ea.com/graphql";
    // API_EADP_GATEWAY_URL è un membro privato di EAGamesAPI (definito nel tuo EAGamesAPI.h)

    EAGamesAPI::EAGamesAPI(EAGamesAuth* auth)
        : mAuth(auth) {}

    template<typename ResultType, typename ParserFunc>
    void EAGamesAPI::executeRequestThreaded(const std::string& url,
                                          const std::string& method, // Il tuo file .h lo chiama httpMethod
                                          const std::string& postBody,
                                          const std::vector<std::string>& customHeaders,
                                          ParserFunc parser,
                                          std::function<void(ResultType result, bool success)> callback)
    {
        std::thread([url, method, postBody, customHeaders, parser, callback, this]() { 
            HttpReqOptions options;
            options.customHeaders = customHeaders;
            if (method == "POST" && !postBody.empty()) { // 'method' è il nome del parametro nella lambda
                options.dataToPost = postBody;
            }

            HttpReq request(url, &options);

            // Uso il tuo loop di attesa, assicurati che HttpReq::status() sia thread-safe
            // o che HttpReq::wait() sia implementato e usato come in EAGamesAuth.cpp
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
                             callback(parsedResult, true); // Fallback se mAuth è nullo
                        }
                    }
                } catch (const std::exception& e) {
                    LOG(LogError) << "EA Games API: Failed to parse response for " << url << ": " << e.what();
                    if (callback) {
                        if (this->mAuth) {
                             this->mAuth->postToUiThread([callback] { callback(ResultType{}, false); });
                        } else if (callback) {
                            callback(ResultType{}, false); // Fallback
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
                        callback(ResultType{}, false); // Fallback
                     }
                }
            }
        }).detach();
    }

    void EAGamesAPI::getOwnedGames(std::function<void(std::vector<GameEntitlement>, bool success)> callback) {
    if (!mAuth || !mAuth->isUserLoggedIn() || mAuth->getPidId().empty()) { // getPidId() non dovrebbe essere necessario per questa query, ma l'accessToken sì.
        LOG(LogError) << "EA Games API: Not logged in for getOwnedGames.";
        if (callback) {
            if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
            else if (callback) callback({}, false);
        }
        return;
    }

    // Query GraphQL per i giochi posseduti, basata sul codice Python
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

    // Rimuovi a capo e spazi extra dalla query GraphQL per l'URL
    std::string cleaned_graphql_query = graphql_query;
    cleaned_graphql_query.erase(std::remove(cleaned_graphql_query.begin(), cleaned_graphql_query.end(), '\n'), cleaned_graphql_query.end());
    cleaned_graphql_query.erase(std::remove(cleaned_graphql_query.begin(), cleaned_graphql_query.end(), '\r'), cleaned_graphql_query.end());
    // Potrebbe essere necessario rimuovere anche spazi multipli se HttpReq::urlEncode non li gestisce bene,
    // ma di solito l'encoding gestisce gli spazi come %20.
    // Per sicurezza, puoi sostituire sequenze di spazi con uno singolo se necessario.
    // std::regex space_re("\\s+");
    // cleaned_graphql_query = std::regex_replace(cleaned_graphql_query, space_re, " ");
    // cleaned_graphql_query = Utils::String::trim(cleaned_graphql_query); // Assumendo che Utils::String::trim esista

    std::string url = GRAPHQL_API_URL + "?query=" + HttpReq::urlEncode(cleaned_graphql_query);

    LOG(LogInfo) << "EA Games API: Fetching owned games from GraphQL URL: " << url;

    std::vector<std::string> headers;
    headers.push_back("Authorization: Bearer " + mAuth->getAccessToken());
    headers.push_back("Accept: application/json");
    headers.push_back("x-client-id: EAX-JUNO-CLIENT"); // Come per fetchUserIdentity
    headers.push_back("User-Agent: EAApp/PC/13.468.0.5981"); // Come per fetchUserIdentity

    executeRequestThreaded<AccountEntitlementsResponse>(url, "GET", "", headers,
        [](const std::string& responseBody) -> AccountEntitlementsResponse {
            if (responseBody.empty()) {
                LOG(LogWarning) << "EA Games API (getOwnedGames parser): Received empty response body from GraphQL ownedGameProducts.";
                return AccountEntitlementsResponse{};
            }
            LOG(LogDebug) << "EA Games API (getOwnedGames parser): Raw response for GraphQL ownedGameProducts: " << responseBody.substr(0, 2000); // Logga una porzione più grande
            try {
                auto jsonResponse = nlohmann::json::parse(responseBody);
                // IMPORTANTE: La struttura di AccountEntitlementsResponse e AccountEntitlementsResponse::fromJson
                // DEVE essere adattata per parsare la risposta di questa query GraphQL.
                // La risposta attesa (basata sulla query) sarà qualcosa tipo:
                // {
                //   "data": {
                //     "me": {
                //       "ownedGameProducts": {
                //         "items": [ /* array di giochi */ ]
                //       }
                //     }
                //   }
                // }
                // Quindi AccountEntitlementsResponse::fromJson dovrebbe probabilmente prendere jsonResponse["data"]["me"]["ownedGameProducts"]
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

    void EAGamesAPI::getOfferStoreData(const std::string& offerId, const std::string& country, const std::string& locale,
                                      std::function<void(GameStoreData, bool success)> callback) {
        LOG(LogWarning) << "EA Games API: getOfferStoreData utilizza un endpoint Origin (" << GRAPHQL_API_URL << ") deprecato e probabilmente fallirà (404).";
        if (offerId.empty()) {
            LOG(LogError) << "EA Games API: Offer ID is empty for getOfferStoreData.";
            if (callback) {
                if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
                else if (callback) callback({}, false);
            }
            return;
        }
        
        std::string currentCountry = !country.empty() ? country : Settings::getInstance()->getString("ThemeRegion");
		if (currentCountry.empty()) currentCountry = "US";

        std::string currentLocale = !locale.empty() ? locale : Settings::getInstance()->getString("Language");
		if (currentLocale.empty() || currentLocale.length() < 2) currentLocale = "en_US";
		else if (currentLocale.length() == 2) {
			if (currentCountry.length() == 2) currentLocale = Utils::String::toLower(currentLocale) + "_" + Utils::String::toUpper(currentCountry);
			else currentLocale = Utils::String::toLower(currentLocale) + "_" + Utils::String::toUpper(currentLocale);
		}

        std::string url = GRAPHQL_API_URL + "/ecommerce2/public/supercat/" + offerId + "/" + currentCountry + "?country=" + currentCountry;
        LOG(LogInfo) << "EA Games API: Fetching store data for OfferID " << offerId << " from " << url;

        std::vector<std::string> headers;
        headers.push_back("Accept-Language: " + currentLocale);

        executeRequestThreaded<GameStoreData>(url, "GET", "", headers,
            [currentCountry, currentLocale](const std::string& responseBody) -> GameStoreData {
                if (responseBody.empty()) return GameStoreData{};
                auto jsonResponse = nlohmann::json::parse(responseBody);
                return GameStoreData::fromJson(jsonResponse, currentCountry, currentLocale);
            },
            [callback, offerId](GameStoreData result, bool success_param) {
                if (success_param) {
                    LOG(LogInfo) << "EA Games API: Successfully fetched store data for OfferID " << offerId << " -> " << result.title;
                }
                if (callback) callback(result, success_param);
            }
        );
    }

    void EAGamesAPI::getMasterTitleStoreData(const std::string& masterTitleId, const std::string& country, const std::string& locale,
                                          std::function<void(GameStoreData, bool success)> callback) {
        LOG(LogWarning) << "EA Games API: getMasterTitleStoreData utilizza un endpoint Origin (" << GRAPHQL_API_URL << ") deprecato e probabilmente fallirà (404).";
        if (masterTitleId.empty()) {
            LOG(LogError) << "EA Games API: MasterTitleID is empty for getMasterTitleStoreData.";
            if (callback) {
                if (this->mAuth) this->mAuth->postToUiThread([callback] { callback({}, false); });
                else if (callback) callback({}, false);
            }
            return;
        }
        
		std::string currentCountry = !country.empty() ? country : Settings::getInstance()->getString("ThemeRegion");
		if (currentCountry.empty()) currentCountry = "US";
        std::string currentLocale = !locale.empty() ? locale : Settings::getInstance()->getString("Language");
		if (currentLocale.empty() || currentLocale.length() < 2) currentLocale = "en_US";
		else if (currentLocale.length() == 2) {
			if (currentCountry.length() == 2) currentLocale = Utils::String::toLower(currentLocale) + "_" + Utils::String::toUpper(currentCountry);
			else currentLocale = Utils::String::toLower(currentLocale) + "_" + Utils::String::toUpper(currentLocale);
		}

        std::string url = GRAPHQL_API_URL + "/ecommerce2/public/franchises/master-titles/" + masterTitleId + "?country=" + currentCountry + "&locale=" + currentLocale;
        LOG(LogInfo) << "EA Games API: Fetching store data for MasterTitleID " << masterTitleId << " from " << url;
        
        std::vector<std::string> headers;
        headers.push_back("Accept-Language: " + currentLocale);

        executeRequestThreaded<GameStoreData>(url, "GET", "", headers,
            [currentCountry, currentLocale](const std::string& responseBody) -> GameStoreData {
                 if (responseBody.empty()) return GameStoreData{};
                auto jsonResponse = nlohmann::json::parse(responseBody);
                return GameStoreData::fromJson(jsonResponse, currentCountry, currentLocale);
            },
            [callback, masterTitleId](GameStoreData result, bool success_param) {
                if (success_param) {
                     LOG(LogInfo) << "EA Games API: Successfully fetched master title data for " << masterTitleId << " -> " << result.title;
                }
                if (callback) callback(result, success_param);
            }
        );
    }

} // namespace EAGames
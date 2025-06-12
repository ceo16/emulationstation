// emulationstation-master/es-app/src/GameStore/EAGames/EAGamesAPI.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "GameStore/EAGames/EAGamesModels.h" // Assume che sia in namespace EAGames
#include "HttpReq.h"                         // Per class HttpReq globale

namespace EAGames {
    class EAGamesAuth;

    class EAGamesAPI
    {
    public:
        EAGamesAPI(EAGamesAuth* auth);

        void getOwnedGames(std::function<void(std::vector<GameEntitlement> games, bool success)> callback);
        void getOfferStoreData(const std::string& offerId, const std::string& country, const std::string& locale,
                               std::function<void(GameStoreData metadata, bool success)> callback);
        void getMasterTitleStoreData(const std::string& masterTitleId, const std::string& country, const std::string& locale,
                                     std::function<void(GameStoreData metadata, bool success)> callback);
									 
	  void getSubscriptions(std::function<void(SubscriptionDetails details, bool success)> callback);

        // Ottiene solo gli "slug" dei giochi in un tier di abbonamento
        void getSubscriptionGameSlugs(const std::string& tier, std::function<void(std::vector<std::string> slugs, bool success)> callback);
        
        // Ottiene i dettagli dei giochi (incluso offerId) a partire dai loro slug
        void getGamesDetailsBySlug(const std::vector<std::string>& slugs, std::function<void(std::vector<SubscriptionGame> games, bool success)> callback); 
    private:
        EAGamesAuth* mAuth;
         const std::string API_EADP_GATEWAY_URL = "https://api_gateway_pct.ea.com";
		const std::string API_JUNO_GRAPHQL_URL = "https://service-aggregation-layer.juno.ea.com/graphql";

        template<typename ResultType, typename ParserFunc>
        void executeRequestThreaded(const std::string& url,
                                    const std::string& httpMethod,
                                    const std::string& postBody,
                                    const std::vector<std::string>& customHeaders,
                                    ParserFunc parser,
                                    std::function<void(ResultType result, bool success)> callback);
    };
} // namespace EAGames
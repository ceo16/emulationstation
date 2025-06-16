// emulationstation-master/es-app/src/GameStore/IGDB/IGDBAPI.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "HttpReq.h" 
#include "GameStore/IGDB/IGDBModels.h" 

namespace IGDB {

class IGDBAPI {
public:
    IGDBAPI(const std::string& clientId, const std::string& accessToken); //

    void searchGames(const std::string& gameName,
                     std::function<void(std::vector<GameMetadata>, bool success)> callback,
                     const std::string& language = "it"); //

    void getGameDetails(const std::string& igdbGameId,
                        std::function<void(GameMetadata, bool success)> callback,
                        const std::string& language = "it"); //
	
    void getGameLogo(const std::string& igdbGameId,
                     std::function<void(std::string, bool success)> callback,
                     const std::string& language);	

private:
    std::string mClientId; //
    std::string mAccessToken; //

    // MODIFICA QUI: Aggiungi const std::string& language alla dichiarazione
    template<typename ResultType, typename ParserFunc>
    void executeRequestThreaded(const std::string& url,
                                const std::string& method,
                                const std::string& postBody,
                                const std::vector<std::string>& customHeaders,
                                const std::string& language, // <--- AGGIUNTO QUESTO PARAMETRO
                                ParserFunc parser,
                                std::function<void(ResultType result, bool success)> callback);
};

} // namespace IGDB
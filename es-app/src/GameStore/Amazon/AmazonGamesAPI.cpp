#include "AmazonGamesAPI.h"
#include "AmazonAuth.h"
#include "HttpReq.h"
#include "Log.h"
#include "json.hpp"
#include <thread>

AmazonGamesAPI::AmazonGamesAPI(AmazonAuth* auth) : mAuth(auth) {}

void AmazonGamesAPI::getOwnedGames(std::function<void(std::vector<Amazon::GameEntitlement> games, bool success)> on_complete) {
    if (!mAuth || !mAuth->isAuthenticated()) {
        if (on_complete) on_complete({}, false);
        return;
    }

    // Esegui in un thread separato
    std::thread([this, on_complete]() {
        std::vector<Amazon::GameEntitlement> allGames;
        std::string nextToken = "";
        bool requestSuccess = true;

        do {
            HttpReqOptions options;
            options.customHeaders.push_back("User-Agent: AGSLauncher/1.0.0");
            options.customHeaders.push_back("Content-Type: application/json");
            options.customHeaders.push_back("Authorization: bearer " + mAuth->getAccessToken());

            Amazon::EntitlementsRequest reqData;
            reqData.nextToken = nextToken;
            options.dataToPost = nlohmann::json(reqData).dump();

            HttpReq request("https://gaming.amazon.com/api/distribution/entitlements", &options);
            request.wait();

            if (request.status() == HttpReq::Status::REQ_SUCCESS) {
                try {
                    auto response = nlohmann::json::parse(request.getContent()).get<Amazon::EntitlementsResponse>();
                    allGames.insert(allGames.end(), response.entitlements.begin(), response.entitlements.end());
                    nextToken = response.nextToken;
                } catch (const std::exception& e) {
                    LOG(LogError) << "Amazon API: Errore parsing risposta giochi: " << e.what();
                    requestSuccess = false;
                    nextToken = "";
                }
            } else {
                LOG(LogError) << "Amazon API: Richiesta giochi fallita. Status: " << request.status();
                // Potrebbe essere necessario un refresh del token qui
                requestSuccess = false;
                nextToken = "";
            }
        } while (!nextToken.empty() && requestSuccess);

        if (on_complete) on_complete(allGames, requestSuccess);

    }).detach();
}
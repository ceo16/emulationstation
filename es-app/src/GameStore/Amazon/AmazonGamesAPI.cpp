#include "AmazonGamesAPI.h"
#include "AmazonAuth.h"
#include "GameStore/Amazon/AmazonGamesHelper.h" // Per il GUID
#include "Log.h"
#include "Window.h"
#include "json.hpp"
#include <thread>
#include <curl/curl.h>

// Funzione di callback per cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AmazonGamesAPI::AmazonGamesAPI(Window* window, AmazonAuth* auth) : mWindow(window), mAuth(auth) {}

void AmazonGamesAPI::getOwnedGames(std::function<void(std::vector<Amazon::GameEntitlement> games, bool success)> on_complete) {
    if (!mAuth || !mAuth->isAuthenticated()) {
        if (on_complete) on_complete({}, false);
        return;
    }

    std::thread([this, on_complete]() {
        std::vector<Amazon::GameEntitlement> allGames;
        std::string nextToken = "";
        bool requestSuccess = true;

        CURL* curl = curl_easy_init();
        if (!curl) {
            if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete({}, false); });
            return;
        }

        do {
            std::string readBuffer;
            long http_code = 0;

            // --- MODIFICA CHIAVE: USA IL NUOVO MODELLO CORRETTO ---
            Amazon::EntitlementsRequest reqData;
            reqData.nextToken = nextToken;
            reqData.hardwareHash = Amazon::Helper::getMachineGuidNoHyphens();
            std::string jsonString = nlohmann::json(reqData).dump();

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "User-Agent: com.amazon.agslauncher.win/3.0.9124.0");
            headers = curl_slist_append(headers, "X-Amz-Target: com.amazon.animusdistributionservice.entitlement.AnimusEntitlementsService.GetEntitlements");
            headers = curl_slist_append(headers, ("x-amzn-token: " + mAuth->getAccessToken()).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Expect: 100-continue");
            headers = curl_slist_append(headers, "Content-Encoding: amz-1.0");

            curl_easy_setopt(curl, CURLOPT_URL, "https://gaming.amazon.com/api/distribution/entitlements");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            CURLcode res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (res == CURLE_OK && http_code == 200) {
                try {
                    auto response = nlohmann::json::parse(readBuffer).get<Amazon::EntitlementsResponse>();
                    if (!response.entitlements.empty()) {
                        allGames.insert(allGames.end(), response.entitlements.begin(), response.entitlements.end());
                    }
                    nextToken = response.nextToken;
                } catch (const std::exception& e) {
                    LOG(LogError) << "Amazon API: Errore parsing risposta giochi: " << e.what();
                    requestSuccess = false;
                    nextToken = "";
                }
            } else {
                LOG(LogError) << "Amazon API: Richiesta giochi fallita. Status: " << http_code << " cURL Error: " << curl_easy_strerror(res);
                requestSuccess = false;
                nextToken = "";
            }

            curl_slist_free_all(headers);

        } while (!nextToken.empty() && requestSuccess);
        
        curl_easy_cleanup(curl);
        
        mWindow->postToUiThread([on_complete, allGames, requestSuccess] {
             if (on_complete) on_complete(allGames, requestSuccess);
        });

    }).detach();
}
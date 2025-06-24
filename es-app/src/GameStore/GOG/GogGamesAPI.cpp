#include "GogGamesAPI.h"
#include "GogAuth.h"
#include "Log.h"
#include "HttpReq.h"
#include "Window.h"
#include <thread>

GogGamesAPI::GogGamesAPI(Window* window, GogAuth* auth) : mWindow(window), mAuth(auth) {}

void GogGamesAPI::getOwnedGames(std::function<void(std::vector<GOG::LibraryGame> games, bool success)> on_complete)
{
    if (!mAuth || !mAuth->isAuthenticated()) {
        LOG(LogError) << "[GOG API] Tentativo di ottenere i giochi senza autenticazione.";
        if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete({}, false); });
        return;
    }

    // Esegui in un thread per non bloccare la UI
    std::thread([this, on_complete]() {
        GOG::AccountInfo accountInfo = mAuth->getAccountInfo();
        if (accountInfo.username.empty()) {
            LOG(LogError) << "[GOG API] Impossibile ottenere il nome utente per la richiesta.";
            if (on_complete) mWindow->postToUiThread([on_complete]{ on_complete({}, false); });
            return;
        }

        std::vector<GOG::LibraryGame> allGames;
        bool requestSuccess = true;
        int currentPage = 1;
        int totalPages = 1;

        LOG(LogInfo) << "[GOG API] Avvio download libreria per l'utente: " << accountInfo.username;

        do {
            std::string url = "https://www.gog.com/u/" + accountInfo.username + "/games/stats?sort=recent_playtime&order=desc&page=" + std::to_string(currentPage);
            
            HttpReq req(url);
            req.wait();

            if (req.status() == HttpReq::Status::REQ_SUCCESS) {
                try {
                    auto response = nlohmann::json::parse(req.getContent()).get<GOG::LibraryGameResponse>();
                    if (!response._embedded.items.empty()) {
                        allGames.insert(allGames.end(), response._embedded.items.begin(), response._embedded.items.end());
                    }
                    totalPages = response.pages;
                    currentPage++;
                } catch (const std::exception& e) {
                    LOG(LogError) << "[GOG API] Errore parsing risposta giochi (pagina " << currentPage << "): " << e.what();
                    requestSuccess = false;
                }
            } else {
                LOG(LogError) << "[GOG API] Richiesta giochi fallita (pagina " << currentPage << "). Status: " << req.status();
                requestSuccess = false;
            }

        } while (currentPage <= totalPages && requestSuccess);

        if (requestSuccess) {
            LOG(LogInfo) << "[GOG API] Download completato. Trovati " << allGames.size() << " giochi.";
        }
        
        // Passa il risultato finale al thread principale
        mWindow->postToUiThread([on_complete, allGames, requestSuccess] {
             if (on_complete) on_complete(allGames, requestSuccess);
        });

    }).detach();
}
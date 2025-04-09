#ifndef EPICGAMESSTORE_H
#define EPICGAMESSTORE_H

#include "GameStore.h"
#include <vector>
#include <string>

class EpicGamesStore : public GameStore {
public:
    EpicGamesStore();
    ~EpicGamesStore() override;

    bool init(Window* window) override;
    void showStoreUI(Window* window) override;
    std::string getStoreName() const override;
    void shutdown() override;

    std::vector<std::string> getGamesList() override;
    bool installGame(const std::string& gameId) override;
    bool uninstallGame(const std::string& gameId) override;
    bool updateGame(const std::string& gameId) override;
};

#endif
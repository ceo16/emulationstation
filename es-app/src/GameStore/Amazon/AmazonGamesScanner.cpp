#include "AmazonGamesScanner.h"
#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "sqlite3.h" // Assicurati di avere questo header e di linkare la libreria

AmazonGamesScanner::AmazonGamesScanner() {}

static int sqlite_callback(void* data, int argc, char** argv, char** azColName) {
    auto* games = static_cast<std::vector<Amazon::InstalledGameInfo>*>(data);
    Amazon::InstalledGameInfo game;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "Id") == 0) {
            game.id = argv[i] ? argv[i] : "";
        } else if (strcmp(azColName[i], "ProductTitle") == 0) {
            game.title = argv[i] ? argv[i] : "";
        } else if (strcmp(azColName[i], "InstallDirectory") == 0) {
            game.installDirectory = argv[i] ? argv[i] : "";
        }
    }
    if (!game.id.empty() && !game.installDirectory.empty()) {
        games->push_back(game);
    }
    return 0;
}

std::vector<Amazon::InstalledGameInfo> AmazonGamesScanner::findInstalledGames() {
    std::vector<Amazon::InstalledGameInfo> installedGames;

    std::string dbPath = Utils::FileSystem::getGenericPath(getenv("LOCALAPPDATA"));
    if (dbPath.empty()) {
        LOG(LogError) << "Amazon Scanner: Impossibile trovare la cartella LOCALAPPDATA.";
        return installedGames;
    }
    dbPath += "/Amazon Games/Data/Games/Sql/GameInstallInfo.sqlite";

    if (!Utils::FileSystem::exists(dbPath)) {
        LOG(LogInfo) << "Amazon Scanner: Database di installazione non trovato. Nessun gioco installato rilevato.";
        return installedGames;
    }

    sqlite3* db;
    if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        LOG(LogError) << "Amazon Scanner: Impossibile aprire il database SQLite: " << sqlite3_errmsg(db);
        return installedGames;
    }

    char* zErrMsg = nullptr;
    const char* sql = "SELECT Id, ProductTitle, InstallDirectory FROM DbSet WHERE ProductTitle IS NOT NULL AND InstallDirectory IS NOT NULL;";

    if (sqlite3_exec(db, sql, sqlite_callback, &installedGames, &zErrMsg) != SQLITE_OK) {
        LOG(LogError) << "Amazon Scanner: Errore query SQL: " << zErrMsg;
        sqlite3_free(zErrMsg);
    } else {
        LOG(LogInfo) << "Amazon Scanner: Trovati " << installedGames.size() << " giochi installati.";
    }

    sqlite3_close(db);
    return installedGames;
}
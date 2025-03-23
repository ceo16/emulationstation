#include "EpicGamesParser.h"
#include "FileData.h"
#include <vector>
#include <string>
#include "json.hpp" // Or your JSON library
#include "EpicGamesStore/EpicGamesStoreAPI.h"
#include <iostream>

using json = nlohmann::json; // If using nlohmann/json

std::vector<FileData*> parseEpicGamesList(const std::string& gamesList, SystemData* system) {
    std::vector<FileData*> games;

    try {
        // 1. Parse the JSON string
        json game_data = json::parse(gamesList);

        // 2. Iterate through the parsed data (assuming it's a JSON array)
        for (auto& game : game_data) {
            std::string title = game["title"]; // Adjust keys based on your JSON structure
            std::string path = game["install_dir"]; //  Adjust keys

            // 3. Create FileData object
            FileData* file_data = new FileData(GAME, path, system->getRootFolder()); //  Adjust parent as needed
            file_data->getMetadata().set(MetaDataId::Name, title);

            games.push_back(file_data);

            std::cout << "Added game: " << title << " from path: " << path << std::endl; // Debugging
        }
    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        // Handle the error appropriately (e.g., log it, return an empty vector, etc.)
    } catch (const std::exception& e) {
        std::cerr << "Error processing game data: " << e.what() << std::endl;
        // Handle other exceptions
    }

    return games;
}

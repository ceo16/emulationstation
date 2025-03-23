#include "EpicGamesParser.h"
#include "FileData.h"
#include <vector>
#include <string>
#include "src/json.hpp"
#include "EpicGamesStore/EpicGamesStoreAPI.h"
#include <iostream>

using json = nlohmann::json;

std::vector<FileData*> parseEpicGamesList(const std::string& gamesList, SystemData* system) {
    std::vector<FileData*> games;

    try {
        // 1. Parse the JSON string
        json games_data = json::parse(gamesList);

        // 2. Iterate through the parsed data
        if (games_data.is_array()) {
            for (auto& game : games_data) {
                std::string title;
                std::string path;

                //  ADAPT THESE KEYS BASED ON YOUR JSON
                if (game.contains("title")) {
                    title = game["title"].get<std::string>();
                } else {
                    title = "Unknown Title";
                }

                if (game.contains("install_dir")) {
                    path = game["install_dir"].get<std::string>();
                } else {
                    path = "/path/not/found";
                }

                // 3. Create FileData object
                FileData* file_data = new FileData(GAME, path, system->getRootFolder());
                file_data->getMetadata().set(MetaDataId::Name, title);

                //  Set other metadata if available (ADAPT AS NEEDED)
                //  Example:
                //  if (game.contains("description")) {
                //      file_data->getMetadata().set(MetaDataId::Description, game["description"].get<std::string>());
                //  }

                games.push_back(file_data);

                std::cout << "Added game: " << title << " from path: " << path << std::endl;  // Debugging
            }
        } else {
            std::cerr << "Error: JSON data is not an array." << std::endl;
            //  Handle the case where the JSON is not in the expected format
        }
    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        //  Consider logging this error to EmulationStation's log
    } catch (const std::exception& e) {
        std::cerr << "Error processing game data: " << e.what() << std::endl;
        //  Consider logging this error to EmulationStation's log
    }

    return games;
}

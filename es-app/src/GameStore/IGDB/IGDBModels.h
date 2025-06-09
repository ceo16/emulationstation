// emulationstation-master/es-app/src/GameStore/IGDB/IGDBModels.h
#pragma once

#include <string>
#include <vector>
#include <json.hpp> // Per il parsing JSON

namespace IGDB {

// Struttura per contenere i metadati di un gioco da IGDB
struct GameMetadata {
    std::string id;
    std::string name;
    std::string summary;
    std::string storyline;
    std::string releaseDate;
    std::vector<std::string> genres;
    std::vector<std::string> developers;
    std::vector<std::string> publishers;
    
    // ID delle immagini invece degli URL diretti per costruire URL con dimensioni specifiche
    std::string coverImageId;       // ID immagine della copertina
    std::string screenshotImageId;  // ID immagine del primo screenshot
    std::string fanartImageId;      // ID immagine per la fanart (da artworks)
    
    std::string videoUrl;           // URL del video (link YouTube)
    std::string aggregatedRating;
	std::vector<std::string> gameModes;

    // Deprecati, si usano gli ID per costruire gli URL dinamicamente
    // std::string coverUrl;        
    // std::string screenshotUrl;   

    static GameMetadata fromJson(const nlohmann::json& json);
};

} // namespace IGDB
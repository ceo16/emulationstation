// emulationstation-master/es-app/src/GameStore/IGDB/IGDBModels.cpp
#include "GameStore/IGDB/IGDBModels.h"
#include "Log.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

namespace IGDB {

GameMetadata GameMetadata::fromJson(const nlohmann::json& json) {
    GameMetadata data;

    if (json.contains("id") && json["id"].is_number()) {
        data.id = std::to_string(json.value("id", 0));
    } else {
        LOG(LogWarning) << "IGDB::GameMetadata::fromJson - ID del gioco mancante o non numerico.";
    }

    data.name = json.value("name", "");
    data.summary = json.value("summary", "");
    data.storyline = json.value("storyline", "");

    if (json.contains("first_release_date") && json["first_release_date"].is_number()) {
        data.releaseDate = std::to_string(json.value("first_release_date", 0LL));
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Data di rilascio non trovata o non numerica per " << data.name;
    }

    if (json.contains("genres") && json["genres"].is_array()) {
        for (const auto& genreJson : json["genres"]) {
            if (genreJson.contains("name") && genreJson["name"].is_string()) {
                data.genres.push_back(genreJson.value("name", ""));
            }
        }
    }
	
	   if (json.contains("game_modes") && json["game_modes"].is_array()) {
        for (const auto& mode : json["game_modes"]) {
            if (mode.contains("name") && mode["name"].is_string()) {
                data.gameModes.push_back(mode["name"]);
            }
        }
    }

    if (json.contains("involved_companies") && json["involved_companies"].is_array()) {
        for (const auto& companyJson : json["involved_companies"]) {
            if (companyJson.contains("company") && companyJson["company"].is_object() && 
                companyJson["company"].contains("name") && companyJson["company"]["name"].is_string()) {
                std::string companyName = companyJson["company"].value("name", "");
                if (companyJson.value("developer", false)) {
                    data.developers.push_back(companyName);
                }
                if (companyJson.value("publisher", false)) {
                    data.publishers.push_back(companyName);
                }
            }
        }
    }

    // Parsa l'ID immagine della copertina
    if (json.contains("cover") && json["cover"].is_object() && 
        json["cover"].contains("image_id") && json["cover"]["image_id"].is_string()) {
        data.coverImageId = json["cover"].value("image_id", "");
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Cover image_id non trovato per " << data.name;
    }

    // Parsa l'ID immagine del primo screenshot
 if (json.contains("screenshots") && json.at("screenshots").is_array() && !json.at("screenshots").empty()) { // Usare .at() è leggermente più sicuro qui dopo .contains()
    const auto& firstScreenshotElement = json.at("screenshots").front(); // O json.at("screenshots")[0]
    if (firstScreenshotElement.is_object()) { // <-- AGGIUNGI QUESTO CONTROLLO
        if (firstScreenshotElement.contains("image_id") && firstScreenshotElement.at("image_id").is_string()) {
            data.screenshotImageId = firstScreenshotElement.value("image_id", "");
        } else {
            LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Screenshot image_id non trovato o non stringa nel primo screenshot per " << data.name;
        }
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Il primo elemento dell'array screenshots non è un oggetto per " << data.name;
    }
} else {
    LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Screenshot array non trovato o vuoto per " << data.name;
}

    // Parsa l'ID immagine della fanart (primo artwork disponibile)
if (json.contains("artworks") && json.at("artworks").is_array() && !json.at("artworks").empty()) {
    const auto& firstArtworkElement = json.at("artworks").front();
    if (firstArtworkElement.is_object()) { // <-- AGGIUNGI QUESTO CONTROLLO
        if (firstArtworkElement.contains("image_id") && firstArtworkElement.at("image_id").is_string()) {
            data.fanartImageId = firstArtworkElement.value("image_id", "");
        } else {
            LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Artwork image_id non trovato o non stringa nel primo artwork per " << data.name;
        }
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Il primo elemento dell'array artworks non è un oggetto per " << data.name;
    }
} else {
    LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Artworks non trovati o array vuoto per " << data.name;
}
    // Parsa il primo video (trailer) e costruisci l'URL di YouTube
    if (json.contains("videos") && json["videos"].is_array() && !json["videos"].empty()) {
        const auto& videoJson = json["videos"][0];
        if (videoJson.contains("video_id") && videoJson["video_id"].is_string()) {
            std::string videoId = videoJson.value("video_id", "");
            if (!videoId.empty()) {
                data.videoUrl = "https://www.youtube.com/watch?v=" + videoId;
            }
        }
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Video non trovato per " << data.name;
    }

    if (json.contains("aggregated_rating") && json["aggregated_rating"].is_number()) {
        data.aggregatedRating = std::to_string(json.value("aggregated_rating", 0.0));
    } else {
        LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Voto aggregato non trovato per " << data.name;
    }

    LOG(LogDebug) << "IGDB::GameMetadata::fromJson - Parsed: " << data.name
                  << " (ID: " << data.id << ")"
                  << ", Summary Length: " << data.summary.length()
                  << ", Release Date: " << data.releaseDate
                  << ", Genres: " << Utils::String::vectorToCommaString(data.genres)
                  << ", Devs: " << Utils::String::vectorToCommaString(data.developers)
                  << ", Pubs: " << Utils::String::vectorToCommaString(data.publishers)
                  << ", CoverImgID: " << data.coverImageId
                  << ", ScreenshotImgID: " << data.screenshotImageId
                  << ", FanartImgID: " << data.fanartImageId
                  << ", Video: " << data.videoUrl
                  << ", Rating: " << data.aggregatedRating;

    return data;
}

} // namespace IGDB
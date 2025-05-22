#ifndef ES_APP_GAMESTORE_XBOX_MODELS_H
#define ES_APP_GAMESTORE_XBOX_MODELS_H

#include <string>
#include <vector>
#include <json.hpp> // Assicurati che sia accessibile

namespace Xbox
{
	

    // Ispirato da PlayniteExtensions-master/source/Libraries/XboxLibrary/Models/AuthenticationData.cs
    // TitleHistoryResponse::Detail
    struct TitleDetail {
        std::string description;
        std::string publisherName;
        std::string developerName;
        std::string releaseDate; // Formato ISO 8601 o stringa testuale
        int minAge = 0;
        std::string productId;   // <--- NUOVO CAMPO AGGIUNTO per lo Store ID alfanumerico (ex: 9P2N57MC619K)

        static TitleDetail fromJson(const nlohmann::json& j);
    };

    // TitleHistoryResponse::TitleHistory
    struct TitleLastPlayedHistory {
        std::string lastTimePlayed; // Formato ISO 8601
        // Altri campi se utili

        static TitleLastPlayedHistory fromJson(const nlohmann::json& j);
    };

    // TitleHistoryResponse::Title
    struct OnlineTitleInfo {
        std::string titleId;        // ID numerico del titolo
        std::string pfn;            // Package Family Name (per giochi PC)
        std::string type;           // Es. "Game"
        std::string name;
        std::string windowsPhoneProductId; // Spesso vuoto
        std::string modernTitleId;  // Altro ID
        std::string mediaItemType;  // Es. "DGame" (Digital Game)
        TitleDetail detail;
        std::vector<std::string> devices; // Es. ["PC", "XboxOne"]
        TitleLastPlayedHistory titleHistory;
        std::string minutesPlayed;  // Come stringa, da convertire
        // Campi per immagini (se l'API li fornisce direttamente, altrimenti andranno recuperati separatamente)
        // std::vector<ApiImage> keyImages; // Esempio

        static OnlineTitleInfo fromJson(const nlohmann::json& j);
    };

    struct TitleHistoryResponse {
        std::string xuid;
        std::vector<OnlineTitleInfo> titles;

        static TitleHistoryResponse fromJson(const nlohmann::json& j);
    };

    // Per le risposte di GetUserStatsMinutesPlayed
    struct UserStat {
        std::string titleid;
        std::string value; // minutes played
    };

    // Struttura per i giochi installati localmente
    struct InstalledXboxGameInfo {
        std::string pfn;            // Package Family Name
        std::string displayName;
        std::string installLocation;
        std::string iconPath;       // Percorso dell'icona, se recuperabile
        std::string applicationId;        // ID dell'applicazione specifica all'interno del pacchetto
        std::string aumid;                // PackageFamilyName!ApplicationId (Application User Model ID)
        std::string installPath;          // Percorso di installazione del pacchetto
        bool        isInstalled;          // Flag per indicare se è attualmente rilevato come installato
        std::string logoPathOnDisk;       // Percorso locale se il logo viene salvato (opzionale per ora)
        std::string applicationDisplayName; // Nome dell'applicazione specifica (se diverso dal pacchetto)
		std::string packageFamilyName;    // Il tuo campo "pfn" esistente
        std::string packageFullName;      // Mantieni
        // --- FINE CAMPI AGGIUNTI ---
    };

} // namespace Xbox

#endif // ES_APP_GAMESTORE_XBOX_MODELS_H
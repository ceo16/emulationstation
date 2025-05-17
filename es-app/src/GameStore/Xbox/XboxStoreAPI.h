#ifndef ES_APP_GAMESTORE_XBOX_STORE_API_H
#define ES_APP_GAMESTORE_XBOX_STORE_API_H

#include <string>
#include <vector>
#include <map>
#include "XboxAuth.h" // Per accedere a XUID, token, ecc.
#include "XboxModels.h" // Per le strutture di ritorno

class XboxStoreAPI
{
public:
    XboxStoreAPI(XboxAuth* auth);

    // Recupera la libreria di titoli dell'utente
    std::vector<Xbox::OnlineTitleInfo> GetLibraryTitles();

    // Recupera dettagli per un PFN specifico (utile per giochi installati non in cronologia)
    Xbox::OnlineTitleInfo GetTitleInfo(const std::string& pfn);

    // (Opzionale) Recupera statistiche di gioco come il tempo di gioco
    // std::map<std::string, uint64_t> GetMinutesPlayed(const std::vector<std::string>& titleIds);

private:
    XboxAuth* mAuth; // Non posseduto, fornito da XboxStore
};

#endif // ES_APP_GAMESTORE_XBOX_STORE_API_H
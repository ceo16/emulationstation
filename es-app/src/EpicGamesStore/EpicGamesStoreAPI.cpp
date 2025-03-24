#include "EpicGamesStoreAPI.h"
#include <iostream>
#include <curl/curl.h>
#include "json.hpp" //  Include nlohmann/json

using json = nlohmann::json;

EpicGamesStoreAPI::EpicGamesStoreAPI() : curlHandle(nullptr) {}

EpicGamesStoreAPI::~EpicGamesStoreAPI() {
    shutdown();
}

bool EpicGamesStoreAPI::initialize() {
    // Initialize libcurl
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        std::cerr << "Error initializing libcurl" << std::endl;
        return false;
    }
    curlHandle = curl_easy_init(); // Get a curl handle
    if (!curlHandle) {
        std::cerr << "Error getting curl handle" << std::endl;
        return false;
    }
    return true;
}

// Helper function to perform HTTP requests

Ok, è fantastico che modificare il codice in quel modo gli abbia permesso di compilare! Questo ci dà un indizio su cosa si aspettava il compilatore.

Analizziamo perché questa modifica ha funzionato e cosa ci dice sul modo corretto di utilizzare la curl_easy_setoptfunzione.

Comprendere il cambiamento

Ecco un confronto tra il codice originale e quello funzionante:

Originale (potenzialmente problematico):

Il codice C++

curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
   (char* contents, size_t size_t, size_t nmemb, std::string* output) -> size_t {
        size_t total_size = size_t * nmemb;
        output->append(contents, total_size);
        return total_size;
    });
Codice funzionante:

Il codice C++

curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
   (char* contents, size_t size_t, size_t nmemb, void* userdata) -> size_t {
        std::string* output = static_cast<std::string*>(userdata); // Cast userdata
        size_t total_size = size_t * nmemb;
        output->append(contents, total_size);
        return total_size;
    });
Differenze chiave e perché sono importanti

void* userdata:

Il codice originale utilizzato std::string* outputcome quarto parametro della funzione lambda.
Il codice funzionante utilizza void* userdata.
Spiegazione: curl_easy_setopt with CURLOPT_WRITEFUNCTIONsi aspetta che la funzione di callback abbia una firma specifica. Il quarto parametro è un void*puntatore. Questo puntatore viene utilizzato per passare i dati alla funzione di callback. È un puntatore generico, quindi può puntare a qualsiasi tipo di dati.
Casting userdata:

Il codice funzionante aggiunge la riga:
Il codice C++

std::string* output = static_cast<std::string*>(userdata); // Cast userdata
Spiegazione: all'interno del callback, devi usare i dati passati tramite userdata. Poiché è un void*, devi convertirlo al tipo corretto prima di poterlo usare. In questo caso, lo stai convertendo a std::string*perché intendi usarlo come puntatore a una stringa.
Perché il codice originale ha fallito

Probabilmente il compilatore si lamentava perché la firma della lambda non corrispondeva a quanto curl_easy_setoptprevisto. curl_easy_setoptrichiede una funzione di callback con un set specifico di parametri e il codice originale si discostava da questo nel tipo del quarto parametro.

Uso corretto diCURLOPT_WRITEDATA

Per usare il userdataparametro in modo efficace, devi anche usare CURLOPT_WRITEDATA. Questa opzione dice a libcurl quali dati passare alla tua funzione di callback.

Nella tua performRequestfunzione, prima di impostare CURLOPT_WRITEFUNCTION, dovresti aggiungere:

Il codice C++

curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response_string);
Questa riga indica a libcurl di passare l' indirizzo del tuo response_stringoggetto alla funzione di callback tramite il userdataparametro.

performRequestFunzione corretta completa

Il codice C++

std::string EpicGamesStoreAPI::performRequest(const std::string& url) {
    std::string response_string;
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());

    // Set the write data to our response_string
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response_string);

    // Set the write function
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
       (char* contents, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* output = static_cast<std::string*>(userdata); // Cast userdata
            size_t total_size = size * nmemb;
            output->append(contents, total_size);
            return total_size;
        });

    CURLcode res = curl_easy_perform(curlHandle);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return ""; // Or handle the error as appropriate
    }
    return response_string;
}

std::string EpicGamesStoreAPI::getGamesList() {
    // Placeholder for getting games list from Epic API
    // Adapt this using the Playnite plugin as a guide

    // Example (Conceptual - Replace with actual API calls):
    // std::string url = "https://example.com/epic/games"; //  Replace with the correct API URL
    // std::string response = performRequest(url);
    // if (response.empty()) {
    //     return ""; //  Return an empty JSON array on error
    // }
    // return response;

    // For now, let's return a placeholder (for testing)
    return "[{\"title\": \"Placeholder Game 1\", \"install_dir\": \"/path/1\"}, {\"title\": \"Placeholder Game 2\", \"install_dir\": \"/path/2\"}]";
}

void EpicGamesStoreAPI::shutdown() {
    if (curlHandle) {
        curl_easy_cleanup(curlHandle); // Clean up the curl handle
        curlHandle = nullptr;
    }
    curl_global_cleanup(); // Clean up libcurl
}

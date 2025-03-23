#include "EpicGamesStoreAPI.h"
#include <iostream>
#include <curl/curl.h> // Include libcurl
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
std::string EpicGamesStoreAPI::performRequest(const std::string& url) {
    std::string response_string;
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION,
       (char* contents, size_t size, size_t nmemb, std::string* output) -> size_t {
            size_t total_size = size * nmemb;
            output->append(contents, total_size);
            return total_size;
        });
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response_string);
    
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

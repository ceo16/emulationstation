#include "EpicGamesStoreAPI.h"
#include <iostream>
#include <curl/curl.h> // Include libcurl

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

std::string EpicGamesStoreAPI::getGamesList() {
    // In this initial stage, let's just return a placeholder
    return "[{\"title\": \"Placeholder Game 1\"}, {\"title\": \"Placeholder Game 2\"}]";
}

void EpicGamesStoreAPI::shutdown() {
    if (curlHandle) {
        curl_easy_cleanup(curlHandle); // Clean up the curl handle
        curlHandle = nullptr;
    }
    curl_global_cleanup(); // Clean up libcurl
}

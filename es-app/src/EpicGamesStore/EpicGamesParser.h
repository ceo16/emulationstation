#ifndef EMULATIONSTATION_EPICGAMESPARSER_H
#define EMULATIONSTATION_EPICGAMESPARSER_H

#include <vector>
#include <string>

class SystemData;  // Forward declaration (avoids circular dependency)
class FileData;    // Forward declaration

std::vector<FileData*> parseEpicGamesList(const std::string& gamesList, SystemData* system);

#endif // EMULATIONSTATION_EPICGAMESPARSER_H

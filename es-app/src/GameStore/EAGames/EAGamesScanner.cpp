#include "EAGamesScanner.h"
#include "Log.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"
#include "pugixml.hpp" // <-- INCLUDO MANCANTE FONDAMENTALE
#include <Windows.h>

std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

namespace EAGames
{
    EAGamesScanner::EAGamesScanner() {}

    std::vector<InstalledGameInfo> EAGamesScanner::scanForInstalledGames() {
        LOG(LogInfo) << "[EAGamesScanner] Starting final scan for installed EA games...";
        std::vector<InstalledGameInfo> installedGames;
    #ifdef _WIN32
        scanUninstallKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_WOW64_64KEY, installedGames);
        scanUninstallKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, installedGames);
    #endif
        LOG(LogInfo) << "[EAGamesScanner] Scan finished. Found " << installedGames.size() << " EA games.";
        return installedGames;
    }

#ifdef _WIN32
    void EAGamesScanner::scanUninstallKey(HKEY rootKey, const WCHAR* keyPath, REGSAM accessFlags, std::vector<InstalledGameInfo>& games) {
        HKEY hKey;
        if (RegOpenKeyExW(rootKey, keyPath, 0, KEY_READ | accessFlags, &hKey) != ERROR_SUCCESS) return;
        for (DWORD index = 0; ; ++index) {
            WCHAR subKeyNameW[256];
            DWORD subKeyNameSize = 256;
            if (RegEnumKeyExW(hKey, index, subKeyNameW, &subKeyNameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
            HKEY hSubKey;
            if (RegOpenKeyExW(hKey, subKeyNameW, 0, KEY_READ | accessFlags, &hSubKey) == ERROR_SUCCESS) {
                if (isPublisherEA(hSubKey)) {
                    auto gameInfoOpt = getGameInfoFromRegistry(hSubKey);
                    if (gameInfoOpt.has_value()) {
                        games.push_back(gameInfoOpt.value());
                    }
                }
                RegCloseKey(hSubKey);
            }
        }
        RegCloseKey(hKey);
    }

    bool EAGamesScanner::isPublisherEA(HKEY hGameKey) {
        WCHAR publisherW[256];
        DWORD publisherSize = sizeof(publisherW);
        if (RegQueryValueExW(hGameKey, L"Publisher", NULL, NULL, (LPBYTE)publisherW, &publisherSize) == ERROR_SUCCESS) {
            return (Utils::String::toLower(wstring_to_utf8(publisherW)).find("electronic arts") != std::string::npos);
        }
        return false;
    }

std::optional<InstalledGameInfo> EAGamesScanner::getGameInfoFromRegistry(HKEY hGameKey)
{
    InstalledGameInfo gameInfo;
    WCHAR buffer[2048];
    DWORD bufferSize = sizeof(buffer);

    // Estrazione di nome, percorso e icona (questa parte è già corretta)
    if (RegQueryValueExW(hGameKey, L"DisplayName", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
        gameInfo.name = wstring_to_utf8(buffer);
    } else { return std::nullopt; }

    if (gameInfo.name == "EA app") { return std::nullopt; }

    bufferSize = sizeof(buffer);
    if (RegQueryValueExW(hGameKey, L"InstallLocation", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
        gameInfo.installPath = wstring_to_utf8(buffer);
    }

    bufferSize = sizeof(buffer);
    if (RegQueryValueExW(hGameKey, L"DisplayIcon", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
        std::string iconPath = wstring_to_utf8(buffer);
        gameInfo.executablePath = Utils::String::replace(iconPath, "\"", "");
        size_t commaPos = gameInfo.executablePath.find(',');
        if (commaPos != std::string::npos) {
            gameInfo.executablePath = gameInfo.executablePath.substr(0, commaPos);
        }
    } else { return std::nullopt; }

    // Diamo la PRIORITÀ all'ID numerico dall'XML, perché è più affidabile.
    if (!gameInfo.installPath.empty()) {
        std::string xmlPath = Utils::FileSystem::combine(gameInfo.installPath, "__Installer/installerdata.xml");
        if (Utils::FileSystem::exists(xmlPath)) {
            pugi::xml_document doc;
            if (doc.load_file(xmlPath.c_str())) {
                pugi::xml_node gameNode = doc.child("game");
                if (gameNode && gameNode.child("contentIDs") && gameNode.child("contentIDs").child("contentID")) {
                    gameInfo.id = gameNode.child("contentIDs").child("contentID").text().as_string();
                }
            }
        }
    }
    
    // Solo se non abbiamo trovato l'ID numerico, cerchiamo l'OfferID completo nel registro come fallback.
    if (gameInfo.id.empty()) {
        for (DWORD i = 0; ; ++i) {
            WCHAR valueNameW[256], valueDataW[2048];
            DWORD valueNameSize = 256, valueDataSize = sizeof(valueDataW), type;
            if (RegEnumValueW(hGameKey, i, valueNameW, &valueNameSize, NULL, &type, (LPBYTE)valueDataW, &valueDataSize) != ERROR_SUCCESS) break;
            if (type == REG_SZ) {
                std::string dataStr = wstring_to_utf8(valueDataW);
                if (dataStr.rfind("Origin.OFR.", 0) == 0) {
                    gameInfo.id = dataStr;
                    break; 
                }
            }
        }
    }
    
    return gameInfo;
}
#endif
}
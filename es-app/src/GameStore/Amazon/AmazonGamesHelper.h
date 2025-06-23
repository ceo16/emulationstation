#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_HELPER_H
#define ES_APP_GAMESTORE_AMAZON_HELPER_H

#include <string>
#include "utils/StringUtil.h" // Per la conversione

#ifdef _WIN32
#include <Windows.h>
#include <Objbase.h>
#include <rpcdce.h>
#include <VersionHelpers.h> // Per la versione di Windows

#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "ole32.lib")

// Struttura per ottenere la versione di Windows
typedef LONG(NTAPI* fnRtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

namespace Amazon::Helper
{
    // Genera un GUID senza trattini, come GetMachineGuid().ToString("N") in C#
    inline std::string getMachineGuidNoHyphens()
    {
        GUID guid;
        if (CoCreateGuid(&guid) == S_OK)
        {
            wchar_t guidString[40] = { 0 };
            StringFromGUID2(guid, guidString, 40);
            std::wstring ws(guidString);
            // Rimuove le parentesi graffe e i trattini
            ws.erase(std::remove(ws.begin(), ws.end(), L'{'), ws.end());
            ws.erase(std::remove(ws.begin(), ws.end(), L'}'), ws.end());
            ws.erase(std::remove(ws.begin(), ws.end(), L'-'), ws.end());
            return Utils::String::convertFromWideString(ws);
        }
        return "00000000000000000000000000000000"; // Fallback
    }
    
    // Ottiene la versione completa di Windows, come Environment.OSVersion.Version.ToString(4)
    inline std::string getWindowsVersionString()
    {
        HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            fnRtlGetVersion pRtlGetVersion = (fnRtlGetVersion)GetProcAddress(hMod, "RtlGetVersion");
            if (pRtlGetVersion != NULL) {
                RTL_OSVERSIONINFOW rovi = { 0 };
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (pRtlGetVersion(&rovi) == 0) {
                    return std::to_string(rovi.dwMajorVersion) + "." +
                           std::to_string(rovi.dwMinorVersion) + "." +
                           std::to_string(rovi.dwBuildNumber) + "." +
                           std::to_string(rovi.dwPlatformId); // Usa platformId come quarto componente se serve
                }
            }
        }
        return "10.0.0.0"; // Fallback
    }

    // Restituisce il nome del computer
    inline std::string getComputerName()
    {
        wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(buffer) / sizeof(buffer[0]);
        if (GetComputerNameW(buffer, &size)) {
            return Utils::String::convertFromWideString(std::wstring(buffer));
        }
        return "Computer"; // Fallback
    }
}

#endif // WIN32
#endif // ES_APP_GAMESTORE_AMAZON_HELPER_H
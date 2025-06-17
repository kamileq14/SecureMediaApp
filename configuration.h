#pragma once

#include <string>
#include <vector>
#include <windows.h>

// ID zasobu dla tapety (musi być zdefiniowane w resources.h i resources.rc)
#define WALLPAPER_RESOURCE_ID 101

struct AppConfig {
    std::string masterPassword = "haslo123";
    int maxFailedAttempts = 3;
    const wchar_t* firewallRuleName = L"SecureAppBlockNetwork";
    std::vector<std::wstring> blockedProcesses = {
        L"taskmgr.exe", L"procexp.exe", L"procexp64.exe", L"processhacker.exe",
        L"chrome.exe", L"firefox.exe", L"msedge.exe", L"opera.exe", L"iexplore.exe", L"brave.exe"
    };

    // Flagi do włączania/wyłączania komponentów
    bool enableTaskScheduler = true;
    bool enableWallpaper = true;
    bool enableExplorerPolicies = true;
    bool enableFirewallRule = true;
    bool enableKeyboardHook = true;
    bool enableProcessMonitoring = true;
    bool enableNarratorManagement = true;
    bool enableWatchdog = true;
    bool enableBeepSound = true;
};

// Globalna instancja konfiguracji
extern AppConfig g_appConfig;
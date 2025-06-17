#include <windows.h>
#include <string>
#include <vector>
#include <iostream>

#include "Components/FirewallManager.h"
#include "Components/ProcessMonitor.h"
#include "Components/SoundPlayer.h"
#include "Components/KeyboardHookManager.h"
#include "Components/TaskSchedulerManager.h"
#include "Components/WallpaperManager.h"
#include "Components/ExplorerPolicyManager.h"
#include "Components/NarratorManager.h"
#include "Components/Watchdog.h"
#include "Components/GuiManager.h"
// #include "configuration.h" // Jeśli plik configuration.h istnieje i jest potrzebny, odkomentuj

// The #pragma comment warnings are expected and harmless for MinGW.
// They are commented out because CMake handles linking.
// #pragma comment(lib, "winmm.lib")
// #pragma comment(lib, "gdiplus.lib")
// #pragma comment(lib, "user32.lib")
// #pragma comment(lib, "kernel32.lib")
// #pragma comment(lib, "advapi32.lib")
// #pragma comment(lib, "ole32.lib")
// #pragma comment(lib, "oleaut32.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutex(nullptr, TRUE, L"SecureMediaAppMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(nullptr, L"Secure Media App is already running!", L"Warning", MB_OK | MB_ICONWARNING);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    // Initialize Managers - providing arguments where constructors require them
    FirewallManager firewallManager; // POWINIEN TERAZ UŻYWAĆ DOMYŚLNEGO KONSTRUKTORA
    SoundPlayer soundPlayer;
    KeyboardHookManager keyboardHookManager; // POWINIEN TERAZ UŻYWAĆ DOMYŚLNEGO KONSTRUKTORA (z GetModuleHandle(nullptr))
    TaskSchedulerManager taskSchedulerManager;
    WallpaperManager wallpaperManager; // POWINIEN TERAZ UŻYWAĆ DOMYŚLNEGO KONSTRUKTORA
    ExplorerPolicyManager explorerPolicyManager;
    NarratorManager narratorManager;
    Watchdog watchdog;

    std::vector<std::wstring> initialBlockedProcesses = {
        L"taskmgr.exe", L"cmd.exe", L"powershell.exe", L"regedit.exe", L"explorer.exe"
    };
    ProcessMonitor processMonitor(initialBlockedProcesses);

    GuiManager guiManager(hInstance, nCmdShow);
    guiManager.SetProcessMonitor(&processMonitor);

    if (!guiManager.Initialize()) {
        MessageBox(nullptr, L"GUI Initialization Failed!", L"Error", MB_OK | MB_ICONERROR);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    bool firewallRuleExists = false;
    // Wywołanie manageRule, teraz, gdy jest publiczne i konstruktor domyślny jest dostępny
    if (firewallManager.manageRule(true, firewallRuleExists)) {
        // Reguła firewall zarządzana pomyślnie
    } else {
        MessageBox(nullptr, L"Failed to manage firewall rule!", L"Error", MB_OK | MB_ICONERROR);
    }

    processMonitor.StartMonitoring();
    keyboardHookManager.StartHook(); // Start the keyboard hook here

    int result = guiManager.Run();

    // Cleanup
    processMonitor.StopMonitoring();
    keyboardHookManager.StopHook();
    // Jeśli chcesz, aby reguła firewall była wyłączana po zamknięciu aplikacji:
    // firewallManager.manageRule(false, firewallRuleExists);

    if (hMutex) CloseHandle(hMutex);
    return result;
}
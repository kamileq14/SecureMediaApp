#include <windows.h>
#include <tlhelp32.h>   // For CreateToolhelp32Snapshot, PROCESSENTRY32W, Process32FirstW/NextW
#include <vector>
#include <string>
#include <algorithm>    // For std::transform, std::find, std::remove
#include <iostream>     // For std::cerr or std::cout for debugging
#include <thread>       // For std::this_thread::sleep_for
#include <chrono>       // For std::chrono::seconds

#include "ProcessMonitor.h" // Your class definition
#include "ToLower.h"        // ToLower utility function

// Constructor - default
ProcessMonitor::ProcessMonitor() : running(false), passwordEntered(false) {
    // Default blocked processes or none
    blockedProcesses.push_back(L"taskmgr.exe");
    blockedProcesses.push_back(L"cmd.exe");
    blockedProcesses.push_back(L"powershell.exe");
    blockedProcesses.push_back(L"regedit.exe");
    blockedProcesses.push_back(L"explorer.exe"); // Commonly blocked
}

// Constructor - with initial blocked processes
ProcessMonitor::ProcessMonitor(const std::vector<std::wstring>& initialBlockedProcesses)
    : running(false), passwordEntered(false), blockedProcesses(initialBlockedProcesses) {}

// Destructor
ProcessMonitor::~ProcessMonitor() {
    StopMonitoring(); // Ensure monitoring thread is stopped and joined
}

void ProcessMonitor::StartMonitoring() {
    if (!running.load()) { // Check current state of 'running' flag
        running.store(true); // Set the flag to true
        monitorThread = std::thread(&ProcessMonitor::MonitorLoop, this);
        std::wcout << L"Process monitoring started." << std::endl;
    } else {
        std::wcout << L"Process monitoring is already running." << std::endl;
    }
}

void ProcessMonitor::StopMonitoring() {
    if (running.load()) { // Check current state of 'running' flag
        running.store(false); // Request the thread to stop
        if (monitorThread.joinable()) {
            monitorThread.join(); // Wait for the thread to finish
        }
        std::wcout << L"Process monitoring stopped." << std::endl;
    } else {
        std::wcout << L"Process monitoring is not running." << std::endl;
    }
}

void ProcessMonitor::AddBlockedProcess(const std::wstring& processName) {
    std::wstring lowerName = ToLower(processName);
    // Check if it's not already in the list to avoid duplicates
    if (std::find(blockedProcesses.begin(), blockedProcesses.end(), lowerName) == blockedProcesses.end()) {
        blockedProcesses.push_back(lowerName);
        std::wcout << L"Added '" << processName << L"' to blocked processes list." << std::endl;
    }
}

void ProcessMonitor::RemoveBlockedProcess(const std::wstring& processName) {
    std::wstring lowerName = ToLower(processName);
    auto it = std::remove(blockedProcesses.begin(), blockedProcesses.end(), lowerName);
    if (it != blockedProcesses.end()) {
        blockedProcesses.erase(it, blockedProcesses.end());
        std::wcout << L"Removed '" << processName << L"' from blocked processes list." << std::endl;
    }
}

// NEW: Implementation of SetPasswordEntered
void ProcessMonitor::SetPasswordEntered(bool entered) {
    passwordEntered.store(entered);
    if (entered) {
        std::wcout << L"Password successfully entered. Process monitor behavior might change." << std::endl;
        // You might decide to stop monitoring or change behavior here, e.g.:
        // StopMonitoring(); // Example: Stop monitoring if password entered
    } else {
        std::wcout << L"Password status reset." << std::endl;
    }
}

void ProcessMonitor::MonitorLoop() {
    // Loop as long as the 'running' flag is true
    while (running.load()) {
        // If password is entered, perhaps we change monitoring behavior or stop
        if (passwordEntered.load()) {
            // For now, if password entered, just sleep longer or do nothing
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue; // Skip the process killing part
        }

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
            continue;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W); // IMPORTANT: Initialize dwSize

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring procNameLower = ToLower(pe32.szExeFile);
                // Check if the current process name is in our blocked list
                if (std::find(blockedProcesses.begin(), blockedProcesses.end(), procNameLower) != blockedProcesses.end()) {
                    std::wcout << L"Detected prohibited process: " << pe32.szExeFile
                               << L" (ID: " << pe32.th32ProcessID << L"). Attempting to kill." << std::endl;
                    KillProcessByID(pe32.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe32) && running.load()); // Continue only if still running
        } else {
            std::cerr << "Process32FirstW failed: " << GetLastError() << std::endl;
        }

        CloseHandle(hSnapshot); // Close the snapshot handle
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Check every 2 seconds
    }
}

void ProcessMonitor::KillProcessByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (ToLower(pe32.szExeFile) == ToLower(processName)) {
                    KillProcessByID(pe32.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    } else {
        std::cerr << "CreateToolhelp32Snapshot failed for KillProcessByName: " << GetLastError() << std::endl;
    }
}

void ProcessMonitor::KillProcessByID(DWORD processID) {
    // Don't try to kill self or system idle process (PID 0 or 4)
    if (processID == 0 || processID == 4 || processID == GetCurrentProcessId()) {
        return;
    }

    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProc) {
        if (TerminateProcess(hProc, 0)) {
            std::wcout << L"Successfully terminated process with ID: " << processID << std::endl;
        } else {
            // Error code 5 (ACCESS_DENIED) is common if process has higher privileges
            std::cerr << "Failed to terminate process with ID " << processID << ": " << GetLastError() << std::endl;
        }
        CloseHandle(hProc);
    } else {
        // Error code 5 (ACCESS_DENIED) if no terminate permission
        // Error code 87 (INVALID_PARAMETER) if PID doesn't exist
        std::cerr << "OpenProcess failed for ID " << processID << ": " << GetLastError() << std::endl;
    }
}
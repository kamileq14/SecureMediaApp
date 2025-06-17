#include "NarratorManager.h"
#include <shellapi.h> // For ShellExecute
#include <tlhelp32.h> // For CreateToolhelp32Snapshot
#include <chrono>     // For std::this_thread::sleep_for

NarratorManager::NarratorManager() : m_running(false), m_passwordEntered(false) {}

NarratorManager::~NarratorManager() {
    Stop();
    KillProcessByName(L"narrator.exe"); // Ensure Narrator is killed on destruction
}

void NarratorManager::Start() {
    if (!m_running) {
        ShellExecute(NULL, L"open", L"narrator.exe", NULL, NULL, SW_SHOWNORMAL);
        m_running = true;
        m_monitorThread = std::thread(&NarratorManager::MonitorLoop, this);
    }
}

void NarratorManager::Stop() {
    if (m_running) {
        m_running = false;
        if (m_monitorThread.joinable()) {
            m_monitorThread.join();
        }
    }
}

void NarratorManager::SetPasswordEntered(bool entered) {
    m_passwordEntered = entered;
}

void NarratorManager::MonitorLoop() {
    while (m_running && !m_passwordEntered) {
        bool isRunning = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(pe32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    if (std::wstring(pe32.szExeFile) == L"narrator.exe") {
                        isRunning = true;
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }

        if (!isRunning) {
            ShellExecute(NULL, L"open", L"narrator.exe", NULL, NULL, SW_SHOWNORMAL);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void NarratorManager::KillProcessByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (std::wstring(pe32.szExeFile) == processName) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                        break;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
}
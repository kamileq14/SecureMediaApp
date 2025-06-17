#include "Watchdog.h"
#include <shellapi.h> // For ShellExecute
#include <chrono>     // For std::this_thread::sleep_for

Watchdog::Watchdog() : m_running(false) {
    GetModuleFileName(NULL, m_exePath, MAX_PATH);
}

Watchdog::~Watchdog() {
    Stop();
}

void Watchdog::Start() {
    if (!m_running) {
        m_running = true;
        m_watchdogThread = std::thread(&Watchdog::WatchdogLoop, this);
    }
}

void Watchdog::Stop() {
    if (m_running) {
        m_running = false;
        if (m_watchdogThread.joinable()) {
            m_watchdogThread.join();
        }
    }
}

void Watchdog::WatchdogLoop() {
    while (m_running) {
        HWND hwnd = FindWindow(L"SecureMediaApp", NULL); // Zakładam, że "SecureMediaApp" to nazwa klasy okna
        if (!hwnd) {
            ShellExecute(NULL, L"open", m_exePath, NULL, NULL, SW_SHOWNORMAL);
            std::this_thread::sleep_for(std::chrono::seconds(3)); // Daj czas na uruchomienie aplikacji
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
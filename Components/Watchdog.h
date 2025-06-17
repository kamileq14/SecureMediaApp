#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>

class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    void Start();
    void Stop();

private:
    void WatchdogLoop();
    std::atomic<bool> m_running;
    std::thread m_watchdogThread;
    wchar_t m_exePath[MAX_PATH];
};
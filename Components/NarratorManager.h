#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>

class NarratorManager {
public:
    NarratorManager();
    ~NarratorManager();

    void Start();
    void Stop();
    void SetPasswordEntered(bool entered);

private:
    void MonitorLoop();
    void KillProcessByName(const std::wstring& processName);
    std::atomic<bool> m_running;
    std::atomic<bool> m_passwordEntered;
    std::thread m_monitorThread;
};
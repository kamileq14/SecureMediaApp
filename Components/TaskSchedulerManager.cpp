#include "TaskSchedulerManager.h"
#include <cstdlib> // For _wsystem

TaskSchedulerManager::TaskSchedulerManager() : m_taskName(L"SecureMediaAppDelayedStart") {
    GetModuleFileName(NULL, m_exePath, MAX_PATH);
}

void TaskSchedulerManager::AddTask() {
    std::wstring deleteCmd = L"schtasks /Delete /TN \"";
    deleteCmd += m_taskName;
    deleteCmd += L"\" /F > nul 2>&1";
    _wsystem(deleteCmd.c_str()); // Używamy _wsystem do uruchamiania poleceń shellowych

    std::wstring createCmd = L"schtasks /Create /TN \"";
    createCmd += m_taskName;
    createCmd += L"\" /TR \"\\\"";
    createCmd += m_exePath;
    createCmd += L"\\\"\" /SC ONLOGON /DELAY 0000:05 /RL HIGHEST /F > nul 2>&1";

    _wsystem(createCmd.c_str());
}

void TaskSchedulerManager::RemoveTask() {
    std::wstring deleteCmd = L"schtasks /Delete /TN \"";
    deleteCmd += m_taskName;
    deleteCmd += L"\" /F > nul 2>&1";
    _wsystem(deleteCmd.c_str());
}
#pragma once
#include <string>
#include <windows.h>

class TaskSchedulerManager {
public:
    TaskSchedulerManager();
    void AddTask();
    void RemoveTask();
private:
    wchar_t m_exePath[MAX_PATH];
    const wchar_t* m_taskName;
};
#ifndef SECUREMEDIAAPP_PROCESSMONITOR_H
#define SECUREMEDIAAPP_PROCESSMONITOR_H

#include <windows.h> // For DWORD, HANDLE, etc.
#include <string>
#include <vector>
#include <thread> // For std::thread
#include <atomic> // For std::atomic_bool (thread-safe boolean flag)
#include <mutex>  // If you need more complex locking, though std::atomic is sufficient for 'running'

class ProcessMonitor {
public:
    // Default constructor - needed if you instantiate ProcessMonitor()
    ProcessMonitor();

    // Constructor that can take an initial list of processes to monitor/block
    ProcessMonitor(const std::vector<std::wstring>& initialBlockedProcesses);

    // Destructor to ensure thread is stopped and joined cleanly
    ~ProcessMonitor();

    // Public methods to control the monitoring
    void StartMonitoring();
    void StopMonitoring();

    // Methods to manage the list of processes to block at runtime
    void AddBlockedProcess(const std::wstring& processName);
    void RemoveBlockedProcess(const std::wstring& processName);

    // Methods to kill processes (can be called publicly or internally by MonitorLoop)
    void KillProcessByName(const std::wstring& processName);
    void KillProcessByID(DWORD processID);

    // NEW: Method to notify ProcessMonitor about password entry
    void SetPasswordEntered(bool entered);

private:
    std::atomic<bool> running;         // Flag to control the monitoring thread loop
    std::atomic<bool> passwordEntered; // Flag indicating if the password has been entered
    std::thread monitorThread;         // The thread that runs the monitoring loop

    // List of processes (by name) that the monitor should look for and terminate
    std::vector<std::wstring> blockedProcesses;

    // The main function that runs in the separate thread
    void MonitorLoop();
};

#endif //SECUREMEDIAAPP_PROCESSMONITOR_H
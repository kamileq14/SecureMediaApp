#pragma once
#include <string>
#include <windows.h>

class ExplorerPolicyManager {
public:
    ExplorerPolicyManager();
    void SetPolicies();
    void RestorePolicies();
private:
    void SetRegistryPolicy(const std::wstring& subKey, const std::wstring& valueName, DWORD data);
};
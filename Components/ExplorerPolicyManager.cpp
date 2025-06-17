#include "ExplorerPolicyManager.h"

ExplorerPolicyManager::ExplorerPolicyManager() {}

void ExplorerPolicyManager::SetRegistryPolicy(const std::wstring& subKey, const std::wstring& valueName, DWORD data) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(hKey);
    }
}

void ExplorerPolicyManager::SetPolicies() {
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ActiveDesktop", L"NoChangingWallPaper", 1);
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"NoFileDelete", 1);
}

void ExplorerPolicyManager::RestorePolicies() {
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ActiveDesktop", L"NoChangingWallPaper", 0);
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"NoFileDelete", 0);
}
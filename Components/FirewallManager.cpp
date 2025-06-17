#include <windows.h>
#include <netfw.h>
#include <objbase.h> // For CoInitializeEx, CoUninitialize, CoCreateInstance
#include <oleauto.h> // For SysAllocString, SysFreeString
#include <iostream>

#include "FirewallManager.h"

// Pomocnicza funkcja do zwalniania BSTR, jeśli nie używamy CComBSTR
inline void FreeBSTR(BSTR bstr) {
    if (bstr) {
        SysFreeString(bstr);
    }
}

// Domyślny konstruktor
FirewallManager::FirewallManager() : pNetFwPolicy2(nullptr) {
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE) {
        std::cerr << "CoInitializeEx failed: " << std::hex << hr << std::endl;
        return;
    }
    // Ręczne użycie CoCreateInstance i przechowywanie interfejsu
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_ALL,
                          __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) {
        std::cerr << "CoCreateInstance for INetFwPolicy2 failed: " << std::hex << hr << std::endl;
        CoUninitialize();
        pNetFwPolicy2 = nullptr;
        return;
    }
}

// Konstruktor z nazwą reguły
FirewallManager::FirewallManager(const wchar_t* ruleName) : pNetFwPolicy2(nullptr) {
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE) {
        std::cerr << "CoInitializeEx failed: " << std::hex << hr << std::endl;
        return;
    }
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_ALL,
                          __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) {
        std::cerr << "CoCreateInstance for INetFwPolicy2 failed: " << std::hex << hr << std::endl;
        CoUninitialize();
        pNetFwPolicy2 = nullptr;
        return;
    }
}

FirewallManager::~FirewallManager() {
    if (pNetFwPolicy2) {
        pNetFwPolicy2->Release();
        pNetFwPolicy2 = nullptr;
    }
    CoUninitialize();
}

bool FirewallManager::manageRule(bool enable, bool& ruleExists) {
    if (!pNetFwPolicy2) {
        std::cerr << "Firewall policy object not initialized." << std::endl;
        return false;
    }

    HRESULT hr = S_OK;
    ruleExists = false;
    INetFwRules* pNetFwRules = nullptr; // Ręczne zarządzanie wskaźnikiem

    hr = pNetFwPolicy2->get_Rules(&pNetFwRules);
    if (FAILED(hr)) {
        std::cerr << "get_Rules failed: " << std::hex << hr << std::endl;
        return false;
    }

    BSTR bstrRuleName = SysAllocString(L"SecureAppBlockOutbound");
    if (!bstrRuleName) {
        std::cerr << "Failed to allocate BSTR for rule name." << std::endl;
        if (pNetFwRules) pNetFwRules->Release();
        return false;
    }

    INetFwRule* pRuleRetrieve = nullptr;
    hr = pNetFwRules->Item(bstrRuleName, &pRuleRetrieve); // Używamy pNetFwRules

    if (SUCCEEDED(hr)) {
        ruleExists = true;
        if (enable) {
            if (pRuleRetrieve->put_Enabled(VARIANT_TRUE) == S_OK) {
                std::wcout << L"Firewall rule '" << bstrRuleName << L"' enabled." << std::endl;
                if (pRuleRetrieve) pRuleRetrieve->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                return true;
            } else {
                std::cerr << "Failed to enable existing rule." << std::endl;
                if (pRuleRetrieve) pRuleRetrieve->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                return false;
            }
        } else {
            if (pRuleRetrieve->put_Enabled(VARIANT_FALSE) == S_OK) {
                std::wcout << L"Firewall rule '" << bstrRuleName << L"' disabled." << std::endl;
                if (pRuleRetrieve) pRuleRetrieve->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                return true;
            } else {
                std::cerr << "Failed to disable existing rule." << std::endl;
                if (pRuleRetrieve) pRuleRetrieve->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                return false;
            }
        }
    } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        if (enable) {
            INetFwRule* pRule = nullptr;
            hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_ALL,
                                  __uuidof(INetFwRule), (void**)&pRule);
            if (FAILED(hr)) {
                std::cerr << "CoCreateInstance for INetFwRule failed: " << std::hex << hr << std::endl;
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                return false;
            }

            BSTR bstrDescription = SysAllocString(L"Blocks all outbound network traffic for SecureApp.");
            BSTR bstrGrouping = SysAllocString(L"SecureMediaApp Firewall Rules");

            if (!bstrDescription || !bstrGrouping) {
                std::cerr << "Failed to allocate BSTR for rule description or grouping." << std::endl;
                if (pRule) pRule->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                FreeBSTR(bstrDescription);
                FreeBSTR(bstrGrouping);
                return false;
            }

            pRule->put_Name(bstrRuleName);
            pRule->put_Description(bstrDescription);
            pRule->put_Action(NET_FW_ACTION_BLOCK);
            pRule->put_Direction(NET_FW_RULE_DIR_OUT);
            pRule->put_Enabled(VARIANT_TRUE);
            pRule->put_Grouping(bstrGrouping);
            // ZMIANA: Użyj NET_FW_PROFILE2_ALL zamiast NET_FW_PROFILE_ALL
            pRule->put_Profiles(NET_FW_PROFILE2_ALL);

            hr = pNetFwRules->Add(pRule); // ZMIANA: Użyj pNetFwRules
            if (SUCCEEDED(hr)) {
                std::wcout << L"Firewall rule '" << bstrRuleName << L"' created and enabled." << std::endl;
                if (pRule) pRule->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                FreeBSTR(bstrDescription);
                FreeBSTR(bstrGrouping);
                return true;
            } else {
                std::cerr << "Failed to add firewall rule: " << std::hex << hr << std::endl;
                if (pRule) pRule->Release();
                if (pNetFwRules) pNetFwRules->Release();
                FreeBSTR(bstrRuleName);
                FreeBSTR(bstrDescription);
                FreeBSTR(bstrGrouping);
                return false;
            }
        } else {
            std::wcout << L"Firewall rule '" << bstrRuleName << L"' does not exist, nothing to disable." << std::endl;
            if (pNetFwRules) pNetFwRules->Release();
            FreeBSTR(bstrRuleName);
            return true;
        }
    } else {
        std::cerr << "Error checking for firewall rule existence: " << std::hex << hr << std::endl;
        if (pRuleRetrieve) pRuleRetrieve->Release();
        if (pNetFwRules) pNetFwRules->Release();
        FreeBSTR(bstrRuleName);
        return false;
    }
}
#ifndef SECUREMEDIAAPP_FIREWALLMANAGER_H
#define SECUREMEDIAAPP_FIREWALLMANAGER_H

#include <windows.h>
#include <netfw.h>
#include <string> // Dodano dla std::wstring

class FirewallManager {
public:
    FirewallManager();
    FirewallManager(const wchar_t* ruleName);
    ~FirewallManager();

    bool manageRule(bool enable, bool& ruleExists);

private:
    INetFwPolicy2* pNetFwPolicy2;
};

#endif //SECUREMEDIAAPP_FIREWALLMANAGER_H
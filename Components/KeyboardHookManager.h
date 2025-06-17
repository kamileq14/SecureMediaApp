#ifndef SECUREMEDIAAPP_KEYBOARDHOOKMANAGER_H
#define SECUREMEDIAAPP_KEYBOARDHOOKMANAGER_H

#include <windows.h>
#include <iostream>

class KeyboardHookManager {
public:
    // Domyślny konstruktor
    KeyboardHookManager();
    KeyboardHookManager(HINSTANCE hInstance); // Istniejący konstruktor
    ~KeyboardHookManager();

    bool StartHook();
    void StopHook();

private:
    HHOOK m_keyboardHook;
    HINSTANCE m_hInstance; // Changed to m_hInstance to avoid shadowing

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};

#endif //SECUREMEDIAAPP_KEYBOARDHOOKMANAGER_H
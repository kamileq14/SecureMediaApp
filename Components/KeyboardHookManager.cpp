#include "KeyboardHookManager.h" // Upewnij się, że ten include jest!

// Domyślny konstruktor
KeyboardHookManager::KeyboardHookManager() : m_keyboardHook(nullptr), m_hInstance(GetModuleHandle(nullptr)) {
    // Domyślnie użyj GetModuleHandle(nullptr) dla hInstance, jeśli nie podano
}

KeyboardHookManager::KeyboardHookManager(HINSTANCE hInstance)
    : m_keyboardHook(nullptr), m_hInstance(hInstance) {}

KeyboardHookManager::~KeyboardHookManager() {
    StopHook();
}

bool KeyboardHookManager::StartHook() {
    if (!m_keyboardHook) {
        // Set a low-level keyboard hook
        m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, m_hInstance, 0);
        if (m_keyboardHook == nullptr) {
            std::cerr << "Failed to set keyboard hook: " << GetLastError() << std::endl;
            return false;
        }
        std::cout << "Keyboard hook set." << std::endl;
    }
    return true;
}

void KeyboardHookManager::StopHook() {
    if (m_keyboardHook) {
        if (!UnhookWindowsHookEx(m_keyboardHook)) {
            std::cerr << "Failed to unhook keyboard: " << GetLastError() << std::endl;
        }
        m_keyboardHook = nullptr;
        std::cout << "Keyboard hook removed." << std::endl;
    }
}

LRESULT CALLBACK KeyboardHookManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        // Block specific keys or combinations
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Block Ctrl+Alt+Del, Alt+Tab, Win key, etc.
            if ((p->vkCode == VK_DELETE && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) || // Ctrl+Alt+Del
                (p->vkCode == VK_TAB && (GetKeyState(VK_MENU) & 0x8000)) || // Alt+Tab
                ((p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) && (wParam == WM_KEYDOWN)) // Win key down
                // Add more keys to block as needed (e.g., Task Manager (Ctrl+Shift+Esc), etc.)
            ) {
                return 1; // Block the key
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
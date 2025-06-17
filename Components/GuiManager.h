#ifndef SECUREMEDIAAPP_GUIMANAGER_H
#define SECUREMEDIAAPP_GUIMANAGER_H

#include <windows.h>
#include <string>
#include <gdiplus.h> // Make sure Gdiplus.h is included
#include "ProcessMonitor.h" // Include the ProcessMonitor header to use its class

class GuiManager {
public:
    GuiManager(HINSTANCE hInstance, int nCmdShow);
    ~GuiManager();

    bool Initialize();
    int Run();

    void SetProcessMonitor(ProcessMonitor* monitor);

private:
    HINSTANCE m_hInstance;
    int m_nCmdShow;
    HWND hMainWnd;
    UINT_PTR timerId;

    // Password related
    std::wstring passwordInput;
    bool isCapsLockOn;
    bool isPasswordVisible;
    bool cursorVisible;
    RECT m_passwordRect; // Stores the rectangle for the password input field

    ProcessMonitor* m_processMonitor;

    static ULONG_PTR gdiplusToken;

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void NotifyPasswordEntered();
};

// Deklaracje funkcji rozszerzających GDI+ - muszą być poza klasą GuiManager, ale w namespace Gdiplus
namespace Gdiplus {
    void FillRoundedRectangle(Gdiplus::Graphics* graphics, Gdiplus::Brush* brush,
                              Gdiplus::REAL x, Gdiplus::REAL y,
                              Gdiplus::REAL width, Gdiplus::REAL height, Gdiplus::REAL radius);
    void DrawRoundedRectangle(Gdiplus::Graphics* graphics, Gdiplus::Pen* pen,
                              Gdiplus::REAL x, Gdiplus::REAL y,
                              Gdiplus::REAL width, Gdiplus::REAL height, Gdiplus::REAL radius);
}

#endif //SECUREMEDIAAPP_GUIMANAGER_H
#include "GuiManager.h"
#include <windows.h>
#include <gdiplus.h>
#include <cwctype>   // For iswprint
#include <iostream>
#include <string>

// Global GDI+ startup token initialization
ULONG_PTR GuiManager::gdiplusToken = 0;

GuiManager::GuiManager(HINSTANCE hInstance, int nCmdShow)
    // Używamy listy inicjalizacyjnej, aby uniknąć ostrzeżeń o cieniowaniu i kolejności
    : m_hInstance(hInstance),
      m_nCmdShow(nCmdShow),
      hMainWnd(nullptr),
      timerId(0),
      passwordInput(L""), // Zgodnie z kolejnością deklaracji w .h
      isCapsLockOn(false),
      isPasswordVisible(true), // Zgodnie z kolejnością deklaracji w .h
      cursorVisible(true),
      m_processMonitor(nullptr)
{
    // Initialize GDI+ here
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    if (status != Gdiplus::Ok) {
        std::cerr << "GdiplusStartup failed: " << status << std::endl;
        // Handle error, maybe throw an exception or set an error flag
    }
}

GuiManager::~GuiManager() {
    // Shutdown GDI+
    if (gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0; // Reset token
    }
    if (hMainWnd) {
        DestroyWindow(hMainWnd);
    }
}

void GuiManager::SetProcessMonitor(ProcessMonitor* monitor) {
    m_processMonitor = monitor;
}

bool GuiManager::Initialize() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = GuiManager::StaticWindowProc;
    wc.hInstance = m_hInstance; // Używamy m_hInstance
    wc.lpszClassName = L"SecureMediaAppClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    hMainWnd = CreateWindowExW(
        0,
        L"SecureMediaAppClass",
        L"Secure Media App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr,
        nullptr,
        m_hInstance, // Używamy m_hInstance
        this
    );

    if (!hMainWnd) {
        MessageBoxW(nullptr, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    ShowWindow(hMainWnd, m_nCmdShow); // Używamy m_nCmdShow
    UpdateWindow(hMainWnd);

    timerId = SetTimer(hMainWnd, 1, 500, nullptr);
    if (timerId == 0) {
        std::cerr << "Failed to create timer." << std::endl;
    }

    return true;
}

int GuiManager::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK GuiManager::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GuiManager* pThis = nullptr;
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<GuiManager*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<GuiManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GuiManager::NotifyPasswordEntered() {
    if (m_processMonitor) {
        m_processMonitor->SetPasswordEntered(true);
    } else {
        std::cerr << "ProcessMonitor instance not set in GuiManager for NotifyPasswordEntered." << std::endl;
    }
}


LRESULT GuiManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            isCapsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

            Gdiplus::SolidBrush backgroundBrush(Gdiplus::Color(255, 30, 30, 30));
            // Corrected: Explicitly cast to INT to resolve ambiguity
            graphics.FillRectangle(&backgroundBrush,
                                   static_cast<INT>(ps.rcPaint.left),
                                   static_cast<INT>(ps.rcPaint.top),
                                   static_cast<INT>(ps.rcPaint.right - ps.rcPaint.left),
                                   static_cast<INT>(ps.rcPaint.bottom - ps.rcPaint.top));


            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            m_passwordRect.left = (clientRect.right - 400) / 2;
            m_passwordRect.top = (clientRect.bottom - 50) / 2;
            m_passwordRect.right = m_passwordRect.left + 400;
            m_passwordRect.bottom = m_passwordRect.top + 50;

            Gdiplus::SolidBrush inputBackgroundBrush(Gdiplus::Color(255, 50, 50, 50));
            // Corrected: Call global Gdiplus::FillRoundedRectangle with graphics object
            Gdiplus::FillRoundedRectangle(&graphics, &inputBackgroundBrush,
                                          static_cast<Gdiplus::REAL>(m_passwordRect.left),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.top),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.right - m_passwordRect.left),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.bottom - m_passwordRect.top),
                                          10);

            Gdiplus::Pen inputBorderPen(Gdiplus::Color(255, 100, 100, 100), 2);
            // Corrected: Call global Gdiplus::DrawRoundedRectangle with graphics object
            Gdiplus::DrawRoundedRectangle(&graphics, &inputBorderPen,
                                          static_cast<Gdiplus::REAL>(m_passwordRect.left),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.top),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.right - m_passwordRect.left),
                                          static_cast<Gdiplus::REAL>(m_passwordRect.bottom - m_passwordRect.top),
                                          10);

            Gdiplus::FontFamily fontFamily(L"Segoe UI");
            Gdiplus::Font font(&fontFamily, 24, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 200, 200, 200));

            std::wstring displayPassword = passwordInput;
            if (!isPasswordVisible) {
                displayPassword = std::wstring(passwordInput.length(), L'*');
            }

            Gdiplus::StringFormat stringFormat;
            stringFormat.SetAlignment(Gdiplus::StringAlignmentNear);
            stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::RectF layoutRect(static_cast<Gdiplus::REAL>(m_passwordRect.left + 15),
                                      static_cast<Gdiplus::REAL>(m_passwordRect.top + 5),
                                      static_cast<Gdiplus::REAL>(m_passwordRect.right - m_passwordRect.left - 30),
                                      static_cast<Gdiplus::REAL>(m_passwordRect.bottom - m_passwordRect.top - 10));
            Gdiplus::CharacterRange characterRange(0, static_cast<INT>(displayPassword.length()));
            stringFormat.SetMeasurableCharacterRanges(1, &characterRange);

            Gdiplus::Region regions[1];
            graphics.MeasureCharacterRanges(displayPassword.c_str(), -1, &font, layoutRect, &stringFormat, 1, regions);
            Gdiplus::RectF textMetrics;
            regions[0].GetBounds(&textMetrics, &graphics);

            graphics.DrawString(displayPassword.c_str(), -1, &font, layoutRect, &stringFormat, &textBrush);

            if (cursorVisible) {
                Gdiplus::Pen cursorPen(Gdiplus::Color(255, 200, 200, 200), 2);
                graphics.DrawLine(&cursorPen,
                                  static_cast<Gdiplus::REAL>(m_passwordRect.left + 15 + textMetrics.Width),
                                  static_cast<Gdiplus::REAL>(m_passwordRect.top + 10),
                                  static_cast<Gdiplus::REAL>(m_passwordRect.left + 15 + textMetrics.Width),
                                  static_cast<Gdiplus::REAL>(m_passwordRect.bottom - 10));
            }

            if (isCapsLockOn) {
                Gdiplus::SolidBrush capsLockBrush(Gdiplus::Color(255, 255, 0, 0));
                Gdiplus::Font capsLockFont(&fontFamily, 16, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                graphics.DrawString(L"CAPS LOCK ON", -1, &capsLockFont,
                                    Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_passwordRect.left),
                                                    static_cast<Gdiplus::REAL>(m_passwordRect.top - 30)),
                                    &textBrush);
            }

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_CAPITAL:
                    isCapsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                    InvalidateRect(hwnd, nullptr, TRUE);
                    break;
                case VK_BACK:
                    if (!passwordInput.empty()) {
                        passwordInput.pop_back();
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    break;
                case VK_RETURN:
                    std::wcout << L"Password entered: " << passwordInput << std::endl;
                    if (passwordInput == L"admin") {
                        MessageBoxW(hwnd, L"Access Granted!", L"Success", MB_OK | MB_ICONINFORMATION);
                        NotifyPasswordEntered();
                    } else {
                        MessageBoxW(hwnd, L"Access Denied!", L"Error", MB_OK | MB_ICONERROR);
                        passwordInput.clear();
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    break;
                case VK_INSERT:
                    isPasswordVisible = !isPasswordVisible;
                    InvalidateRect(hwnd, nullptr, TRUE);
                    break;
                default:
                    if (iswprint(static_cast<wint_t>(wParam))) {
                        passwordInput += static_cast<wchar_t>(wParam);
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    break;
            }
            break;
        }
        case WM_TIMER: {
            if (wParam == timerId) {
                cursorVisible = !cursorVisible;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        }
        case WM_DESTROY:
            KillTimer(hwnd, timerId);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Globalne funkcje rozszerzające GDI+, umieszczone w przestrzeni nazw Gdiplus
namespace Gdiplus {
void FillRoundedRectangle(Gdiplus::Graphics* graphics, Gdiplus::Brush* brush,
                                            Gdiplus::REAL x, Gdiplus::REAL y,
                                            Gdiplus::REAL width, Gdiplus::REAL height, Gdiplus::REAL radius) {
    if (radius <= 0) {
        graphics->FillRectangle(brush, x, y, width, height);
        return;
    }

    Gdiplus::GraphicsPath path;
    path.AddArc(x, y, 2 * radius, 2 * radius, 180, 90);                  // Top-left corner
    path.AddArc(x + width - 2 * radius, y, 2 * radius, 2 * radius, 270, 90); // Top-right corner
    path.AddArc(x + width - 2 * radius, y + height - 2 * radius, 2 * radius, 2 * radius, 0, 90); // Bottom-right corner
    path.AddArc(x, y + height - 2 * radius, 2 * radius, 2 * radius, 90, 90);    // Bottom-left corner
    path.CloseFigure();

    graphics->FillPath(brush, &path);
}

void DrawRoundedRectangle(Gdiplus::Graphics* graphics, Gdiplus::Pen* pen,
                                            Gdiplus::REAL x, Gdiplus::REAL y,
                                            Gdiplus::REAL width, Gdiplus::REAL height, Gdiplus::REAL radius) {
    if (radius <= 0) {
        graphics->DrawRectangle(pen, x, y, width, height);
        return;
    }

    Gdiplus::GraphicsPath path;
    path.AddArc(x, y, 2 * radius, 2 * radius, 180, 90);                  // Top-left corner
    path.AddArc(x + width - 2 * radius, y, 2 * radius, 2 * radius, 270, 90); // Top-right corner
    path.AddArc(x + width - 2 * radius, y + height - 2 * radius, 2 * radius, 2 * radius, 0, 90); // Bottom-right corner
    path.AddArc(x, y + height - 2 * radius, 2 * radius, 2 * radius, 90, 90);    // Bottom-left corner
    path.CloseFigure();

    graphics->DrawPath(pen, &path);
}
} // namespace Gdiplus
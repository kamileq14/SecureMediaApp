#ifndef SECUREMEDIAAPP_WALLPAPERMANAGER_H
#define SECUREMEDIAAPP_WALLPAPERMANAGER_H

#include <windows.h>
#include <string>
#include <iostream> // For debugging

class WallpaperManager {
public:
    // Domyślny konstruktor
    WallpaperManager();
    WallpaperManager(HINSTANCE hInstance, int resourceId); // Istniejący konstruktor
    ~WallpaperManager();

    bool setCustomWallpaper(const std::wstring& imagePath);
    bool resetWallpaper();

private:
    HINSTANCE m_hInstance;
    int m_resourceId;
    std::wstring m_originalWallpaperPath;
    bool m_wallpaperChanged;

    bool saveResourceToFile(const std::wstring& filePath);
};

#endif //SECUREMEDIAAPP_WALLPAPERMANAGER_H
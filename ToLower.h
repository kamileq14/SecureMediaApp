#ifndef SECUREMEDIAAPP_TOLOWER_H
#define SECUREMEDIAAPP_TOLOWER_H

#include <string>
#include <algorithm> // For std::transform
#include <locale>    // For std::tolower with std::locale

// Function to convert a wide string to lowercase
// This version takes a const reference and returns a new string
inline std::wstring ToLower(const std::wstring& str) {
    std::wstring lowerStr = str;
    // Use std::tolower with a default locale (which handles wide chars correctly on Windows)
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](wchar_t c){ return std::tolower(c, std::locale()); });
    return lowerStr;
}

#endif //SECUREMEDIAAPP_TOLOWER_H
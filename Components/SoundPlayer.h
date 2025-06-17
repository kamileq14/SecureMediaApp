#pragma once
#include <windows.h>
#pragma comment(lib, "winmm.lib") // Link with winmm.lib for Beep

class SoundPlayer {
public:
    SoundPlayer();
    void PlayBeep();
};
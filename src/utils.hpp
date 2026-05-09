#ifndef UTILS_HPP
#define UTILS_HPP

#include "raylib.h"
#include <fstream>

struct Settings {
    float masterVolume = 0.8f;
    float musicVolume = 0.5f;
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;

    void Save() {
        std::ofstream f("settings.dat", std::ios::binary);
        if (f.is_open()) {
            f.write((char*)this, sizeof(Settings));
            f.close();
        }
    }

    void Load() {
        std::ifstream f("settings.dat", std::ios::binary);
        if (f.is_open()) {
            f.read((char*)this, sizeof(Settings));
            f.close();
            // Apply immediately
            SetMasterVolume(masterVolume);
            if (fullscreen && !IsWindowFullscreen()) ToggleFullscreen();
        }
    }
};

extern Settings gSettings;

#endif

#ifndef MENU_HPP
#define MENU_HPP

#include "raylib.h"
#include <vector>
#include <memory>
#include <string>

// Forward declarations or include after fixing path
#include "../bosses/adas_gooner.hpp"
#include "../enemies/enemy.hpp"

enum class MenuState { MAIN, HOST, JOIN, OPTIONS, ADMIN_SETTINGS };

class Menu {
public:
    Menu();
    void Update();
    void Draw(bool drawUI = true);
    Camera3D GetCamera() { return menuCam; }
    
    bool shouldStartHost = false;
    bool shouldStartJoin = false;
    bool shouldToggleFullscreen = false;
    std::string joinIP = "127.0.0.1";
    std::string playerNick = "Gooner";
    std::string hostName = "MY SERVER";
    bool vsync = true;
    int resIndex = 1; // 0: 800x600, 1: 1280x720, 2: 1600x900, 3: 1920x1080
    bool useWafelModel = false;
    MenuState currentState = MenuState::MAIN;

private:
    Camera3D menuCam;
    float camAngle = 0.0f;
    float time = 0.0f;
    
    std::unique_ptr<AdasGooner> bgAdas;
    std::vector<std::shared_ptr<Enemy>> minions;
    
    bool DrawButton(Rectangle rect, const char* text, Color baseColor);
    void LoadSettings();
    void SaveSettings();
};


#endif

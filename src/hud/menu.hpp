#ifndef MENU_HPP
#define MENU_HPP

#include "raylib.h"
#include <vector>
#include <memory>
#include <string>

// Forward declarations or include after fixing path
#include "../bosses/adas_gooner.hpp"
#include "../enemies/enemy.hpp"

class Menu {
public:
    Menu();
    void Update();
    void Draw();
    
    bool shouldStartHost = false;

private:
    Camera3D menuCam;
    float camAngle = 0.0f;
    float time = 0.0f;
    
    std::unique_ptr<AdasGooner> bgAdas;
    std::vector<std::shared_ptr<Enemy>> minions;
    
    bool DrawButton(Rectangle rect, const char* text, Color baseColor);
};

#endif

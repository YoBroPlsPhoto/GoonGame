#include "menu.hpp"
#include "raymath.h"
#include "rlgl.h"

Menu::Menu() {
    menuCam.position = (Vector3){ 20.0f, 12.0f, 20.0f };
    menuCam.target = (Vector3){ 0.0f, 8.0f, 0.0f };
    menuCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    menuCam.fovy = 50.0f;
    menuCam.projection = CAMERA_PERSPECTIVE;

    bgAdas = std::make_unique<AdasGooner>((Vector3){ 0, 0, 0 }, -999);
    bgAdas->cutsceneState = CutsceneState::WALKING_OUT; 
    
    for (int i = 0; i < 6; i++) {
        float ang = (i / 6.0f) * PI * 2.0f;
        Vector3 pos = { cosf(ang) * 12.0f, 0, sinf(ang) * 12.0f };
        minions.push_back(std::make_shared<Enemy>(pos, EnemyType::MINION, WeaponType::MACHETE, -1000 - i));
    }
}

void Menu::Update() {
    time += GetFrameTime();
    camAngle += GetFrameTime() * 0.2f;
    
    menuCam.position.x = cosf(camAngle) * 35.0f;
    menuCam.position.z = sinf(camAngle) * 35.0f;
    menuCam.position.y = 12.0f + sinf(time * 0.5f) * 4.0f;
    
    bgAdas->walkTimer += GetFrameTime();
    bgAdas->isMoving = true;
    bgAdas->angle = (camAngle * RAD2DEG) + 180.0f;

    for (auto& m : minions) {
        m->walkTimer += GetFrameTime();
        m->isMoving = true;
        Vector3 dir = Vector3Subtract(bgAdas->position, m->position);
        m->angle = atan2f(dir.x, dir.z) * RAD2DEG;
    }
}

bool Menu::DrawButton(Rectangle rect, const char* text, Color baseColor) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    bool pressed = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    float pulse = hovered ? (sinf(time * 10.0f) * 0.02f + 1.02f) : 1.0f;
    
    Rectangle drawRect = rect;
    if (hovered) {
        drawRect.x -= (rect.width * (pulse - 1.0f)) / 2.0f;
        drawRect.y -= (rect.height * (pulse - 1.0f)) / 2.0f;
        drawRect.width *= pulse;
        drawRect.height *= pulse;
    }

    // Border
    DrawRectangleRounded(drawRect, 0.2f, 8, hovered ? WHITE : LIGHTGRAY);
    // Inner
    Rectangle inner = { drawRect.x + 2, drawRect.y + 2, drawRect.width - 4, drawRect.height - 4 };
    DrawRectangleRounded(inner, 0.2f, 8, Fade(hovered ? SKYBLUE : baseColor, 0.8f));
    
    int fontSize = 24;
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(drawRect.x + drawRect.width/2.0f - tw/2.0f), (int)(drawRect.y + drawRect.height/2.0f - fontSize/2.0f), fontSize, WHITE);
    
    return pressed;
}

void Menu::Draw() {
    BeginMode3D(menuCam);
    DrawGrid(100, 5.0f);
    bgAdas->Draw();
    for (auto& m : minions) m->Draw();
    EndMode3D();
    
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    DrawRectangleGradientEx((Rectangle){0, 0, (float)sw, (float)sh}, Fade(BLACK, 0.2f), Fade(BLACK, 0.3f), Fade(BLACK, 0.6f), Fade(BLACK, 0.2f));

    const char* title = "ADAS GOONER: ULTRA NYC";
    float titleY = 100.0f + sinf(time * 2.0f) * 10.0f;
    int tw = MeasureText(title, 60);
    DrawText(title, sw/2 - tw/2, (int)titleY, 60, GOLD);
    
    const char* subtitle = "THE FINAL GOONING SESSION";
    int stw = MeasureText(subtitle, 20);
    DrawText(subtitle, sw/2 - stw/2, (int)titleY + 90, 20, RAYWHITE);
    
    if (DrawButton((Rectangle){sw/2.0f - 150, (float)sh/2.0f - 20.0f, 300, 60}, "HOST MISSION", DARKGRAY)) {
        shouldStartHost = true;
    }
}

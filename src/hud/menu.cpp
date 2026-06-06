#include "menu.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <fstream>
#include <iostream>
#include <cctype>

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
    LoadSettings();
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

void Menu::Draw(bool drawUI) {
    BeginMode3D(menuCam);
    DrawGrid(100, 5.0f);
    bgAdas->Draw();
    for (auto& m : minions) m->Draw();
    EndMode3D();
    
    if (!drawUI) return;
    
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    DrawRectangleGradientEx((Rectangle){0, 0, (float)sw, (float)sh}, Fade(BLACK, 0.9f), Fade(DARKBLUE, 0.3f), Fade(BLACK, 0.9f), Fade(BLACK, 0.8f));

    const char* title = "ADAS GOONER: ULTRA NYC";
    float titleY = 60.0f + sinf(time * 2.0f) * 5.0f;
    int tw = MeasureText(title, 60);
    DrawText(title, sw/2 - tw/2 + 3, (int)titleY + 3, 60, Fade(BLACK, 0.5f));
    DrawText(title, sw/2 - tw/2, (int)titleY, 60, GOLD);
    
    float startX = sw/2.0f - 150.0f;
    float startY = sh/2.0f - 100.0f;

    if (currentState == MenuState::MAIN) {
        if (disconnectedByHost) {
            const char* msg = "CONNECTION LOST: HOST DISCONNECTED";
            int mw = MeasureText(msg, 25);
            DrawRectangle(sw/2 - mw/2 - 20, startY - 60, mw + 40, 40, Fade(BLACK, 0.8f));
            DrawText(msg, sw/2 - mw/2, startY - 52, 25, RED);
        }

        if (DrawButton((Rectangle){startX, startY, 300, 60}, "HOST MISSION", DARKGRAY)) {
            disconnectedByHost = false;
            currentState = MenuState::HOST;
        }
        if (DrawButton((Rectangle){startX, startY + 80, 300, 60}, "JOIN MISSION", DARKGRAY)) {
            disconnectedByHost = false;
            currentState = MenuState::JOIN;
        }
        if (DrawButton((Rectangle){startX, startY + 160, 300, 60}, "OPTIONS", DARKGRAY)) {
            disconnectedByHost = false;
            currentState = MenuState::OPTIONS;
        }
        if (DrawButton((Rectangle){startX, startY + 240, 300, 60}, "EXIT", MAROON)) {
             CloseWindow();
        }
    } else if (currentState == MenuState::HOST) {
        DrawText("ENTER HOSTNAME:", (int)startX, (int)startY - 40, 20, RAYWHITE);
        DrawRectangleRec((Rectangle){startX, startY, 300, 60}, WHITE);
        DrawText(hostName.c_str(), (int)startX + 10, (int)startY + 15, 30, BLACK);
        
        int key = GetCharPressed();
        while (key > 0) {
            if (hostName.length() < 20) hostName += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !hostName.empty()) hostName.pop_back();

        if (DrawButton((Rectangle){startX, startY + 80, 300, 60}, "START SERVER", GREEN)) {
            SaveSettings();
            shouldStartHost = true;
        }
        if (DrawButton((Rectangle){startX, startY + 160, 300, 60}, "BACK", DARKGRAY)) {
            SaveSettings();
            currentState = MenuState::MAIN;
        }
    } else if (currentState == MenuState::JOIN) {
        DrawText("ENTER IP ADDRESS:", (int)startX, (int)startY - 40, 20, RAYWHITE);
        DrawRectangleRec((Rectangle){startX, startY, 300, 60}, WHITE);
        DrawText(joinIP.c_str(), (int)startX + 10, (int)startY + 15, 30, BLACK);
        
        int key = GetCharPressed();
        while (key > 0) {
            if (joinIP.length() < 64) joinIP += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !joinIP.empty()) joinIP.pop_back();

        if (DrawButton((Rectangle){startX, startY + 80, 300, 60}, "CONNECT TO IP", GREEN)) {
            SaveSettings();
            shouldStartJoin = true;
        }
        
        DrawText("AVAILABLE LOBBIES:", (int)startX - 70, (int)startY + 160, 20, GOLD);
        DrawText(TextFormat("LOBBY SERVER: %s:%d", relayHost.c_str(), relayPort),
                 (int)startX - 115, (int)startY + 185, 16, LIGHTGRAY);
        // Note: Server list drawing moved to Game::Render to access NetworkManager,
        // but we leave space here for it.

        if (DrawButton((Rectangle){startX, (float)sh - 100, 300, 60}, "BACK", DARKGRAY)) {
            SaveSettings();
            currentState = MenuState::MAIN;
        }
    } else if (currentState == MenuState::OPTIONS) {
        DrawText("PLAYER NICKNAME:", (int)startX, (int)startY - 40, 20, GOLD);
        DrawRectangleRec((Rectangle){startX, startY, 300, 50}, WHITE);
        DrawText(playerNick.c_str(), (int)startX + 10, (int)startY + 10, 30, BLACK);
        
        int key = GetCharPressed();
        while (key > 0) {
            if (playerNick.length() < 16) playerNick += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !playerNick.empty()) playerNick.pop_back();

        DrawText("AUDIO SETTINGS", (int)startX, (int)startY + 80, 25, GOLD);
        
        DrawText(TextFormat("MASTER VOLUME: %d%%", (int)(GetMasterVolume() * 100)), (int)startX, (int)startY + 110, 20, RAYWHITE);
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (m.x > startX && m.x < startX + 300 && m.y > startY + 140 && m.y < startY + 160) {
                SetMasterVolume((m.x - startX) / 300.0f);
            }
        }
        DrawRectangle((int)startX, (int)startY + 140, 300, 20, BLACK);
        DrawRectangle((int)startX, (int)startY + 140, (int)(GetMasterVolume() * 300), 20, SKYBLUE);

        if (DrawButton((Rectangle){startX, startY + 180, 300, 40}, vsync ? "VSYNC: ON" : "VSYNC: OFF", vsync ? GREEN : MAROON)) {
            vsync = !vsync;
        }

        if (DrawButton((Rectangle){startX, startY + 230, 300, 40}, useWafelModel ? "MODEL: WAFEL" : "MODEL: NORMAL", useWafelModel ? YELLOW : DARKGRAY)) {
            useWafelModel = !useWafelModel;
        }

        if (DrawButton((Rectangle){startX, startY + 280, 300, 40}, "TOGGLE FULLSCREEN", DARKBLUE)) {
            shouldToggleFullscreen = true;
        }

        if (DrawButton((Rectangle){startX, startY + 330, 300, 40}, "RELAY SERVER", DARKPURPLE)) {
            SaveSettings();
            currentState = MenuState::RELAY_SETTINGS;
        }
        
        if (DrawButton((Rectangle){startX, startY + 380, 300, 60}, "BACK", DARKGRAY)) {
            SaveSettings();
            currentState = MenuState::MAIN;
        }
    } else if (currentState == MenuState::RELAY_SETTINGS) {
        DrawText("LOBBY / RELAY SERVER:", (int)startX - 25, (int)startY - 55, 20, GOLD);

        Rectangle hostBox = {startX - 80, startY, 460, 52};
        Rectangle portBox = {startX - 80, startY + 95, 180, 52};
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, hostBox)) relayEditField = 0;
            if (CheckCollisionPointRec(mouse, portBox)) relayEditField = 1;
        }
        if (IsKeyPressed(KEY_TAB)) relayEditField = (relayEditField + 1) % 2;

        DrawText("ADDRESS:", (int)hostBox.x, (int)hostBox.y - 25, 18, RAYWHITE);
        DrawRectangleRec(hostBox, relayEditField == 0 ? SKYBLUE : WHITE);
        DrawText(relayHost.c_str(), (int)hostBox.x + 10, (int)hostBox.y + 13, 24, BLACK);

        DrawText("PORT:", (int)portBox.x, (int)portBox.y - 25, 18, RAYWHITE);
        DrawRectangleRec(portBox, relayEditField == 1 ? SKYBLUE : WHITE);
        DrawText(TextFormat("%d", relayPort), (int)portBox.x + 10, (int)portBox.y + 13, 24, BLACK);

        int key = GetCharPressed();
        while (key > 0) {
            if (relayEditField == 0) {
                if (relayHost.length() < 128 && key >= 32 && key < 127) relayHost += (char)key;
            } else if (std::isdigit(key)) {
                std::string portText = TextFormat("%d", relayPort);
                if (relayPort == 0) portText.clear();
                if (portText.length() < 5) {
                    portText += (char)key;
                    try { relayPort = std::stoi(portText); } catch (...) { relayPort = 1240; }
                    if (relayPort > 65535) relayPort = 65535;
                }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (relayEditField == 0 && !relayHost.empty()) {
                relayHost.pop_back();
            } else if (relayEditField == 1) {
                std::string portText = TextFormat("%d", relayPort);
                if (!portText.empty()) portText.pop_back();
                relayPort = portText.empty() ? 0 : std::stoi(portText);
            }
        }

        DrawText("Used for internet lobby list and hosted lobby registration.", (int)startX - 110, (int)startY + 165, 16, LIGHTGRAY);
        
        if (DrawButton((Rectangle){startX, startY + 195, 300, 45}, "TEST CONNECTION", ORANGE)) {
            if (relayHost.empty()) relayHost = "127.0.0.1";
            if (relayPort <= 0) relayPort = 1240;
            SaveSettings();
            relayPingMs = -2.0f; // Pinging state
            shouldTestRelay = true;
        }

        if (relayPingMs == -2.0f) {
            DrawText("PINGING...", (int)startX + 105, (int)startY + 250, 18, YELLOW);
        } else if (relayPingMs >= 0.0f) {
            DrawText(TextFormat("PING: %.0f ms", relayPingMs), (int)startX + 100, (int)startY + 250, 18, GREEN);
        } else if (relayPingMs == -3.0f) {
            DrawText("FAILED", (int)startX + 120, (int)startY + 250, 18, RED);
        }

        if (DrawButton((Rectangle){startX, startY + 280, 300, 50}, "SAVE", GREEN)) {
            if (relayHost.empty()) relayHost = "127.0.0.1";
            if (relayPort <= 0) relayPort = 1240;
            SaveSettings();
            currentState = MenuState::OPTIONS;
        }
        if (DrawButton((Rectangle){startX, startY + 340, 300, 50}, "BACK", DARKGRAY)) {
            SaveSettings();
            currentState = MenuState::OPTIONS;
        }
    }
}

void Menu::SaveSettings() {
    std::ofstream f("settings.txt");
    if (f.is_open()) {
        f << playerNick << "\n";
        f << hostName << "\n";
        f << joinIP << "\n";
        f << GetMasterVolume() << "\n";
        f << vsync << "\n";
        f << resIndex << "\n";
        f << useWafelModel << "\n";
        f << relayHost << "\n";
        f << relayPort << "\n";
        f.close();
    }
}

void Menu::LoadSettings() {
    std::ifstream f("settings.txt");
    if (f.is_open()) {
        if (!std::getline(f, playerNick)) playerNick = "Gooner";
        if (!std::getline(f, hostName)) hostName = "MY SERVER";
        if (!std::getline(f, joinIP)) joinIP = "127.0.0.1";
        std::string volStr;
        if (std::getline(f, volStr)) {
            try { SetMasterVolume(std::stof(volStr)); } catch(...) {}
        }
        std::string vsyncStr, resStr, modelStr;
        if (std::getline(f, vsyncStr)) vsync = (vsyncStr == "1");
        if (std::getline(f, resStr)) try { resIndex = std::stoi(resStr); } catch(...) {}
        if (std::getline(f, modelStr)) useWafelModel = (modelStr == "1");
        if (!std::getline(f, relayHost)) relayHost = "127.0.0.1";
        std::string relayPortStr;
        if (std::getline(f, relayPortStr)) try { relayPort = std::stoi(relayPortStr); } catch(...) { relayPort = 1240; }
        f.close();
    }
}

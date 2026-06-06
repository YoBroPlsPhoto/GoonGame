#include "core/game.hpp"

void Game::RenderFinal() {
    // --- FINAL BLIT TO SCREEN ---
    BeginDrawing();
    ClearBackground(BLACK);

    // 1. Draw the world (scaled to current window resolution)
    DrawTexturePro(
        worldTarget.texture,
        (Rectangle){0, 0, (float)worldTarget.texture.width,
                    (float)-worldTarget.texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0, WHITE);

    // 2. Draw HUD and Menu at NATIVE RESOLUTION
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (state == GameState::MENU || state == GameState::LOBBY) {
      menu.Update();
      menu.Draw(state == GameState::MENU);
    }

    if (state == GameState::LOBBY) {
      // 1. Premium Dark Gradient Overlay
      DrawRectangleGradientV(0, 0, sw, sh, Fade(BLACK, 0.95f),
                             Fade(DARKBLUE, 0.4f));

      // 2. Title
      float time = GetTime();
      int titleW = MeasureText("MISSION BRIEFING - LOBBY", 45);
      DrawText("MISSION BRIEFING - LOBBY", sw / 2 - titleW / 2 + 3, 80 + 3, 45,
               Fade(BLACK, 0.5f));
      DrawText("MISSION BRIEFING - LOBBY", sw / 2 - titleW / 2, 80, 45, GOLD);
      DrawLine(sw / 2 - 250, 140, sw / 2 + 250, 140, Fade(GOLD, 0.5f));

      // --- MAP SELECTION (Left Pane) ---
      if (localPlayer.isAdmin) {
        DrawText("SELECT DEPLOYMENT ZONE", 100, 180, 25, SKYBLUE);
        DrawLine(100, 215, 400, 215, Fade(SKYBLUE, 0.5f));

        Rectangle cityRec = {100, 240, 400, 100};
        Rectangle desertRec = {100, 360, 400, 100};

        Vector2 m = GetMousePosition();
        bool hovCity = CheckCollisionPointRec(m, cityRec);
        bool hovDesert = CheckCollisionPointRec(m, desertRec);

        if (hovCity && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          delete currentMap;
          currentMap = new CityMap();
        }
        if (hovDesert && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          delete currentMap;
          currentMap = new DesertMap();
        }

        // City Rect Smooth Hover
        float animCity = hovCity ? (sinf(time * 8.0f) * 0.05f + 1.05f) : 1.0f;
        Rectangle drawCity = {
            cityRec.x - (cityRec.width * (animCity - 1.f)) / 2.f,
            cityRec.y - (cityRec.height * (animCity - 1.f)) / 2.f,
            cityRec.width * animCity, cityRec.height * animCity};
        Color cityBase = currentMap->type == MapType::CITY
                             ? (Color){20, 80, 40, 255}
                             : (Color){20, 20, 30, 255};
        DrawRectangleRounded(drawCity, 0.1f, 8,
                             hovCity ? RAYWHITE : Fade(cityBase, 0.8f));
        DrawRectangleRounded((Rectangle){drawCity.x + 3, drawCity.y + 3,
                                         drawCity.width - 6,
                                         drawCity.height - 6},
                             0.1f, 8, cityBase);

        DrawText("NEW YORK CITY", (int)drawCity.x + 20, (int)drawCity.y + 20,
                 25, WHITE);
        DrawText("Standard Urban Warfare", (int)drawCity.x + 20,
                 (int)drawCity.y + 55, 18,
                 (currentMap->type == MapType::CITY ? LIME : GRAY));

        // Desert Rect
        float animDes = hovDesert ? (sinf(time * 8.0f) * 0.03f + 1.03f) : 1.0f;
        Rectangle drawDes = {
            desertRec.x - (desertRec.width * (animDes - 1.f)) / 2.f,
            desertRec.y - (desertRec.height * (animDes - 1.f)) / 2.f,
            desertRec.width * animDes, desertRec.height * animDes};
        Color desBase = currentMap->type == MapType::DESERT
                            ? (Color){100, 80, 20, 255}
                            : (Color){20, 20, 30, 255};
        DrawRectangleRounded(drawDes, 0.1f, 8,
                             hovDesert ? RAYWHITE : Fade(desBase, 0.8f));
        DrawRectangleRounded((Rectangle){drawDes.x + 3, drawDes.y + 3,
                                         drawDes.width - 6, drawDes.height - 6},
                             0.1f, 8, desBase);

        DrawText("DESERT (WIP)", (int)drawDes.x + 20, (int)drawDes.y + 20, 25,
                 LIGHTGRAY);
        DrawText("Open Range Combat", (int)drawDes.x + 20, (int)drawDes.y + 55,
                 18, DARKGRAY);
      }

      // --- BACK/DISCONNECT BUTTON ---
      Rectangle backBtn = {(float)sw - 200, 40, 150, 45};
      Vector2 mPos = GetMousePosition();
      bool hovBack = CheckCollisionPointRec(mPos, backBtn);
      if (hovBack && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        net.SendDisconnect(localPlayer.playerId);
        net.shouldQuit = true;
        menu.currentState = MenuState::MAIN;
      }
      DrawRectangleRounded(backBtn, 0.2f, 8,
                           hovBack ? Fade(MAROON, 0.8f) : Fade(BLACK, 0.6f));
      DrawRectangleRounded((Rectangle){backBtn.x + 2, backBtn.y + 2,
                                       backBtn.width - 4, backBtn.height - 4},
                           0.2f, 8, hovBack ? RED : Fade(DARKGRAY, 0.4f));
      DrawText("LEAVE LOBBY", (int)backBtn.x + 18, (int)backBtn.y + 13, 18,
               RAYWHITE);

      // --- SQUAD STATUS (Right Pane) ---
      DrawText("SQUAD STATUS", sw - 450, 180, 25, SKYBLUE);
      DrawLine(sw - 450, 215, sw - 100, 215, Fade(SKYBLUE, 0.5f));

      int py = 240;
      DrawRectangleRounded(
          (Rectangle){(float)sw - 460, (float)py - 10, 360, 50}, 0.1f, 8,
          Fade(BLACK, 0.6f));
      DrawText(TextFormat("CMD - YOU (ID: %d)", localPlayer.playerId), sw - 440,
               py, 25, GOLD);
      py += 60;
      for (auto const &[id, p] : net.remotePlayers) {
        if (!p.active)
          continue;
        DrawRectangleRounded(
            (Rectangle){(float)sw - 460, (float)py - 10, 360, 50}, 0.1f, 8,
            Fade(BLACK, 0.6f));
        DrawText(TextFormat("OPR - %s (ID: %d)", p.name, id), sw - 440, py, 25,
                 RAYWHITE);
        py += 60;
      }

      // --- EXECUTE BUTTON ---
      if (localPlayer.isAdmin) {
        Rectangle startBtn = {(float)sw / 2 - 200, (float)sh - 140, 400, 70};
        bool hov = CheckCollisionPointRec(GetMousePosition(), startBtn);
        if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
          state = GameState::GAME;

        float pulseBtn = hov ? (sinf(time * 12.0f) * 0.05f + 1.05f) : 1.0f;
        Rectangle drawBtn = {
            startBtn.x - (startBtn.width * (pulseBtn - 1.f)) / 2.f,
            startBtn.y - (startBtn.height * (pulseBtn - 1.f)) / 2.f,
            startBtn.width * pulseBtn, startBtn.height * pulseBtn};

        DrawRectangleRounded(drawBtn, 0.3f, 8,
                             hov ? WHITE : Fade(MAROON, 0.8f));
        DrawRectangleRounded((Rectangle){drawBtn.x + 3, drawBtn.y + 3,
                                         drawBtn.width - 6, drawBtn.height - 6},
                             0.3f, 8, hov ? RED : MAROON);

        int btnW = MeasureText("EXECUTE MISSION", 28);
        DrawText("EXECUTE MISSION",
                 (int)(drawBtn.x + drawBtn.width / 2 - btnW / 2),
                 (int)(drawBtn.y + drawBtn.height / 2 - 14), 28, WHITE);
      } else {
        float pulse = sinf(time * 3.0f) * 0.2f + 0.8f;
        int textW = MeasureText("WAITING FOR COMMANDER...", 25);
        DrawText("WAITING FOR COMMANDER...", sw / 2 - textW / 2, sh - 110, 25,
                 Fade(RAYWHITE, pulse));
      }
    }

    if (state == GameState::MENU) {

      if (menu.shouldStartHost) {
        // Try relay first, fallback to direct
        bool started = net.StartServerRelay(menu.hostName);
        if (!started) started = net.StartServerAutoPort(1234, 10, menu.hostName);
        if (started) state = GameState::LOBBY;
        menu.shouldStartHost = false;
        localPlayer.isAdmin = true;
      }
      if (menu.shouldStartJoin) {
        if (net.StartClient(menu.joinIP, menu.joinPort > 0 ? menu.joinPort : 1234))
          state = GameState::LOBBY;
        menu.shouldStartJoin = false;
      }

      if (menu.currentState == MenuState::JOIN) {
        int listY = 480;
        for (auto const &[ip, info] : net.discoveredServers) {
          Rectangle rec = {(float)GetScreenWidth() / 2 - 150, (float)listY, 340,
                           45};
          bool hov = CheckCollisionPointRec(GetMousePosition(), rec);
          Color bgCol = info.relayRoomId != 0 ? DARKPURPLE : DARKBLUE;
          DrawRectangleRounded(rec, 0.2f, 8,
                               Fade(hov ? SKYBLUE : bgCol, 0.8f));
          const char* tag = info.relayRoomId != 0 ? "[RELAY]" : (info.internet ? "[NET]" : "[LAN]");
          DrawText(TextFormat("%s %s (%s)", tag, info.name.c_str(), info.ip.c_str()),
                   (int)rec.x + 10, (int)rec.y + 5, 16, RAYWHITE);
          DrawText(TextFormat("%d/%d players%s", info.players, info.maxPlayers,
                              info.started ? " [IN GAME]" : ""),
                   (int)rec.x + 10, (int)rec.y + 25, 14, LIGHTGRAY);
          if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            bool joined = false;
            if (info.relayRoomId != 0) {
              joined = net.StartClientRelay(info.relayRoomId);
            } else {
              joined = net.StartClient(info.ip, info.port);
            }
            if (joined) state = GameState::LOBBY;
          }
          listY += 55;
          if (listY > GetScreenHeight() - 150)
            break;
        }
      }
    }

    EndDrawing();
  }
}

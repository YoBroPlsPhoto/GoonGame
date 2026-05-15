#include "core/game.hpp"

void Game::RenderHUD() {
      // --- Enemy HUD Pass ---
      if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies) {
          if (e->type != EnemyType::BOSS && e->type != EnemyType::GIBON_BOSS &&
              e->type != EnemyType::GANG_BOSS &&
              e->type != EnemyType::ADAS_PRIME) {
            e->DrawHUD(activeCam);
          }
        }
      } else {
        for (auto const &[id, e] : net.syncedEnemies) {
          std::shared_ptr<Enemy> temp;
          if (e.type == (int)EnemyType::BOSS) {
            auto adas = std::make_shared<AdasGooner>(e.pos, e.id);
            adas->cutsceneState = (CutsceneState)(int)e.attackTimer;
            adas->cutsceneTimer = e.walkTimer;
            temp = adas;
          } else if (e.type == (int)EnemyType::GIBON_BOSS) {
            auto gibon = std::make_shared<GibonRzygacz>(e.pos, e.id);
            gibon->gibonState = (GibonState)(int)e.attackTimer;
            gibon->stateTimer = e.walkTimer;
            temp = gibon;
          } else if (e.type == (int)EnemyType::GANG_BOSS) {
            auto gang = std::make_shared<GangBoss>(e.pos, e.id);
            gang->cutsceneState = (GangCutscene)(int)e.attackTimer;
            gang->stateTimer = e.walkTimer;
            temp = gang;
          } else {
            temp = std::make_shared<Enemy>(e.pos, (EnemyType)e.type,
                                           (WeaponType)e.weapon, e.id);
            temp->angle = e.angle;
            temp->isMoving = e.isMoving;
            temp->walkTimer = e.walkTimer;
            temp->attackTimer = e.attackTimer;
          }
          temp->hp = e.hp;
          temp->maxHp = e.maxHp;

          // Only draw individual HUD if not a boss (bosses get global bar)
          if (temp->type != EnemyType::BOSS &&
              temp->type != EnemyType::GIBON_BOSS &&
              temp->type != EnemyType::GANG_BOSS &&
              temp->type != EnemyType::ADAS_PRIME) {
            temp->DrawHUD(activeCam);
          }
        }
      }

      // --- Structure HUD Pass (Health Bars) ---
      for (auto &s : structures) {
        if (!s->active || s->hp <= 0)
          continue;
        float viewDist = Vector3Distance(s->position, activeCam.position);
        if (viewDist > 80.0f)
          continue;

        float barHeight = (s->type == StructureType::TURRET) ? 5.5f : 5.0f;
        Vector3 barPos = {s->position.x, s->position.y + barHeight,
                          s->position.z};

        // Don't render if behind camera
        Vector3 toStruct = Vector3Subtract(barPos, activeCam.position);
        Vector3 camFwd = Vector3Subtract(activeCam.target, activeCam.position);
        if (Vector3DotProduct(toStruct, camFwd) < 0)
          continue;

        Vector2 screenPos = GetWorldToScreen(barPos, activeCam);
        float hpRatio =
            std::max(0.0f, std::min(1.0f, (float)s->hp / (float)s->maxHp));

        // Scale bar size by distance
        float distScale = std::max(0.5f, 1.0f - (viewDist - 20.0f) / 100.0f);
        float bW = 50.0f * distScale;
        float bH = 6.0f * distScale;

        // Background
        DrawRectangle((int)(screenPos.x - bW / 2 - 1),
                      (int)(screenPos.y - bH - 1), (int)(bW + 2), (int)(bH + 2),
                      Fade(BLACK, 0.8f));

        // HP bar fill
        Color barColor;
        if (s->type == StructureType::WALL) {
          barColor = hpRatio > 0.5f ? (Color){50, 180, 80, 255}
                                    : (hpRatio > 0.25f ? ORANGE : RED);
        } else {
          barColor = hpRatio > 0.5f
                         ? (Color){220, 150, 30, 255}
                         : (hpRatio > 0.25f ? (Color){220, 100, 30, 255} : RED);
        }
        DrawRectangle((int)(screenPos.x - bW / 2), (int)(screenPos.y - bH),
                      (int)(bW * hpRatio), (int)bH, barColor);

        // Type label
        const char *label =
            (s->type == StructureType::WALL) ? "WALL" : "TURRET";
        int fontSize = std::max(8, (int)(12 * distScale));
        int labelWidth = MeasureText(label, fontSize);
        DrawText(label, (int)(screenPos.x - labelWidth / 2),
                 (int)(screenPos.y - bH - fontSize - 2), fontSize,
                 s->type == StructureType::WALL ? (Color){180, 150, 100, 255}
                                                : (Color){220, 160, 50, 255});

        // HP text
        if (viewDist < 40.0f) {
          const char *hpText = TextFormat("%d/%d", s->hp, s->maxHp);
          int hpFontSize = std::max(7, (int)(9 * distScale));
          int hpWidth = MeasureText(hpText, hpFontSize);
          DrawText(hpText, (int)(screenPos.x - hpWidth / 2),
                   (int)(screenPos.y + 2), hpFontSize, RAYWHITE);
        }
      }

      // UI / HUD - UNIFIED SYSTEM
      DrawRectangle(10, 10, 580, 120, Fade(BLACK, 0.7f));
      DrawText(TextFormat("FPS: %d | ULTRA GRAPHICS", GetFPS()), 25, 20, 15,
               LIME);

      // --- SNIPER SCOPE OVERLAY ---
      if (fovToUse < preferredFOV) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float radius = sh * 0.45f;

        // Solid black background for everything outside the scope area
        // Left
        DrawRectangle(0, 0, (float)sw / 2.0f - radius + 1.0f, sh, BLACK);
        // Right
        DrawRectangle((float)sw / 2.0f + radius - 1.0f, 0, sw, sh, BLACK);
        // Top
        DrawRectangle(0, 0, sw, (float)sh / 2.0f - radius + 1.0f, BLACK);
        // Bottom
        DrawRectangle(0, (float)sh / 2.0f + radius - 1.0f, sw, sh, BLACK);

        // Outer vignette smoothing (rounds the square hole)
        DrawCircleGradient((Vector2){(float)sw / 2.0f, (float)sh / 2.0f},
                           radius + 2.0f, Fade(BLACK, 0), BLACK);

        // Sniper Crosshair
        DrawLine((float)sw / 2.0f - radius, (float)sh / 2.0f,
                 (float)sw / 2.0f + radius, (float)sh / 2.0f, BLACK);
        DrawLine((float)sw / 2.0f, (float)sh / 2.0f - radius, (float)sw / 2.0f,
                 (float)sh / 2.0f + radius, BLACK);

        // Center dot
        DrawCircle((float)sw / 2.0f, (float)sh / 2.0f, 2, RED);
      } else {
        // CROSSHAIR SYSTEM
        if (localPlayer.inVehicle) {
          Tank *tank =
              dynamic_cast<Tank *>(vehicles[localPlayer.vehicleIndex].get());
          if (tank) {
            int sw = GetScreenWidth();
            int sh = GetScreenHeight();
            int cx = sw / 2;
            int cy = sh / 2;

            // Tactical Tank Crosshair
            DrawCircleLines(cx, cy, 40, LIME);
            DrawCircleLines(cx, cy, 42, Fade(GREEN, 0.3f));

            // Horizontal lines
            DrawLine(cx - 60, cy, cx - 15, cy, LIME);
            DrawLine(cx + 15, cy, cx + 60, cy, LIME);

            // Vertical lines
            DrawLine(cx, cy - 60, cx, cy - 15, LIME);
            DrawLine(cx, cy + 15, cx, cy + 60, LIME);

            // Corner brackets
            int b = 25;
            DrawLine(cx - b, cy - b, cx - b + 10, cy - b, LIME);
            DrawLine(cx - b, cy - b, cx - b, cy - b + 10, LIME);

            DrawLine(cx + b, cy - b, cx + b - 10, cy - b, LIME);
            DrawLine(cx + b, cy - b, cx + b, cy - b + 10, LIME);

            DrawLine(cx - b, cy + b, cx - b + 10, cy + b, LIME);
            DrawLine(cx - b, cy + b, cx - b, cy + b - 10, LIME);

            DrawLine(cx + b, cy + b, cx + b - 10, cy + b, LIME);
            DrawLine(cx + b, cy + b, cx + b, cy + b - 10, LIME);

            // Distance/Status info
            DrawText("READY", cx + 50, cy + 50, 15, LIME);
            if (tank->cannonCooldown > 0) {
              float p = 1.0f - (tank->cannonCooldown / 1.2f);
              DrawRectangle(cx - 30, cy + 45, 60, 5, DARKGRAY);
              DrawRectangle(cx - 30, cy + 45, (int)(60 * p), 5, ORANGE);
              DrawText("RELOADING", cx - 40, cy + 55, 12, ORANGE);
            }
          } else {
            // Normal vehicle crosshair (none or default)
            DrawLine(GetScreenWidth() / 2 - 10, GetScreenHeight() / 2,
                     GetScreenWidth() / 2 + 10, GetScreenHeight() / 2, LIME);
            DrawLine(GetScreenWidth() / 2, GetScreenHeight() / 2 - 10,
                     GetScreenWidth() / 2, GetScreenHeight() / 2 + 10, LIME);
          }
        } else {
          // NORMAL CROSSHAIR
          DrawLine(GetScreenWidth() / 2 - 10, GetScreenHeight() / 2,
                   GetScreenWidth() / 2 + 10, GetScreenHeight() / 2, LIME);
          DrawLine(GetScreenWidth() / 2, GetScreenHeight() / 2 - 10,
                   GetScreenWidth() / 2, GetScreenHeight() / 2 + 10, LIME);
        }
      }

      // PLAYER HP
      int displayPlayerHp = std::max(0, localPlayer.hp);
      float playerRatio = std::max(
          0.0f, std::min(1.0f, localPlayer.hp / (float)localPlayer.maxHp));
      DrawText(
          TextFormat("PLAYER HP: %d / %d", displayPlayerHp, localPlayer.maxHp),
          25, 45, 18, RAYWHITE);
      DrawRectangle(320, 45, 250, 15, DARKGRAY);
      DrawRectangle(320, 45, (int)(250 * playerRatio), 15, RED);

      // VEHICLE HP
      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
        auto v = vehicles[localPlayer.vehicleIndex];
        int displayVehHp = std::max(0, v->health);
        float vehRatio =
            std::max(0.0f, std::min(1.0f, v->health / (float)v->maxHealth));
        DrawText(TextFormat("VEHICLE HP: %d / %d", displayVehHp, v->maxHealth),
                 25, 65, 18, SKYBLUE);
        DrawRectangle(320, 65, 250, 15, DARKGRAY);
        DrawRectangle(320, 65, (int)(250 * vehRatio), 15, BLUE);
      }

      // BASE HP
      float baseRatio = std::max(0.0f, std::min(1.0f, baseHP / maxBaseHP));
      DrawText(TextFormat("BASE HP: %d / %d", (int)std::max(0.0f, baseHP),
                          (int)maxBaseHP),
               25, 80, 20, GOLD);
      DrawRectangle(320, 80, 250, 20, DARKGRAY);
      DrawRectangle(320, 80, (int)(250 * baseRatio), 20, YELLOW);

      // WAVE INFO
      DrawText(TextFormat("WAVE: %d", currentWave), 25, 110, 20, ORANGE);
      if (!waveActive) {
        DrawText(TextFormat("NEXT WAVE IN: %.1fs", waveWaitTimer), 180, 110, 20,
                 SKYBLUE);
      } else {
        DrawText(TextFormat("ENEMIES: %d", (int)enemies.size()), 180, 110, 20,
                 LIGHTGRAY);
      }
      if (!structures.empty()) {
        DrawText(TextFormat("DEFENSES: %d", (int)structures.size()), 400, 110,
                 20, (Color){180, 150, 100, 255});
      }

      // --- GLOBAL BOSS HP BAR ---
      std::shared_ptr<Enemy> activeBoss = nullptr;
      if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies) {
          if (e->type == EnemyType::BOSS || e->type == EnemyType::GIBON_BOSS ||
              e->type == EnemyType::GANG_BOSS ||
              e->type == EnemyType::ADAS_PRIME) {
            activeBoss = e;
            break;
          }
        }
      } else {
        // Client side boss detection
        for (auto const &[id, e] : net.syncedEnemies) {
          if (e.type == (int)EnemyType::BOSS ||
              e.type == (int)EnemyType::GIBON_BOSS ||
              e.type == (int)EnemyType::GANG_BOSS ||
              e.type == (int)EnemyType::ADAS_PRIME) {
            activeBoss = std::make_shared<Enemy>(e.pos, (EnemyType)e.type,
                                                 WeaponType::KATANA, e.id);
            activeBoss->hp = e.hp;
            activeBoss->maxHp = e.maxHp;
            break;
          }
        }
      }

      if (activeBoss && activeBoss->hp > 0) {
        int sw = GetScreenWidth();
        int bw = 600;
        int bh = 30;
        int bx = sw / 2 - bw / 2;
        int by = 50;

        float bossHpRatio = (float)activeBoss->hp / (float)activeBoss->maxHp;
        DrawRectangle(bx - 4, by - 4, bw + 8, bh + 8, BLACK);
        DrawRectangle(bx, by, bw, bh, DARKGRAY);
        DrawRectangle(bx, by, (int)(bw * bossHpRatio), bh, RED);

        const char *bossName = "BOSS";
        if (activeBoss->type == EnemyType::BOSS)
          bossName = "ADAS GOONER";
        else if (activeBoss->type == EnemyType::GIBON_BOSS)
          bossName = "GIBON RZYGACZ";
        else if (activeBoss->type == EnemyType::GANG_BOSS)
          bossName = "THE GANG";
        else if (activeBoss->type == EnemyType::ADAS_PRIME)
          bossName = "ADAS PRIME";

        int tw = MeasureText(bossName, 25);
        DrawText(bossName, sw / 2 - tw / 2, by - 30, 25, GOLD);

        const char *hpText =
            TextFormat("%d / %d", activeBoss->hp, activeBoss->maxHp);
        int htw = MeasureText(hpText, 15);
        DrawText(hpText, sw / 2 - htw / 2, by + 7, 15, WHITE);
      }

      if (baseHP <= 0) {
        DrawText("BASE LOST - NYC HAS FALLEN", GetScreenWidth() / 2 - 200,
                 GetScreenHeight() / 2, 30, RED);
      }

      // MONEY HUD
      DrawText(TextFormat("CASH: $%d", localPlayer.money), 25, 140, 25, GREEN);

      // AMMO HUD (bottom-right)
      if (localPlayer.currentWeapon && localPlayer.currentWeapon->magSize > 0) {
        auto *w = localPlayer.currentWeapon;
        const char *ammoText =
            TextFormat("%d / %d", w->currentAmmo, w->magSize);
        const char *reserveText = TextFormat("RESERVE: %d", w->reserveAmmo);
        int sw2 = GetScreenWidth();
        int sh2 = GetScreenHeight();

        // Background
        DrawRectangle(sw2 - 220, sh2 - 80, 210, 70, Fade(BLACK, 0.7f));

        // Ammo count (big)
        Color ammoColor = (w->currentAmmo > 0) ? WHITE : RED;
        DrawText(ammoText, sw2 - 210, sh2 - 70, 35, ammoColor);

        // Reserve (smaller)
        DrawText(reserveText, sw2 - 210, sh2 - 30, 16, LIGHTGRAY);

        // Reload indicator
        if (w->isReloading) {
          float progress = 1.0f - (w->reloadTimer / w->reloadTime);
          DrawRectangle(sw2 - 220, sh2 - 90, 210, 8, DARKGRAY);
          DrawRectangle(sw2 - 220, sh2 - 90, (int)(210 * progress), 8, YELLOW);
          DrawText("RELOADING...", sw2 / 2 - 60, sh2 / 2 + 50, 20, YELLOW);
        }
      } else if (localPlayer.currentWeapon &&
                 (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0 ||
                  strncmp(localPlayer.currentWeapon->name, "WALL", 4) == 0)) {
        // Builder tool placement count HUD
        auto *w = localPlayer.currentWeapon;
        int sw2 = GetScreenWidth();
        int sh2 = GetScreenHeight();
        bool isTurret = (strncmp(w->name, "TURRET", 6) == 0);

        Color bgCol = isTurret ? Fade((Color){80, 20, 20, 255}, 0.8f)
                               : Fade((Color){60, 40, 20, 255}, 0.8f);
        DrawRectangle(sw2 - 220, sh2 - 80, 210, 70, bgCol);

        int totalPlacements = w->currentAmmo + w->reserveAmmo;
        const char *placeText = TextFormat("PLACEMENTS: %d", totalPlacements);
        Color placeColor = (totalPlacements > 0)
                               ? (isTurret ? (Color){255, 150, 50, 255}
                                           : (Color){200, 170, 120, 255})
                               : RED;
        DrawText(placeText, sw2 - 210, sh2 - 70, 25, placeColor);

        const char *typeText = isTurret ? "TURRET SCHEMATIC" : "WALL SCHEMATIC";
        DrawText(typeText, sw2 - 210, sh2 - 35, 14, LIGHTGRAY);

        // LMB to place prompt
        DrawText("LMB TO PLACE", sw2 - 210, sh2 - 18, 12, LIME);
      }

      // --- CS STYLE VERTICAL/HORIZONTAL EQUIPMENT HUD ---
      if (!localPlayer.showInventory) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int eqWidth = 150;
        int eqHeight = 35;
        int spacing = 5;

        std::vector<Weapon *> categories[5];
        for (auto *w : localPlayer.inventory) {
          if (w->magSize == 30 || w->magSize == 200)
            categories[1].push_back(w);
          else if (w->magSize == 17 || w->magSize == 6)
            categories[2].push_back(w);
          else if (w->magSize == 0)
            categories[3].push_back(w);
          else if (w->magSize == 8 || w->magSize == 5 || w->magSize == 1)
            categories[4].push_back(w);
        }

        int numActiveCategories = 0;
        for (int c = 1; c <= 4; c++) {
          if (!categories[c].empty())
            numActiveCategories++;
        }

        int startY = sh / 2 - (numActiveCategories * (eqHeight + spacing)) / 2;
        int currentY = startY;

        for (int c = 1; c <= 4; c++) {
          if (categories[c].empty())
            continue;

          for (size_t i = 0; i < categories[c].size(); i++) {
            auto *w = categories[c][i];
            bool isEquipped = (w == localPlayer.currentWeapon);

            // Same category weapons expand horizontally to the left
            Rectangle slot = {
                (float)(sw - eqWidth - 20 - i * (eqWidth + spacing)),
                (float)currentY, (float)eqWidth, (float)eqHeight};

            DrawRectangleRec(slot, isEquipped ? Fade(DARKGRAY, 0.9f)
                                              : Fade(BLACK, 0.4f));
            if (isEquipped)
              DrawRectangleLinesEx(slot, 2, LIME);

            const char *wName = "WEAPON";
            if (w->magSize == 17)
              wName = "GLOCK";
            else if (w->magSize == 30)
              wName = "AK47";
            else if (w->magSize == 6)
              wName = "REVOLVR";
            else if (w->magSize == 8)
              wName = "SHOTGUN";
            else if (w->magSize == 5)
              wName = "AWP";
            else if (w->magSize == 200)
              wName = "MINIGUN";
            else if (w->magSize == 1)
              wName = "RPG";
            else if (w->magSize == 0)
              wName = "KNIFE";

            DrawText(TextFormat("[%d] %s", c, wName), slot.x + 10, slot.y + 10,
                     16, isEquipped ? WHITE : GRAY);
          }
          currentY += (eqHeight + spacing);
        }
      }

      // Interaction Prompt
      if (shopNearby) {
        DrawText("PRESS [E] TO BUY", GetScreenWidth() / 2 - 80,
                 GetScreenHeight() / 2 + 30, 20, GOLD);
      }

      // --- DEATH TIMER ---
      if (localPlayer.isDead) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Fade(BLACK, 0.5f));
        const char *deathMsg = "YOU DIED";
        int tw = MeasureText(deathMsg, 60);
        DrawText(deathMsg, GetScreenWidth() / 2 - tw / 2,
                 GetScreenHeight() / 2 - 60, 60, RED);

        const char *respawnMsg =
            TextFormat("RESPAWNING IN %.1fs", localPlayer.respawnTimer);
        int rtw = MeasureText(respawnMsg, 30);
        DrawText(respawnMsg, GetScreenWidth() / 2 - rtw / 2,
                 GetScreenHeight() / 2 + 20, 30, RAYWHITE);
      }

      // --- Kebab Interaction ---
      Vector3 bp = {0, 0, 150}; // mainBasePos
      Vector3 standPos = {bp.x + 15, 0, bp.z + 10};

      if (Vector3Distance(localPlayer.position,
                          {standPos.x - 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY SMALL KEBAB ($50, +10 HP)",
                 GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 50 &&
            localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 50;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 10);
        }
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY MEDIUM KEBAB ($100, +25 HP)",
                 GetScreenWidth() / 2 - 220, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 100 &&
            localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 100;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 25);
        }
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x + 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY LARGE KEBAB ($150, +50 HP)",
                 GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 150 &&
            localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 150;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 50);
        }
      }

      // --- INVENTORY UI ---
      if (state == GameState::GAME && localPlayer.showInventory) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));
        DrawText("INVENTORY", sw / 2 - MeasureText("INVENTORY", 40) / 2, 50, 40,
                 GOLD);

        int startX = sw / 2 - 400;
        int startY = 150;
        for (size_t i = 0; i < localPlayer.inventory.size(); i++) {
          int row = i / 4;
          int col = i % 4;
          Rectangle slot = {(float)(startX + col * 200),
                            (float)(startY + row * 150), 180, 120};
          bool isHovered = CheckCollisionPointRec(GetMousePosition(), slot);
          bool isEquipped =
              (localPlayer.currentWeapon == localPlayer.inventory[i]);

          DrawRectangleRec(slot, isEquipped
                                     ? Fade(DARKGREEN, 0.6f)
                                     : (isHovered ? Fade(GRAY, 0.6f)
                                                  : Fade(DARKGRAY, 0.6f)));
          DrawRectangleLinesEx(slot, 2,
                               isEquipped ? LIME : (isHovered ? WHITE : BLACK));

          const char *wName = "WEAPON";
          if (localPlayer.inventory[i]->magSize == 17)
            wName = "GLOCK";
          else if (localPlayer.inventory[i]->magSize == 30)
            wName = "AK47";
          else if (localPlayer.inventory[i]->magSize == 6)
            wName = "REVOLVER";
          else if (localPlayer.inventory[i]->magSize == 8)
            wName = "SHOTGUN";
          else if (localPlayer.inventory[i]->magSize == 10)
            wName = "AWP";
          else if (localPlayer.inventory[i]->magSize == 100)
            wName = "MINIGUN";
          else if (localPlayer.inventory[i]->magSize == 1)
            wName = "RPG";
          else if (localPlayer.inventory[i]->magSize == 0)
            wName = "KNIFE";

          DrawText(wName, slot.x + 10, slot.y + 10, 20, WHITE);
          if (localPlayer.inventory[i]->magSize > 0) {
            DrawText(
                TextFormat("AMMO: %d", localPlayer.inventory[i]->currentAmmo),
                slot.x + 10, slot.y + 40, 15, RAYWHITE);
            DrawText(
                TextFormat("RES: %d", localPlayer.inventory[i]->reserveAmmo),
                slot.x + 10, slot.y + 60, 15, LIGHTGRAY);
          } else {
            DrawText("MELEE / NO AMMO", slot.x + 10, slot.y + 40, 15, GRAY);
          }

          if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (localPlayer.currentWeapon != localPlayer.inventory[i]) {
              localPlayer.previousWeapon = localPlayer.currentWeapon;
            }
            localPlayer.currentWeapon = localPlayer.inventory[i];
          }
        }
        DrawText("PRESS ~ TO CLOSE",
                 sw / 2 - MeasureText("PRESS ~ TO CLOSE", 20) / 2, sh - 50, 20,
                 LIGHTGRAY);
      }

      if (state == GameState::PAUSED) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.6f));

        Rectangle menuBox = {(float)sw / 2 - 200, (float)sh / 2 - 250, 400,
                             500};
        DrawRectangleRounded(menuBox, 0.1f, 8, Fade(DARKGRAY, 0.9f));
        DrawRectangleLinesEx(menuBox, 2, GOLD);
        DrawText("CONTROL TERMINAL", menuBox.x + 80, menuBox.y + 30, 25, GOLD);

        float menuY = menuBox.y + 100;
        auto menuBtn = [&](const char *text, Color col) {
          Rectangle r = {menuBox.x + 50, menuY, 300, 50};
          bool hov = CheckCollisionPointRec(GetMousePosition(), r);
          DrawRectangleRounded(r, 0.2f, 8,
                               hov ? Fade(col, 0.8f) : Fade(BLACK, 0.5f));
          DrawText(text, r.x + 150 - MeasureText(text, 20) / 2, r.y + 15, 20,
                   WHITE);
          menuY += 70;
          return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        };

        if (menuBtn("RESUME MISSION", DARKGREEN))
          state = GameState::GAME;

        if (localPlayer.showSettings) {
          DrawText("SETTINGS", menuBox.x + 50, menuY - 40, 20, SKYBLUE);
          DrawText(TextFormat("FOV: %d", (int)preferredFOV), menuBox.x + 50,
                   menuY, 18, RAYWHITE);
          Rectangle fovBar = {menuBox.x + 50, menuY + 25, 300, 10};
          DrawRectangleRec(fovBar, BLACK);
          DrawRectangle(fovBar.x, fovBar.y,
                        (int)((preferredFOV - 40.0f) / 80.0f * 300.0f), 10,
                        SKYBLUE);
          if (CheckCollisionPointRec(GetMousePosition(), fovBar) &&
              IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            preferredFOV = 40.0f + ((GetMouseX() - fovBar.x) / 300.0f) * 80.0f;
            if (preferredFOV < 40)
              preferredFOV = 40;
            if (preferredFOV > 120)
              preferredFOV = 120;
          }
          menuY += 60;
          if (menuBtn("BACK", DARKGRAY))
            localPlayer.showSettings = false;
        } else {
          if (menuBtn("OPTIONS", DARKBLUE))
            localPlayer.showSettings = true;

          bool isAdmin = (net.mode == NetworkManager::Mode::SERVER);
          if (isAdmin) {
            if (menuBtn("ADMIN SETTINGS", MAROON)) {
              localPlayer.isAdmin = true;
              localPlayer.isFlying = !localPlayer.isFlying;
            }
          }

          if (menuBtn("DISCONNECT", RED)) {
            net.SendDisconnect(localPlayer.playerId);
            net.shouldQuit = true;
            menu.currentState = MenuState::MAIN;
          }
        }
      }
    }

    EndTextureMode();

}

#include "core/game.hpp"

void Game::ApplySettings() {
    // --- APPLY SETTINGS AT START OF FRAME ---
    SetTargetFPS(menu.vsync ? 60 : 144);

    // Auto-scale target to match current window (always native)
    int targetW = GetScreenWidth();
    int targetH = GetScreenHeight();
    if (targetW <= 0)
      targetW = 800;
    if (targetH <= 0)
      targetH = 600;
    if (worldTarget.texture.width != targetW ||
        worldTarget.texture.height != targetH) {
      UnloadRenderTexture(worldTarget);
      worldTarget = LoadRenderTexture(targetW, targetH);
      SetTextureFilter(worldTarget.texture, TEXTURE_FILTER_BILINEAR);
    }

    if (menu.shouldToggleFullscreen) {
      ToggleFullscreen();
      menu.shouldToggleFullscreen = false;
    }

    if (net.shouldQuit) {
      state = GameState::MENU;
      net.shouldQuit = false;
      net.mode = NetworkManager::Mode::NONE;
    }

    bool inCutscene = false;
    Vector3 bossPos = {0};
    std::shared_ptr<Enemy> activeBoss = nullptr;

    if (state == GameState::GAME || state == GameState::PAUSED) {
      if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies) {
          if (e->type == EnemyType::BOSS) {
            auto boss = std::dynamic_pointer_cast<AdasGooner>(e);
            if (boss && boss->cutsceneState != CutsceneState::FINISHED) {
              inCutscene = true;
              bossPos = boss->wardrobePos;
              activeBoss = boss;
              break;
            }
          }
          if (e->type == EnemyType::GIBON_BOSS) {
            auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
            if (gibon && gibon->gibonState != GibonState::FINISHED_FALLING) {
              inCutscene = true;
              bossPos = gibon->position;
              activeBoss = gibon;
              break;
            }
          }
          if (e->type == EnemyType::GANG_BOSS) {
            auto gang = std::dynamic_pointer_cast<GangBoss>(e);
            if (gang && gang->cutsceneState != GangCutscene::FIGHT) {
              inCutscene = true;
              bossPos = gang->wardrobePos;
              activeBoss = gang;
              break;
            }
          }
        }
      } else {
        for (auto const &[id, e] : net.syncedEnemies) {
          if (e.type == (int)EnemyType::BOSS &&
              e.attackTimer != (float)(int)CutsceneState::FINISHED) {
            inCutscene = true;
            bossPos = e.pos;
            break;
          }
          if (e.type == (int)EnemyType::GIBON_BOSS &&
              e.attackTimer != (float)(int)GibonState::FINISHED_FALLING) {
            inCutscene = true;
            bossPos = e.pos;
            break;
          }
          if (e.type == (int)EnemyType::GANG_BOSS &&
              e.attackTimer != (float)(int)GangCutscene::FIGHT) {
            inCutscene = true;
            bossPos = e.pos;
            break;
          }
        }
      }
    }

    if (state == GameState::GAME || state == GameState::PAUSED) {
      if (IsKeyPressed(KEY_ESCAPE)) {
        state =
            (state == GameState::GAME) ? GameState::PAUSED : GameState::GAME;
      }
    }

    if (state == GameState::LOBBY) {
      if (net.mode == NetworkManager::Mode::SERVER && IsKeyPressed(KEY_ENTER)) {
        state = GameState::GAME;
      }
    }

    effects.Update(GetFrameTime());

    float preferredFOV = 70.0f;
    int worldDetailLevel = 1; // 0 = Low, 1 = High
    float fovToUse = preferredFOV;
    if (localPlayer.currentWeapon && !localPlayer.inVehicle) {
      fovToUse = localPlayer.currentWeapon->GetFOV(preferredFOV);
    }
    localPlayer.camera.fovy = fovToUse;

    if (state == GameState::GAME || state == GameState::LOBBY ||
        state == GameState::PAUSED) {
      if (state == GameState::GAME && !localPlayer.showInventory &&
          !IsCursorHidden())
        DisableCursor();
      else if ((state == GameState::LOBBY || state == GameState::PAUSED ||
                (state == GameState::GAME && localPlayer.showInventory)) &&
               IsCursorHidden())
        EnableCursor();

      if (state == GameState::GAME) {
        if (IsKeyPressed(KEY_GRAVE)) {
          localPlayer.showInventory = !localPlayer.showInventory;
        }
        if (IsKeyPressed(KEY_Q)) {
          if (localPlayer.previousWeapon &&
              localPlayer.previousWeapon != localPlayer.currentWeapon) {
            Weapon *temp = localPlayer.currentWeapon;
            localPlayer.currentWeapon = localPlayer.previousWeapon;
            localPlayer.previousWeapon = temp;
          }
        }
        auto cycleWeapon = [&](const std::vector<int> &validSizes) {
          int currentIndex = -1;
          for (size_t i = 0; i < localPlayer.inventory.size(); i++) {
            if (localPlayer.inventory[i] == localPlayer.currentWeapon) {
              currentIndex = (int)i;
              break;
            }
          }
          // If not matched, default to end to scan from zero
          if (currentIndex == -1)
            currentIndex = localPlayer.inventory.size() - 1;

          for (size_t i = 1; i <= localPlayer.inventory.size(); i++) {
            size_t idx = (currentIndex + i) % localPlayer.inventory.size();
            auto *w = localPlayer.inventory[idx];
            if (std::find(validSizes.begin(), validSizes.end(), w->magSize) !=
                validSizes.end()) {
              if (localPlayer.currentWeapon != w) {
                localPlayer.previousWeapon = localPlayer.currentWeapon;
                localPlayer.currentWeapon = w;
              }
              break;
            }
          }
        };

        if (IsKeyPressed(KEY_ONE))
          cycleWeapon({30, 200});
        if (IsKeyPressed(KEY_TWO))
          cycleWeapon({17, 6});
        if (IsKeyPressed(KEY_THREE))
          cycleWeapon({0});
        if (IsKeyPressed(KEY_FOUR))
          cycleWeapon({8, 5, 1});
        // CHEAT CODE LOGIC
        int charPressed = GetCharPressed();
        while (charPressed > 0) {
          if ((char)charPressed == cheatCode[cheatIdx]) {
            cheatIdx++;
            if (cheatIdx == 4) {
              currentWave = 10;
              enemies.clear();
              enemiesSpawnedSinceStartOfWave = 0;
              waveActive = true;
              cheatIdx = 0;
            }
          } else {
            cheatIdx = ((char)charPressed == cheatCode[0]) ? 1 : 0;
          }

          if ((char)charPressed == moneyCheat[moneyIdx]) {
            moneyIdx++;
            if (moneyIdx == 4) {
              localPlayer.AddMoney(10000);
              moneyIdx = 0;
            }
          } else {
            moneyIdx = ((char)charPressed == moneyCheat[0]) ? 1 : 0;
          }

          if ((char)charPressed == gibonCheat[gibonIdx]) {
            gibonIdx++;
            if (gibonIdx == 5) {
              currentWave = 20;
              enemies.clear();
              enemiesSpawnedSinceStartOfWave = 0;
              waveActive = true;
              gibonIdx = 0;
            }
          } else {
            gibonIdx = ((char)charPressed == gibonCheat[0]) ? 1 : 0;
          }

          if ((char)charPressed == gangCheat[gangIdx]) {
            gangIdx++;
            if (gangIdx == 8) {
              currentWave = 30;
              enemies.clear();
              enemiesSpawnedSinceStartOfWave = 0;
              waveActive = true;
              gangIdx = 0;
            }
          } else {
            gangIdx = ((char)charPressed == gangCheat[0]) ? 1 : 0;
          }

          if ((char)charPressed == primeCheat[primeIdx]) {
            primeIdx++;
            if (primeIdx == 4) {
              currentWave = 40;
              enemies.clear();
              enemiesSpawnedSinceStartOfWave = 0;
              waveActive = true;
              primeIdx = 0;
            }
          } else {
            primeIdx = ((char)charPressed == primeCheat[0]) ? 1 : 0;
          }

          charPressed = GetCharPressed();
        }

        if (!inCutscene) {
          localPlayer.Update();
          for (const auto &box : currentMap->GetObstacles())
            localPlayer.HandleCollision(box);
          for (const auto &s : structures) {
            if (s->active)
              localPlayer.HandleCollision(s->GetBoundingBox());
          }
        }
      }

      }
    }
}

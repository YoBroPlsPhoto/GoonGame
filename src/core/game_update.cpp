#include "core/game.hpp"

void Game::UpdateCore() {

    bool shopNearby = false;
    net.SetLocalPlayerName(menu.playerNick);
    if (menu.currentState != MenuState::RELAY_SETTINGS) {
      net.SetLobbyServer(menu.relayHost, menu.relayPort);
    }
    
    if (menu.shouldTestRelay) {
        net.SetLobbyServer(menu.relayHost, menu.relayPort);
        net.SendRelayPing();
        menu.shouldTestRelay = false;
    }
    
    if (menu.relayPingMs == -2.0f) {
        if (net.currentPingMs >= 0.0f) {
            menu.relayPingMs = net.currentPingMs;
            net.currentPingMs = -1.0f; // Consume
        } else if (GetTime() - net.lastPingTime > 2.0f) {
            menu.relayPingMs = -3.0f; // Timeout
            net.pinging = false;
        }
    }

    if (net.mode != NetworkManager::Mode::CLIENT) {
      net.localPlayerId = localPlayer.playerId;
      net.localPlayerAdmin = localPlayer.isAdmin;
    }
    net.Update();
    if (net.mode == NetworkManager::Mode::CLIENT && net.localPlayerId >= 0) {
      localPlayer.playerId = net.localPlayerId;
      localPlayer.isAdmin = net.localPlayerAdmin;
    }
    AdasGooner::globalUseWafelModel = menu.useWafelModel;

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
      if (net.hostDisconnected) {
          menu.disconnectedByHost = true;
          net.hostDisconnected = false;
      }
      state = GameState::MENU;
      net.shouldQuit = false;
      net.remotePlayers.clear();
      net.syncedEnemies.clear();
      net.mode = NetworkManager::Mode::NONE;
      localPlayer.playerId = 0;
      localPlayer.isAdmin = false;
    }

    inCutscene = false;
    bossPos = {0};
    activeBoss = nullptr;

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
          if (e->type == EnemyType::ADAS_PRIME) {
            auto prime = std::dynamic_pointer_cast<AdasPrime>(e);
            if (prime && prime->cutsceneState != PrimeCutscene::FIGHT) {
              inCutscene = true;
              bossPos = prime->wardrobePos;
              activeBoss = prime;
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
          if (e.type == (int)EnemyType::ADAS_PRIME &&
              e.attackTimer != (float)(int)PrimeCutscene::FIGHT) {
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

    
}

#include "core/game.hpp"

void Game::Render3D() {
    // --- PASS 1: SHADOW MAP (Depth) ---
    BeginTextureMode(shadowTarget);
    ClearBackground(WHITE);
    BeginMode3D(shadowCam);
    currentMap->Draw(0, shadowCam.position, currentWave);
    EndMode3D();
    EndTextureMode();

    // --- PASS 2: REFLECTION ---
    BeginTextureMode(reflectionTarget);
    ClearBackground(SKYBLUE);
    Camera3D rCam = {0};
    rCam.position = localPlayer.position;
    Vector3 viewDir = Vector3Normalize(Vector3Subtract(
        localPlayer.camera.target, localPlayer.camera.position));
    rCam.target = Vector3Subtract(localPlayer.position, viewDir);
    rCam.up = (Vector3){0, 1, 0};
    rCam.fovy = 90;
    rCam.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(rCam);
    currentMap->Draw(0, rCam.position, currentWave);
    EndMode3D();
    EndTextureMode();

    // --- FINAL RENDER ---
    Matrix shadowVP =
        MatrixMultiply(GetCameraMatrix(shadowCam), rlGetMatrixProjection());
    if (shadowVPLoc >= 0)
      SetShaderValueMatrix(lighting, shadowVPLoc, shadowVP);
    if (matViewLoc >= 0)
      SetShaderValueMatrix(lighting, matViewLoc,
                           GetCameraMatrix(localPlayer.camera));
    if (viewPosLoc >= 0)
      SetShaderValue(lighting, viewPosLoc, &localPlayer.position,
                     SHADER_UNIFORM_VEC3);
    if (reflTexLoc >= 0)
      SetShaderValueTexture(lighting, reflTexLoc, reflectionTarget.texture);
    if (shadowTexLoc >= 0)
      SetShaderValueTexture(lighting, shadowTexLoc, shadowTarget.texture);

    // --- RENDER TO WORLD TARGET (3D ONLY) ---
    BeginTextureMode(worldTarget);
    ClearBackground(SKYBLUE);

    if (state == GameState::GAME || state == GameState::PAUSED) {
      Camera3D activeCam = localPlayer.camera;

      float vehicleCamOrbitH = 0.0f;
      float vehicleCamOrbitV = 0.3f;

      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
        auto v = vehicles[localPlayer.vehicleIndex];

        // Orbit Camera Control (Inverted for correct player feel)
        Vector2 mouseDelta = GetMouseDelta();
        vehicleCamOrbitH -= mouseDelta.x * 0.015f;
        vehicleCamOrbitV += mouseDelta.y * 0.015f;

        if (vehicleCamOrbitV > 1.2f)
          vehicleCamOrbitV = 1.2f;
        if (vehicleCamOrbitV < -0.1f)
          vehicleCamOrbitV = -0.1f;

        float dist = 30.0f;
        activeCam.position =
            (Vector3){v->position.x - cosf(vehicleCamOrbitV) *
                                          sinf(vehicleCamOrbitH) * dist,
                      v->position.y + sinf(vehicleCamOrbitV) * dist + 8.0f,
                      v->position.z - cosf(vehicleCamOrbitV) *
                                          cosf(vehicleCamOrbitH) * dist};
        activeCam.target = Vector3Add(
            v->position, (Vector3){0, 4.0f, 0}); // Target the turret height
        activeCam.up = (Vector3){0, 1, 0};
        activeCam.fovy = 70.0f;

        // Sync Turret with Camera
        Tank *tank = dynamic_cast<Tank *>(v.get());
        if (tank) {
          tank->turretRotation = (vehicleCamOrbitH * RAD2DEG) - tank->rotation;
          tank->barrelPitch =
              -(vehicleCamOrbitV * RAD2DEG); // Removed offset for precision
        }
      }

      // --- GIBON FPS STUTTER EFFECT ---
      bool gibonStuttering = false;
      if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies) {
          if (e->type == EnemyType::GIBON_BOSS) {
            auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
            if (gibon && gibon->active) {
              if (gibon->isStuttering ||
                  gibon->gibonState == GibonState::FALLING ||
                  gibon->gibonState == GibonState::IMPACT) {
                gibonStuttering = true;
              }
            }
          }
        }
      }
      if (gibonStuttering) {
        // Simulate FPS drop by burning CPU time
        double waitUntil = GetTime() + 0.04 + (double)(rand() % 60) / 1000.0;
        while (GetTime() < waitUntil) { /* spin */
        }
      }

      if (inCutscene) {
        // Check if it's a Gibon cutscene (falling from sky) - use different
        // camera angle
        bool isGibonCutscene = false;
        if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto &e : enemies) {
            if (e->type == EnemyType::GIBON_BOSS) {
              auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
              if (gibon && gibon->gibonState != GibonState::FINISHED_FALLING) {
                isGibonCutscene = true;
                bossPos = gibon->position;
              }
            }
          }
        } else {
          for (auto const &[id, e] : net.syncedEnemies) {
            if (e.type == (int)EnemyType::GIBON_BOSS) {
              isGibonCutscene = true;
              bossPos = e.pos;
            }
          }
        }

        bool isGangCutscene = false;
        if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto &e : enemies) {
            if (e->type == EnemyType::GANG_BOSS) {
              auto gang = std::dynamic_pointer_cast<GangBoss>(e);
              if (gang && gang->cutsceneState != GangCutscene::FIGHT) {
                isGangCutscene = true;
                bossPos = gang->wardrobePos;
              }
            }
          }
        } else {
          for (auto const &[id, e] : net.syncedEnemies) {
            if (e.type == (int)EnemyType::GANG_BOSS) {
              isGangCutscene = true;
              bossPos = e.pos;
            }
          }
        }

        bool isPrimeCutscene = false;
        if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto &e : enemies) {
            if (e->type == EnemyType::ADAS_PRIME) {
              auto p = std::dynamic_pointer_cast<AdasPrime>(e);
              if (p && p->cutsceneState != PrimeCutscene::FIGHT) {
                isPrimeCutscene = true;
                bossPos = p->wardrobePos;
              }
            }
          }
        } else {
          for (auto const &[id, e] : net.syncedEnemies) {
            if (e.type == (int)EnemyType::ADAS_PRIME &&
                e.attackTimer != (float)(int)PrimeCutscene::FIGHT) {
              isPrimeCutscene = true;
              bossPos = e.pos;
            }
          }
        }

        if (isGibonCutscene) {
          // Camera on the ground looking up at the falling ball, staying still
          // on XZ
          Vector3 camPos = {bossPos.x + 30.0f, 5.0f, bossPos.z + 40.0f};
          activeCam.position = camPos;
          activeCam.target = bossPos;
        } else {
          activeCam.position =
              Vector3Add(bossPos, (Vector3){-15.0f, 15.0f, 25.0f});
          activeCam.target = Vector3Add(bossPos, (Vector3){0, 10.0f, 0});
        }
      }

      BeginMode3D(activeCam);
      // Draw Sun Billboard only
      DrawBillboard(activeCam, sunTex, lightPos, 120.0f, WHITE);

      BeginShaderMode(lighting);
      currentMap->Draw(worldDetailLevel, activeCam.position, currentWave);
      for (auto &v : vehicles)
        v->Draw();

      for (auto &s : structures) {
        if (!s->active)
          continue;

        // Damage-based color tinting
        float hpRatio = (float)s->hp / (float)s->maxHp;
        Color baseColor = s->color;
        // Darken color as HP drops
        if (hpRatio < 0.5f) {
          float darkFactor = 0.4f + 0.6f * (hpRatio / 0.5f);
          baseColor.r = (unsigned char)(baseColor.r * darkFactor);
          baseColor.g = (unsigned char)(baseColor.g * darkFactor);
          baseColor.b = (unsigned char)(baseColor.b * darkFactor);
        }
        // Flash red when hit
        if (s->damageFlash > 0) {
          float flashIntensity = s->damageFlash / 0.3f;
          baseColor.r = (unsigned char)std::min(
              255.0f, baseColor.r + 180.0f * flashIntensity);
          baseColor.g =
              (unsigned char)(baseColor.g * (1.0f - flashIntensity * 0.7f));
          baseColor.b =
              (unsigned char)(baseColor.b * (1.0f - flashIntensity * 0.7f));
        }

        if (s->type == StructureType::WALL) {
          // === REINFORCED WALL ===
          Vector3 p = s->position;

          // Main body - reinforced concrete
          DrawCube({p.x, p.y + 2.0f, p.z}, 8.0f, 4.0f, 2.5f, baseColor);

          // Steel reinforcement plates on front and back
          Color steelCol = {90, 95, 100, 255};
          if (s->damageFlash > 0)
            steelCol = baseColor;
          DrawCube({p.x, p.y + 2.0f, p.z + 1.3f}, 7.5f, 3.5f, 0.15f, steelCol);
          DrawCube({p.x, p.y + 2.0f, p.z - 1.3f}, 7.5f, 3.5f, 0.15f, steelCol);

          // Top edge (concrete lip)
          Color topCol = {(unsigned char)(baseColor.r * 0.8f),
                          (unsigned char)(baseColor.g * 0.8f),
                          (unsigned char)(baseColor.b * 0.8f), 255};
          DrawCube({p.x, p.y + 4.1f, p.z}, 8.4f, 0.3f, 2.9f, topCol);

          // Vertical support pillars on edges
          Color pillarCol = {60, 60, 65, 255};
          DrawCube({p.x - 3.7f, p.y + 2.0f, p.z}, 0.6f, 4.0f, 2.7f, pillarCol);
          DrawCube({p.x + 3.7f, p.y + 2.0f, p.z}, 0.6f, 4.0f, 2.7f, pillarCol);

          // Center cross brace
          DrawCube({p.x, p.y + 2.0f, p.z}, 0.4f, 4.0f, 2.7f, pillarCol);

          // Bottom foundation
          DrawCube({p.x, p.y + 0.15f, p.z}, 8.6f, 0.3f, 3.0f,
                   {50, 50, 55, 255});

          // Damage cracks (visible when < 60% HP)
          if (hpRatio < 0.6f) {
            Color crackCol = {30, 25, 20, 255};
            float crackSize = (1.0f - hpRatio) * 0.3f;
            DrawCube({p.x - 1.5f, p.y + 2.5f, p.z + 1.35f}, 0.8f * crackSize,
                     2.0f, 0.05f, crackCol);
            DrawCube({p.x + 2.0f, p.y + 1.5f, p.z + 1.35f}, 1.2f * crackSize,
                     1.5f, 0.05f, crackCol);
            if (hpRatio < 0.3f) {
              DrawCube({p.x + 0.5f, p.y + 3.0f, p.z - 1.35f}, 1.5f * crackSize,
                       2.5f, 0.05f, crackCol);
            }
          }

          // Wire outline
          DrawCubeWires({p.x, p.y + 2.0f, p.z}, 8.0f, 4.0f, 2.5f,
                        Fade(BLACK, 0.3f));

        } else { // === TURRET ===
          Vector3 p = s->position;

          // Base platform (heavy concrete foundation)
          DrawCube({p.x, p.y + 0.3f, p.z}, 4.0f, 0.6f, 4.0f, {55, 55, 60, 255});
          DrawCubeWires({p.x, p.y + 0.3f, p.z}, 4.0f, 0.6f, 4.0f,
                        Fade(BLACK, 0.2f));

          // Platform ring (beveled concrete)
          DrawCube({p.x, p.y + 0.7f, p.z}, 3.4f, 0.2f, 3.4f, {65, 65, 70, 255});

          // Central pedestal
          DrawCube({p.x, p.y + 1.4f, p.z}, 1.6f, 1.2f, 1.6f, baseColor);
          DrawCubeWires({p.x, p.y + 1.4f, p.z}, 1.6f, 1.2f, 1.6f,
                        Fade(BLACK, 0.3f));

          // Rotating turret assembly
          rlPushMatrix();
          rlTranslatef(p.x, p.y + 2.0f, p.z);
          rlRotatef(s->turretAngle, 0, 1, 0);

          // Gun housing / shield
          Color housingCol = {
              (unsigned char)std::min(255, (int)baseColor.r + 20),
              (unsigned char)std::min(255, (int)baseColor.g + 15),
              (unsigned char)std::min(255, (int)baseColor.b + 15), 255};
          DrawCube({0, 0.4f, 0}, 2.0f, 1.2f, 1.8f, housingCol);

          // Front armor plate (angled)
          Color steelColT = {80, 85, 90, 255};
          DrawCube({0, 0.5f, 1.2f}, 2.2f, 1.4f, 0.3f, steelColT);

          // Barrel recoil offset
          float recoilOffset = s->barrelRecoil * 0.4f;

          // Dual barrels
          Color barrelCol = {45, 45, 50, 255};
          DrawCube({-0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.25f, 0.25f,
                   3.0f, barrelCol);
          DrawCube({0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.25f, 0.25f,
                   3.0f, barrelCol);
          DrawCubeWires({-0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.25f,
                        0.25f, 3.0f, Fade(BLACK, 0.3f));
          DrawCubeWires({0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.25f, 0.25f,
                        3.0f, Fade(BLACK, 0.3f));

          // Barrel tips (flash guard)
          DrawCube({-0.35f, 0.5f, 3.2f - recoilOffset}, 0.35f, 0.35f, 0.15f,
                   {35, 35, 40, 255});
          DrawCube({0.35f, 0.5f, 3.2f - recoilOffset}, 0.35f, 0.35f, 0.15f,
                   {35, 35, 40, 255});

          // Ammo box on the side
          DrawCube({-1.2f, 0.0f, 0.3f}, 0.6f, 0.7f, 0.8f, {60, 70, 50, 255});
          DrawCubeWires({-1.2f, 0.0f, 0.3f}, 0.6f, 0.7f, 0.8f,
                        Fade(BLACK, 0.2f));

          // Antenna
          DrawCube({0.8f, 1.4f, -0.5f}, 0.06f, 1.5f, 0.06f, DARKGRAY);
          DrawCube({0.8f, 2.15f, -0.5f}, 0.15f, 0.08f, 0.15f, RED);

          rlPopMatrix(); // End turret rotation

          // Range indicator ring (subtle, on ground)
          float pulse = sinf((float)GetTime() * 2.0f) * 0.1f + 0.15f;
          DrawCircle3D({p.x, 0.05f, p.z}, 60.0f, {1, 0, 0}, 90.0f,
                       Fade(RED, pulse * 0.3f));
        }
      }
      EndShaderMode();

      // --- PLACEMENT GHOST PREVIEW ---
      if (localPlayer.currentWeapon && !localPlayer.inVehicle &&
          !localPlayer.showInventory &&
          (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0 ||
           strncmp(localPlayer.currentWeapon->name, "WALL", 4) == 0)) {
        Ray previewRay = {
            localPlayer.camera.position,
            Vector3Normalize(Vector3Subtract(localPlayer.camera.target,
                                             localPlayer.camera.position))};

        Vector3 ghostPos = localPlayer.position;
        float maxPlaceDist = 25.0f;
        if (previewRay.direction.y < -0.01f) {
          float t = -(previewRay.position.y) / previewRay.direction.y;
          if (t > 0 && t < maxPlaceDist * 2.0f) {
            ghostPos = Vector3Add(previewRay.position,
                                  Vector3Scale(previewRay.direction, t));
          }
        }
        float placeDist = Vector3Distance(localPlayer.position, ghostPos);
        if (placeDist > maxPlaceDist || previewRay.direction.y >= -0.01f) {
          Vector3 forward = previewRay.direction;
          forward.y = 0;
          if (Vector3Length(forward) > 0.001f)
            forward = Vector3Normalize(forward);
          else
            forward = {0, 0, 1};
          ghostPos = {localPlayer.position.x + forward.x * 10.0f, 0.0f,
                      localPlayer.position.z + forward.z * 10.0f};
        }
        ghostPos.y = 0.0f;

        bool isTurretPreview =
            (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0);
        float ghostPulse = sinf((float)GetTime() * 4.0f) * 0.15f + 0.35f;
        Color ghostColor =
            isTurretPreview ? Fade(RED, ghostPulse) : Fade(BROWN, ghostPulse);

        if (isTurretPreview) {
          // Ghost turret
          DrawCube({ghostPos.x, ghostPos.y + 0.3f, ghostPos.z}, 4.0f, 0.6f,
                   4.0f, Fade(DARKGRAY, ghostPulse * 0.5f));
          DrawCube({ghostPos.x, ghostPos.y + 1.4f, ghostPos.z}, 1.6f, 1.2f,
                   1.6f, ghostColor);
          DrawCube({ghostPos.x, ghostPos.y + 2.5f, ghostPos.z}, 2.0f, 1.2f,
                   1.8f, Fade(MAROON, ghostPulse * 0.7f));
          DrawCube({ghostPos.x, ghostPos.y + 2.5f, ghostPos.z + 2.5f}, 0.3f,
                   0.3f, 3.0f, Fade(DARKGRAY, ghostPulse * 0.7f));
          DrawCubeWires({ghostPos.x, ghostPos.y + 1.4f, ghostPos.z}, 4.0f, 3.0f,
                        4.0f, Fade(LIME, ghostPulse));
          // Range ring
          DrawCircle3D({ghostPos.x, 0.06f, ghostPos.z}, 60.0f, {1, 0, 0}, 90.0f,
                       Fade(LIME, ghostPulse * 0.5f));
        } else {
          // Ghost wall
          DrawCube({ghostPos.x, ghostPos.y + 2.0f, ghostPos.z}, 8.0f, 4.0f,
                   2.5f, ghostColor);
          DrawCubeWires({ghostPos.x, ghostPos.y + 2.0f, ghostPos.z}, 8.0f, 4.0f,
                        2.5f, Fade(LIME, ghostPulse));
        }

        // Ground marker
        DrawCircle3D({ghostPos.x, 0.06f, ghostPos.z}, 2.0f, {1, 0, 0}, 90.0f,
                     Fade(LIME, ghostPulse));
      }

      effects.Draw();

      if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies)
          e->Draw();
      } else {
        for (auto const &[id, e] : net.syncedEnemies) {
          std::shared_ptr<Enemy> temp;
          if (e.type == (int)EnemyType::BOSS) {
            auto adas = std::make_shared<AdasGooner>(e.pos, e.id);
            adas->cutsceneState = (CutsceneState)(int)e.attackTimer;
            adas->cutsceneTimer = e.walkTimer;
            adas->isMoving = e.isMoving;
            adas->angle = e.angle;
            temp = adas;
          } else if (e.type == (int)EnemyType::GIBON_BOSS) {
            auto gibon = std::make_shared<GibonRzygacz>(e.pos, e.id);
            gibon->gibonState = (GibonState)(int)e.attackTimer;
            gibon->stateTimer = e.walkTimer;
            gibon->isMoving = e.isMoving;
            gibon->angle = e.angle;
            gibon->craterCreated =
                ((int)e.attackTimer >= (int)GibonState::IMPACT);
            gibon->impactCrater = {e.pos, 20.0f};
            temp = gibon;
          } else if (e.type == (int)EnemyType::GANG_BOSS) {
            auto gang = std::make_shared<GangBoss>(e.pos, e.id);
            gang->cutsceneState = (GangCutscene)(int)e.attackTimer;
            gang->stateTimer = e.walkTimer;
            gang->wardrobePos = e.pos;
            gang->wardrobePos.y =
                (gang->cutsceneState == GangCutscene::WARDROBE_FALLING)
                    ? (400.0f - e.walkTimer * 60.0f)
                    : 0.0f;
            temp = gang;
          } else if (e.type == (int)EnemyType::ADAS_PRIME) {
            auto prime = std::make_shared<AdasPrime>(e.pos, e.id);
            prime->cutsceneState = (PrimeCutscene)(int)e.attackTimer;
            prime->cutsceneTimer = e.walkTimer;
            prime->angle = e.angle;
            prime->isMoving = e.isMoving;
            prime->wardrobePos = e.pos;
            temp = prime;
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
          temp->Draw();
        }
      }

      for (auto const &[id, data] : net.remotePlayers) {
        // Skip drawing player if they are inside a vehicle
        bool playerInVehicle = false;
        for (auto &v : vehicles) {
          if (Vector3Distance(data.position, v->position) < 8.0f &&
              v->isOccupied) {
            playerInVehicle = true;
            break;
          }
        }
        if (data.active) {
          if (!playerInVehicle) {
            DrawCapsule((Vector3){data.position.x, data.position.y - 1.0f,
                                  data.position.z},
                        (Vector3){data.position.x, data.position.y + 1.0f,
                                  data.position.z},
                        0.5f, 8, 4, (id == 0) ? BLUE : DARKPURPLE);
          }

          // Draw Nameplate
          Vector3 namePos = {data.position.x, data.position.y + 2.5f,
                             data.position.z};
          if (playerInVehicle)
            namePos.y += 2.0f; // Shift higher for vehicles

          Vector2 screenPos = GetWorldToScreen(namePos, activeCam);
          if (screenPos.x > 0) {
            const char *n = data.name[0] ? data.name : "Gooner";
            DrawText(n, (int)screenPos.x - MeasureText(n, 20) / 2,
                     (int)screenPos.y, 20, YELLOW);
          }
        }
      }

      // Draw local player name if in vehicle (3rd person)
      if (localPlayer.inVehicle) {
        Vector3 namePos = {localPlayer.position.x,
                           localPlayer.position.y + 4.5f,
                           localPlayer.position.z};
        Vector2 screenPos = GetWorldToScreen(namePos, activeCam);
        if (screenPos.x > 0) {
          DrawText(menu.playerNick.c_str(),
                   (int)screenPos.x -
                       MeasureText(menu.playerNick.c_str(), 20) / 2,
                   (int)screenPos.y, 20, GOLD);
        }
      }

      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
        // No viewmodel in vehicle
      } else if (!inCutscene) {
        localPlayer.DrawViewModel();
      }
      EndMode3D();

}

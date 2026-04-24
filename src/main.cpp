#include "raylib.h"
#include "map/city_map.hpp"
#include "map/desert_map.hpp"
#include "map/map.hpp"
#include "network/network_manager.hpp"
#include "player/player.hpp"
#include "enemies/enemy.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <memory>
#include "bosses/adas_gooner.hpp"
#include "weapons/ak47.hpp"
#include "weapons/minigun.hpp"
#include "weapons/awp.hpp"
#include "weapons/glock.hpp"
#include "weapons/revolver.hpp"
#include "weapons/shotgun.hpp"
#include "weapons/rpg.hpp"
#include "vehicles/tank.hpp"
#include "hud/menu.hpp" // We'll keep the class but fix it or just use its logic

enum class GameState { MENU, LOBBY, GAME, PAUSED };

int main(int argc, char *argv[]) {
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "AdasGooner - NYC Ultra Graphics");
  SetExitKey(KEY_NULL);

  NetworkManager net;
  net.StartDiscovery();

  GameState state = GameState::MENU;
  Menu menu; // Use the new menu class
  Player localPlayer((Vector3){0.0f, 2.0f, 0.0f}, 0);
  localPlayer.playerId = (int)(GetTime() * 1000);

  Map *currentMap = new CityMap();

  // SHADER LOAD
  Shader lighting = LoadShader("../resources/shaders/lighting.vs",
                               "../resources/shaders/lighting.fs");
  
  Image sunImg = LoadImage("../textures/sun.png");
  ImageColorReplace(&sunImg, (Color){102, 191, 255, 255}, BLANK); // Replace skyblue with transparency
  Texture2D sunTex = LoadTextureFromImage(sunImg);
  UnloadImage(sunImg);
  
  int viewPosLoc = GetShaderLocation(lighting, "viewPos");
  int lightPosLoc = GetShaderLocation(lighting, "lightPos");
  int lightColorLoc = GetShaderLocation(lighting, "lightColor");
  int reflTexLoc = GetShaderLocation(lighting, "reflectionTexture");
  int shadowTexLoc = GetShaderLocation(lighting, "shadowMap");
  int shadowVPLoc = GetShaderLocation(lighting, "shadowVP");
  int matViewLoc = GetShaderLocation(lighting, "matView");

  Vector3 lightPos = {-150.0f, 450.0f, 350.0f};
  Vector4 lightCol = {1.5f, 1.5f, 1.4f, 1.0f};
  if (lightPosLoc >= 0)
    SetShaderValue(lighting, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
  if (lightColorLoc >= 0)
    SetShaderValue(lighting, lightColorLoc, &lightCol, SHADER_UNIFORM_VEC4);

  // RENDER TARGETS
  RenderTexture2D reflectionTarget = LoadRenderTexture(512, 512); // Optimized
  RenderTexture2D shadowTarget = LoadRenderTexture(1024, 1024); // Optimized
  SetTextureFilter(reflectionTarget.texture, TEXTURE_FILTER_BILINEAR);
  SetTextureFilter(shadowTarget.texture, TEXTURE_FILTER_BILINEAR);

  // SHADOW CAMERA (ORTHO for Sun)
  Camera3D shadowCam = {0};
  shadowCam.position = lightPos;
  shadowCam.target = (Vector3){0, 0, 0};
  shadowCam.up = (Vector3){0, 1, 0};
  shadowCam.fovy = 800.0f; // Adjusted
  shadowCam.projection = CAMERA_ORTHOGRAPHIC;

  SetTargetFPS(144); // Unlock more frames
  bool shouldExit = false;

  // --- GAMEPLAY STATS ---
  float maxBaseHP = 60000.0f;
  float baseHP = 60000.0f;
  std::vector<std::shared_ptr<Enemy>> enemies;
  float spawnTimer = 0.0f;
  Vector3 basePosition = {0, 0, 150};
  
  // Wave System
  int currentWave = 1;
  int enemiesSpawnedSinceStartOfWave = 0;
  std::vector<std::shared_ptr<Vehicle>> vehicles;
  bool waveActive = true;
  float waveWaitTimer = 0.0f;



  static int cheatIdx = 0;
  const char cheatCode[] = "goon";
  const char moneyCheat[] = "cash";
  static int moneyIdx = 0;

  while (!shouldExit && !WindowShouldClose()) {
    bool shopNearby = false;
    net.Update();

    if (state == GameState::GAME || state == GameState::PAUSED) {
      if (IsKeyPressed(KEY_ESCAPE)) {
        state = (state == GameState::GAME) ? GameState::PAUSED : GameState::GAME;
      }
    }

    if (state == GameState::LOBBY) {
        if (net.mode == NetworkManager::Mode::SERVER && IsKeyPressed(KEY_ENTER)) {
            state = GameState::GAME;
        }
    }

    static float preferredFOV = 70.0f;
    static int worldDetailLevel = 1; // 0 = Low, 1 = High
    float fovToUse = preferredFOV;
    if (localPlayer.currentWeapon && !localPlayer.inVehicle) {
        fovToUse = localPlayer.currentWeapon->GetFOV(preferredFOV);
    }
    localPlayer.camera.fovy = fovToUse;
    
    if (state == GameState::GAME || state == GameState::LOBBY || state == GameState::PAUSED) {
      if (state == GameState::GAME && !IsCursorHidden())
        DisableCursor();
      else if ((state == GameState::LOBBY || state == GameState::PAUSED) && IsCursorHidden())
        EnableCursor();
        
      if (state == GameState::GAME) {
          // CHEAT CODE LOGIC
          int charPressed = GetCharPressed();
          while (charPressed > 0) {
              if ((char)charPressed == cheatCode[cheatIdx]) {
                  cheatIdx++;
                  if (cheatIdx == 4) { 
                      currentWave = 10; enemies.clear(); enemiesSpawnedSinceStartOfWave = 0; waveActive = true; cheatIdx = 0; 
                  }
              } else {
                  cheatIdx = ((char)charPressed == cheatCode[0]) ? 1 : 0;
              }
              
              if ((char)charPressed == moneyCheat[moneyIdx]) {
                  moneyIdx++;
                  if (moneyIdx == 4) { localPlayer.AddMoney(10000); moneyIdx = 0; }
              } else {
                  moneyIdx = ((char)charPressed == moneyCheat[0]) ? 1 : 0;
              }
              charPressed = GetCharPressed();
          }

          localPlayer.Update();
          for (const auto &box : currentMap->GetObstacles())
            localPlayer.HandleCollision(box);
      }
        
      if (state == GameState::GAME) {
          // --- INTERACTION & SHOOTING LOGIC ---
          Ray ray = {localPlayer.camera.position,
                     Vector3Normalize(Vector3Subtract(localPlayer.camera.target, localPlayer.camera.position))};

          // --- VEHICLE INTERACTION (F Key) ---
          if (IsKeyPressed(KEY_F)) {
              if (localPlayer.inVehicle) {
                  if (localPlayer.vehicleIndex >= 0 && localPlayer.vehicleIndex < (int)vehicles.size()) {
                      Vector3 exitPos = vehicles[localPlayer.vehicleIndex]->position;
                      Vector3 right = vehicles[localPlayer.vehicleIndex]->GetRight();
                      localPlayer.position = Vector3Add(exitPos, Vector3Scale(right, 8.0f));
                      localPlayer.position.y = 2.0f;
                      vehicles[localPlayer.vehicleIndex]->Exit();
                  }
                  localPlayer.inVehicle = false;
                  localPlayer.vehicleIndex = -1;
              } else {
                  for (int i = 0; i < (int)vehicles.size(); i++) {
                      if (Vector3Distance(localPlayer.position, vehicles[i]->position) < 12.0f) {
                          vehicles[i]->Enter();
                          localPlayer.inVehicle = true;
                          localPlayer.vehicleIndex = i;
                          break;
                      }
                  }
              }
          }

          // --- TANK COMBAT ---
          if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
              auto v = vehicles[localPlayer.vehicleIndex];
              Tank* tank = dynamic_cast<Tank*>(v.get());
              if (tank && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && tank->CanFireCannon()) {
                  tank->FireCannon();
                  Ray cannonRay = {tank->GetBarrelTip(), tank->GetTurretForward()};
                  float t = -tank->GetBarrelTip().y / tank->GetTurretForward().y;
                  Vector3 impactPoint = Vector3Add(tank->GetBarrelTip(), Vector3Scale(tank->GetTurretForward(), t));
                  float closest = 1000.0f;
                  for (auto& e : enemies) {
                      auto coll = GetRayCollisionBox(cannonRay, e->GetBoundingBox());
                      if (coll.hit && coll.distance < closest) { closest = coll.distance; impactPoint = coll.point; }
                  }
                  float splashRadius = 15.0f;
                  for (auto& e : enemies) {
                      float dist = Vector3Distance(e->position, impactPoint);
                      if (dist < splashRadius) {
                          float damageMult = 1.0f - (dist / splashRadius);
                          e->hp -= tank->cannonDamage * (0.3f + 0.7f * damageMult);
                          if (e->hp <= 0) localPlayer.AddMoney(150);
                      }
                  }
              }
          }

          // --- WEAPON SHOOTING ---
          bool isFiring = localPlayer.currentWeapon && (localPlayer.currentWeapon->IsAutomatic() ? IsMouseButtonDown(MOUSE_LEFT_BUTTON) : IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
          if (isFiring && localPlayer.currentWeapon->CanFire() && !localPlayer.inVehicle) {
              localPlayer.currentWeapon->Fire();
              if (net.mode == NetworkManager::Mode::SERVER) {
                  float closestDist = 200.0f; Enemy* hitEnemy = nullptr;
                  for (auto& e : enemies) {
                      if (!e->active || e->hp <= 0) continue;
                      BoundingBox box = e->GetBoundingBox();
                      RayCollision hit = GetRayCollisionBox(ray, box);
                      if (hit.hit && hit.distance < closestDist) { closestDist = hit.distance; hitEnemy = e.get(); }
                  }
                  if (hitEnemy) {
                      float oldHp = hitEnemy->hp;
                      hitEnemy->hp -= localPlayer.currentWeapon->damage; 
                      if (hitEnemy->hp <= 0) {
                          hitEnemy->hp = 0;
                          if (oldHp > 0) {
                              int reward = 50;
                              if (hitEnemy->type == EnemyType::SHOOTER) reward = 75;
                              else if (hitEnemy->type == EnemyType::FAST) reward = 100;
                              else if (hitEnemy->type == EnemyType::TANK) reward = 250;
                              else if (hitEnemy->type == EnemyType::BOSS) reward = 2000;
                              localPlayer.AddMoney(reward);
                          }
                      }
                  }
              }
          }

          // --- INTERACTION / SHOP ---
          Vector3 bp = {0, 0, 150};
          BoundingBox shopBox = {{bp.x - 24.6f, 0, bp.z - 9.0f}, {bp.x - 24.4f, 10, bp.z + 9.0f}};
          RayCollision shopHit = GetRayCollisionBox(ray, shopBox);
          shopNearby = shopHit.hit && shopHit.distance < 15.0f;

          if (shopNearby && IsKeyPressed(KEY_E)) {
              int cost = 0; Weapon* newWep = nullptr; bool buyAmmo = false; bool healing = false;
              if (shopHit.point.z < bp.z) { // Left Column
                  if (shopHit.point.y > 6.75f) { cost = 1000; newWep = new Revolver(); }
                  else if (shopHit.point.y > 5.25f) { cost = 1500; newWep = new AK47(); }
                  else if (shopHit.point.y > 3.75f) { cost = 2000; newWep = new Shotgun(); }
                  else if (shopHit.point.y > 2.25f) { cost = 5000; newWep = new Minigun(); }
              } else { // Right Column
                  if (shopHit.point.y > 6.75f) { cost = 8000; newWep = new AWP(); }
                  else if (shopHit.point.y > 5.25f) { cost = 12000; newWep = new RPG(); }
                  else if (shopHit.point.y > 3.75f) { 
                      // Removed Tank from wall shop - now at terminal
                      cost = 500; healing = true; 
                  }
                  else if (shopHit.point.y > 2.25f) { 
                      cost = 500; 
                      if (localPlayer.currentWeapon && localPlayer.currentWeapon->reserveAmmo < localPlayer.currentWeapon->maxReserve) buyAmmo = true;
                      else healing = true;
                  }
              }

              if (localPlayer.money >= cost) {
                  if (newWep) {
                      localPlayer.money -= cost;
                      if (localPlayer.currentWeapon) delete localPlayer.currentWeapon;
                      localPlayer.currentWeapon = newWep;
                  } else if (buyAmmo) {
                      localPlayer.money -= cost;
                      if (localPlayer.currentWeapon) localPlayer.currentWeapon->RefillAmmo();
                  } else if (healing) {
                      localPlayer.money -= cost;
                      localPlayer.hp = localPlayer.maxHp;
                  }
              } else if (newWep) delete newWep;
          }

          // --- TANK TERMINAL (IN FRONT OF BASE) ---
          Vector3 terminalPos = {15, 0, 120};
          bool terminalNearby = Vector3Distance(localPlayer.position, terminalPos) < 6.0f;
          if (terminalNearby && IsKeyPressed(KEY_E)) {
              int tankCost = 100000;
              if (localPlayer.money >= tankCost) {
                  localPlayer.money -= tankCost;
                  vehicles.push_back(std::make_shared<Tank>((Vector3){terminalPos.x, 0.5f, terminalPos.z - 20.0f}));
                  localPlayer.position = (Vector3){terminalPos.x, 2.0f, terminalPos.z - 10.0f};
              }
          }
          if (terminalNearby) shopNearby = true; // Show "Press E to Buy" prompt
      }
      
      // --- NETWORK SYNC ---
      if (net.mode == NetworkManager::Mode::SERVER) {
          if (state == GameState::GAME || state == GameState::PAUSED) {
              static int enemyIdCounter = 0;
              int totalEnemiesThisWave = 5 + currentWave * 4;
              if (currentWave % 10 == 0) totalEnemiesThisWave = 25; 
              if (waveActive) {
                  spawnTimer += GetFrameTime();
                  float spawnDelay = std::max(0.2f, 1.5f - currentWave * 0.05f);
                  if (spawnTimer > spawnDelay && enemiesSpawnedSinceStartOfWave < totalEnemiesThisWave) {
                      Vector3 spawnPos = { (float)GetRandomValue(-15, 15), 0.1f, -350.0f };
                      EnemyType t = EnemyType::MELEE;
                      if (currentWave % 10 == 0 && enemiesSpawnedSinceStartOfWave == 0) enemies.push_back(std::make_shared<AdasGooner>(spawnPos, ++enemyIdCounter));
                      else {
                           int roll = GetRandomValue(0, 100);
                           if (roll < 30) t = EnemyType::MELEE; else if (roll < 50) t = EnemyType::SHOOTER; else if (roll < 65) t = EnemyType::FAST; else if (roll < 75) t = EnemyType::TANK; else if (roll < 85) t = EnemyType::MINION; else if (roll < 92) t = EnemyType::ELITE; else if (roll < 97) t = EnemyType::KAMIKAZE; else t = EnemyType::GIGA_TANK;
                           WeaponType w = WeaponType::PISTOL;
                           if (t == EnemyType::MELEE) w = (GetRandomValue(0, 1) == 0 ? WeaponType::KATANA : WeaponType::MACHETE); else if (t == EnemyType::SHOOTER) w = WeaponType::PISTOL; else if (t == EnemyType::FAST) w = WeaponType::MACHETE; else if (t == EnemyType::TANK) w = WeaponType::KATANA; else if (t == EnemyType::MINION) w = WeaponType::MACHETE; else if (t == EnemyType::ELITE) w = WeaponType::SHOTGUN; else if (t == EnemyType::KAMIKAZE) w = WeaponType::EXPLOSIVE; else if (t == EnemyType::GIGA_TANK) w = WeaponType::MACHETE;
                           auto newEnemy = std::make_shared<Enemy>(spawnPos, t, w, ++enemyIdCounter);
                           newEnemy->hp *= (1.0f + currentWave * 0.15f); newEnemy->maxHp = newEnemy->hp;
                           enemies.push_back(newEnemy);
                      }
                      enemiesSpawnedSinceStartOfWave++; spawnTimer = 0;
                  }
                  if (enemiesSpawnedSinceStartOfWave >= totalEnemiesThisWave && enemies.empty()) {
                      waveActive = false; waveWaitTimer = 5.0f;
                  }
              } else {
                  waveWaitTimer -= GetFrameTime();
                  if (waveWaitTimer <= 0 && currentWave < 40) { currentWave++; enemiesSpawnedSinceStartOfWave = 0; waveActive = true; }
              }
          }

          std::vector<TargetInfo> targets;
          targets.push_back({ localPlayer.position, &localPlayer.hp, true });
          for (auto& [id, data] : net.remotePlayers) {
              if (data.active) targets.push_back({ data.position, &data.hp, true });
          }

          std::vector<EnemySync> syncList;
          for (auto& e : enemies) {
              if (state == GameState::GAME || state == GameState::PAUSED) {
                  e->Update(targets, &baseHP, basePosition);
                  for (const auto &box : currentMap->GetObstacles()) e->HandleCollision(box);
              }
              syncList.push_back({ e->id, e->position, e->hp, e->maxHp, (int)e->type, (int)e->weapon, e->angle, e->isMoving, e->walkTimer, e->attackTimer });
          }
          
          // REMOTE PLAYER SHOOTING (PROCESS ON SERVER)
          static std::map<int, float> remoteFireCooldowns;
          for (auto& [id, data] : net.remotePlayers) {
              if (remoteFireCooldowns[id] > 0) remoteFireCooldowns[id] -= GetFrameTime();
              if (data.active && data.firing && remoteFireCooldowns[id] <= 0) {
                  Vector3 camOrigin = { data.position.x, data.position.y + 1.0f, data.position.z };
                  Vector3 camDir = Vector3Normalize(Vector3Subtract(data.camTarget, camOrigin));
                  Ray remoteRay = { camOrigin, camDir };
                  float closest = 250.0f; Enemy* hit = nullptr;
                  for (auto& e : enemies) {
                      RayCollision coll = GetRayCollisionSphere(remoteRay, (Vector3){e->position.x, e->position.y + 1.5f, e->position.z}, 1.2f);
                      if (coll.hit && coll.distance < closest) { closest = coll.distance; hit = e.get(); }
                  }
                  if (hit) {
                      hit->hp -= 35;
                      if (hit->hp <= 0) { hit->hp = 0; data.money += 150; } // Reward money to remote player
                  }
                  remoteFireCooldowns[id] = 0.12f;
                  data.firing = false; 
              }
          }

          enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::shared_ptr<Enemy>& e) { return !e->active; }), enemies.end());
          for (auto& v : vehicles) v->Update(GetFrameTime());
          
          std::vector<PlayerSyncData> pSync;
          pSync.push_back({ localPlayer.playerId, localPlayer.hp, localPlayer.money });
          for (auto& [id, data] : net.remotePlayers) {
              if (data.active) pSync.push_back({ id, data.hp, data.money });
          }

          net.gameStarted = (state == GameState::GAME || state == GameState::PAUSED);
          net.SendWorldUpdate(baseHP, currentWave, syncList, pSync);
      } else if (net.mode == NetworkManager::Mode::CLIENT) {
          baseHP = net.remoteBaseHP;
          currentWave = net.remoteWave;
          if (net.gameStarted && state == GameState::LOBBY) state = GameState::GAME;
          
          if (net.remotePlayers.count(localPlayer.playerId)) {
              localPlayer.hp = net.remotePlayers[localPlayer.playerId].hp;
              localPlayer.money = net.remotePlayers[localPlayer.playerId].money;
          }
      }

      bool isFiringNow = localPlayer.currentWeapon && (localPlayer.currentWeapon->IsAutomatic() ? IsMouseButtonDown(MOUSE_LEFT_BUTTON) : IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
      static float firingStickyTimer = 0.0f;
      if (isFiringNow) firingStickyTimer = 0.1f;
      else if (firingStickyTimer > 0) firingStickyTimer -= GetFrameTime();
      
      net.SendPlayerUpdate(localPlayer.playerId, localPlayer.position, localPlayer.camera.target, localPlayer.hp, localPlayer.money, (firingStickyTimer > 0));
    } else {
      if (IsCursorHidden())
        EnableCursor();
    }

    // --- PASS 1: SHADOW MAP (Depth) ---
    BeginTextureMode(shadowTarget);
    ClearBackground(WHITE);
    BeginMode3D(shadowCam);
    currentMap->Draw(0, shadowCam.position);
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
    currentMap->Draw(0, rCam.position);
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

    BeginDrawing();
    ClearBackground(state == GameState::GAME ? SKYBLUE : DARKGRAY);

    if (state == GameState::GAME || state == GameState::PAUSED) {
      Camera3D activeCam = localPlayer.camera;
      bool inCutscene = false;
      Vector3 bossPos = {0};

      static float vehicleCamOrbitH = 0.0f;
      static float vehicleCamOrbitV = 0.3f;

      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
          auto v = vehicles[localPlayer.vehicleIndex];
          
          // Orbit Camera Control (Lowered sensitivity)
          Vector2 mouseDelta = GetMouseDelta();
          vehicleCamOrbitH += mouseDelta.x * 0.03f; 
          vehicleCamOrbitV += mouseDelta.y * 0.03f;
          if (vehicleCamOrbitV > 1.2f) vehicleCamOrbitV = 1.2f;
          if (vehicleCamOrbitV < -0.1f) vehicleCamOrbitV = -0.1f;

          float dist = 30.0f;
          activeCam.position = (Vector3){
              v->position.x - cosf(vehicleCamOrbitV) * sinf(vehicleCamOrbitH) * dist,
              v->position.y + sinf(vehicleCamOrbitV) * dist + 8.0f,
              v->position.z - cosf(vehicleCamOrbitV) * cosf(vehicleCamOrbitH) * dist
          };
          activeCam.target = Vector3Add(v->position, (Vector3){0, 5.0f, 0});
          activeCam.up = (Vector3){0, 1, 0};
          activeCam.fovy = 70.0f;

          // Sync Turret with Camera
          Tank* tank = dynamic_cast<Tank*>(v.get());
          if (tank) {
              tank->turretRotation = (vehicleCamOrbitH * RAD2DEG) - tank->rotation;
              tank->barrelPitch = -(vehicleCamOrbitV * RAD2DEG) + 10.0f; // Offset for better aiming
          }
      } else if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto& e : enemies) {
              if (e->type == EnemyType::BOSS) {
                  auto boss = std::dynamic_pointer_cast<AdasGooner>(e);
                  if (boss && boss->cutsceneState != CutsceneState::FINISHED) {
                      inCutscene = true;
                      bossPos = boss->wardrobePos;
                      break;
                  }
              }
          }
      } else {
          for (auto const& [id, e] : net.syncedEnemies) {
              if (e.type == (int)EnemyType::BOSS && e.attackTimer != (float)(int)CutsceneState::FINISHED) {
                  inCutscene = true;
                  bossPos = e.pos; 
                  break;
              }
          }
      }

      if (inCutscene) {
          activeCam.position = Vector3Add(bossPos, (Vector3){-15.0f, 15.0f, 25.0f});
          activeCam.target = Vector3Add(bossPos, (Vector3){0, 10.0f, 0});
      }

      BeginMode3D(activeCam);
      // Draw Sun Billboard only
      DrawBillboard(activeCam, sunTex, lightPos, 120.0f, WHITE);
      
      BeginShaderMode(lighting);
      currentMap->Draw(worldDetailLevel, activeCam.position);
      for (auto& v : vehicles) v->Draw();
      EndShaderMode();
      
      if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto& e : enemies) e->Draw();
      } else {
          for (auto const& [id, e] : net.syncedEnemies) {
              std::shared_ptr<Enemy> temp;
              if (e.type == (int)EnemyType::BOSS) {
                  auto adas = std::make_shared<AdasGooner>(e.pos, e.id);
                  adas->cutsceneState = (CutsceneState)(int)e.attackTimer;
                  adas->cutsceneTimer = e.walkTimer;
                  adas->isMoving = e.isMoving;
                  adas->angle = e.angle;
                  temp = adas;
              } else {
                  temp = std::make_shared<Enemy>(e.pos, (EnemyType)e.type, (WeaponType)e.weapon, e.id);
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
        if (data.active) {
          DrawCapsule((Vector3){data.position.x, data.position.y - 1.0f,
                                data.position.z},
                      (Vector3){data.position.x, data.position.y + 1.0f,
                                data.position.z},
                      0.5f, 8, 4, (id == 0) ? BLUE : DARKPURPLE);
        }
      }
      
      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
          // No viewmodel in vehicle
      } else if (!inCutscene) {
          localPlayer.DrawViewModel();
      }
      EndMode3D();

      // --- Enemy HUD Pass ---
      if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto& e : enemies) e->DrawHUD(localPlayer.camera);
      } else {
          for (auto const& [id, e] : net.syncedEnemies) {
              std::shared_ptr<Enemy> temp;
              if (e.type == (int)EnemyType::BOSS) {
                  auto adas = std::make_shared<AdasGooner>(e.pos, e.id);
                  adas->cutsceneState = (CutsceneState)(int)e.attackTimer;
                  adas->cutsceneTimer = e.walkTimer;
                  temp = adas;
              } else {
                  temp = std::make_shared<Enemy>(e.pos, (EnemyType)e.type, (WeaponType)e.weapon, e.id);
                  temp->angle = e.angle;
                  temp->isMoving = e.isMoving;
                  temp->walkTimer = e.walkTimer;
                  temp->attackTimer = e.attackTimer;
              }
              temp->hp = e.hp;
              temp->maxHp = e.maxHp;
              temp->DrawHUD(localPlayer.camera);
          }
      }

      // UI / HUD - UNIFIED SYSTEM
      DrawRectangle(10, 10, 580, 120, Fade(BLACK, 0.7f));
      DrawText(TextFormat("FPS: %d | ULTRA GRAPHICS", GetFPS()), 25, 20, 15, LIME);
      
      // --- SNIPER SCOPE OVERLAY ---
      if (fovToUse < preferredFOV) {
          int sw = GetScreenWidth();
          int sh = GetScreenHeight();
          float radius = sh * 0.45f;
          
          // Solid black background for everything outside the scope area
          // Left
          DrawRectangle(0, 0, (float)sw/2.0f - radius + 1.0f, sh, BLACK);
          // Right
          DrawRectangle((float)sw/2.0f + radius - 1.0f, 0, sw, sh, BLACK);
          // Top
          DrawRectangle(0, 0, sw, (float)sh/2.0f - radius + 1.0f, BLACK);
          // Bottom
          DrawRectangle(0, (float)sh/2.0f + radius - 1.0f, sw, sh, BLACK);
          
          // Outer vignette smoothing (rounds the square hole)
          DrawCircleGradient(Vector2{(float)sw/2.0f, (float)sh/2.0f}, radius + 2.0f, Fade(BLACK, 0), BLACK);
          
          // Sniper Crosshair
          DrawLine((float)sw/2.0f - radius, (float)sh/2.0f, (float)sw/2.0f + radius, (float)sh/2.0f, BLACK);
          DrawLine((float)sw/2.0f, (float)sh/2.0f - radius, (float)sw/2.0f, (float)sh/2.0f + radius, BLACK);
          
          // Center dot
          DrawCircle((float)sw/2.0f, (float)sh/2.0f, 2, RED);
      } else {
          // CROSSHAIR SYSTEM
          if (localPlayer.inVehicle) {
              Tank* tank = dynamic_cast<Tank*>(vehicles[localPlayer.vehicleIndex].get());
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
                  DrawLine(GetScreenWidth()/2 - 10, GetScreenHeight()/2, GetScreenWidth()/2 + 10, GetScreenHeight()/2, LIME);
                  DrawLine(GetScreenWidth()/2, GetScreenHeight()/2 - 10, GetScreenWidth()/2, GetScreenHeight()/2 + 10, LIME);
              }
          } else {
              // NORMAL CROSSHAIR
              DrawLine(GetScreenWidth()/2 - 10, GetScreenHeight()/2, GetScreenWidth()/2 + 10, GetScreenHeight()/2, LIME);
              DrawLine(GetScreenWidth()/2, GetScreenHeight()/2 - 10, GetScreenWidth()/2, GetScreenHeight()/2 + 10, LIME);
          }
      }
      
      // PLAYER HP
      DrawText(TextFormat("PLAYER HP: %d / %d", localPlayer.hp, localPlayer.maxHp), 25, 45, 20, RAYWHITE);
      DrawRectangle(320, 45, 250, 20, DARKGRAY);
      DrawRectangle(320, 45, (int)(250 * (localPlayer.hp / (float)localPlayer.maxHp)), 20, RED);

      // BASE HP
      DrawText(TextFormat("BASE HP: %d / %d", (int)baseHP, (int)maxBaseHP), 25, 80, 20, GOLD);
      DrawRectangle(320, 80, 250, 20, DARKGRAY);
      DrawRectangle(320, 80, (int)(250 * (std::max(0.0f, baseHP) / maxBaseHP)), 20, YELLOW);

      // WAVE INFO
      DrawText(TextFormat("WAVE: %d / 40", currentWave), 25, 110, 20, ORANGE);
      if (!waveActive) {
          DrawText(TextFormat("NEXT WAVE IN: %.1fs", waveWaitTimer), 180, 110, 20, SKYBLUE);
      } else {
          DrawText(TextFormat("ENEMIES: %d", (int)enemies.size()), 180, 110, 20, LIGHTGRAY);
      }


      if (baseHP <= 0) {
          DrawText("BASE LOST - NYC HAS FALLEN", GetScreenWidth() / 2 - 200, GetScreenHeight() / 2, 30, RED);
      }

      // MONEY HUD
      DrawText(TextFormat("CASH: $%d", localPlayer.money), 25, 140, 25, GREEN);

      // AMMO HUD (bottom-right)
      if (localPlayer.currentWeapon) {
          auto* w = localPlayer.currentWeapon;
          const char* ammoText = TextFormat("%d / %d", w->currentAmmo, w->magSize);
          const char* reserveText = TextFormat("RESERVE: %d", w->reserveAmmo);
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
              DrawText("RELOADING...", sw2/2 - 60, sh2/2 + 50, 20, YELLOW);
          }
      }

      // Interaction Prompt
      if (shopNearby) {
          DrawText("PRESS [E] TO BUY", GetScreenWidth()/2 - 80, GetScreenHeight()/2 + 30, 20, GOLD);
      }

      // --- DEATH TIMER ---
      if (localPlayer.isDead) {
          DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
          const char* deathMsg = "YOU DIED";
          int tw = MeasureText(deathMsg, 60);
          DrawText(deathMsg, GetScreenWidth()/2 - tw/2, GetScreenHeight()/2 - 60, 60, RED);

          const char* respawnMsg = TextFormat("RESPAWNING IN %.1fs", localPlayer.respawnTimer);
          int rtw = MeasureText(respawnMsg, 30);
          DrawText(respawnMsg, GetScreenWidth()/2 - rtw/2, GetScreenHeight()/2 + 20, 30, RAYWHITE);
      }

      // --- Kebab Interaction ---
      Vector3 bp = {0, 0, 150}; // mainBasePos
      Vector3 standPos = {bp.x + 15, 0, bp.z + 10};

      if (Vector3Distance(localPlayer.position,
                          {standPos.x - 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY SMALL KEBAB (+10 HP)",
                 GetScreenWidth() / 2 - 180, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E))
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 10);
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY MEDIUM KEBAB (+25 HP)",
                 GetScreenWidth() / 2 - 180, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E))
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 25);
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x + 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY LARGE KEBAB (+50 HP)",
                 GetScreenWidth() / 2 - 180, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E))
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 50);
      }

       if (state == GameState::PAUSED) {
         DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
         
         Rectangle menuBox = {(float)GetScreenWidth()/2 - 200, (float)GetScreenHeight()/2 - 250, 400, 500};
         DrawRectangleRounded(menuBox, 0.1f, 8, Fade(DARKGRAY, 0.9f));
         
         float p = sinf((float)GetTime() * 4.0f) * 0.2f + 0.8f;
         DrawText("SYSTEM PAUSED", (float)menuBox.x + 100, (float)menuBox.y + 30, 30, Fade(RAYWHITE, p));
         
         int menuY = (int)menuBox.y + 100;
         
         // RESUME
         Rectangle resumeRec = {menuBox.x + 50, (float)menuY, 300, 45};
         if (CheckCollisionPointRec(GetMousePosition(), resumeRec)) {
             DrawRectangleRounded(resumeRec, 0.2f, 8, SKYBLUE);
             if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = GameState::GAME;
         } else {
             DrawRectangleRounded(resumeRec, 0.2f, 8, GRAY);
         }
         DrawText("RESUME", resumeRec.x + 110, resumeRec.y + 12, 20, WHITE);
         menuY += 65;

         // FOV SETTING
         DrawText(TextFormat("FOV: %d", (int)preferredFOV), menuBox.x + 50, (float)menuY, 20, GOLD);
         Rectangle fovBar = {menuBox.x + 50, (float)menuY + 30, 300, 10};
         DrawRectangleRec(fovBar, BLACK);
         float fovPos = (preferredFOV - 40.0f) / 80.0f; 
         Rectangle fovHandle = {fovBar.x + fovPos * 300 - 5, fovBar.y - 5, 10, 20};
         DrawRectangleRec(fovHandle, YELLOW);
         if (CheckCollisionPointRec(GetMousePosition(), fovBar) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
             preferredFOV = 40.0f + ((GetMouseX() - fovBar.x) / 300.0f) * 80.0f;
             if (preferredFOV < 40) preferredFOV = 40;
             if (preferredFOV > 120) preferredFOV = 120;
         }
         menuY += 70;

         // LEVEL OF DETAIL
         DrawText("WORLD DETAIL:", menuBox.x + 50, (float)menuY, 20, GOLD);
         Rectangle lodLow = {menuBox.x + 50, (float)menuY + 30, 140, 40};
         Rectangle lodHigh = {menuBox.x + 210, (float)menuY + 30, 140, 40};
         
         if (CheckCollisionPointRec(GetMousePosition(), lodLow) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) worldDetailLevel = 0;
         if (CheckCollisionPointRec(GetMousePosition(), lodHigh) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) worldDetailLevel = 1;
         
         DrawRectangleRounded(lodLow, 0.2f, 8, worldDetailLevel == 0 ? LIME : DARKGRAY);
         DrawText("LOW", lodLow.x + 50, lodLow.y + 10, 20, WHITE);
         DrawRectangleRounded(lodHigh, 0.2f, 8, worldDetailLevel == 1 ? LIME : DARKGRAY);
         DrawText("HIGH", lodHigh.x + 45, lodHigh.y + 10, 20, WHITE);
         menuY += 90;

         if (net.mode == NetworkManager::Mode::SERVER) {
             Rectangle flyRec = {menuBox.x + 50, (float)menuY, 300, 45};
             if (CheckCollisionPointRec(GetMousePosition(), flyRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                 localPlayer.isFlying = !localPlayer.isFlying;
             DrawRectangleRounded(flyRec, 0.2f, 8, localPlayer.isFlying ? SKYBLUE : MAROON);
             DrawText(localPlayer.isFlying ? "FLY MODE: ACTIVE" : "FLY MODE: DISABLED", flyRec.x + 50, flyRec.y + 12, 18, WHITE);
         }
         
         Rectangle exitRec = {menuBox.x + 50, menuBox.y + menuBox.height - 70, 300, 45};
         if (CheckCollisionPointRec(GetMousePosition(), exitRec)) {
             DrawRectangleRounded(exitRec, 0.2f, 8, RED);
             if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) shouldExit = true;
         } else {
             DrawRectangleRounded(exitRec, 0.2f, 8, DARKPURPLE);
         }
         DrawText("QUIT TO DESKTOP", exitRec.x + 60, exitRec.y + 12, 20, WHITE);
       }
       } else if (state == GameState::LOBBY) {
           int sw = GetScreenWidth();
           int sh = GetScreenHeight();
           DrawRectangleGradientV(0, 0, sw, sh, BLACK, DARKBLUE);
           
           Rectangle lobbyBox = {50, 50, (float)sw - 100, (float)sh - 100};
           DrawRectangleRounded(lobbyBox, 0.05f, 8, Fade(BLACK, 0.5f));

           DrawText("MISSION BRIEFING", 100, 100, 35, GOLD);
           Rectangle cityRec = {100, 180, 400, 100};
           Rectangle desertRec = {100, 300, 400, 100};
           
           if (CheckCollisionPointRec(GetMousePosition(), cityRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
               delete currentMap; currentMap = new CityMap();
           }
           
           DrawRectangleRounded(cityRec, 0.2f, 8, currentMap->type == MapType::CITY ? DARKGREEN : DARKGRAY);
           DrawText("NEW YORK CITY", cityRec.x + 20, cityRec.y + 20, 25, WHITE);
           DrawText("Standard Urban Warfare", cityRec.x + 20, cityRec.y + 55, 18, (currentMap->type == MapType::CITY ? WHITE : GRAY));
           
           DrawRectangleRounded(desertRec, 0.2f, 8, currentMap->type == MapType::DESERT ? DARKGREEN : DARKGRAY);
           DrawText("DESERT (WIP)", desertRec.x + 20, desertRec.y + 20, 25, DARKGRAY);
           DrawText("Open Range Combat", desertRec.x + 20, desertRec.y + 55, 18, DARKGRAY);

           DrawText("SQUAD STATUS", (float)sw - 500, 100, 30, SKYBLUE);
           int py = 160;
           DrawText(TextFormat(">> HOST (ID:%d)", localPlayer.playerId), (float)sw - 480, (float)py, 22, LIME); py += 45;
           for (auto const& [id, data] : net.remotePlayers) {
               if (data.active) {
                 DrawText(TextFormat(">> AGENT #%d", id), (float)sw - 480, (float)py, 22, RAYWHITE); py += 45;
               }
           }

           if (net.mode == NetworkManager::Mode::SERVER) {
               Rectangle startBtn = {(float)sw/2 - 200, (float)sh - 140, 400, 70};
               bool hov = CheckCollisionPointRec(GetMousePosition(), startBtn);
               if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = GameState::GAME;
               DrawRectangleRounded(startBtn, 0.3f, 8, hov ? RED : MAROON);
               DrawText("EXECUTE MISSION", startBtn.x + 85, startBtn.y + 20, 25, WHITE);
           } else {
               float pulse = sinf((float)GetTime() * 3.0f) * 0.2f + 0.8f;
               DrawText("WAITING FOR COMMANDER...", (float)sw/2 - 200, (float)sh - 110, 25, Fade(RAYWHITE, pulse));
           }

      } else if (state == GameState::MENU) {
          menu.Update();
          menu.Draw();
          if (menu.shouldStartHost) {
              if (net.StartServer(1234)) state = GameState::LOBBY;
              menu.shouldStartHost = false;
          }
          
          // Discover and draw servers
          int listY = 450;
          for (auto const &[ip, info] : net.discoveredServers) {
            Rectangle rec = {(float)GetScreenWidth() / 2 - 150, (float)listY, 300, 45};
            bool hov = CheckCollisionPointRec(GetMousePosition(), rec);
            DrawRectangleRounded(rec, 0.2f, 8, Fade(hov ? SKYBLUE : DARKBLUE, 0.8f));
            DrawText(TextFormat("%s (%s)", info.name.c_str(), info.ip.c_str()), (int)rec.x + 15, (int)rec.y + 12, 18, RAYWHITE);
            if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (net.StartClient(info.ip, 1234)) state = GameState::LOBBY;
            }
            listY += 55;
          }
       }
    EndDrawing();
  }
  UnloadRenderTexture(reflectionTarget);
  UnloadRenderTexture(shadowTarget);
  UnloadShader(lighting);
  net.SendDisconnect(localPlayer.playerId);
  delete currentMap;
  CloseWindow();
  return 0;
}

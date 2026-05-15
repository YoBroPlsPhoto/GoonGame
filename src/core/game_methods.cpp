#include "core/game.hpp"

void Game::UpdateCore() {
                    if (gang && gang->cutsceneState != GangCutscene::FIGHT) {
                        inCutscene = true; bossPos = gang->wardrobePos; activeBoss = gang; break;
                    }
                }
            }
        } else {
            for (auto const& [id, e] : net.syncedEnemies) {
                if (e.type == (int)EnemyType::BOSS && e.attackTimer != (float)(int)CutsceneState::FINISHED) {
                    inCutscene = true; bossPos = e.pos; break;
                }
                if (e.type == (int)EnemyType::GIBON_BOSS && e.attackTimer != (float)(int)GibonState::FINISHED_FALLING) {
                    inCutscene = true; bossPos = e.pos; break;
                }
                if (e.type == (int)EnemyType::GANG_BOSS && e.attackTimer != (float)(int)GangCutscene::FIGHT) {
                    inCutscene = true; bossPos = e.pos; break;
                }
            }
        }
    }

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


}

void Game::UpdateCutscenes() {
    effects.Update(GetFrameTime());

    float preferredFOV = 70.0f;
    int worldDetailLevel = 1; // 0 = Low, 1 = High
    float fovToUse = preferredFOV;
    if (localPlayer.currentWeapon && !localPlayer.inVehicle) {
        fovToUse = localPlayer.currentWeapon->GetFOV(preferredFOV);
    }
    localPlayer.camera.fovy = fovToUse;
    
    if (state == GameState::GAME || state == GameState::LOBBY || state == GameState::PAUSED) {
      if (state == GameState::GAME && !localPlayer.showInventory && !IsCursorHidden())
        DisableCursor();
      else if ((state == GameState::LOBBY || state == GameState::PAUSED || (state == GameState::GAME && localPlayer.showInventory)) && IsCursorHidden())
        EnableCursor();
        
      if (state == GameState::GAME) {
          if (IsKeyPressed(KEY_GRAVE)) {
              localPlayer.showInventory = !localPlayer.showInventory;
          }
          if (IsKeyPressed(KEY_Q)) {
              if (localPlayer.previousWeapon && localPlayer.previousWeapon != localPlayer.currentWeapon) {
                  Weapon* temp = localPlayer.currentWeapon;
                  localPlayer.currentWeapon = localPlayer.previousWeapon;
                  localPlayer.previousWeapon = temp;
              }
          }
          auto cycleWeapon = [&](const std::vector<int>& validSizes) {
              int currentIndex = -1;
              for (size_t i = 0; i < localPlayer.inventory.size(); i++) {
                  if (localPlayer.inventory[i] == localPlayer.currentWeapon) {
                      currentIndex = (int)i; break;
                  }
              }
              // If not matched, default to end to scan from zero
              if (currentIndex == -1) currentIndex = localPlayer.inventory.size() - 1;
              
              for (size_t i = 1; i <= localPlayer.inventory.size(); i++) {
                  size_t idx = (currentIndex + i) % localPlayer.inventory.size();
                  auto* w = localPlayer.inventory[idx];
                  if (std::find(validSizes.begin(), validSizes.end(), w->magSize) != validSizes.end()) {
                      if (localPlayer.currentWeapon != w) {
                          localPlayer.previousWeapon = localPlayer.currentWeapon;
                          localPlayer.currentWeapon = w;
                      }
                      break;
                  }
              }
          };

          if (IsKeyPressed(KEY_ONE)) cycleWeapon({30, 200});
          if (IsKeyPressed(KEY_TWO)) cycleWeapon({17, 6});
          if (IsKeyPressed(KEY_THREE)) cycleWeapon({0});
          if (IsKeyPressed(KEY_FOUR)) cycleWeapon({8, 5, 1});
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

}

void Game::UpdateLobby() {
              if ((char)charPressed == moneyCheat[moneyIdx]) {
                  moneyIdx++;
                  if (moneyIdx == 4) { localPlayer.AddMoney(10000); moneyIdx = 0; }
              } else {
                  moneyIdx = ((char)charPressed == moneyCheat[0]) ? 1 : 0;

}

void Game::UpdateEffects() {
              
              if ((char)charPressed == gibonCheat[gibonIdx]) {
                  gibonIdx++;
                  if (gibonIdx == 5) { 
                      currentWave = 20; enemies.clear(); enemiesSpawnedSinceStartOfWave = 0; waveActive = true; gibonIdx = 0; 
                  }
              } else {
                  gibonIdx = ((char)charPressed == gibonCheat[0]) ? 1 : 0;
              }

}

void Game::UpdateCursor() {
                  if (gangIdx == 8) { 
                      currentWave = 30; enemies.clear(); enemiesSpawnedSinceStartOfWave = 0; waveActive = true; gangIdx = 0; 
                  }
              } else {
                  gangIdx = ((char)charPressed == gangCheat[0]) ? 1 : 0;
              }
              

}

void Game::HandleInput() {
          }

          if (!inCutscene) {
              localPlayer.Update();
              for (const auto &box : currentMap->GetObstacles())
                localPlayer.HandleCollision(box);
          }
      }
        
      if (state == GameState::GAME && !inCutscene) {
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
              if (tank) {
                  // Sync localPlayer position to tank for networking/logic
                  localPlayer.position = tank->position;

                  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && tank->CanFireCannon()) {
                      tank->FireCannon();
                      Ray cannonRay = {tank->GetBarrelTip(), tank->GetTurretForward()};
                      
                      // Perform comprehensive raycast
                      float closestDist = 2000.0f;
                      Vector3 impactPoint = Vector3Add(tank->GetBarrelTip(), Vector3Scale(tank->GetTurretForward(), 2000.0f));
                      
                      // 1. Check world obstacles
                      for (const auto& box : currentMap->GetObstacles()) {
                          auto coll = GetRayCollisionBox(cannonRay, box);
                          if (coll.hit && coll.distance < closestDist) {
                              closestDist = coll.distance;
                              impactPoint = coll.point;
                          }
                      }
                      
                      // 2. Check enemies
                      for (auto& e : enemies) {
                          if (!e->active || e->hp <= 0) continue;
                          float hitDist;
                          if (e->RayHit(cannonRay, hitDist) && hitDist < closestDist) {
                              closestDist = hitDist;
                              impactPoint = Vector3Add(cannonRay.position, Vector3Scale(cannonRay.direction, hitDist));
                          }
                      }
                      
                      // 3. Ground collision (if nothing hit)
                      if (closestDist > 1999.0f) {
                          if (cannonRay.direction.y < 0) {
                              float t = -cannonRay.position.y / cannonRay.direction.y;
                              impactPoint = Vector3Add(cannonRay.position, Vector3Scale(cannonRay.direction, t));
                          }
                      }

                      // SPAWN EFFECTS
                      effects.SpawnExplosion(impactPoint);
                      effects.SpawnBulletTracer(tank->GetBarrelTip(), impactPoint, {255, 150, 50, 255});
                      effects.SpawnHitSparks(impactPoint, 40);
                      
                      // APPLY AOE DAMAGE (Splash)
                      float splashRadius = 25.0f;
                      float baseDamage = tank->cannonDamage;
                      
                      for (auto& e : enemies) {
                          if (!e->active || e->hp <= 0) continue;
                          float oldHp = e->hp;
                          e->TakeAOEDamage(impactPoint, splashRadius, (int)baseDamage);
                          if (oldHp > 0 && e->hp <= 0) {
                              e->hp = 0;
                              localPlayer.AddMoney(250);
                          }
                      }
                  }
              }
          }

          // --- WEAPON SHOOTING ---
          bool isFiring = localPlayer.currentWeapon && !localPlayer.showInventory && (localPlayer.currentWeapon->IsAutomatic() ? IsMouseButtonDown(MOUSE_LEFT_BUTTON) : IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
          if (isFiring && localPlayer.currentWeapon->CanFire() && !localPlayer.inVehicle) {
              localPlayer.currentWeapon->Fire();
              
              Vector3 up = {0, 1, 0};
              Vector3 camRight = Vector3Normalize(Vector3CrossProduct(ray.direction, up));
              Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, ray.direction));
              
              Vector3 muzzlePos = Vector3Add(ray.position, Vector3Scale(ray.direction, 1.2f)); // forward
              muzzlePos = Vector3Add(muzzlePos, Vector3Scale(camRight, 0.4f)); // right offset
              muzzlePos = Vector3Add(muzzlePos, Vector3Scale(camUp, -0.2f));   // down offset
              
              if (localPlayer.currentWeapon->magSize > 0) {
                  effects.SpawnMuzzleFlash(muzzlePos);
              }

              if (net.mode == NetworkManager::Mode::SERVER) {
                  float maxDist = 200.0f;
                  if (localPlayer.currentWeapon->magSize == 0) maxDist = 3.5f;       // Knife
                  else if (localPlayer.currentWeapon->magSize == 8) maxDist = 40.0f; // Shotgun

}

void Game::HandleShooting() {
                  float closestDist = maxDist; Enemy* hitEnemy = nullptr;
                  for (auto& e : enemies) {
                      if (!e->active || e->hp <= 0) continue;
                      float hitDist;
                      if (e->RayHit(ray, hitDist) && hitDist < closestDist) { 
                          closestDist = hitDist; 
                          hitEnemy = e.get(); 
                      }
                  }
                  if (hitEnemy) {
                      float oldHp = hitEnemy->hp;
                      float dealtDamage = localPlayer.currentWeapon->damage;
                      
                      Vector3 hitPoint = Vector3Add(ray.position, Vector3Scale(ray.direction, closestDist));
                      if (localPlayer.currentWeapon->magSize > 0) {
                          effects.SpawnBloodSplash(hitPoint, 8);
                      }
                      
                      // Shotgun damage falloff (starts dropping after 10 meters)
                      if (localPlayer.currentWeapon->magSize == 8 && closestDist > 10.0f) {
                          float falloff = 10.0f / closestDist; // Example: at 20m -> 50%, at 40m -> 25% damage
                          dealtDamage *= falloff;
                      }
                      
                      hitEnemy->TakeDamage((int)dealtDamage); 
                      if (hitEnemy->hp <= 0) {
                          hitEnemy->hp = 0;
                          if (oldHp > 0) {
                              int reward = 50;
                              if (hitEnemy->type == EnemyType::SHOOTER) reward = 75;
                              else if (hitEnemy->type == EnemyType::FAST) reward = 100;
                              else if (hitEnemy->type == EnemyType::TANK) reward = 250;
                              else if (hitEnemy->type == EnemyType::MINION) reward = 60;
                              else if (hitEnemy->type == EnemyType::ELITE) reward = 400;
                              else if (hitEnemy->type == EnemyType::KAMIKAZE) reward = 150;
                              else if (hitEnemy->type == EnemyType::GIGA_TANK) reward = 1000;
                              else if (hitEnemy->type == EnemyType::BOSS) reward = 2000;
                              localPlayer.AddMoney(reward);
                          }
                      }
                  } else {
                      // Missed enemy, bullet goes to max distance
                      if (localPlayer.currentWeapon->magSize > 0) {
                          Vector3 hitPoint = Vector3Add(ray.position, Vector3Scale(ray.direction, closestDist));
                          effects.SpawnHitSparks(hitPoint, 5);
                      }
                  }
                  
                  // Spawn Bullet Tracers
                  if (localPlayer.currentWeapon->magSize > 0) {
                      Vector3 tracerEnd = Vector3Add(ray.position, Vector3Scale(ray.direction, closestDist));
                      if (localPlayer.currentWeapon->magSize == 8) { // Shotgun
                          effects.SpawnShotgunPelletTrails(muzzlePos, tracerEnd);
                          for (int i=0; i<3; i++) {
                              Vector3 randOffset = {(float)GetRandomValue(-200, 200)/100.0f, (float)GetRandomValue(-200, 200)/100.0f, (float)GetRandomValue(-200, 200)/100.0f};
                              effects.SpawnShotgunPelletTrails(muzzlePos, Vector3Add(tracerEnd, randOffset));
                          }
                      } else if (localPlayer.currentWeapon->magSize == 1) { // RPG
                          effects.SpawnExplosion(tracerEnd);
                          effects.SpawnHitSparks(tracerEnd, 20);
                      } else {
                          effects.SpawnBulletTracer(muzzlePos, tracerEnd);
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
                      localPlayer.inventory.push_back(newWep);
                      if (!localPlayer.currentWeapon) localPlayer.currentWeapon = newWep;
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
              int enemyIdCounter = 0;
              int totalEnemiesThisWave = 5 + currentWave * 4;
              if (currentWave == 10 || currentWave == 20 || currentWave == 30) totalEnemiesThisWave = 1;
              if (currentWave == 40) totalEnemiesThisWave = 15; // 1 boss + 14 guards
               if (((int)(GetTime()*60)) % 60 == 0) printf("Wave: %d, Spawned: %d/%d, Size: %d, inCutscene: %d\n", currentWave, enemiesSpawnedSinceStartOfWave, totalEnemiesThisWave, (int)enemies.size(), inCutscene);
              if (waveActive && !inCutscene) {
                  spawnTimer += GetFrameTime();
                  float spawnDelay = std::max(0.2f, 1.5f - currentWave * 0.05f);
                  if (spawnTimer > spawnDelay && enemiesSpawnedSinceStartOfWave < totalEnemiesThisWave) {
                      Vector3 spawnPos = { (float)GetRandomValue(-15, 15), 1.0f, -350.0f };
                      EnemyType t = EnemyType::MELEE;
                      if (currentWave == 40) {
                          if (enemiesSpawnedSinceStartOfWave == 0) {
                              Vector3 primeSpawn = { 0.0f, 0.1f, -400.0f };
                              enemies.push_back(std::make_shared<AdasPrime>(primeSpawn, ++enemyIdCounter));
                              if (!goonMusicPlaying) { PlayMusicStream(goonMusic); goonMusicPlaying = true; }
                          } else {
                              // Spawn guards for the prime
                              EnemyType gt = (GetRandomValue(0, 1) == 0 ? EnemyType::TANK : EnemyType::ELITE);
                              auto guard = std::make_shared<Enemy>(spawnPos, gt, WeaponType::SHOTGUN, ++enemyIdCounter);
                              guard->hp *= 2.0f; guard->maxHp = guard->hp;
                              enemies.push_back(guard);
                          }
                      } else if (currentWave == 30) {
                          if (enemiesSpawnedSinceStartOfWave == 0) {
                              Vector3 gangSpawn = { 0.0f, 0.1f, -400.0f };
                              enemies.push_back(std::make_shared<GangBoss>(gangSpawn, ++enemyIdCounter));
                          }
                      } else if (currentWave == 20) {
                          if (enemiesSpawnedSinceStartOfWave == 0) {
                              Vector3 gibonLanding = { 0.0f, 0.1f, -450.0f };
                              enemies.push_back(std::make_shared<GibonRzygacz>(gibonLanding, ++enemyIdCounter));
                          }
                      } else if (currentWave == 10) {
                          if (enemiesSpawnedSinceStartOfWave == 0) {
                              enemies.push_back(std::make_shared<AdasGooner>(spawnPos, ++enemyIdCounter));
                              if (!goonMusicPlaying) { PlayMusicStream(goonMusic); goonMusicPlaying = true; }
                          }
                      } else {
                           int roll = GetRandomValue(0, 100);
                           if (currentWave < 3) {
                               if (roll < 45) t = EnemyType::MELEE; else if (roll < 75) t = EnemyType::SHOOTER; else t = EnemyType::MINION;
                           } else if (currentWave < 6) {
                               if (roll < 30) t = EnemyType::MELEE; else if (roll < 50) t = EnemyType::SHOOTER; else if (roll < 70) t = EnemyType::FAST; else if (roll < 85) t = EnemyType::TANK; else t = EnemyType::MINION;
                           } else if (currentWave < 12) {
                               if (roll < 25) t = EnemyType::MELEE; else if (roll < 45) t = EnemyType::SHOOTER; else if (roll < 60) t = EnemyType::FAST; else if (roll < 75) t = EnemyType::TANK; else if (roll < 85) t = EnemyType::MINION; else if (roll < 93) t = EnemyType::ELITE; else t = EnemyType::KAMIKAZE;
                           } else {
                               if (roll < 20) t = EnemyType::MELEE; else if (roll < 35) t = EnemyType::SHOOTER; else if (roll < 50) t = EnemyType::FAST; else if (roll < 65) t = EnemyType::TANK; else if (roll < 75) t = EnemyType::MINION; else if (roll < 85) t = EnemyType::ELITE; else if (roll < 95) t = EnemyType::KAMIKAZE; else t = EnemyType::GIGA_TANK;
                           }
                           WeaponType w = WeaponType::PISTOL;
                           if (t == EnemyType::MELEE) w = (GetRandomValue(0, 1) == 0 ? WeaponType::KATANA : WeaponType::MACHETE); else if (t == EnemyType::SHOOTER) w = WeaponType::PISTOL; else if (t == EnemyType::FAST) w = WeaponType::MACHETE; else if (t == EnemyType::TANK) w = WeaponType::KATANA; else if (t == EnemyType::MINION) w = WeaponType::MACHETE; else if (t == EnemyType::ELITE) w = WeaponType::SHOTGUN; else if (t == EnemyType::KAMIKAZE) w = WeaponType::EXPLOSIVE; else if (t == EnemyType::GIGA_TANK) w = WeaponType::MACHETE;
                           auto newEnemy = std::make_shared<Enemy>(spawnPos, t, w, ++enemyIdCounter);
                           newEnemy->hp *= (1.0f + currentWave * 0.15f); newEnemy->maxHp = newEnemy->hp;
                           enemies.push_back(newEnemy);
                      }
                      enemiesSpawnedSinceStartOfWave++; spawnTimer = 0;
                  }
                  if (enemiesSpawnedSinceStartOfWave >= totalEnemiesThisWave && enemies.empty()) {
                      if (currentWave == 40) {
                          // GAME END - VICTORY
                          state = GameState::MENU;
                          net.gameStarted = false;
                          currentWave = 1;
                          if (goonMusicPlaying) { StopMusicStream(goonMusic); goonMusicPlaying = false; }
                      } else {
                          waveActive = false; waveWaitTimer = 5.0f;
                      }
                  }
                  
                  if (goonMusicPlaying) UpdateMusicStream(goonMusic);
              }

              if (!waveActive) {
                  waveWaitTimer -= GetFrameTime();
                  if (waveWaitTimer <= 0 && currentWave < 999) { currentWave++; enemiesSpawnedSinceStartOfWave = 0; waveActive = true; }
              }

              std::vector<TargetInfo> targets;
              // Redirect damage to tank if player is inside
              int* localTargetHP = &localPlayer.hp;
              if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
                  localTargetHP = &vehicles[localPlayer.vehicleIndex]->health;
              }
              targets.push_back({ localPlayer.position, localTargetHP, true });
          
          for (auto& [id, data] : net.remotePlayers) {
              if (data.active) targets.push_back({ data.position, &data.hp, true });
          }

          std::vector<EnemySync> syncList;
          for (auto& e : enemies) {
              if (state == GameState::GAME || state == GameState::PAUSED) {
                  if (!inCutscene || e == activeBoss) {
                      e->Update(targets, &baseHP, basePosition);
                      if (!inCutscene) {
                          for (const auto &box : currentMap->GetObstacles()) e->HandleCollision(box);
                      }
                  }
              }
              syncList.push_back({ e->id, e->position, e->hp, e->maxHp, (int)e->type, (int)e->weapon, e->angle, e->isMoving, e->walkTimer, e->attackTimer });
              // Override sync fields for Gibon boss (same pattern as AdasGooner)
              if (e->type == EnemyType::GIBON_BOSS) {
                  auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
                  if (gibon) {
                      syncList.back().attackTimer = (float)(int)gibon->gibonState;
                      syncList.back().walkTimer = gibon->stateTimer;
                  }
              }
              // Override sync fields for Gang boss
              if (e->type == EnemyType::GANG_BOSS) {
                  auto gang = std::dynamic_pointer_cast<GangBoss>(e);
                  if (gang) {
                      syncList.back().attackTimer = (float)(int)gang->cutsceneState;
                      syncList.back().walkTimer = gang->stateTimer;
                  }
              }
              // Override sync fields for Adas Prime
              if (e->type == EnemyType::ADAS_PRIME) {
                  auto prime = std::dynamic_pointer_cast<AdasPrime>(e);
                  if (prime) {
                      syncList.back().attackTimer = (float)(int)prime->cutsceneState;
                      syncList.back().walkTimer = prime->cutsceneTimer;
                  }
              }
          }
          
          // REMOTE PLAYER SHOOTING (PROCESS ON SERVER)
          std::map<int, float> remoteFireCooldowns;
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
                      float oldHp = hit->hp;
                      hit->TakeDamage(35);
                      if (hit->hp <= 0 && oldHp > 0) { data.money += 150; } // Reward money to remote player
                  }
                  remoteFireCooldowns[id] = 0.12f;
                  data.firing = false; 
              }
          }

          enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::shared_ptr<Enemy>& e) { return !e->active; }), enemies.end());
          for (auto it = vehicles.begin(); it != vehicles.end(); ) {
              auto& v = *it;
              v->Update(GetFrameTime());
              
              // --- VEHICLE DESTRUCTION ---
              if (v->health <= 0) {
                  effects.SpawnExplosion(v->position);
                  if (v->isOccupied) {

}

void Game::HandleShop() {
                          localPlayer.hp = 0; // DIE!
                          localPlayer.inVehicle = false;
                          localPlayer.vehicleIndex = -1;
                      }
                  }
                  it = vehicles.erase(it);
                  continue;
              }

              // --- TANK CRUSHING LOGIC ---
              Tank* tank = dynamic_cast<Tank*>(v.get());
              if (tank && fabsf(tank->speed) > 0.1f) {
                  BoundingBox tankBox = tank->GetBoundingBox();
                  for (auto& e : enemies) {
                      if (!e->active || e->hp <= 0) continue;
                      if (CheckCollisionBoxes(tankBox, e->GetBoundingBox())) {
                          float crushDamage = (1000.0f + fabsf(tank->speed) * 4000.0f) * GetFrameTime();
                          float oldHp = e->hp;
                          e->TakeDamage((int)crushDamage);
                          if (e->hp <= 0 && oldHp > 0) {
                              localPlayer.AddMoney(100);
                          }
                      }
                  }
              }
              it++;
          }
          
          std::vector<PlayerSyncData> pSync;
          pSync.push_back({ localPlayer.playerId, localPlayer.hp, localPlayer.money });
          for (auto& [id, data] : net.remotePlayers) {
              if (data.active) pSync.push_back({ id, data.hp, data.money });
          }

          net.gameStarted = (state == GameState::GAME || state == GameState::PAUSED);
          net.SendWorldUpdate(baseHP, currentWave, syncList, pSync);
      }
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
      float firingStickyTimer = 0.0f;
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

      float vehicleCamOrbitH = 0.0f;
      float vehicleCamOrbitV = 0.3f;

      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
          auto v = vehicles[localPlayer.vehicleIndex];
          

}

void Game::UpdateNetworkAndEnemies() {
          Vector2 mouseDelta = GetMouseDelta();
          vehicleCamOrbitH -= mouseDelta.x * 0.015f; 
          vehicleCamOrbitV += mouseDelta.y * 0.015f;

          if (vehicleCamOrbitV > 1.2f) vehicleCamOrbitV = 1.2f;
          if (vehicleCamOrbitV < -0.1f) vehicleCamOrbitV = -0.1f;

          float dist = 30.0f;
          activeCam.position = (Vector3){
              v->position.x - cosf(vehicleCamOrbitV) * sinf(vehicleCamOrbitH) * dist,
              v->position.y + sinf(vehicleCamOrbitV) * dist + 8.0f,
              v->position.z - cosf(vehicleCamOrbitV) * cosf(vehicleCamOrbitH) * dist
          };
          activeCam.target = Vector3Add(v->position, (Vector3){0, 4.0f, 0}); // Target the turret height
          activeCam.up = (Vector3){0, 1, 0};
          activeCam.fovy = 70.0f;

          // Sync Turret with Camera
          Tank* tank = dynamic_cast<Tank*>(v.get());
          if (tank) {
              tank->turretRotation = (vehicleCamOrbitH * RAD2DEG) - tank->rotation;
              tank->barrelPitch = -(vehicleCamOrbitV * RAD2DEG); // Removed offset for precision
          }
      }

      // --- GIBON FPS STUTTER EFFECT ---
      bool gibonStuttering = false;
      if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto& e : enemies) {
              if (e->type == EnemyType::GIBON_BOSS) {
                  auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
                  if (gibon && gibon->active) {
                      if (gibon->isStuttering || gibon->gibonState == GibonState::FALLING || gibon->gibonState == GibonState::IMPACT) {
                          gibonStuttering = true;
                      }
                  }
              }
          }
      }
      if (gibonStuttering) {
          // Simulate FPS drop by burning CPU time
          double waitUntil = GetTime() + 0.04 + (double)(rand() % 60) / 1000.0;
          while (GetTime() < waitUntil) { /* spin */ }
      }

      if (inCutscene) {
          // Check if it's a Gibon cutscene (falling from sky) - use different camera angle
          bool isGibonCutscene = false;
          if (net.mode == NetworkManager::Mode::SERVER) {
              for (auto& e : enemies) {
                  if (e->type == EnemyType::GIBON_BOSS) {
                      auto gibon = std::dynamic_pointer_cast<GibonRzygacz>(e);
                      if (gibon && gibon->gibonState != GibonState::FINISHED_FALLING) {
                          isGibonCutscene = true;
                          bossPos = gibon->position;
                      }
                  }
              }
          } else {
              for (auto const& [id, e] : net.syncedEnemies) {
                  if (e.type == (int)EnemyType::GIBON_BOSS) { isGibonCutscene = true; bossPos = e.pos; }
              }
          }
          
          bool isGangCutscene = false;
          if (net.mode == NetworkManager::Mode::SERVER) {
              for (auto& e : enemies) {
                  if (e->type == EnemyType::GANG_BOSS) {
                      auto gang = std::dynamic_pointer_cast<GangBoss>(e);
                      if (gang && gang->cutsceneState != GangCutscene::FIGHT) {
                          isGangCutscene = true;
                          bossPos = gang->wardrobePos;
                      }
                  }
              }
          } else {
              for (auto const& [id, e] : net.syncedEnemies) {
                  if (e.type == (int)EnemyType::GANG_BOSS) { isGangCutscene = true; bossPos = e.pos; }
              }
          }

          bool isPrimeCutscene = false;
          if (net.mode == NetworkManager::Mode::SERVER) {
              for (auto& e : enemies) {
                  if (e->type == EnemyType::ADAS_PRIME) {
                      auto p = std::dynamic_pointer_cast<AdasPrime>(e);
                      if (p && p->cutsceneState != PrimeCutscene::FIGHT) {
                          isPrimeCutscene = true;
                          bossPos = p->wardrobePos;
                      }
                  }
              }
          } else {
              for (auto const& [id, e] : net.syncedEnemies) {
                  if (e.type == (int)EnemyType::ADAS_PRIME && e.attackTimer != (float)(int)PrimeCutscene::FIGHT) {
                      isPrimeCutscene = true;
                      bossPos = e.pos; 
                  }
              }
          }
          
          if (isGibonCutscene) {
              // Camera on the ground looking up at the falling ball, staying still on XZ
              Vector3 camPos = { bossPos.x + 30.0f, 5.0f, bossPos.z + 40.0f };
              activeCam.position = camPos;
              activeCam.target = bossPos;
          } else {
              activeCam.position = Vector3Add(bossPos, (Vector3){-15.0f, 15.0f, 25.0f});
              activeCam.target = Vector3Add(bossPos, (Vector3){0, 10.0f, 0});
          }
      }

      BeginMode3D(activeCam);
      // Draw Sun Billboard only
      DrawBillboard(activeCam, sunTex, lightPos, 120.0f, WHITE);
      
      BeginShaderMode(lighting);
      currentMap->Draw(worldDetailLevel, activeCam.position);
      for (auto& v : vehicles) v->Draw();
      EndShaderMode();
      
      effects.Draw();
      
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
              } else if (e.type == (int)EnemyType::GIBON_BOSS) {
                  auto gibon = std::make_shared<GibonRzygacz>(e.pos, e.id);
                  gibon->gibonState = (GibonState)(int)e.attackTimer;
                  gibon->stateTimer = e.walkTimer;
                  gibon->isMoving = e.isMoving;
                  gibon->angle = e.angle;
                  gibon->craterCreated = ((int)e.attackTimer >= (int)GibonState::IMPACT);
                  gibon->impactCrater = {e.pos, 20.0f};
                  temp = gibon;
              } else if (e.type == (int)EnemyType::GANG_BOSS) {
                  auto gang = std::make_shared<GangBoss>(e.pos, e.id);
                  gang->cutsceneState = (GangCutscene)(int)e.attackTimer;
                  gang->stateTimer = e.walkTimer;
                  gang->wardrobePos = e.pos;
                  gang->wardrobePos.y = (gang->cutsceneState == GangCutscene::WARDROBE_FALLING) ? (400.0f - e.walkTimer * 60.0f) : 0.0f;
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
        // Skip drawing player if they are inside a vehicle
        bool playerInVehicle = false;
        for (auto& v : vehicles) {
            if (Vector3Distance(data.position, v->position) < 8.0f && v->isOccupied) {
                playerInVehicle = true;
                break;
            }
        }
        if (data.active && !playerInVehicle) {
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
          for (auto& e : enemies) {
              if (e->type != EnemyType::BOSS && e->type != EnemyType::GIBON_BOSS && 
                  e->type != EnemyType::GANG_BOSS && e->type != EnemyType::ADAS_PRIME) {
                  e->DrawHUD(activeCam);
              }
          }
      } else {
          for (auto const& [id, e] : net.syncedEnemies) {
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
                  temp = std::make_shared<Enemy>(e.pos, (EnemyType)e.type, (WeaponType)e.weapon, e.id);
                  temp->angle = e.angle;
                  temp->isMoving = e.isMoving;
                  temp->walkTimer = e.walkTimer;
                  temp->attackTimer = e.attackTimer;
              }
              temp->hp = e.hp;
              temp->maxHp = e.maxHp;
              
              // Only draw individual HUD if not a boss (bosses get global bar)
              if (temp->type != EnemyType::BOSS && temp->type != EnemyType::GIBON_BOSS && 
                  temp->type != EnemyType::GANG_BOSS && temp->type != EnemyType::ADAS_PRIME) {
                  temp->DrawHUD(activeCam);
              }
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
          DrawCircleGradient((Vector2){(float)sw/2.0f, (float)sh/2.0f}, radius + 2.0f, Fade(BLACK, 0), BLACK);
          
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
      int displayPlayerHp = std::max(0, localPlayer.hp);
      float playerRatio = std::max(0.0f, std::min(1.0f, localPlayer.hp / (float)localPlayer.maxHp));
      DrawText(TextFormat("PLAYER HP: %d / %d", displayPlayerHp, localPlayer.maxHp), 25, 45, 18, RAYWHITE);
      DrawRectangle(320, 45, 250, 15, DARKGRAY);
      DrawRectangle(320, 45, (int)(250 * playerRatio), 15, RED);

      // VEHICLE HP
      if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
          auto v = vehicles[localPlayer.vehicleIndex];
          int displayVehHp = std::max(0, v->health);
          float vehRatio = std::max(0.0f, std::min(1.0f, v->health / (float)v->maxHealth));
          DrawText(TextFormat("VEHICLE HP: %d / %d", displayVehHp, v->maxHealth), 25, 65, 18, SKYBLUE);
          DrawRectangle(320, 65, 250, 15, DARKGRAY);
          DrawRectangle(320, 65, (int)(250 * vehRatio), 15, BLUE);
      }


      // BASE HP
      float baseRatio = std::max(0.0f, std::min(1.0f, baseHP / maxBaseHP));
      DrawText(TextFormat("BASE HP: %d / %d", (int)std::max(0.0f, baseHP), (int)maxBaseHP), 25, 80, 20, GOLD);
      DrawRectangle(320, 80, 250, 20, DARKGRAY);
      DrawRectangle(320, 80, (int)(250 * baseRatio), 20, YELLOW);

      // WAVE INFO
      DrawText(TextFormat("WAVE: %d", currentWave), 25, 110, 20, ORANGE);
      if (!waveActive) {
          DrawText(TextFormat("NEXT WAVE IN: %.1fs", waveWaitTimer), 180, 110, 20, SKYBLUE);
      } else {
          DrawText(TextFormat("ENEMIES: %d", (int)enemies.size()), 180, 110, 20, LIGHTGRAY);
      }

      // --- GLOBAL BOSS HP BAR ---
      std::shared_ptr<Enemy> activeBoss = nullptr;
      if (net.mode == NetworkManager::Mode::SERVER) {
          for (auto& e : enemies) {
              if (e->type == EnemyType::BOSS || e->type == EnemyType::GIBON_BOSS || 
                  e->type == EnemyType::GANG_BOSS || e->type == EnemyType::ADAS_PRIME) {
                  activeBoss = e; break;
              }
          }
      } else {
          // Client side boss detection
          for (auto const& [id, e] : net.syncedEnemies) {
               if (e.type == (int)EnemyType::BOSS || e.type == (int)EnemyType::GIBON_BOSS || 
                   e.type == (int)EnemyType::GANG_BOSS || e.type == (int)EnemyType::ADAS_PRIME) {
                   activeBoss = std::make_shared<Enemy>(e.pos, (EnemyType)e.type, WeaponType::KATANA, e.id);
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
          int bx = sw/2 - bw/2;
          int by = 50;
          
          float bossHpRatio = (float)activeBoss->hp / (float)activeBoss->maxHp;
          DrawRectangle(bx - 4, by - 4, bw + 8, bh + 8, BLACK);
          DrawRectangle(bx, by, bw, bh, DARKGRAY);
          DrawRectangle(bx, by, (int)(bw * bossHpRatio), bh, RED);
          
          const char* bossName = "BOSS";
          if (activeBoss->type == EnemyType::BOSS) bossName = "ADAS GOONER";
          else if (activeBoss->type == EnemyType::GIBON_BOSS) bossName = "GIBON RZYGACZ";
          else if (activeBoss->type == EnemyType::GANG_BOSS) bossName = "THE GANG";
          else if (activeBoss->type == EnemyType::ADAS_PRIME) bossName = "ADAS PRIME";
          
          int tw = MeasureText(bossName, 25);
          DrawText(bossName, sw/2 - tw/2, by - 30, 25, GOLD);
          
          const char* hpText = TextFormat("%d / %d", activeBoss->hp, activeBoss->maxHp);
          int htw = MeasureText(hpText, 15);
          DrawText(hpText, sw/2 - htw/2, by + 7, 15, WHITE);
      }


      if (baseHP <= 0) {
          DrawText("BASE LOST - NYC HAS FALLEN", GetScreenWidth() / 2 - 200, GetScreenHeight() / 2, 30, RED);
      }

      // MONEY HUD
      DrawText(TextFormat("CASH: $%d", localPlayer.money), 25, 140, 25, GREEN);

      // AMMO HUD (bottom-right)
      if (localPlayer.currentWeapon && localPlayer.currentWeapon->magSize > 0) {
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

      // --- CS STYLE VERTICAL/HORIZONTAL EQUIPMENT HUD ---
      if (!localPlayer.showInventory) {
          int sw = GetScreenWidth();
          int sh = GetScreenHeight();
          int eqWidth = 150;
          int eqHeight = 35;
          int spacing = 5;

          std::vector<Weapon*> categories[5];
          for (auto* w : localPlayer.inventory) {
              if (w->magSize == 30 || w->magSize == 200) categories[1].push_back(w);
              else if (w->magSize == 17 || w->magSize == 6) categories[2].push_back(w);
              else if (w->magSize == 0) categories[3].push_back(w);
              else if (w->magSize == 8 || w->magSize == 5 || w->magSize == 1) categories[4].push_back(w);
          }

          int numActiveCategories = 0;
          for (int c = 1; c <= 4; c++) {
              if (!categories[c].empty()) numActiveCategories++;
          }
          
          int startY = sh / 2 - (numActiveCategories * (eqHeight + spacing)) / 2;
          int currentY = startY;

          for (int c = 1; c <= 4; c++) {
              if (categories[c].empty()) continue;
              
              for (size_t i = 0; i < categories[c].size(); i++) {
                  auto* w = categories[c][i];
                  bool isEquipped = (w == localPlayer.currentWeapon);
                  
                  // Same category weapons expand horizontally to the left
                  Rectangle slot = {(float)(sw - eqWidth - 20 - i * (eqWidth + spacing)), (float)currentY, (float)eqWidth, (float)eqHeight};
                  
                  DrawRectangleRec(slot, isEquipped ? Fade(DARKGRAY, 0.9f) : Fade(BLACK, 0.4f));
                  if (isEquipped) DrawRectangleLinesEx(slot, 2, LIME);
                  
                  const char* wName = "WEAPON";
                  if (w->magSize == 17) wName = "GLOCK";
                  else if (w->magSize == 30) wName = "AK47";

}

void Game::RenderShadowMap() {
                  else if (w->magSize == 0) wName = "KNIFE";
                  
                  DrawText(TextFormat("[%d] %s", c, wName), slot.x + 10, slot.y + 10, 16, isEquipped ? WHITE : GRAY);
              }
              currentY += (eqHeight + spacing);
          }
      }

}

void Game::RenderReflection() {
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

void Game::Render3D() {

      // --- Kebab Interaction ---
      Vector3 bp = {0, 0, 150}; // mainBasePos
      Vector3 standPos = {bp.x + 15, 0, bp.z + 10};

      if (Vector3Distance(localPlayer.position,
                          {standPos.x - 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY SMALL KEBAB ($50, +10 HP)",
                 GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 50 && localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 50;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 10);
        }
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY MEDIUM KEBAB ($100, +25 HP)",
                 GetScreenWidth() / 2 - 220, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 100 && localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 100;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 25);
        }
      } else if (Vector3Distance(localPlayer.position,
                                 {standPos.x + 3, 1, standPos.z - 4}) < 4.0f) {
        DrawText("PRESS [E] TO BUY LARGE KEBAB ($150, +50 HP)",
                 GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 + 100, 20,
                 GOLD);
        if (IsKeyPressed(KEY_E) && localPlayer.money >= 150 && localPlayer.hp < localPlayer.maxHp) {
          localPlayer.money -= 150;
          localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + 50);
        }
      }

      // --- INVENTORY UI ---
      if (state == GameState::GAME && localPlayer.showInventory) {
          int sw = GetScreenWidth();
          int sh = GetScreenHeight();
          DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));
          DrawText("INVENTORY", sw / 2 - MeasureText("INVENTORY", 40) / 2, 50, 40, GOLD);

          int startX = sw / 2 - 400;
          int startY = 150;
          for (size_t i = 0; i < localPlayer.inventory.size(); i++) {
              int row = i / 4;
              int col = i % 4;
              Rectangle slot = {(float)(startX + col * 200), (float)(startY + row * 150), 180, 120};
              bool isHovered = CheckCollisionPointRec(GetMousePosition(), slot);
              bool isEquipped = (localPlayer.currentWeapon == localPlayer.inventory[i]);

              DrawRectangleRec(slot, isEquipped ? Fade(DARKGREEN, 0.6f) : (isHovered ? Fade(GRAY, 0.6f) : Fade(DARKGRAY, 0.6f)));
              DrawRectangleLinesEx(slot, 2, isEquipped ? LIME : (isHovered ? WHITE : BLACK));
              
              const char* wName = "WEAPON";
              if (localPlayer.inventory[i]->magSize == 17) wName = "GLOCK";
              else if (localPlayer.inventory[i]->magSize == 30) wName = "AK47";
              else if (localPlayer.inventory[i]->magSize == 6) wName = "REVOLVER";
              else if (localPlayer.inventory[i]->magSize == 8) wName = "SHOTGUN";
              else if (localPlayer.inventory[i]->magSize == 10) wName = "AWP";
              else if (localPlayer.inventory[i]->magSize == 100) wName = "MINIGUN";
              else if (localPlayer.inventory[i]->magSize == 1) wName = "RPG";
              else if (localPlayer.inventory[i]->magSize == 0) wName = "KNIFE";

              DrawText(wName, slot.x + 10, slot.y + 10, 20, WHITE);
              if (localPlayer.inventory[i]->magSize > 0) {
                  DrawText(TextFormat("AMMO: %d", localPlayer.inventory[i]->currentAmmo), slot.x + 10, slot.y + 40, 15, RAYWHITE);
                  DrawText(TextFormat("RES: %d", localPlayer.inventory[i]->reserveAmmo), slot.x + 10, slot.y + 60, 15, LIGHTGRAY);
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
          DrawText("PRESS ~ TO CLOSE", sw / 2 - MeasureText("PRESS ~ TO CLOSE", 20) / 2, sh - 50, 20, LIGHTGRAY);
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

}

void Game::RenderHUD() {

}

void Game::RenderBlit() {

}


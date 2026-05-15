#include "core/game.hpp"

void Game::HandleInput() {
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

      if (state == GameState::GAME && !inCutscene) {
        // --- INTERACTION & SHOOTING LOGIC ---
        Ray ray = {
            localPlayer.camera.position,
            Vector3Normalize(Vector3Subtract(localPlayer.camera.target,
                                             localPlayer.camera.position))};

        // --- VEHICLE INTERACTION (F Key) ---
        if (IsKeyPressed(KEY_F)) {
          if (localPlayer.inVehicle) {
            if (localPlayer.vehicleIndex >= 0 &&
                localPlayer.vehicleIndex < (int)vehicles.size()) {
              Vector3 exitPos = vehicles[localPlayer.vehicleIndex]->position;
              Vector3 right = vehicles[localPlayer.vehicleIndex]->GetRight();
              localPlayer.position =
                  Vector3Add(exitPos, Vector3Scale(right, 8.0f));
              localPlayer.position.y = 2.0f;
              vehicles[localPlayer.vehicleIndex]->Exit();
            }
            localPlayer.inVehicle = false;
            localPlayer.vehicleIndex = -1;
          } else {
            for (int i = 0; i < (int)vehicles.size(); i++) {
              if (Vector3Distance(localPlayer.position, vehicles[i]->position) <
                  12.0f) {
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
          Tank *tank = dynamic_cast<Tank *>(v.get());
          if (tank) {
            // Sync localPlayer position to tank for networking/logic
            localPlayer.position = tank->position;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                tank->CanFireCannon()) {
              tank->FireCannon();
              Ray cannonRay = {tank->GetBarrelTip(), tank->GetTurretForward()};

              // Perform comprehensive raycast
              float closestDist = 2000.0f;
              Vector3 impactPoint =
                  Vector3Add(tank->GetBarrelTip(),
                             Vector3Scale(tank->GetTurretForward(), 2000.0f));

              // 1. Check world obstacles
              for (const auto &box : currentMap->GetObstacles()) {
                auto coll = GetRayCollisionBox(cannonRay, box);
                if (coll.hit && coll.distance < closestDist) {
                  closestDist = coll.distance;
                  impactPoint = coll.point;
                }
              }

              // 2. Check enemies
              for (auto &e : enemies) {
                if (!e->active || e->hp <= 0)
                  continue;
                float hitDist;
                if (e->RayHit(cannonRay, hitDist) && hitDist < closestDist) {
                  closestDist = hitDist;
                  impactPoint =
                      Vector3Add(cannonRay.position,
                                 Vector3Scale(cannonRay.direction, hitDist));
                }
              }

              // 3. Ground collision (if nothing hit)
              if (closestDist > 1999.0f) {
                if (cannonRay.direction.y < 0) {
                  float t = -cannonRay.position.y / cannonRay.direction.y;
                  impactPoint = Vector3Add(
                      cannonRay.position, Vector3Scale(cannonRay.direction, t));
                }
              }

              // SPAWN EFFECTS
              effects.SpawnExplosion(impactPoint);
              effects.SpawnBulletTracer(tank->GetBarrelTip(), impactPoint,
                                        {255, 150, 50, 255});
              effects.SpawnHitSparks(impactPoint, 40);

              // APPLY AOE DAMAGE (Splash)
              float splashRadius = 25.0f;
              float baseDamage = tank->cannonDamage;

              for (auto &e : enemies) {
                if (!e->active || e->hp <= 0)
                  continue;
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
        bool isFiring = localPlayer.currentWeapon &&
                        !localPlayer.showInventory &&
                        (localPlayer.currentWeapon->IsAutomatic()
                             ? IsMouseButtonDown(MOUSE_LEFT_BUTTON)
                             : IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
        if (isFiring && localPlayer.currentWeapon->CanFire() &&
            !localPlayer.inVehicle) {
          localPlayer.currentWeapon->Fire();

          if (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0 ||
              strncmp(localPlayer.currentWeapon->name, "WALL", 4) == 0) {
            // Ray-based placement: cast ray from camera to find ground position
            Vector3 placePos = localPlayer.position;
            float maxPlaceDist = 25.0f;
            if (ray.direction.y < -0.01f) {
              float t = -(ray.position.y) / ray.direction.y;
              if (t > 0 && t < maxPlaceDist * 2.0f) {
                placePos =
                    Vector3Add(ray.position, Vector3Scale(ray.direction, t));
              }
            }
            // Clamp distance
            float placeDist = Vector3Distance(localPlayer.position, placePos);
            if (placeDist > maxPlaceDist || ray.direction.y >= -0.01f) {
              Vector3 forward = ray.direction;
              forward.y = 0;
              if (Vector3Length(forward) > 0.001f)
                forward = Vector3Normalize(forward);
              else
                forward = {0, 0, 1};
              placePos = {localPlayer.position.x + forward.x * 10.0f, 0.0f,
                          localPlayer.position.z + forward.z * 10.0f};
            }
            placePos.y = 0.0f; // Ground level

            bool isTurret =
                (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0);
            auto s = std::make_shared<Structure>();
            s->position = placePos;
            s->type = isTurret ? StructureType::TURRET : StructureType::WALL;
            s->maxHp = isTurret ? 1200 : 4000;
            s->hp = s->maxHp;
            s->lastHp = s->hp;
            s->color =
                isTurret ? (Color){140, 30, 30, 255} : (Color){80, 65, 45, 255};
            s->ownerId = localPlayer.playerId;
            structures.push_back(s);

            // Single-use item: Remove schematic from inventory once ammo is
            // depleted
            if (localPlayer.currentWeapon->currentAmmo <= 0 &&
                localPlayer.currentWeapon->reserveAmmo <= 0) {
              auto it = std::find(localPlayer.inventory.begin(),
                                  localPlayer.inventory.end(),
                                  localPlayer.currentWeapon);
              if (it != localPlayer.inventory.end()) {
                localPlayer.inventory.erase(it);
              }
              delete localPlayer.currentWeapon;
              localPlayer.currentWeapon = localPlayer.inventory.empty()
                                              ? nullptr
                                              : localPlayer.inventory[0];
            }
          }

          Vector3 up = {0, 1, 0};
          Vector3 camRight =
              Vector3Normalize(Vector3CrossProduct(ray.direction, up));
          Vector3 camUp =
              Vector3Normalize(Vector3CrossProduct(camRight, ray.direction));

          Vector3 muzzlePos = Vector3Add(
              ray.position, Vector3Scale(ray.direction, 1.2f)); // forward
          muzzlePos = Vector3Add(muzzlePos,
                                 Vector3Scale(camRight, 0.4f)); // right offset
          muzzlePos =
              Vector3Add(muzzlePos, Vector3Scale(camUp, -0.2f)); // down offset

          if (localPlayer.currentWeapon->magSize > 0) {
            effects.SpawnMuzzleFlash(muzzlePos);
          }

          if (net.mode == NetworkManager::Mode::SERVER) {
            float maxDist = 200.0f;
            if (localPlayer.currentWeapon->magSize == 0)
              maxDist = 3.5f; // Knife
            else if (localPlayer.currentWeapon->magSize == 8)
              maxDist = 40.0f; // Shotgun

            float closestDist = maxDist;
            Enemy *hitEnemy = nullptr;
            for (auto &e : enemies) {
              if (!e->active || e->hp <= 0)
                continue;
              float hitDist;
              if (e->RayHit(ray, hitDist) && hitDist < closestDist) {
                closestDist = hitDist;
                hitEnemy = e.get();
              }
            }
            if (hitEnemy) {
              float oldHp = hitEnemy->hp;
              float dealtDamage = localPlayer.currentWeapon->damage;

              Vector3 hitPoint = Vector3Add(
                  ray.position, Vector3Scale(ray.direction, closestDist));
              if (localPlayer.currentWeapon->magSize > 0) {
                effects.SpawnBloodSplash(hitPoint, 8);
              }

              // Shotgun damage falloff (starts dropping after 10 meters)
              if (localPlayer.currentWeapon->magSize == 8 &&
                  closestDist > 10.0f) {
                float falloff =
                    10.0f /
                    closestDist; // Example: at 20m -> 50%, at 40m -> 25% damage
                dealtDamage *= falloff;
              }

              hitEnemy->TakeDamage((int)dealtDamage);
              if (hitEnemy->hp <= 0) {
                hitEnemy->hp = 0;
                if (oldHp > 0) {
                  int reward = 50;
                  if (hitEnemy->type == EnemyType::SHOOTER)
                    reward = 75;
                  else if (hitEnemy->type == EnemyType::FAST)
                    reward = 100;
                  else if (hitEnemy->type == EnemyType::TANK)
                    reward = 250;
                  else if (hitEnemy->type == EnemyType::MINION)
                    reward = 60;
                  else if (hitEnemy->type == EnemyType::ELITE)
                    reward = 400;
                  else if (hitEnemy->type == EnemyType::KAMIKAZE)
                    reward = 150;
                  else if (hitEnemy->type == EnemyType::GIGA_TANK)
                    reward = 1000;
                  else if (hitEnemy->type == EnemyType::BOSS)
                    reward = 2000;
                  localPlayer.AddMoney(reward);
                }
              }
            } else {
              // Missed enemy, bullet goes to max distance
              if (localPlayer.currentWeapon->magSize > 0) {
                Vector3 hitPoint = Vector3Add(
                    ray.position, Vector3Scale(ray.direction, closestDist));
                effects.SpawnHitSparks(hitPoint, 5);
              }
            }

            // Spawn Bullet Tracers
            if (localPlayer.currentWeapon->magSize > 0) {
              Vector3 tracerEnd = Vector3Add(
                  ray.position, Vector3Scale(ray.direction, closestDist));
              if (localPlayer.currentWeapon->magSize == 8) { // Shotgun
                effects.SpawnShotgunPelletTrails(muzzlePos, tracerEnd);
                for (int i = 0; i < 3; i++) {
                  Vector3 randOffset = {
                      (float)GetRandomValue(-200, 200) / 100.0f,
                      (float)GetRandomValue(-200, 200) / 100.0f,
                      (float)GetRandomValue(-200, 200) / 100.0f};
                  effects.SpawnShotgunPelletTrails(
                      muzzlePos, Vector3Add(tracerEnd, randOffset));
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
        BoundingBox shopBox = {{bp.x - 24.6f, 0, bp.z - 9.0f},
                               {bp.x - 24.4f, 30, bp.z + 9.0f}};
        RayCollision shopHit = GetRayCollisionBox(ray, shopBox);
        shopNearby = shopHit.hit && shopHit.distance < 15.0f;

        if (shopNearby && IsKeyPressed(KEY_E)) {
          int shopLevel = (int)(shopHit.point.y / 8.0f);
          if (shopLevel > 2)
            shopLevel = 2; // Clamp
          int requiredWave = (shopLevel == 1) ? 5 : (shopLevel == 2) ? 10 : 1;

          if (currentWave >= requiredWave) {
            float offsetY = shopHit.point.y - (shopLevel * 8.0f);
            int cost = 0;
            Weapon *newWep = nullptr;
            bool buyAmmo = false;
            bool healing = false;

            if (shopLevel == 0) {           // TIER 1
              if (shopHit.point.z < bp.z) { // Left Column
                if (offsetY > 4.8f) {       /* GLOCK: FREE (already have it) */
                } else if (offsetY > 3.6f) {
                  cost = 1000;
                  newWep = new Revolver();
                } else if (offsetY > 2.4f) {
                  cost = 1500;
                  newWep = new AK47();
                } else if (offsetY > 1.2f) {
                  cost = 2000;
                  newWep = new Shotgun();
                }
              } else { // Right Column
                if (offsetY > 4.8f) {
                  cost = 500;
                  buyAmmo = true;
                }
              }
            } else if (shopLevel == 1) { // TIER 2
              if (shopHit.point.z < bp.z) {
                if (offsetY > 4.8f) {
                  cost = 5000;
                  newWep = new Minigun();
                } else if (offsetY > 3.6f) {
                  cost = 12000;
                  newWep = new RPG();
                } else if (offsetY > 2.4f) {
                  cost = 8000;
                  newWep = new AWP();
                } else if (offsetY > 1.2f) {
                  cost = 800;
                  newWep = new BuilderTool(0);
                } // WALL
              } else {
                if (offsetY > 4.8f) {
                  cost = 500;
                  buyAmmo = true;
                } else if (offsetY > 2.4f) {
                  cost = 3000;
                  newWep = new BuilderTool(1);
                } // TURRET
              }
            } else if (shopLevel == 2) { // TIER 3
              if (shopHit.point.z < bp.z) {
                // Work in progress
              } else {
                if (offsetY > 4.8f) {
                  cost = 500;
                  buyAmmo = true;
                }
              }
            }

            if (localPlayer.money >= cost) {
              if (newWep) {
                localPlayer.money -= cost;
                localPlayer.inventory.push_back(newWep);
                if (!localPlayer.currentWeapon)
                  localPlayer.currentWeapon = newWep;
              } else if (buyAmmo) {
                localPlayer.money -= cost;
                if (localPlayer.currentWeapon)
                  localPlayer.currentWeapon->RefillAmmo();
              } else if (healing) {
                localPlayer.money -= cost;
                localPlayer.hp = localPlayer.maxHp;
              }
            } else if (newWep) {
              delete newWep;
            }
          } // End of currentWave check
        }

        // --- TANK TERMINAL (IN FRONT OF BASE) ---
        Vector3 terminalPos = {15, 0, 120};
        bool terminalNearby =
            Vector3Distance(localPlayer.position, terminalPos) < 6.0f;
        if (terminalNearby && IsKeyPressed(KEY_E)) {
          int tankCost = 100000;
          if (localPlayer.money >= tankCost) {
            localPlayer.money -= tankCost;
            vehicles.push_back(std::make_shared<Tank>(
                (Vector3){terminalPos.x, 0.5f, terminalPos.z - 20.0f}));
            localPlayer.position =
                (Vector3){terminalPos.x, 2.0f, terminalPos.z - 10.0f};
          }
        }
        if (terminalNearby)
          shopNearby = true; // Show "Press E to Buy" prompt
      }

      }
}

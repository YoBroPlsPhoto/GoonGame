#include "core/game.hpp"

void Game::UpdateNetworkAndEnemies() {
// --- NETWORK SYNC ---
      if (net.mode == NetworkManager::Mode::SERVER) {
        if (state == GameState::GAME || state == GameState::PAUSED) {
          int enemyIdCounter = 0;
          int totalEnemiesThisWave = 5 + currentWave * 4;
          if (currentWave == 10 || currentWave == 20 || currentWave == 30)
            totalEnemiesThisWave = 1;
          if (currentWave == 40)
            totalEnemiesThisWave = 15; // 1 boss + 14 guards
          if (((int)(GetTime() * 60)) % 60 == 0)
            printf("Wave: %d, Spawned: %d/%d, Size: %d, inCutscene: %d\n",
                   currentWave, enemiesSpawnedSinceStartOfWave,
                   totalEnemiesThisWave, (int)enemies.size(), inCutscene);
          if (waveActive && !inCutscene) {
            spawnTimer += GetFrameTime();
            float spawnDelay = std::max(0.2f, 1.5f - currentWave * 0.05f);
            if (spawnTimer > spawnDelay &&
                enemiesSpawnedSinceStartOfWave < totalEnemiesThisWave) {
              Vector3 spawnPos = {(float)GetRandomValue(-15, 15), 1.0f,
                                  -350.0f};
              EnemyType t = EnemyType::MELEE;
              if (currentWave == 40) {
                if (enemiesSpawnedSinceStartOfWave == 0) {
                  Vector3 primeSpawn = {0.0f, 0.1f, -400.0f};
                  enemies.push_back(std::make_shared<AdasPrime>(
                      primeSpawn, ++enemyIdCounter));
                  if (!goonMusicPlaying) {
                    PlayMusicStream(goonMusic);
                    goonMusicPlaying = true;
                  }
                } else {
                  // Spawn guards for the prime
                  EnemyType gt = (GetRandomValue(0, 1) == 0 ? EnemyType::TANK
                                                            : EnemyType::ELITE);
                  auto guard = std::make_shared<Enemy>(
                      spawnPos, gt, WeaponType::SHOTGUN, ++enemyIdCounter);
                  guard->hp *= 2.0f;
                  guard->maxHp = guard->hp;
                  enemies.push_back(guard);
                }
              } else if (currentWave == 30) {
                if (enemiesSpawnedSinceStartOfWave == 0) {
                  Vector3 gangSpawn = {0.0f, 0.1f, -400.0f};
                  enemies.push_back(
                      std::make_shared<GangBoss>(gangSpawn, ++enemyIdCounter));
                }
              } else if (currentWave == 20) {
                if (enemiesSpawnedSinceStartOfWave == 0) {
                  Vector3 gibonLanding = {0.0f, 0.1f, -450.0f};
                  enemies.push_back(std::make_shared<GibonRzygacz>(
                      gibonLanding, ++enemyIdCounter));
                  if (!gibonMusicPlaying) {
                    PlayMusicStream(gibonMusic);
                    gibonMusicPlaying = true;
                  }
                }
              } else if (currentWave == 10) {
                if (enemiesSpawnedSinceStartOfWave == 0) {
                  enemies.push_back(
                      std::make_shared<AdasGooner>(spawnPos, ++enemyIdCounter));
                  if (!goonMusicPlaying) {
                    PlayMusicStream(goonMusic);
                    goonMusicPlaying = true;
                  }
                }
              } else {
                int roll = GetRandomValue(0, 100);
                if (currentWave < 3) {
                  if (roll < 45)
                    t = EnemyType::MELEE;
                  else if (roll < 75)
                    t = EnemyType::SHOOTER;
                  else
                    t = EnemyType::MINION;
                } else if (currentWave < 6) {
                  if (roll < 30)
                    t = EnemyType::MELEE;
                  else if (roll < 50)
                    t = EnemyType::SHOOTER;
                  else if (roll < 70)
                    t = EnemyType::FAST;
                  else if (roll < 85)
                    t = EnemyType::TANK;
                  else
                    t = EnemyType::MINION;
                } else if (currentWave < 12) {
                  if (roll < 25)
                    t = EnemyType::MELEE;
                  else if (roll < 45)
                    t = EnemyType::SHOOTER;
                  else if (roll < 60)
                    t = EnemyType::FAST;
                  else if (roll < 75)
                    t = EnemyType::TANK;
                  else if (roll < 85)
                    t = EnemyType::MINION;
                  else if (roll < 93)
                    t = EnemyType::ELITE;
                  else
                    t = EnemyType::KAMIKAZE;
                } else {
                  if (roll < 20)
                    t = EnemyType::MELEE;
                  else if (roll < 35)
                    t = EnemyType::SHOOTER;
                  else if (roll < 50)
                    t = EnemyType::FAST;
                  else if (roll < 65)
                    t = EnemyType::TANK;
                  else if (roll < 75)
                    t = EnemyType::MINION;
                  else if (roll < 85)
                    t = EnemyType::ELITE;
                  else if (roll < 95)
                    t = EnemyType::KAMIKAZE;
                  else
                    t = EnemyType::GIGA_TANK;
                }
                WeaponType w = WeaponType::PISTOL;
                if (t == EnemyType::MELEE)
                  w = (GetRandomValue(0, 1) == 0 ? WeaponType::KATANA
                                                 : WeaponType::MACHETE);
                else if (t == EnemyType::SHOOTER)
                  w = WeaponType::PISTOL;
                else if (t == EnemyType::FAST)
                  w = WeaponType::MACHETE;
                else if (t == EnemyType::TANK)
                  w = WeaponType::KATANA;
                else if (t == EnemyType::MINION)
                  w = WeaponType::MACHETE;
                else if (t == EnemyType::ELITE)
                  w = WeaponType::SHOTGUN;
                else if (t == EnemyType::KAMIKAZE)
                  w = WeaponType::EXPLOSIVE;
                else if (t == EnemyType::GIGA_TANK)
                  w = WeaponType::MACHETE;
                auto newEnemy =
                    std::make_shared<Enemy>(spawnPos, t, w, ++enemyIdCounter);
                newEnemy->hp *= (1.0f + currentWave * 0.15f);
                newEnemy->maxHp = newEnemy->hp;
                enemies.push_back(newEnemy);
              }
              enemiesSpawnedSinceStartOfWave++;
              spawnTimer = 0;
            }
            if (enemiesSpawnedSinceStartOfWave >= totalEnemiesThisWave &&
                enemies.empty()) {
              if (currentWave == 40) {
                // GAME END - VICTORY
                state = GameState::MENU;
                net.gameStarted = false;
                currentWave = 1;
                if (goonMusicPlaying) {
                  StopMusicStream(goonMusic);
                  goonMusicPlaying = false;
                }
              } else {
                waveActive = false;
                waveWaitTimer = 5.0f;
                if (gibonMusicPlaying) {
                  StopMusicStream(gibonMusic);
                  gibonMusicPlaying = false;
                }
                if (goonMusicPlaying) {
                  StopMusicStream(goonMusic);
                  goonMusicPlaying = false;
                }
              }
            }

            if (goonMusicPlaying)
              UpdateMusicStream(goonMusic);
            if (gibonMusicPlaying)
              UpdateMusicStream(gibonMusic);
          }

          if (!waveActive) {
            waveWaitTimer -= GetFrameTime();
            if (waveWaitTimer <= 0 && currentWave < 999) {
              currentWave++;
              enemiesSpawnedSinceStartOfWave = 0;
              waveActive = true;
            }
          }

          std::vector<TargetInfo> targets;
          // Redirect damage to tank if player is inside
          int *localTargetHP = &localPlayer.hp;
          if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0) {
            localTargetHP = &vehicles[localPlayer.vehicleIndex]->health;
          }
          targets.push_back({localPlayer.position, localTargetHP, true, false});

          for (auto &[id, data] : net.remotePlayers) {
            if (data.active)
              targets.push_back({data.position, &data.hp, true, false});
          }

          for (auto &s : structures) {
            if (s->active && s->hp > 0)
              targets.push_back({s->position, &s->hp, true, true});
          }

          // --- UPDATE STRUCTURES ---
          {
            float dt = GetFrameTime();
            for (auto &s : structures) {
              if (!s->active || s->hp <= 0)
                continue;

              // Damage flash detection
              if (s->hp < s->lastHp) {
                s->damageFlash = 0.3f;
              }
              s->lastHp = s->hp;
              if (s->damageFlash > 0)
                s->damageFlash -= dt;

              // Barrel recoil recovery
              if (s->barrelRecoil > 0)
                s->barrelRecoil -= dt * 4.0f;
              if (s->barrelRecoil < 0)
                s->barrelRecoil = 0;

              if (s->type == StructureType::TURRET) {
                // Find closest enemy within range
                float turretRange = 60.0f;
                float closestDist = turretRange;
                Enemy *target = nullptr;
                for (auto &e : enemies) {
                  if (!e->active || e->hp <= 0)
                    continue;
                  float d = Vector3Distance(s->position, e->position);
                  if (d < closestDist) {
                    closestDist = d;
                    target = e.get();
                  }
                }

                // Smooth turret rotation toward target
                if (target) {
                  s->targetAngle = atan2f(target->position.x - s->position.x,
                                          target->position.z - s->position.z) *
                                   RAD2DEG;
                }
                // Lerp rotation smoothly
                float angleDiff = s->targetAngle - s->turretAngle;
                while (angleDiff > 180.0f)
                  angleDiff -= 360.0f;
                while (angleDiff < -180.0f)
                  angleDiff += 360.0f;
                s->turretAngle +=
                    angleDiff * dt * 8.0f; // Smooth rotation speed

                // Fire at target
                s->attackTimer -= dt;
                if (s->attackTimer <= 0 && target) {
                  s->attackTimer = 0.3f; // ~3.3 shots per sec
                  int dmg = 50;
                  float oldHp = target->hp;
                  target->TakeDamage(dmg);

                  // Kill reward to owner
                  if (target->hp <= 0 && oldHp > 0) {
                    if (s->ownerId == localPlayer.playerId) {
                      int reward = 50;
                      if (target->type == EnemyType::TANK)
                        reward = 200;
                      else if (target->type == EnemyType::ELITE)
                        reward = 300;
                      else if (target->type == EnemyType::GIGA_TANK)
                        reward = 500;
                      else if (target->type == EnemyType::FAST)
                        reward = 80;
                      else if (target->type == EnemyType::SHOOTER)
                        reward = 60;
                      localPlayer.AddMoney(reward);
                    } else {
                      for (auto &[rid, rdata] : net.remotePlayers) {
                        if (rid == s->ownerId) {
                          rdata.money += 50;
                          break;
                        }
                      }
                    }
                  }

                  // Barrel recoil
                  s->barrelRecoil = 1.0f;

                  // Compute barrel tip position for effects
                  float turretRad = s->turretAngle * DEG2RAD;
                  Vector3 barrelTip = {s->position.x + sinf(turretRad) * 3.5f,
                                       s->position.y + 3.8f,
                                       s->position.z + cosf(turretRad) * 3.5f};
                  Vector3 targetHit = {target->position.x,
                                       target->position.y + 1.0f,
                                       target->position.z};

                  effects.SpawnBulletTracer(barrelTip, targetHit,
                                            {255, 200, 50, 255});
                  effects.SpawnMuzzleFlash(barrelTip);
                  effects.SpawnHitSparks(targetHit, 8);
                  if (target->hp <= 0) {
                    effects.SpawnBloodSplash(targetHit, 10);
                  }
                }
              }
            }
            // Remove destroyed structures
            structures.erase(
                std::remove_if(structures.begin(), structures.end(),
                               [](const std::shared_ptr<Structure> &s) {
                                 return s->hp <= 0;
                               }),
                structures.end());
          }

          std::vector<EnemySync> syncList;
          for (auto &e : enemies) {
            if (state == GameState::GAME || state == GameState::PAUSED) {
              if (!inCutscene || e == activeBoss) {
                e->Update(targets, &baseHP, basePosition);
                if (!inCutscene) {
                  for (const auto &box : currentMap->GetObstacles())
                    e->HandleCollision(box);
                  for (const auto &s : structures) {
                    if (s->active)
                      e->HandleCollision(s->GetBoundingBox());
                  }
                }
              }
            }
            syncList.push_back({e->id, e->position, e->hp, e->maxHp,
                                (int)e->type, (int)e->weapon, e->angle,
                                e->isMoving, e->walkTimer, e->attackTimer});
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
          for (auto &[id, data] : net.remotePlayers) {
            if (remoteFireCooldowns[id] > 0)
              remoteFireCooldowns[id] -= GetFrameTime();
            if (data.active && data.firing && remoteFireCooldowns[id] <= 0) {
              Vector3 camOrigin = {data.position.x, data.position.y + 1.0f,
                                   data.position.z};
              Vector3 camDir =
                  Vector3Normalize(Vector3Subtract(data.camTarget, camOrigin));
              Ray remoteRay = {camOrigin, camDir};
              float closest = 250.0f;
              Enemy *hit = nullptr;
              for (auto &e : enemies) {
                RayCollision coll = GetRayCollisionSphere(
                    remoteRay,
                    (Vector3){e->position.x, e->position.y + 1.5f,
                              e->position.z},
                    1.2f);
                if (coll.hit && coll.distance < closest) {
                  closest = coll.distance;
                  hit = e.get();
                }
              }
              if (hit) {
                float oldHp = hit->hp;
                hit->TakeDamage(35);
                if (hit->hp <= 0 && oldHp > 0) {
                  data.money += 150;
                } // Reward money to remote player
              }
              remoteFireCooldowns[id] = 0.12f;
              data.firing = false;
            }
          }

          enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                       [](const std::shared_ptr<Enemy> &e) {
                                         return !e->active;
                                       }),
                        enemies.end());

          UpdateAdasHazards();

          for (auto it = vehicles.begin(); it != vehicles.end();) {
            auto &v = *it;
            v->Update(GetFrameTime());

            // --- VEHICLE DESTRUCTION ---
            if (v->health <= 0) {
              effects.SpawnExplosion(v->position);
              if (v->isOccupied) {
                if (localPlayer.inVehicle && localPlayer.vehicleIndex >= 0 &&
                    vehicles[localPlayer.vehicleIndex] == v) {
                  localPlayer.hp = 0; // DIE!
                  localPlayer.inVehicle = false;
                  localPlayer.vehicleIndex = -1;
                }
              }
              it = vehicles.erase(it);
              continue;
            }

            // --- TANK CRUSHING LOGIC ---
            Tank *tank = dynamic_cast<Tank *>(v.get());
            if (tank && fabsf(tank->speed) > 0.1f) {
              BoundingBox tankBox = tank->GetBoundingBox();
              for (auto &e : enemies) {
                if (!e->active || e->hp <= 0)
                  continue;
                if (CheckCollisionBoxes(tankBox, e->GetBoundingBox())) {
                  bool isBoss = (e->type == EnemyType::BOSS || e->type == EnemyType::GIBON_BOSS || 
                                 e->type == EnemyType::GANG_BOSS || e->type == EnemyType::ADAS_PRIME);
                  
                  float currentSpeedVal = fabsf(tank->speed);
                  float threshold = isBoss ? 0.76f : 0.40f; // 76 is 95% of 80, 40 is half
                  
                  if (currentSpeedVal >= threshold) {
                    float crushDamage = 1500.0f;
                    float oldHp = e->hp;
                    e->TakeDamage((int)crushDamage);
                    
                    float speedSign = tank->speed > 0 ? 1.0f : -1.0f;
                    if (isBoss) {
                        tank->speed = 0.30f * speedSign;
                    } else {
                        currentSpeedVal -= 0.05f;
                        if (currentSpeedVal < 0.30f) currentSpeedVal = 0.30f;
                        tank->speed = currentSpeedVal * speedSign;
                    }

                    if (e->hp <= 0 && oldHp > 0) {
                      localPlayer.AddMoney(100);
                    }
                  }
                }
              }
            }
            it++;
          }

          std::vector<PlayerSyncData> pSync;
          pSync.push_back(
              {localPlayer.playerId, localPlayer.hp, localPlayer.money, localPlayer.isAdmin});
          for (auto &[id, data] : net.remotePlayers) {
            if (data.active)
              pSync.push_back({id, data.hp, data.money, data.admin});
          }

          net.gameStarted =
              (state == GameState::GAME || state == GameState::PAUSED);
          net.SendWorldUpdate(baseHP, currentWave, syncList, pSync);
        }
      } else if (net.mode == NetworkManager::Mode::CLIENT) {
        baseHP = net.remoteBaseHP;
        currentWave = net.remoteWave;
        if (net.gameStarted && state == GameState::LOBBY)
          state = GameState::GAME;

        if (net.remotePlayers.count(localPlayer.playerId)) {
          localPlayer.hp = net.remotePlayers[localPlayer.playerId].hp;
          localPlayer.money = net.remotePlayers[localPlayer.playerId].money;
        }
      }

      bool isFiringNow = localPlayer.currentWeapon &&
                         (localPlayer.currentWeapon->IsAutomatic()
                              ? IsMouseButtonDown(MOUSE_LEFT_BUTTON)
                              : IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
      if (isFiringNow)
        firingStickyTimer = 0.1f;
      else if (firingStickyTimer > 0)
        firingStickyTimer -= GetFrameTime();

      net.SendPlayerUpdate(localPlayer.playerId, menu.playerNick.c_str(),
                           localPlayer.position, localPlayer.camera.target,
                           localPlayer.hp, localPlayer.money,
                           (firingStickyTimer > 0));
    
}

void Game::UpdateAdasHazards() {
  float dt = GetFrameTime();
  std::shared_ptr<AdasGooner> adas = nullptr;

  for (auto &e : enemies) {
    if (e->type == EnemyType::BOSS && e->active && e->hp > 0) {
      adas = std::dynamic_pointer_cast<AdasGooner>(e);
      break;
    }
  }

  if (!adas) {
    adasDrops.clear();
    adasStains.clear();
    adasDropTimer = 0.0f;
    return;
  }

  if (adas->hp < adas->maxHp / 2 &&
      adas->cutsceneState == CutsceneState::FINISHED) {
    adasDropTimer -= dt;
    if (adasDropTimer <= 0.0f) {
      adasDropTimer = 1.4f;
      Vector3 target = {
          localPlayer.position.x + (float)GetRandomValue(-9000, 9000) / 100.0f,
          95.0f,
          localPlayer.position.z + (float)GetRandomValue(-9000, 9000) / 100.0f};
      target.x = std::max(-230.0f, std::min(230.0f, target.x));
      target.z = std::max(-520.0f, std::min(260.0f, target.z));
      adasDrops.push_back({target, (float)GetRandomValue(8, 14), 42.0f});
    }
  }

  for (auto &drop : adasDrops) {
    drop.position.y -= drop.fallSpeed * dt;
  }

  for (auto it = adasDrops.begin(); it != adasDrops.end();) {
    if (it->position.y <= 0.4f) {
      adasStains.push_back({(Vector3){it->position.x, 0.08f, it->position.z},
                            it->radius * 1.7f});
      effects.SpawnHitSparks((Vector3){it->position.x, 0.8f, it->position.z},
                             20);
      it = adasDrops.erase(it);
    } else {
      ++it;
    }
  }

  const size_t maxStains = 24;
  if (adasStains.size() > maxStains) {
    adasStains.erase(adasStains.begin(),
                     adasStains.begin() + (adasStains.size() - maxStains));
  }
}

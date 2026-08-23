#include "adas_gooner.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>
#include <cmath>

namespace {
constexpr float BASE_HALF_SIZE = 25.0f;
constexpr float BASE_EDGE_AGGRO_DISTANCE = 8.0f;
constexpr float SMOKE_DURATION = 6.0f;
constexpr float SMOKE_RADIUS = 54.0f;
constexpr float SMOKE_DAMAGE_INTERVAL = 0.75f;
constexpr int SMOKE_DAMAGE = 7;
constexpr float LASER_MAX_RANGE = 80.0f;
constexpr float LASER_NEAR_DAMAGE = 1.5f;
constexpr float LASER_MID_DAMAGE = 0.8f;
constexpr float LASER_FAR_DAMAGE = 0.35f;
constexpr float SNUS_CHARGE_TIME = 0.9f;
constexpr float SNUS_SPEED = 140.0f;
constexpr float SNUS_EXPLOSION_RADIUS = 16.0f;
constexpr int SNUS_DAMAGE = 50;
constexpr float SNUS_EXPLOSION_LINGER = 0.60f;
constexpr float SNUS_COOLDOWN_TIME = 30.0f;

Vector3 GetSnusMouthPosition(const Vector3& bossPos, float angle, bool useWafelModel) {
    float facingRad = angle * DEG2RAD;
    if (useWafelModel) {
        return {
            bossPos.x + sinf(facingRad) * 0.95f,
            bossPos.y + 4.65f,
            bossPos.z + cosf(facingRad) * 0.95f
        };
    }

    return {
        bossPos.x + sinf(facingRad) * 1.15f,
        bossPos.y + 20.2f,
        bossPos.z + cosf(facingRad) * 1.15f
    };
}

bool IsInsideBaseFootprint(Vector3 pos, Vector3 basePos) {
    return fabsf(pos.x - basePos.x) <= BASE_HALF_SIZE &&
           fabsf(pos.z - basePos.z) <= BASE_HALF_SIZE;
}

float DistanceToBaseFootprint(Vector3 pos, Vector3 basePos) {
    float dx = fmaxf(0.0f, fabsf(pos.x - basePos.x) - BASE_HALF_SIZE);
    float dz = fmaxf(0.0f, fabsf(pos.z - basePos.z) - BASE_HALF_SIZE);
    return sqrtf(dx * dx + dz * dz);
}
}

Model AdasGooner::wafelModel;
bool AdasGooner::wafelModelLoaded = false;
bool AdasGooner::globalUseWafelModel = false;

void AdasGooner::LoadSharedResources() {
    if (!wafelModelLoaded) {
        wafelModel = LoadModel("../models/wafel.obj");
        wafelModelLoaded = true;
    }
}

void AdasGooner::UnloadSharedResources() {
    if (wafelModelLoaded) {
        UnloadModel(wafelModel);
        wafelModelLoaded = false;
    }
}

AdasGooner::AdasGooner(Vector3 startPos, int enemyId) 
    : Enemy(startPos, EnemyType::BOSS, WeaponType::KATANA, enemyId) {
    cutsceneState = CutsceneState::WARDROBE_CLOSED;
    cutsceneTimer = 0.0f;
    wardrobePos = startPos; // The wardrobe acts as his starting point
    wardrobePos.y = 0.0f;
    position = startPos;
    
    // Stats for Adas Gooner
    hp = 80000;
    maxHp = 80000;
    speed = 0.04f; // Extremely slow but threatening
    radius = 8.0f;
    color = {255, 100, 100, 255}; // Deep red
    snusCooldown = 28.0f;
}

void AdasGooner::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0 && cutsceneState != CutsceneState::RETREATING && cutsceneState != CutsceneState::WARDROBE_RETREAT_OPENING && cutsceneState != CutsceneState::WARDROBE_CLOSING && cutsceneState != CutsceneState::RETREATED) { 
        cutsceneState = CutsceneState::RETREATING; 
        cutsceneTimer = 0.0f;
    }

    cutsceneTimer += GetFrameTime();

    if (cutsceneState == CutsceneState::WARDROBE_CLOSED) {
        if (cutsceneTimer > 3.0f) {
            cutsceneState = CutsceneState::WARDROBE_OPENING;
            cutsceneTimer = 0.0f;
        }
        isMoving = false; walkTimer = 0; angle = 0;
    } else if (cutsceneState == CutsceneState::WARDROBE_OPENING) {
        if (cutsceneTimer > 5.0f) {
            cutsceneState = CutsceneState::WALKING_OUT;
            cutsceneTimer = 0.0f;
        }
        isMoving = false; walkTimer = 0;
    } else if (cutsceneState == CutsceneState::WALKING_OUT) {
        float dt = GetFrameTime();
        float timeScale = dt * 60.0f;
        position.z += speed * 2.0f * timeScale; isMoving = true; walkTimer += dt;
        if (cutsceneTimer > 6.0f) cutsceneState = CutsceneState::FINISHED;
    } else if (cutsceneState == CutsceneState::RETREATING) {
        float dt = GetFrameTime();
        float timeScale = dt * 60.0f;
        
        Vector3 dir = Vector3Subtract(wardrobePos, position);
        float dist = Vector3Length(dir);
        if (dist > 1.0f) {
            dir = Vector3Normalize(dir);
            position.x += dir.x * speed * 2.0f * timeScale;
            position.z += dir.z * speed * 2.0f * timeScale;
            angle = atan2f(dir.x, dir.z) * RAD2DEG;
            isMoving = true;
            walkTimer += dt;
        } else {
            // Reached wardrobe
            position = wardrobePos;
            angle = 0;
            cutsceneState = CutsceneState::WARDROBE_RETREAT_OPENING;
            cutsceneTimer = 0.0f;
        }
    } else if (cutsceneState == CutsceneState::WARDROBE_RETREAT_OPENING) {
        // Portal opens
        isMoving = false; walkTimer = 0; angle = 0;
        if (cutsceneTimer > 3.0f) {
            cutsceneState = CutsceneState::WARDROBE_CLOSING;
            cutsceneTimer = 0.0f;
        }
    } else if (cutsceneState == CutsceneState::WARDROBE_CLOSING) {
        // Doors close
        isMoving = false; walkTimer = 0;
        if (cutsceneTimer > 2.0f) {
            cutsceneState = CutsceneState::RETREATED;
            active = false;
        }
    } else {
        // Normal Behavior
        
        float dt = GetFrameTime();

        // --- SMOKE CLOUD SPECIAL ATTACK ---
        // The cloud is a persistent, close-range hazard.  It has its own
        // cooldown so it does not replace the laser attack.
        if (isSprayingSmoke) {
            smokeActiveTimer += dt;
            smokeDamageTimer += dt;
            isMoving = false;

            if (smokeDamageTimer >= SMOKE_DAMAGE_INTERVAL) {
                smokeDamageTimer = 0.0f;
                float cloudRadius = SMOKE_RADIUS * fminf(smokeActiveTimer / 1.25f, 1.0f);
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0) || p.isStructure) continue;
                    if (Vector3Distance(position, p.pos) <= cloudRadius && p.hp) {
                        *p.hp -= SMOKE_DAMAGE;
                    }
                }
            }

            if (smokeActiveTimer >= SMOKE_DURATION) {
                isSprayingSmoke = false;
                smokeActiveTimer = 0.0f;
                smokeDamageTimer = 0.0f;
                smokeCooldown = 15.0f;
            }
        } else if (isShootingSnus) {
            snusShootTimer += dt;
            isMoving = false;

            bool targetStillValid = false;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0) || p.isStructure) continue;
                if (Vector3Distance(p.pos, laserTargetPos) <= 3.5f) {
                    targetStillValid = !IsInsideBaseFootprint(p.pos, basePos);
                    break;
                }
            }

            Vector3 mouthPos = GetSnusMouthPosition(position, angle, globalUseWafelModel && wafelModelLoaded);
            if (snusShootTimer >= SNUS_CHARGE_TIME && snusShotsFired == 0) {
                SnusProjectile shot;
                shot.position = mouthPos;
                Vector3 dir = Vector3Subtract(laserTargetPos, mouthPos);
                if (Vector3Length(dir) < 0.1f) {
                    dir = (Vector3){0.0f, 0.0f, 1.0f};
                } else {
                    dir = Vector3Normalize(dir);
                }
                shot.velocity = Vector3Scale(dir, SNUS_SPEED);
                shot.radius = 1.4f;
                shot.lifetime = 0.0f;
                shot.active = true;
                snusProjectiles.push_back(shot);
                snusShotsFired = 1;
            }

            for (auto& shot : snusProjectiles) {
                if (!shot.active) continue;
                shot.lifetime += dt;
                shot.position = Vector3Add(shot.position, Vector3Scale(shot.velocity, dt));

                float targetDistance = Vector3Distance(shot.position, laserTargetPos);
                if (targetDistance <= shot.radius + 1.8f) {
                    if (targetStillValid && !IsInsideBaseFootprint(laserTargetPos, basePos)) {
                        for (const auto& p : players) {
                            if (!p.active || (p.hp && *p.hp <= 0) || p.isStructure) continue;
                            if (IsInsideBaseFootprint(p.pos, basePos)) continue;
                            if (Vector3Distance(p.pos, laserTargetPos) <= SNUS_EXPLOSION_RADIUS && p.hp) {
                                *p.hp -= SNUS_DAMAGE;
                            }
                        }
                    }
                    shot.active = false;
                    snusShotsFired = 2;
                }

                if (shot.lifetime > 18.0f) {
                    shot.active = false;
                    snusShotsFired = 2;
                }
            }

            if (snusShotsFired == 2 && !snusProjectiles.empty()) {
                if (snusShootTimer >= SNUS_CHARGE_TIME + snusProjectiles.front().lifetime + SNUS_EXPLOSION_LINGER) {
                    snusProjectiles.clear();
                    isShootingSnus = false;
                    snusShootTimer = 0.0f;
                    snusShotsFired = 0;
                    snusCooldown = SNUS_COOLDOWN_TIME;
                }
            }
        } else if (laserFiring) {
        
            // Laser is firing - deal damage and track target
            laserFireTimer += dt;
            isMoving = false;
            
            // Update laser target to track nearest player in real-time
            float closestD = 999.0f;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (p.isStructure) continue;
                float d = Vector3Distance(position, p.pos);
                if (d < closestD) {
                    closestD = d;
                    laserTargetPos = p.pos;
                    laserTargetPos.y += 1.0f; // Aim at chest height
                }
            }
            
            // Deal damage to players in laser path
            Vector3 laserOrigin = position;
            laserOrigin.y += 0.8f * 9.0f; // Crotch height in world space (scale=9)
            Vector3 laserDir = Vector3Subtract(laserTargetPos, laserOrigin);
            float laserLen = Vector3Length(laserDir);
            if (laserLen > 0.1f) {
                laserDir = Vector3Normalize(laserDir);
                // Face the target
                angle = atan2f(laserDir.x, laserDir.z) * RAD2DEG;
                
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0)) continue;
                    if (p.isStructure) continue;
                    
                    // Check if player is close to the laser line
                    Vector3 toPlayer = Vector3Subtract(p.pos, laserOrigin);
                    float proj = Vector3DotProduct(toPlayer, laserDir);
                    if (proj > 0 && proj < laserLen + 5.0f) {
                        Vector3 closestPoint = Vector3Add(laserOrigin, Vector3Scale(laserDir, proj));
                        float distToBeam = Vector3Distance(p.pos, closestPoint);
                        if (distToBeam < 5.5f && p.hp) { // Widerszy beam z lekkim spadkiem obrażeń
                            float playerDistance = Vector3Distance(laserOrigin, p.pos);
                            float damage = LASER_FAR_DAMAGE;
                            if (playerDistance <= 30.0f) {
                                damage = LASER_NEAR_DAMAGE;
                            } else if (playerDistance <= 55.0f) {
                                float t = (playerDistance - 30.0f) / 25.0f;
                                damage = LASER_NEAR_DAMAGE + (LASER_MID_DAMAGE - LASER_NEAR_DAMAGE) * t;
                            } else {
                                float t = fminf((playerDistance - 55.0f) / (LASER_MAX_RANGE - 55.0f), 1.0f);
                                damage = LASER_MID_DAMAGE + (LASER_FAR_DAMAGE - LASER_MID_DAMAGE) * t;
                            }
                            *p.hp -= (int)ceilf(damage);
                        }
                    }
                }
            }
            
            if (laserFireTimer >= 2.5f) {
                laserFiring = false;
                laserFireTimer = 0.0f;
                laserCooldown = 20.0f; // 20 seconds until next laser
            }
        } else if (laserCharging) {
            // Charging up - boss stops and glows
            laserChargeTimer += dt;
            isMoving = false;
            
            // Find target during charge
            float closestD = 999.0f;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (p.isStructure) continue;
                float d = Vector3Distance(position, p.pos);
                if (d < closestD) {
                    closestD = d;
                    laserTargetPos = p.pos;
                    laserTargetPos.y += 1.0f;
                }
            }
            
            // Face the target during charge
            Vector3 laserOrigin = position;
            laserOrigin.y += 0.8f * 9.0f;
            Vector3 dirToTarget = Vector3Subtract(laserTargetPos, laserOrigin);
            if (Vector3Length(dirToTarget) > 0.1f) {
                angle = atan2f(dirToTarget.x, dirToTarget.z) * RAD2DEG;
            }
            
            if (laserChargeTimer >= 1.5f) {
                laserCharging = false;
                laserChargeTimer = 0.0f;
                laserFiring = true;
            }
        } else {
            // Normal movement + count down special-attack cooldowns
            Enemy::Update(players, baseHp, basePos);
            
            laserCooldown -= dt;
            smokeCooldown -= dt;
            snusCooldown -= dt;

            if (!isShootingSnus && snusCooldown <= 0.0f) {
                bool hasSnusTarget = false;
                float closestD = 999.0f;
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0) || p.isStructure) continue;
                    if (IsInsideBaseFootprint(p.pos, basePos)) continue;
                    float d = Vector3Distance(position, p.pos);
                    if (d < closestD) {
                        closestD = d;
                        laserTargetPos = p.pos;
                        laserTargetPos.y += 1.0f;
                        hasSnusTarget = true;
                    }
                }
                if (hasSnusTarget) {
                    isShootingSnus = true;
                    snusShootTimer = 0.0f;
                    snusShotsFired = 0;
                    snusProjectiles.clear();
                } else {
                    snusCooldown = 2.0f;
                }
            }

            // Start the smoke only if at least one living player is close
            // enough to be threatened.  This prevents wasting the attack
            // while the boss is far away from the fight.
            if (!isShootingSnus && smokeCooldown <= 0.0f) {
                bool hasSmokeTarget = false;
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0) || p.isStructure) continue;
                    if (Vector3Distance(position, p.pos) <= SMOKE_RADIUS) {
                        hasSmokeTarget = true;
                        break;
                    }
                }
                if (hasSmokeTarget) {
                    isSprayingSmoke = true;
                    smokeActiveTimer = 0.0f;
                    smokeDamageTimer = 0.0f;
                }
            }

            if (!isSprayingSmoke && !isShootingSnus && laserCooldown <= 0.0f) {
                // Check if there's a player nearby to target
                bool hasTarget = false;
                float closestD = 60.0f; // 60 unit detection range for laser
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0)) continue;
                    if (p.isStructure) continue;
                    float d = Vector3Distance(position, p.pos);
                    if (d < closestD) {
                        closestD = d;
                        laserTargetPos = p.pos;
                        laserTargetPos.y += 1.0f;
                        hasTarget = true;
                    }
                }
                if (hasTarget) {
                    laserCharging = true;
                    laserChargeTimer = 0.0f;
                } else {
                    laserCooldown = 2.0f; // Check again in 2s
                }
            }
        }
        
        // --- SHOCKWAVE ATTACK ---
        shockwaveTimer += GetFrameTime();
        if (shockwaveTimer > 10.0f) {
            bool atBaseEdge = DistanceToBaseFootprint(position, basePos) <= BASE_EDGE_AGGRO_DISTANCE;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (atBaseEdge && !p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
                float d = Vector3Distance(position, p.pos);
                if (d < 20.0f) {
                    if (p.hp) *p.hp -= 10;
                }
            }
            shockwaveTimer = 0;
        }
        
        if (!isEnraged && hp < maxHp * 0.3f) {
            isEnraged = true; speed *= 2.5f; color = RED;
        }
    }
}

void AdasGooner::Draw() {
    if (!active) return;

    // --- DRAW WARDROBE --- always visible (including during retreat)
    {
        rlPushMatrix();
        rlTranslatef(wardrobePos.x, wardrobePos.y, wardrobePos.z);
        rlRotatef(180, 0, 1, 0);
        
        // Structure
        DrawCube((Vector3){0, 9.0f, 3.0f}, 12.0f, 18.0f, 1.0f, DARKBROWN);
        DrawCube((Vector3){-6.0f, 9.0f, 0}, 1.0f, 18.0f, 6.0f, DARKBROWN);
        DrawCube((Vector3){6.0f, 9.0f, 0}, 1.0f, 18.0f, 6.0f, DARKBROWN);
        DrawCube((Vector3){0, 18.5f, 0}, 13.0f, 1.0f, 6.0f, BLACK);
        DrawCube((Vector3){0, 0.5f, 0}, 13.0f, 1.0f, 6.0f, BLACK);
        
        // Door angle
        float doorAngle = 0;
        if (cutsceneState == CutsceneState::WARDROBE_OPENING) {
            doorAngle = (cutsceneTimer / 5.0f) * 110.0f;
        } else if (cutsceneState == CutsceneState::WALKING_OUT ||
                   cutsceneState == CutsceneState::FINISHED ||
                   cutsceneState == CutsceneState::RETREATING ||
                   cutsceneState == CutsceneState::WARDROBE_RETREAT_OPENING) {
            doorAngle = 110.0f; // stay open
        } else if (cutsceneState == CutsceneState::WARDROBE_CLOSING) {
            doorAngle = (1.0f - (cutsceneTimer / 2.0f)) * 110.0f;
            if (doorAngle < 0) doorAngle = 0;
        }

        // White stain during opening/closing
        if (cutsceneState == CutsceneState::WARDROBE_OPENING ||
            cutsceneState == CutsceneState::WALKING_OUT ||
            cutsceneState == CutsceneState::WARDROBE_RETREAT_OPENING ||
            (cutsceneState == CutsceneState::WARDROBE_CLOSING && doorAngle > 5.0f)) {
            float openProgress = doorAngle / 110.0f;
            float pulse = sinf((float)GetTime() * 8.0f) * 0.08f + 0.92f;
            float mainRadius = (8.0f + openProgress * 18.0f) * pulse;
            DrawCylinder((Vector3){0.0f, 0.08f, 0.0f}, mainRadius, mainRadius, 0.08f, 64, WHITE);
            DrawCylinder((Vector3){-6.0f, 0.10f, -1.0f}, mainRadius * 0.55f, mainRadius * 0.55f, 0.08f, 64, WHITE);
            DrawCylinder((Vector3){6.0f, 0.12f, -1.0f}, mainRadius * 0.55f, mainRadius * 0.55f, 0.08f, 64, WHITE);
        }

        // Left door
        rlPushMatrix();
        rlTranslatef(-5.5f, 9.0f, -3.0f);
        rlRotatef(doorAngle, 0, 1, 0);
        DrawCube((Vector3){2.75f, 0, 0}, 5.5f, 17.0f, 0.5f, BROWN);
        rlPopMatrix();

        // Right door
        rlPushMatrix();
        rlTranslatef(5.5f, 9.0f, -3.0f);
        rlRotatef(-doorAngle, 0, 1, 0);
        DrawCube((Vector3){-2.75f, 0, 0}, 5.5f, 17.0f, 0.5f, BROWN);
        rlPopMatrix();

        rlPopMatrix();
    }

    // Don't draw the character if still in wardrobe or fully retreated
    if (cutsceneState == CutsceneState::WARDROBE_CLOSED) return;
    if (cutsceneState == CutsceneState::WARDROBE_CLOSING) return;
    if (cutsceneState == CutsceneState::RETREATED) return;

    if (globalUseWafelModel && wafelModelLoaded) {
        float animWalk = isMoving ? sinf(walkTimer * speed * 30.0f) : 0.0f;
        rlPushMatrix();
        // Bobbing up and down
        rlTranslatef(position.x, position.y + fabsf(animWalk) * 2.0f, position.z);
        rlRotatef(angle, 0, 1, 0);
        // Wobble left and right
        rlRotatef(animWalk * 15.0f, 0, 0, 1);
        // Tilt forward slightly while walking
        if (isMoving) rlRotatef(15.0f, 1, 0, 0);
        
        // Scale and rotation for wafel model
        rlScalef(0.75f, 0.75f, 0.75f); 
        DrawModel(wafelModel, (Vector3){0, 0, 0}, 1.0f, WHITE);
        rlPopMatrix();
    } else {
        // --- DRAW ADAS GOONER ---
        float scale = 9.0f; // BOSS SIZE
        float animWalk = isMoving ? sinf(walkTimer * speed * 30.0f) : 0.0f;
        float animAttack = (attackTimer > 0) ? (1.0f - attackTimer / 1.5f) : 0.0f; // Slower cooldown logic mapped
        
        rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlRotatef(angle, 0, 1, 0);
        rlScalef(scale, scale, scale);

        Color skin = { 255, 180, 120, 255 }; // slightly paler
        Color clothes = { 50, 50, 50, 255 }; // dark suit

        // Legs
        rlPushMatrix();
        rlTranslatef(-0.2f, 0.4f, 0);
        rlRotatef(animWalk * 35.0f, 1, 0, 0);
        DrawCube((Vector3){0, 0, 0}, 0.25f, 0.8f, 0.25f, BLACK);
        rlPopMatrix();

        rlPushMatrix();
        rlTranslatef(0.2f, 0.4f, 0);
        rlRotatef(-animWalk * 35.0f, 1, 0, 0);
        DrawCube((Vector3){0, 0, 0}, 0.25f, 0.8f, 0.25f, BLACK);
        rlPopMatrix();

        // Torso (Buffed)
        rlPushMatrix();
        DrawCube((Vector3){ 0, 1.25f, 0 }, 0.9f, 0.9f, 0.5f, clothes); 
        DrawCube((Vector3){ 0, 1.75f, 0 }, 1.1f, 0.3f, 0.6f, clothes); // huge shoulders
        rlPopMatrix();

        // Arms
        rlPushMatrix();
        rlTranslatef(-0.55f, 1.5f, 0);
        rlRotatef(-animWalk * 45.0f, 1, 0, 0);
        DrawCube((Vector3){0, -0.35f, 0}, 0.25f, 0.8f, 0.25f, skin);
        rlPopMatrix();

        rlPushMatrix();
        rlTranslatef(0.55f, 1.5f, 0);
        if (animAttack > 0) rlRotatef(animAttack * -130.0f, 1, 0, 0);
        else rlRotatef(animWalk * 45.0f, 1, 0, 0);
        DrawCube((Vector3){0, -0.35f, 0}, 0.25f, 0.8f, 0.25f, skin);

        // Giant Katana
        rlPushMatrix();
        rlTranslatef(0, -0.35f, 0.2f);
        rlRotatef(90, 1, 0, 0);
        DrawCube((Vector3){0, 1.2f, 0}, 0.1f, 2.8f, 0.1f, LIGHTGRAY);
        DrawCube((Vector3){0, -0.1f, 0}, 0.2f, 0.4f, 0.2f, DARKPURPLE); // evil sword hilt
        rlPopMatrix();
        rlPopMatrix();

        // Head
        rlPushMatrix();
        rlTranslatef(0, 2.1f, 0);
        DrawCube((Vector3){0, 0, 0}, 0.5f, 0.5f, 0.5f, skin);
        DrawCube((Vector3){-0.15f, 0.1f, 0.25f}, 0.1f, 0.1f, 0.1f, RED); // Red glowing glowing eyes
        DrawCube((Vector3){0.15f, 0.1f, 0.25f}, 0.1f, 0.1f, 0.1f, RED);
        
        // Huge Crown
        DrawCube((Vector3){0, 0.4f, 0}, 0.6f, 0.8f, 0.6f, GOLD); 
        rlPopMatrix();

        // Członek (Crotch detail)
        rlPushMatrix();
        rlTranslatef(0, 0.8f, 0.4f);
        DrawCube((Vector3){0, 0, 0}, 0.2f, 0.2f, 0.4f, RED);
        DrawCube((Vector3){0, 0, 0.2f}, 0.1f, 0.1f, 0.1f, BLACK); // tip
        rlPopMatrix();

        rlPopMatrix(); 
    }

    // --- DRAW WHITE LASER BEAM ---
    if (laserCharging || laserFiring) {
        Vector3 laserOrigin = position;
        
        // Set laser origin to the crotch (member) position based on the active model
        if (globalUseWafelModel && wafelModelLoaded) {
            laserOrigin.y += 3.8f; // Crotch height for 3D Wafel Model
            float facingRad = angle * DEG2RAD;
            laserOrigin.x += sinf(facingRad) * 1.5f; // Forward offset for Wafel model
            laserOrigin.z += cosf(facingRad) * 1.5f;
        } else {
            laserOrigin.y += 0.8f * 9.0f; // Crotch height in world space for blocky model
            float facingRad = angle * DEG2RAD;
            laserOrigin.x += sinf(facingRad) * 3.6f; // Forward offset for blocky model (0.4 * 9.0)
            laserOrigin.z += cosf(facingRad) * 3.6f;
        }
        
        if (laserCharging) {
            // Charge-up glow - big pulsing white ball at crotch
            float chargeProgress = laserChargeTimer / 1.5f;
            float pulse = sinf(laserChargeTimer * 12.0f) * 0.4f + 0.6f;
            float glowRadius = 2.1f + chargeProgress * 6.25f;
            
            DrawSphere(laserOrigin, glowRadius * pulse, WHITE);
            DrawSphere(laserOrigin, glowRadius * pulse * 1.5f, (Color){255, 255, 255, 170});
            DrawSphere(laserOrigin, glowRadius * pulse * 2.35f, (Color){255, 230, 230, 60});
            
            // Orbiting particles
            for (int i = 0; i < 10; i++) {
                float orbitAngle = (laserChargeTimer * 6.0f + i * 1.256f);
                Vector3 particlePos = laserOrigin;
                particlePos.x += cosf(orbitAngle) * glowRadius * 2.45f;
                particlePos.y += sinf(orbitAngle * 1.3f) * glowRadius * 1.9f;
                particlePos.z += sinf(orbitAngle) * glowRadius * 2.45f;
                DrawSphere(particlePos, 0.95f + chargeProgress * 1.45f, WHITE);
            }
        }
        
        if (laserFiring) {
            Vector3 beamDir = Vector3Subtract(laserTargetPos, laserOrigin);
            float beamLen = Vector3Length(beamDir);
            
            if (beamLen > 0.1f) {
                Vector3 stepDir = Vector3Normalize(beamDir);
                float thickness = 0.95f;
                float outerThickness = 1.45f;
                
                // Rysujemy gęsto ułożone sześciany wzdłuż całej linii - to w 100% gwarantuje,
                // że wiązka będzie widoczna jako gruby prostokąt i żadne błędy kamery jej nie ukryją.
                int numSteps = (int)(beamLen / 0.24f);
                for (int i = 0; i <= numSteps; i++) {
                    Vector3 pos = {
                        laserOrigin.x + stepDir.x * (i * 0.24f),
                        laserOrigin.y + stepDir.y * (i * 0.24f),
                        laserOrigin.z + stepDir.z * (i * 0.24f)
                    };
                    DrawCube(pos, thickness, thickness, thickness, WHITE);
                    if (i % 2 == 0) {
                        DrawCube(pos, outerThickness, outerThickness, outerThickness, (Color){255, 120, 120, 80});
                    }
                }
                
                // Extra duży blok na końcu, żeby było wyraźnie widać miejsce uderzenia w gracza
                DrawCube(laserTargetPos, 1.6f, 1.6f, 1.6f, RED);
                DrawCube(laserOrigin, 1.5f, 1.5f, 1.5f, WHITE);
            }
        }
    }

    // --- DRAW SNUS SHOT ---
    if (isShootingSnus) {
        Vector3 mouthPos = GetSnusMouthPosition(position, angle, globalUseWafelModel && wafelModelLoaded);
        float travelTime = fmaxf(Vector3Distance(mouthPos, laserTargetPos) / SNUS_SPEED, 0.2f);
        float hitTime = SNUS_CHARGE_TIME + travelTime;

        BeginBlendMode(BLEND_ALPHA);
        if (snusShootTimer < SNUS_CHARGE_TIME) {
            float chargeProgress = snusShootTimer / SNUS_CHARGE_TIME;
            float pulse = sinf(snusShootTimer * 18.0f) * 0.35f + 0.65f;
            float glow = 1.0f + chargeProgress * 2.8f;

            DrawSphere(mouthPos, glow * pulse, (Color){255, 90, 40, 210});
            DrawSphere(mouthPos, glow * pulse * 1.7f, (Color){255, 150, 90, 90});
            for (int i = 0; i < 6; ++i) {
                float a = snusShootTimer * 8.0f + i * 1.047f;
                Vector3 puff = {
                    mouthPos.x + cosf(a) * glow * 0.9f,
                    mouthPos.y + sinf(a * 1.2f) * glow * 0.55f,
                    mouthPos.z + sinf(a) * glow * 0.9f
                };
                DrawSphere(puff, 0.35f + chargeProgress * 0.45f, (Color){255, 110, 70, 170});
            }
        } else if (snusShootTimer < hitTime) {
            Vector3 beamDir = Vector3Subtract(laserTargetPos, mouthPos);
            if (Vector3Length(beamDir) > 0.1f) {
                beamDir = Vector3Normalize(beamDir);
                float progress = fminf((snusShootTimer - SNUS_CHARGE_TIME) * SNUS_SPEED, Vector3Distance(mouthPos, laserTargetPos));
                Vector3 snusPos = Vector3Add(mouthPos, Vector3Scale(beamDir, progress));

                DrawSphere(snusPos, 1.6f, (Color){90, 35, 20, 255});
                DrawSphere(snusPos, 2.4f, (Color){220, 80, 50, 90});

                for (int i = 0; i < 4; ++i) {
                    float tail = 1.2f + i * 0.8f;
                    Vector3 trailPos = Vector3Add(snusPos, Vector3Scale(beamDir, -tail));
                    DrawSphere(trailPos, 1.0f - i * 0.18f, (Color){120, 45, 25, 120 - i * 20});
                }
            }
        } else if (snusShootTimer < hitTime + SNUS_EXPLOSION_LINGER) {
            float boomAge = (snusShootTimer - hitTime) / SNUS_EXPLOSION_LINGER;
            float boomPulse = 1.0f + boomAge * 1.2f;
            BeginBlendMode(BLEND_ALPHA);
            DrawSphere(laserTargetPos, 4.5f * boomPulse, (Color){255, 70, 40, 235});
            DrawSphere(laserTargetPos, 8.5f * boomPulse, (Color){255, 150, 70, 125});
            DrawSphere(laserTargetPos, 13.5f * boomPulse, (Color){255, 220, 140, 65});
            DrawSphere(laserTargetPos, 18.5f * boomPulse, (Color){255, 255, 255, 35});
            DrawCylinder((Vector3){laserTargetPos.x, laserTargetPos.y - 0.5f, laserTargetPos.z},
                         4.0f * boomPulse, 4.0f * boomPulse, 0.35f, 18, (Color){255, 60, 30, 120});
            for (int i = 0; i < 12; ++i) {
                float a = (float)i * 0.5236f + boomAge * 8.0f;
                float r = 4.0f + boomAge * 8.0f;
                Vector3 sparkPos = {
                    laserTargetPos.x + cosf(a) * r,
                    laserTargetPos.y + sinf(a * 1.7f) * 2.0f,
                    laserTargetPos.z + sinf(a) * r
                };
                DrawSphere(sparkPos, 0.45f + boomAge * 0.25f, (Color){255, 120, 70, 180});
            }
            EndBlendMode();
        }
        EndBlendMode();
    }

    // --- DRAW SMOKE CLOUD ---
    if (isSprayingSmoke) {
        float progress = fminf(smokeActiveTimer / 1.25f, 1.0f);
        float cloudRadius = SMOKE_RADIUS * progress;
        float time = (float)GetTime();

        BeginBlendMode(BLEND_ALPHA);
        // Several drifting semi-transparent puffs make the danger zone
        // legible without hiding the whole battle.
        for (int i = 0; i < 24; ++i) {
            float a = time * (0.55f + (i % 4) * 0.10f) + i * 2.399f;
            float ring = cloudRadius * (0.16f + (i % 6) * 0.14f);
            Vector3 puffPos = {
                position.x + cosf(a) * ring,
                position.y + 1.35f + (i % 5) * 1.0f + sinf(a * 1.7f) * 0.8f,
                position.z + sinf(a) * ring
            };
            float puffSize = 3.8f + (i % 5) * 1.1f + progress * 2.6f;
            DrawSphere(puffPos, puffSize, (Color){60, 60, 60, 95});
        }
        DrawCylinder((Vector3){position.x, position.y + 0.05f, position.z},
                     cloudRadius, cloudRadius, 0.1f, 48, (Color){55, 55, 55, 78});
        EndBlendMode();
    }
}

BoundingBox AdasGooner::GetBoundingBox() {
    // Ponieważ scale = 9.0f, musimy to uwzględnić w boksie kolizji.
    // Boss ma ok. 2.5 jednostek wysokości w skali lokalnej, co daje ~22 jednostki w świecie.
    float halfWidth = 4.5f; 
    float height = 22.0f;

    return (BoundingBox){
        (Vector3){ position.x - halfWidth, position.y, position.z - halfWidth },
        (Vector3){ position.x + halfWidth, position.y + height, position.z + halfWidth }
    };
}

#include "adas_gooner.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>
#include <cmath>

namespace {
constexpr float BASE_HALF_SIZE = 25.0f;
constexpr float BASE_EDGE_AGGRO_DISTANCE = 8.0f;

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
        
        // --- WHITE LASER SPECIAL ATTACK ---
        if (laserFiring) {
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
                        if (distToBeam < 4.0f && p.hp) { // 4 unit beam width
                            *p.hp -= 1; // Damage per frame (~60 dps at 60fps), much more survivable
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
            // Normal movement + count down laser cooldown
            Enemy::Update(players, baseHp, basePos);
            
            laserCooldown -= dt;
            if (laserCooldown <= 0.0f) {
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
            float glowRadius = 1.5f + chargeProgress * 4.0f;
            
            DrawSphere(laserOrigin, glowRadius * pulse, WHITE);
            DrawSphere(laserOrigin, glowRadius * pulse * 1.4f, (Color){255, 255, 255, 150});
            
            // Orbiting particles
            for (int i = 0; i < 5; i++) {
                float orbitAngle = (laserChargeTimer * 6.0f + i * 1.256f);
                Vector3 particlePos = laserOrigin;
                particlePos.x += cosf(orbitAngle) * glowRadius * 2.0f;
                particlePos.y += sinf(orbitAngle * 1.3f) * glowRadius * 1.5f;
                particlePos.z += sinf(orbitAngle) * glowRadius * 2.0f;
                DrawSphere(particlePos, 0.8f + chargeProgress * 1.2f, WHITE);
            }
        }
        
        if (laserFiring) {
            Vector3 beamDir = Vector3Subtract(laserTargetPos, laserOrigin);
            float beamLen = Vector3Length(beamDir);
            
            if (beamLen > 0.1f) {
                Vector3 stepDir = Vector3Normalize(beamDir);
                float thickness = 0.6f;
                
                // Rysujemy gęsto ułożone sześciany wzdłuż całej linii - to w 100% gwarantuje,
                // że wiązka będzie widoczna jako gruby prostokąt i żadne błędy kamery jej nie ukryją.
                int numSteps = (int)(beamLen / 0.3f); // Sześcian co 0.3 jednostki
                for (int i = 0; i <= numSteps; i++) {
                    Vector3 pos = {
                        laserOrigin.x + stepDir.x * (i * 0.3f),
                        laserOrigin.y + stepDir.y * (i * 0.3f),
                        laserOrigin.z + stepDir.z * (i * 0.3f)
                    };
                    DrawCube(pos, thickness, thickness, thickness, WHITE);
                }
                
                // Extra duży blok na końcu, żeby było wyraźnie widać miejsce uderzenia w gracza
                DrawCube(laserTargetPos, 1.2f, 1.2f, 1.2f, RED);
                DrawCube(laserOrigin, 1.2f, 1.2f, 1.2f, WHITE);
            }
        }
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

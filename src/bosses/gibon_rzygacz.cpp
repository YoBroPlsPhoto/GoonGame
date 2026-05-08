#include "gibon_rzygacz.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

GibonRzygacz::GibonRzygacz(Vector3 landingPos, int enemyId) 
    : Enemy(landingPos, EnemyType::GIBON_BOSS, WeaponType::EXPLOSIVE, enemyId) {
    gibonState = GibonState::FALLING;
    stateTimer = 0.0f;
    landingTarget = landingPos;
    landingTarget.y = 0.0f;
    
    // Start way up in the sky
    position = landingTarget;
    position.y = 500.0f;
    fallHeight = 500.0f;
    
    // Stats
    hp = 100000;
    maxHp = 100000;
    speed = 0.05f;
    radius = 6.0f;
    color = {80, 200, 50, 255}; // Toxic green
    
    rollAngle = 0.0f;
    rollSpeed = 0.0f;
    
    vomitCooldown = 0.0f;
    
    stutterTimer = 0.0f;
    nextStutterTime = 0.5f + (float)(rand() % 200) / 100.0f;
    isStuttering = false;
    
    craterCreated = false;
    impactCrater = {{0, 0, 0}, 0};
    
    pulseTimer = 0.0f;
    bodyScale = 8.0f;
    toxicColor = {80, 200, 50, 255};
}

void GibonRzygacz::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) { active = false; return; }

    float dt = GetFrameTime();
    stateTimer += dt;
    pulseTimer += dt;
    
    // --- FPS STUTTER EFFECT ---
    stutterTimer += dt;
    if (stutterTimer >= nextStutterTime) {
        stutterTimer = 0.0f;
        nextStutterTime = 0.3f + (float)(rand() % 300) / 100.0f; // Random 0.3 - 3.3 seconds
        isStuttering = true;
    } else {
        isStuttering = false;
    }

    if (gibonState == GibonState::FALLING) {
        // Fall from the sky with acceleration
        float fallSpeed = 2.0f + stateTimer * 8.0f; // Accelerating
        position.y -= fallSpeed * dt * 60.0f;
        
        // Shake effect - wobble position slightly
        position.x = landingTarget.x + sinf(stateTimer * 15.0f) * 2.0f;
        position.z = landingTarget.z + cosf(stateTimer * 12.0f) * 2.0f;
        
        fallHeight = position.y;
        
        if (position.y <= 0.5f) {
            position.y = 0.5f;
            position.x = landingTarget.x;
            position.z = landingTarget.z;
            gibonState = GibonState::IMPACT;
            stateTimer = 0.0f;
            
            // Create crater
            craterCreated = true;
            impactCrater.position = landingTarget;
            impactCrater.position.y = 0.01f;
            impactCrater.radius = 20.0f;
        }
        
        isMoving = false;
        walkTimer = 0;
        angle = 0;
        
    } else if (gibonState == GibonState::IMPACT) {
        // Ground pound impact - pause for dramatic effect
        // Screen shake handled in main via stateTimer
        isMoving = false;
        
        if (stateTimer > 3.0f) {
            gibonState = GibonState::CRATER_PAUSE;
            stateTimer = 0.0f;
        }
        
    } else if (gibonState == GibonState::CRATER_PAUSE) {
        // Rising out of crater slowly
        isMoving = false;
        
        if (stateTimer > 2.0f) {
            gibonState = GibonState::FINISHED_FALLING;
            stateTimer = 0.0f;
        }
        
    } else if (gibonState == GibonState::FINISHED_FALLING) {
        // --- NORMAL COMBAT BEHAVIOR ---
        float timeScale = dt * 60.0f;
        
        // Find nearest target (player or base)
        Vector3 targetPos = basePos;
        int* targetHp = nullptr;
        bool targetingPlayer = false;
        
        float closestDist = 1000.0f;
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            float d = Vector3Distance(p.pos, position);
            if (d < 40.0f && d < closestDist) {
                closestDist = d;
                targetPos = p.pos;
                targetHp = p.hp;
                targetingPlayer = true;
            }
        }
        
        Vector3 direction = Vector3Subtract(targetPos, position);
        direction.y = 0;
        float dist = Vector3Length(direction);
        
        float stopDist = targetingPlayer ? 3.0f : 27.0f;
        
        isMoving = false;
        if (dist > stopDist) {
            isMoving = true;
            Vector3 moveDir = Vector3Normalize(direction);
            angle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
            
            rollSpeed = speed;
            position.x += moveDir.x * rollSpeed * timeScale;
            position.z += moveDir.z * rollSpeed * timeScale;
            
            // Rolling animation
            rollAngle += rollSpeed * timeScale * 20.0f;
        } else {
            rollSpeed *= 0.95f;
            angle = atan2f(direction.x, direction.z) * RAD2DEG;
        }
        
        if (isMoving) walkTimer += dt;
        else walkTimer = 0;
        
        // --- CRUSH ATTACK (rolling over players) ---
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            float d = Vector3Distance(p.pos, position);
            if (d < 5.0f && p.hp) {
                *p.hp -= (int)(150 * dt); // Constant crush damage while overlapping
            }
        }
        
        // --- TOXIC VOMIT RAIN ATTACK ---
        vomitCooldown -= dt;
        if (vomitCooldown <= 0) {
            vomitCooldown = 2.5f; // Every 2.5 seconds
            
            // Spawn a barrage of toxic vomit projectiles that rain down
            for (int i = 0; i < 12; i++) {
                ToxicVomit v;
                // Spread around the boss, targeting area around players
                Vector3 target = targetPos;
                target.x += (float)(rand() % 60 - 30);
                target.z += (float)(rand() % 60 - 30);
                
                v.position = {target.x + (float)(rand() % 10 - 5), 80.0f + (float)(rand() % 40), target.z + (float)(rand() % 10 - 5)};
                v.velocity = {0, -25.0f - (float)(rand() % 15), 0};
                v.lifetime = 5.0f;
                v.active = true;
                vomitProjectiles.push_back(v);
            }
        }
        
        // --- BASE DAMAGE (melee range) ---
        if (attackTimer > 0) attackTimer -= dt;
        if (!targetingPlayer && dist < 31.0f && attackTimer <= 0) {
            if (baseHp && *baseHp > 0) {
                *baseHp -= 200; // Heavy base damage
                if (*baseHp < 0) *baseHp = 0;
            }
            attackTimer = 1.5f;
        }
        
        // Enrage below 30% HP
        if (hp < maxHp * 0.3f) {
            speed = 0.1f;
            toxicColor = {200, 255, 50, 255}; // Brighter toxic
            vomitCooldown = std::min(vomitCooldown, 1.2f); // Faster vomit
        }
    }
    
    // --- UPDATE VOMIT PROJECTILES ---
    for (auto& v : vomitProjectiles) {
        if (!v.active) continue;
        v.position.x += v.velocity.x * dt;
        v.position.y += v.velocity.y * dt;
        v.position.z += v.velocity.z * dt;
        v.lifetime -= dt;
        
        if (v.position.y <= 0.5f || v.lifetime <= 0) {
            v.active = false;
            
            // Damage players near impact
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                float d = Vector3Distance(p.pos, v.position);
                if (d < 5.0f && p.hp) {
                    float falloff = 1.0f - (d / 5.0f);
                    *p.hp -= (int)(25 * falloff);
                }
            }
            
            // Also damage base if near
            if (baseHp && *baseHp > 0) {
                float baseDist = Vector3Distance(basePos, v.position);
                if (baseDist < 30.0f) {
                    *baseHp -= 30;
                    if (*baseHp < 0) *baseHp = 0;
                }
            }
        }
    }
    
    // Clean up inactive vomits
    vomitProjectiles.erase(
        std::remove_if(vomitProjectiles.begin(), vomitProjectiles.end(), 
            [](const ToxicVomit& v) { return !v.active; }),
        vomitProjectiles.end()
    );
}

void GibonRzygacz::Draw() {
    if (!active) return;
    
    float pulse = sinf(pulseTimer * 3.0f) * 0.15f + 1.0f;
    float currentScale = bodyScale * pulse;
    
    // --- CRATER ---
    if (craterCreated) {
        // Deep black hole (decal on ground)
        DrawCircle3D({impactCrater.position.x, 0.05f, impactCrater.position.z}, impactCrater.radius, {1, 0, 0}, 90.0f, BLACK);
        
        // Jagged toxic inner pool
        DrawCircle3D({impactCrater.position.x, 0.06f, impactCrater.position.z}, impactCrater.radius * 0.7f, {1, 0, 0}, 90.0f, 
                     Fade(toxicColor, 0.6f + sinf(pulseTimer * 2.0f) * 0.2f));

        // Create jagged, broken asphalt chunks pushing upwards around the edge
        for (int i = 0; i < 20; i++) {
            float a = (float)i / 20.0f * PI * 2.0f;
            // Randomize size and offset based on angle
            float offsetR = impactCrater.radius + sinf(a * 7.0f) * 1.5f;
            float cx = impactCrater.position.x + cosf(a) * offsetR;
            float cz = impactCrater.position.z + sinf(a) * offsetR;
            
            rlPushMatrix();
            rlTranslatef(cx, 0.0f, cz);
            rlRotatef(a * RAD2DEG, 0, 1, 0); // Face outward
            rlRotatef(-25.0f - sinf(a * 4) * 15.0f, 1, 0, 0); // Pitch up (breaking out of ground)
            
            // Draw chunk of asphalt
            DrawCube({0, 1.0f, 0}, 4.0f + sinf(a * 5) * 2.0f, 2.0f, 3.0f, {60, 60, 60, 255});
            DrawCubeWires({0, 1.0f, 0}, 4.0f + sinf(a * 5) * 2.0f, 2.0f, 3.0f, BLACK);
            rlPopMatrix();
            
            // Cracks spreading outward on the road
            if (i % 3 == 0) {
                Vector3 crackEnd = {
                    impactCrater.position.x + cosf(a) * (impactCrater.radius * 2.5f + (float)(rand()%10)),
                    0.07f,
                    impactCrater.position.z + sinf(a) * (impactCrater.radius * 2.5f + (float)(rand()%10))
                };
                Vector3 crackStart = {cx, 0.07f, cz};
                DrawLine3D(crackStart, crackEnd, BLACK);
            }
        }
    }
    
    // --- MAIN BODY (Giant toxic ball) ---
    rlPushMatrix();
    rlTranslatef(position.x, position.y + currentScale * 0.5f, position.z);
    
    // Roll rotation
    rlRotatef(angle, 0, 1, 0);
    rlRotatef(rollAngle, 1, 0, 0);
    
    // Core sphere
    DrawSphere({0, 0, 0}, currentScale, toxicColor);
    
    // Toxic slime dripping effect - bumps on surface
    for (int i = 0; i < 20; i++) {
        float a1 = (float)i / 20.0f * PI * 2.0f;
        float a2 = sinf(a1 * 3.0f + pulseTimer) * 0.8f;
        Vector3 bumpPos = {
            cosf(a1) * cosf(a2) * currentScale * 0.95f,
            sinf(a2) * currentScale * 0.95f,
            sinf(a1) * cosf(a2) * currentScale * 0.95f
        };
        float bumpSize = 1.0f + sinf(a1 * 5 + pulseTimer * 2) * 0.5f;
        DrawSphere(bumpPos, bumpSize, Fade({50, 150, 30, 255}, 0.7f));
    }
    
    // Gross face on the ball
    rlPushMatrix();
    rlRotatef(-rollAngle, 1, 0, 0); // Counter-rotate so face stays visible
    rlRotatef(-angle, 0, 1, 0);
    
    // Eyes (bloodshot)
    float eyeOffset = currentScale * 0.3f;
    DrawSphere({-eyeOffset, eyeOffset * 0.5f, currentScale * 0.85f}, 1.2f, WHITE);
    DrawSphere({eyeOffset, eyeOffset * 0.5f, currentScale * 0.85f}, 1.2f, WHITE);
    DrawSphere({-eyeOffset, eyeOffset * 0.5f, currentScale * 0.9f}, 0.6f, RED);
    DrawSphere({eyeOffset, eyeOffset * 0.5f, currentScale * 0.9f}, 0.6f, RED);
    
    // Mouth (open, drooling)
    DrawSphere({0, -eyeOffset * 0.3f, currentScale * 0.85f}, 2.0f, {40, 100, 20, 255});
    // Drool/vomit dripping from mouth
    float droolLen = 2.0f + sinf(pulseTimer * 4.0f) * 1.5f;
    DrawCylinder({0, -eyeOffset * 0.3f - droolLen, currentScale * 0.8f}, 0.3f, 0.8f, droolLen, 6, 
                 Fade({120, 200, 40, 255}, 0.6f));
    
    rlPopMatrix();
    
    // Impact shockwave during IMPACT state
    if (gibonState == GibonState::IMPACT && stateTimer < 2.0f) {
        float shockRadius = stateTimer * 40.0f;
        float alpha = 1.0f - stateTimer / 2.0f;
        DrawCircle3D({0, -currentScale * 0.5f + 0.1f, 0}, shockRadius, {1, 0, 0}, 90.0f, 
                     Fade({200, 255, 50, 255}, alpha * 0.5f));
    }
    
    rlPopMatrix();
    
    // --- SHADOW ---
    if (gibonState == GibonState::FALLING) {
        // Growing shadow on ground as ball falls
        float shadowScale = (500.0f - position.y) / 500.0f;
        DrawCircle3D({landingTarget.x, 0.05f, landingTarget.z}, currentScale * shadowScale * 1.5f, 
                     {1, 0, 0}, 90.0f, Fade(BLACK, 0.3f + shadowScale * 0.4f));
    } else {
        DrawCircle3D({position.x, 0.05f, position.z}, currentScale * 1.2f, {1, 0, 0}, 90.0f, Fade(BLACK, 0.5f));
    }
    
    // --- TOXIC VOMIT PROJECTILES ---
    for (const auto& v : vomitProjectiles) {
        if (!v.active) continue;
        // Green toxic ball
        DrawSphere(v.position, 0.8f, {100, 220, 40, 255});
        // Trail
        DrawSphere({v.position.x, v.position.y + 1.5f, v.position.z}, 0.4f, Fade({150, 255, 50, 255}, 0.4f));
        DrawSphere({v.position.x, v.position.y + 3.0f, v.position.z}, 0.25f, Fade({150, 255, 50, 255}, 0.2f));
    }
    
    // --- TOXIC AURA (particles around the ball) ---
    for (int i = 0; i < 8; i++) {
        float a = pulseTimer * 2.0f + (float)i * (PI * 2.0f / 8.0f);
        float r = currentScale * 1.3f;
        Vector3 particlePos = {
            position.x + cosf(a) * r,
            position.y + currentScale * 0.5f + sinf(a * 2.0f + pulseTimer) * 2.0f,
            position.z + sinf(a) * r
        };
        float pSize = 0.5f + sinf(pulseTimer * 3.0f + (float)i) * 0.3f;
        DrawSphere(particlePos, pSize, Fade({100, 255, 50, 255}, 0.3f));
    }
}

void GibonRzygacz::DrawHUD(Camera3D camera) {
    if (!active || hp <= 0) return;
    
    float viewDist = 500.0f; // Boss visible from far
    if (Vector3Distance(position, camera.position) > viewDist) return;
    
    float barY = position.y + bodyScale * 2.5f;
    Vector3 barPos = {position.x, barY, position.z};
    
    Vector3 toEnemy = Vector3Subtract(barPos, camera.position);
    Vector3 camFwd = Vector3Subtract(camera.target, camera.position);
    if (Vector3DotProduct(toEnemy, camFwd) < 0) return;
    
    float hpPercent = std::max(0.0f, std::min(1.0f, (float)hp / maxHp));
    Vector2 screenPos = GetWorldToScreen(barPos, camera);
    
    // Big boss HP bar
    float bW = 200.0f;
    float bH = 16.0f;
    
    // Background
    DrawRectangle((int)(screenPos.x - bW/2) - 2, (int)(screenPos.y - bH) - 2, (int)bW + 4, (int)bH + 4, BLACK);
    DrawRectangle((int)(screenPos.x - bW/2), (int)(screenPos.y - bH), (int)bW, (int)bH, DARKGRAY);
    
    // HP fill - toxic green
    Color hpColor = (hp > maxHp * 0.3f) ? (Color){80, 200, 50, 255} : (Color){200, 255, 50, 255};
    DrawRectangle((int)(screenPos.x - bW/2), (int)(screenPos.y - bH), (int)(bW * hpPercent), (int)bH, hpColor);
    
    // Boss name
    const char* name = "GIBON RZYGACZ";
    int nameW = MeasureText(name, 22);
    DrawText(name, (int)(screenPos.x - nameW/2.0f), (int)(screenPos.y - bH - 28), 22, {100, 255, 50, 255});
    
    // HP text
    const char* hpText = TextFormat("%d / %d", hp, maxHp);
    int textW = MeasureText(hpText, 14);
    DrawText(hpText, (int)(screenPos.x - textW/2.0f), (int)(screenPos.y - bH + 1), 14, WHITE);
}

BoundingBox GibonRzygacz::GetBoundingBox() {
    float s = bodyScale;
    return (BoundingBox){
        (Vector3){ position.x - s, position.y, position.z - s },
        (Vector3){ position.x + s, position.y + s * 2.0f, position.z + s }
    };
}

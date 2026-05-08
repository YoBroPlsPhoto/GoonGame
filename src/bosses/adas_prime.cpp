#include "adas_prime.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>

AdasPrime::AdasPrime(Vector3 startPos, int enemyId) 
    : Enemy(startPos, EnemyType::ADAS_PRIME, WeaponType::KATANA, enemyId) {
    cutsceneState = PrimeCutscene::WARDROBE_CLOSED;
    cutsceneTimer = 0.0f;
    wardrobePos = startPos;
    wardrobePos.y = 0.0f;
    position = startPos;
    
    // Stats for Adas Gooner Prime
    hp = 160000;
    maxHp = 160000;
    speed = 0.12f; // Much faster than original
    radius = 6.0f;
    color = {255, 215, 0, 255}; // Gold
}

void AdasPrime::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) { active = false; return; }

    float dt = GetFrameTime();
    cutsceneTimer += dt;
    auraTimer += dt;

    if (cutsceneState == PrimeCutscene::WARDROBE_CLOSED) {
        if (cutsceneTimer > 3.0f) {
            cutsceneState = PrimeCutscene::WARDROBE_OPENING;
            cutsceneTimer = 0.0f;
        }
        isMoving = false; walkTimer = 0; angle = 0;
    } else if (cutsceneState == PrimeCutscene::WARDROBE_OPENING) {
        if (cutsceneTimer > 4.0f) {
            cutsceneState = PrimeCutscene::WALKING_OUT;
            cutsceneTimer = 0.0f;
        }
        isMoving = false; walkTimer = 0;
    } else if (cutsceneState == PrimeCutscene::WALKING_OUT) {
        float timeScale = dt * 60.0f;
        position.z += speed * 2.5f * timeScale; 
        isMoving = true; 
        walkTimer += dt;
        if (cutsceneTimer > 4.0f) cutsceneState = PrimeCutscene::FIGHT;
    } else {
        // Fight Behavior
        Enemy::Update(players, baseHp, basePos);
        
        // --- PRIME SPECIAL ATTACKS ---
        shockwaveTimer += dt;
        if (shockwaveTimer > 8.0f) {
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                float d = Vector3Distance(position, p.pos);
                if (d < 35.0f) {
                    if (p.hp) *p.hp -= 25; // Massive shockwave
                }
            }
            shockwaveTimer = 0;
        }
        
        // Summoning minions more frequently
        summonTimer += dt;
        if (summonTimer > 15.0f) {
            // Minions are spawned via main wave logic usually, but here we just signify activity
            summonTimer = 0;
        }
        
        if (!isEnraged && hp < maxHp * 0.4f) {
            isEnraged = true; 
            speed *= 1.8f; 
        }
        
        // Heavy base damage
        float dx = fmaxf(0.0f, fabsf(position.x - basePos.x) - 25.0f);
        float dz = fmaxf(0.0f, fabsf(position.z - basePos.z) - 25.0f);
        float distToBase = sqrtf(dx*dx + dz*dz);
        if (distToBase < 6.0f && attackTimer <= 0) {
            if (baseHp && *baseHp > 0) {
                *baseHp -= 600.0f; // Extremely heavy damage
                if (*baseHp < 0) *baseHp = 0;
            }
            attackTimer = 1.0f;
        }
    }
}

void AdasPrime::Draw() {
    if (!active) return;

    if (cutsceneState != PrimeCutscene::FIGHT) {
        // --- DRAW GOLDEN WARDROBE ---
        rlPushMatrix();
        rlTranslatef(wardrobePos.x, wardrobePos.y, wardrobePos.z);
        rlRotatef(180, 0, 1, 0);
        
        DrawCube((Vector3){0, 10.0f, 3.0f}, 14.0f, 20.0f, 1.2f, {200, 170, 0, 255}); // Gold back
        
        float doorAngle = 0;
        if (cutsceneState == PrimeCutscene::WARDROBE_OPENING) doorAngle = (cutsceneTimer / 4.0f) * 110.0f;
        else if (cutsceneState == PrimeCutscene::WALKING_OUT) doorAngle = 110.0f;

        // Doors
        rlPushMatrix(); rlTranslatef(-6.5f, 10.0f, -3.0f); rlRotatef(doorAngle, 0, 1, 0);
        DrawCube((Vector3){3.25f, 0, 0}, 6.5f, 19.0f, 0.6f, {218, 165, 32, 255}); rlPopMatrix();
        
        rlPushMatrix(); rlTranslatef(6.5f, 10.0f, -3.0f); rlRotatef(-doorAngle, 0, 1, 0);
        DrawCube((Vector3){-3.25f, 0, 0}, 6.5f, 19.0f, 0.6f, {218, 165, 32, 255}); rlPopMatrix();
        rlPopMatrix();
    }

    if (cutsceneState == PrimeCutscene::WARDROBE_CLOSED) return;

    // --- GOLDEN AURA ---
    float auraRadius = 8.0f + sinf(auraTimer * 5.0f) * 2.0f;
    DrawCircle3D(position, auraRadius, {1, 0, 0}, 90.0f, Fade(GOLD, 0.4f));
    for(int i=0; i<3; i++) {
        DrawCircle3D({position.x, position.y + 2.0f + i*4.0f, position.z}, auraRadius * (1.0f - i*0.2f), {1, 0, 0}, 90.0f, Fade(GOLD, 0.2f));
    }

    // --- DRAW ADAS PRIME ---
    float scale = 14.0f; // Even bigger than regular Adas
    float animWalk = isMoving ? sinf(walkTimer * 10.0f) : 0.0f;
    
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(angle, 0, 1, 0);
    rlScalef(scale, scale, scale);

    Color skin = { 255, 220, 180, 255 }; 
    Color clothes = { 20, 20, 20, 255 }; // Jet black suit

    // Legs
    rlPushMatrix(); rlTranslatef(-0.2f, 0.4f, 0); rlRotatef(animWalk * 35.0f, 1, 0, 0);
    DrawCube({0,0,0}, 0.25f, 0.8f, 0.25f, BLACK); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(0.2f, 0.4f, 0); rlRotatef(-animWalk * 35.0f, 1, 0, 0);
    DrawCube({0,0,0}, 0.25f, 0.8f, 0.25f, BLACK); rlPopMatrix();

    // Torso
    DrawCube({ 0, 1.25f, 0 }, 1.0f, 0.9f, 0.6f, clothes); 
    DrawCube({ 0, 1.75f, 0 }, 1.3f, 0.4f, 0.7f, clothes); // huge shoulders

    // Golden Tie
    DrawCube({ 0, 1.6f, 0.31f }, 0.15f, 0.6f, 0.05f, GOLD);

    // Head
    rlPushMatrix(); rlTranslatef(0, 2.2f, 0);
    DrawCube({0,0,0}, 0.5f, 0.5f, 0.5f, skin);
    // Golden Crown/Sunglasses
    DrawCube({0, 0.15f, 0.26f}, 0.4f, 0.15f, 0.1f, GOLD); // Golden visor
    rlPopMatrix();
    
    rlPopMatrix();
}

BoundingBox AdasPrime::GetBoundingBox() {
    float s = 7.0f;
    return (BoundingBox){
        (Vector3){ position.x - s, position.y, position.z - s },
        (Vector3){ position.x + s, position.y + 15.0f, position.z + s }
    };
}

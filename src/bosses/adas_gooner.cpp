#include "adas_gooner.hpp"
#include "rlgl.h"
#include "raymath.h"

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
    speed = 0.05f; // Extremely slow but threatening
    radius = 3.0f;
    color = {255, 100, 100, 255}; // Deep red
}

void AdasGooner::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) { active = false; return; }

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
    } else {
        // Normal Behavior
        Enemy::Update(players, baseHp, basePos);
        
        // --- SPECIAL ATTACKS ---
        shockwaveTimer += GetFrameTime();
        if (shockwaveTimer > 10.0f) {
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                float d = Vector3Distance(position, p.pos);
                if (d < 20.0f) {
                    // Push logic (vague since we don't have direct access to their pos as reference in a way to push them back to server?)
                    // For now, we apply damage.
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

    if (cutsceneState != CutsceneState::FINISHED) {
        // --- DRAW WARDROBE ---
        rlPushMatrix();
        rlTranslatef(wardrobePos.x, wardrobePos.y, wardrobePos.z);
        // Turn around so doors face forward
        rlRotatef(180, 0, 1, 0);
        
        // Structure
        DrawCube((Vector3){0, 9.0f, 3.0f}, 12.0f, 18.0f, 1.0f, DARKBROWN); // Back
        DrawCube((Vector3){-6.0f, 9.0f, 0}, 1.0f, 18.0f, 6.0f, DARKBROWN); // Left
        DrawCube((Vector3){6.0f, 9.0f, 0}, 1.0f, 18.0f, 6.0f, DARKBROWN);  // Right
        DrawCube((Vector3){0, 18.5f, 0}, 13.0f, 1.0f, 6.0f, BLACK);       // Top roof
        DrawCube((Vector3){0, 0.5f, 0}, 13.0f, 1.0f, 6.0f, BLACK);        // Base
        
        // Doors
        float doorAngle = 0;
        if (cutsceneState == CutsceneState::WARDROBE_OPENING) {
            doorAngle = (cutsceneTimer / 5.0f) * 110.0f; // Opens gradually
        } else if (cutsceneState == CutsceneState::WALKING_OUT || cutsceneState == CutsceneState::FINISHED) {
            doorAngle = 110.0f;
        }

        // Left door
        rlPushMatrix();
        rlTranslatef(-5.5f, 9.0f, -3.0f);
        rlRotatef(doorAngle, 0, 1, 0); // Open outwards
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

    // Only draw him if doors are opening or he's walked out
    if (cutsceneState == CutsceneState::WARDROBE_CLOSED) return;

    // --- DRAW ADAS GOONER ---
    float scale = 2.5f; // Scaled down to player-like size
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

    rlPopMatrix(); 
}

BoundingBox AdasGooner::GetBoundingBox() {
    // Ponieważ scale = 9.0f, musimy to uwzględnić w boksie kolizji.
    // Boss ma ok. 2.5 jednostek wysokości w skali lokalnej, co daje ~22 jednostki w świecie.
    float halfWidth = 1.5f; 
    float height = 5.0f;

    return (BoundingBox){
        (Vector3){ position.x - halfWidth, position.y, position.z - halfWidth },
        (Vector3){ position.x + halfWidth, position.y + height, position.z + halfWidth }
    };
}

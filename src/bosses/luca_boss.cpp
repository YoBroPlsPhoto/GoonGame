#include "luca_boss.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

LucaBoss::LucaBoss(Vector3 startPos, int enemyId)
    : Enemy(startPos, EnemyType::LUCA_BOSS, WeaponType::MACHETE, enemyId) {
    hp = 90000;
    maxHp = hp;
    speed = 0.11f;
    radius = 5.0f;
    color = {150, 0, 24, 255};
    portalPos = startPos;
    portalPos.y = 0.0f;
    position.z = portalPos.z - 12.0f;
    angle = 0.0f;
}

void LucaBoss::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) {
        active = false;
        return;
    }

    float dt = GetFrameTime();
    portalTimer += dt;
    auraTimer += dt;

    // Portal intro animation - unchanged
    if (portalTimer < 2.6f) {
        isMoving = false;
        walkTimer = 0.0f;
        position.x = portalPos.x;
        position.z = portalPos.z - 12.0f;
        angle = 0.0f;
        return;
    }

    if (portalTimer < 5.4f) {
        float t = (portalTimer - 2.6f) / 2.8f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        position.x = portalPos.x;
        position.z = portalPos.z - 12.0f + t * 15.0f;
        angle = 0.0f;
        isMoving = true;
        walkTimer += dt;
        return;
    }

    // --- Luca targeting: player first, base second ---
    // Find the nearest player who is NOT inside the base
    constexpr float BASE_HALF = 25.0f;
    constexpr float DETECT_RANGE = 50.0f;
    constexpr float MELEE_RANGE = 8.0f;

    Vector3 targetPos = basePos;
    int* targetHp = nullptr;
    bool chasingPlayer = false;
    float closestDist = DETECT_RANGE + 1.0f;

    for (const auto& p : players) {
        if (!p.active || (p.hp && *p.hp <= 0)) continue;
        if (p.isStructure) continue; // Only target actual players, not structures

        // Skip players inside the base - Luca attacks the base instead
        bool playerInBase = fabsf(p.pos.x - basePos.x) <= BASE_HALF &&
                            fabsf(p.pos.z - basePos.z) <= BASE_HALF;
        if (playerInBase) continue;

        float d = Vector3Distance(p.pos, position);
        if (d < closestDist) {
            closestDist = d;
            targetPos = p.pos;
            targetHp = p.hp;
            chasingPlayer = true;
        }
    }

    if (attackTimer > 0) attackTimer -= dt;
    if (isMoving) walkTimer += dt;
    else walkTimer = 0;

    if (chasingPlayer) {
        // Chase the player
        Vector3 dir = Vector3Subtract(targetPos, position);
        dir.y = 0;
        float dist = Vector3Length(dir);

        angle = atan2f(dir.x, dir.z) * RAD2DEG;

        if (dist > MELEE_RANGE) {
            isMoving = true;
            Vector3 moveDir = Vector3Normalize(dir);
            float timeScale = dt * 60.0f;
            position.x += moveDir.x * speed * timeScale;
            position.z += moveDir.z * speed * timeScale;
            velocity = {moveDir.x * speed * timeScale, 0, moveDir.z * speed * timeScale};
        } else {
            isMoving = false;
            velocity = {0, 0, 0};

            // Attack the player
            if (attackTimer <= 0.0f && targetHp && *targetHp > 0) {
                *targetHp -= 70; // Player damage (350 / 5 like other bosses)
                attackTimer = 1.1f;
            }
        }
    } else {
        // No player target - march to the base and attack it
        Vector3 dir = Vector3Subtract(basePos, position);
        dir.y = 0;
        float moveDist = Vector3Length(dir);

        angle = atan2f(dir.x, dir.z) * RAD2DEG;

        if (moveDist > 28.0f) {
            isMoving = true;
            Vector3 moveDir = Vector3Normalize(dir);
            float timeScale = dt * 60.0f;
            position.x += moveDir.x * speed * timeScale;
            position.z += moveDir.z * speed * timeScale;
            velocity = {moveDir.x * speed * timeScale, 0, moveDir.z * speed * timeScale};
        } else {
            isMoving = false;
            velocity = {0, 0, 0};
        }

        // Attack the base when close enough
        float bdx = fmaxf(0.0f, fabsf(position.x - basePos.x) - BASE_HALF);
        float bdz = fmaxf(0.0f, fabsf(position.z - basePos.z) - BASE_HALF);
        float distToBase = sqrtf(bdx * bdx + bdz * bdz);
        if (distToBase < 7.0f && attackTimer <= 0.0f && baseHp && *baseHp > 0) {
            *baseHp -= 350.0f;
            if (*baseHp < 0.0f) *baseHp = 0.0f;
            attackTimer = 1.1f;
        }
    }
}

void LucaBoss::Draw() {
    if (!active) return;

    if (portalTimer < 6.8f) {
        float fadeOut = portalTimer < 5.4f ? 1.0f : 1.0f - (portalTimer - 5.4f) / 1.4f;
        if (fadeOut < 0.0f) fadeOut = 0.0f;
        float pulse = sinf(auraTimer * 8.0f) * 0.5f + 0.5f;
        float open = portalTimer / 2.6f;
        if (open > 1.0f) open = 1.0f;
        if (open < 0.0f) open = 0.0f;
        open = open * open * (3.0f - 2.0f * open);
        float portalW = 1.0f + open * (11.4f + pulse * 0.35f);
        float portalH = 1.8f + open * (20.5f + pulse * 0.55f);

        rlPushMatrix();
        rlTranslatef(portalPos.x, portalPos.y, portalPos.z);
        rlPushMatrix();
        rlScalef(portalW * 0.50f, portalH * 0.50f, 0.12f);
        DrawSphere({0.0f, 1.0f, 0.0f}, 1.0f, Fade(WHITE, 0.78f * fadeOut));
        rlPopMatrix();
        rlPopMatrix();
    }

    if (portalTimer < 2.6f) return;

    float scale = 9.0f;
    float walk = isMoving ? sinf(walkTimer * 8.0f) : 0.0f;
    Color robe = {150, 0, 24, 255};
    Color darkRobe = {105, 0, 18, 255};
    Color skin = {118, 76, 52, 255};
    Color crackRed = {255, 35, 45, 255};
    Color boot = {184, 105, 57, 255};
    Color bootGold = {255, 220, 35, 255};

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(angle, 0, 1, 0);
    rlScalef(scale, scale, scale);

    // Back red jagged wings/hair from the reference.
    DrawCube({-0.62f, 2.15f, -0.16f}, 0.95f, 0.22f, 0.16f, darkRobe);
    DrawCube({0.62f, 2.15f, -0.16f}, 0.95f, 0.22f, 0.16f, darkRobe);
    DrawCube({-0.72f, 2.38f, -0.17f}, 0.55f, 0.28f, 0.14f, darkRobe);
    DrawCube({0.72f, 2.38f, -0.17f}, 0.55f, 0.28f, 0.14f, darkRobe);

    // Arms.
    rlPushMatrix();
    rlTranslatef(-0.82f, 1.35f, 0.0f);
    rlRotatef(18.0f + walk * 8.0f, 0, 0, 1);
    DrawCube({0, 0, 0}, 0.24f, 1.2f, 0.22f, skin);
    DrawCube({0.01f, 0.2f, 0.13f}, 0.025f, 0.65f, 0.025f, crackRed);
    DrawCube({-0.04f, 0.08f, 0.14f}, 0.025f, 0.42f, 0.025f, crackRed);
    DrawCube({0.02f, -0.7f, 0.0f}, 0.34f, 0.18f, 0.22f, skin);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.82f, 1.35f, 0.0f);
    rlRotatef(-18.0f - walk * 8.0f, 0, 0, 1);
    DrawCube({0, 0, 0}, 0.24f, 1.2f, 0.22f, skin);
    DrawCube({0.0f, 0.18f, 0.14f}, 0.03f, 0.52f, 0.025f, BLACK);
    DrawCube({0.07f, -0.05f, 0.14f}, 0.03f, 0.44f, 0.025f, BLACK);
    DrawCube({0.02f, -0.7f, 0.0f}, 0.34f, 0.18f, 0.22f, skin);
    rlPopMatrix();

    // Legs and curled golden boots.
    for (int side = -1; side <= 1; side += 2) {
        rlPushMatrix();
        rlTranslatef(0.22f * side, 0.34f, 0.0f);
        rlRotatef(-walk * side * 18.0f, 1, 0, 0);
        DrawCube({0, 0.0f, 0}, 0.22f, 0.85f, 0.18f, skin);
        DrawCube({0.04f * side, 0.1f, 0.11f}, 0.025f, 0.62f, 0.02f, BLACK);
        DrawCube({0.11f * side, -0.42f, 0.16f}, 0.52f, 0.18f, 0.28f, boot);
        DrawCube({0.17f * side, -0.40f, 0.25f}, 0.36f, 0.06f, 0.08f, bootGold);
        rlPopMatrix();
    }

    // Big red robe/body.
    DrawCube({0, 1.28f, 0}, 1.35f, 1.8f, 0.55f, robe);
    DrawCube({0, 2.16f, 0}, 1.18f, 0.46f, 0.54f, robe);
    DrawCube({0, 2.02f, 0.29f}, 0.64f, 0.34f, 0.05f, BLACK);

    // One long red eye/mouth shape, black slit, white shine.
    DrawCube({0, 1.62f, 0.31f}, 0.92f, 0.16f, 0.06f, {255, 38, 45, 255});
    DrawCube({0.08f, 1.62f, 0.35f}, 0.68f, 0.07f, 0.04f, BLACK);
    DrawCube({-0.35f, 1.67f, 0.36f}, 0.14f, 0.05f, 0.03f, {255, 175, 205, 255});

    // Chest badge.
    DrawCube({0.34f, 1.25f, 0.32f}, 0.32f, 0.22f, 0.05f, WHITE);
    DrawCube({0.34f, 1.25f, 0.355f}, 0.20f, 0.055f, 0.03f, BLACK);
    DrawCube({0.34f, 1.31f, 0.355f}, 0.16f, 0.035f, 0.03f, BLACK);

    // Head jewel.
    DrawCube({0, 2.52f, 0.02f}, 0.10f, 0.34f, 0.10f, {255, 35, 55, 255});
    DrawCube({0, 2.40f, 0.08f}, 0.32f, 0.045f, 0.045f, BLACK);

    rlPopMatrix();
}

BoundingBox LucaBoss::GetBoundingBox() {
    return (BoundingBox){
        {position.x - 6.2f, position.y, position.z - 4.2f},
        {position.x + 6.2f, position.y + 22.0f, position.z + 4.2f}
    };
}

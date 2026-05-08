#include "enemy.hpp"
#include "raymath.h"

Enemy::Enemy(Vector3 startPos, EnemyType t, WeaponType w, int enemyId) {
    position = startPos;
    type = t;
    weapon = w;
    id = enemyId;
    if (type == EnemyType::MELEE) {
        speed = 0.25f; hp = 100; color = RED; attackCooldown = 0.8f;
    } else if (type == EnemyType::SHOOTER) {
        speed = 0.15f; hp = 120; color = BLUE; attackCooldown = 2.0f;
    } else if (type == EnemyType::FAST) {
        speed = 0.45f; hp = 50; color = YELLOW; attackCooldown = 0.5f;
    } else if (type == EnemyType::TANK) {
        speed = 0.1f; hp = 400; color = PURPLE; attackCooldown = 1.5f;
    } else if (type == EnemyType::BOSS) {
        speed = 0.08f; hp = 3000; color = ORANGE; attackCooldown = 3.0f;
    } else if (type == EnemyType::MINION) {
        speed = 0.6f; hp = 20; color = WHITE; attackCooldown = 0.4f;
    } else if (type == EnemyType::ELITE) {
        speed = 0.12f; hp = 300; color = SKYBLUE; attackCooldown = 1.2f;
    } else if (type == EnemyType::GIGA_TANK) {
        speed = 0.05f; hp = 1500; color = DARKGREEN; attackCooldown = 2.5f;
    } else if (type == EnemyType::KAMIKAZE) {
        speed = 0.8f; hp = 30; color = ORANGE; attackCooldown = 0.1f;
    } else if (type == EnemyType::GIBON_BOSS) {
        speed = 0.05f; hp = 100000; color = {80, 200, 50, 255}; attackCooldown = 1.5f;
    } else if (type == EnemyType::GANG_BOSS) {
        speed = 0.12f; hp = 125000; color = {50, 50, 50, 255}; attackCooldown = 1.8f;
    }
    maxHp = hp;

    radius = 0.5f;
    active = true;
    attackTimer = 0.0f;
    angle = 0.0f;
    isMoving = false;
    velocity = {0,0,0};
    walkTimer = 0.0f;
}

void Enemy::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) { active = false; return; }

    if (isMoving) walkTimer += GetFrameTime();
    else walkTimer = 0;

    Vector3 targetPos = basePos;
    int* targetHp = nullptr;
    bool targetingPlayer = false;

    // Target nearest player if close and alive
    float closestDist = 1000.0f;
    for (const auto& p : players) {
        if (!p.active || (p.hp && *p.hp <= 0)) continue;
        float d = Vector3Distance(p.pos, position);
        if (d < 35.0f && d < closestDist) { // Increased detection range
            closestDist = d;
            targetPos = p.pos;
            targetHp = p.hp;
            targetingPlayer = true;
        }
    }

    // Bosses should NOT follow the player, they only face them to attack
    Vector3 moveTarget = targetPos;
    if (type == EnemyType::BOSS || type == EnemyType::GIBON_BOSS || 
        type == EnemyType::GANG_BOSS || type == EnemyType::ADAS_PRIME) {
        moveTarget = basePos; // Always move towards base
    }

    Vector3 direction = Vector3Subtract(targetPos, position); // For facing/attacking
    Vector3 moveDirection = Vector3Subtract(moveTarget, position); // For moving
    direction.y = 0;
    moveDirection.y = 0;
    
    float dist = Vector3Length(direction);
    float moveDist = Vector3Length(moveDirection);

    // Stop distance: 28m for base (don't enter!), 1.5m for player
    float stopDist = targetingPlayer ? 1.5f : ((type == EnemyType::SHOOTER || type == EnemyType::BOSS) ? 40.0f : 28.0f);
    if (!targetingPlayer && moveDist < 28.0f) moveDist = 0; // Strict base boundary

    float dt = GetFrameTime();
    float timeScale = dt * 60.0f;

    isMoving = false;
    if (moveDist > stopDist) {
        isMoving = true;
        Vector3 moveDir = Vector3Normalize(moveDirection);
        angle = atan2f(direction.x, direction.z) * RAD2DEG; // Face target (player or base)
        position.x += moveDir.x * speed * timeScale;
        position.z += moveDir.z * speed * timeScale;
        velocity = {moveDir.x * speed * timeScale, 0, moveDir.z * speed * timeScale};
    } else {
        angle = atan2f(direction.x, direction.z) * RAD2DEG;
        velocity = {0,0,0};
    }

    // Attack logic
    if (attackTimer > 0) attackTimer -= GetFrameTime();

    if (attackTimer <= 0) {
        float attackRange = targetingPlayer ? 2.5f : ((type == EnemyType::SHOOTER || type == EnemyType::BOSS) ? 45.0f : 31.0f);
        
        if (dist < attackRange) {
            float damage = (weapon == WeaponType::KATANA) ? 60 : (weapon == WeaponType::MACHETE ? 40 : 20);
            if (type == EnemyType::BOSS) damage *= 3.0f;

            if (targetingPlayer && targetHp) {
                *targetHp -= (int)damage / 5; // Players take less damage than the base
            } else if (baseHp && *baseHp > 0) {
                *baseHp -= damage;
                if (*baseHp < 0) *baseHp = 0;
            }
            
            if (type == EnemyType::KAMIKAZE) {
                hp = 0; // Explode on hit
                active = false;
            }
            attackTimer = attackCooldown * 0.5f; 
        }
    }
}

#include "rlgl.h"

void Enemy::Draw() {
    if (!active) return;
    
    float scale = 1.2f; // Player size
    if (type == EnemyType::BOSS) scale = 2.0f;
    else if (type == EnemyType::MINION) scale = 0.7f;
    else if (type == EnemyType::KAMIKAZE) scale = 0.9f;
    else if (type == EnemyType::GIGA_TANK) scale = 2.5f;
    else if (type == EnemyType::ELITE) scale = 1.5f;
    float walkAnim = isMoving ? sinf(walkTimer * speed * 40.0f) : 0.0f;
    float attackAnim = (attackTimer > 0) ? (1.0f - attackTimer / (attackCooldown * 0.5f)) : 0.0f;
    
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(angle, 0, 1, 0);
    rlScalef(scale, scale, scale);

    Color skin = { 255, 200, 150, 255 };
    Color clothes = color;
    Color eyeColor = BLACK;

    // --- LEGS ---
    rlPushMatrix();
    rlTranslatef(-0.2f, 0.4f, 0);
    rlRotatef(walkAnim * 35.0f, 1, 0, 0);
    DrawCube((Vector3){0, 0, 0}, 0.2f, 0.8f, 0.2f, DARKGRAY);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.2f, 0.4f, 0);
    rlRotatef(-walkAnim * 35.0f, 1, 0, 0);
    DrawCube((Vector3){0, 0, 0}, 0.2f, 0.8f, 0.2f, DARKGRAY);
    rlPopMatrix();

    // --- TORSO ---
    rlPushMatrix();
    if (type == EnemyType::SHOOTER) rlRotatef(10, 1, 0, 0); // Tactical hunch
    DrawCube((Vector3){ 0, 1.2f, 0 }, 0.8f, 1.0f, 0.4f, clothes); // Larger torso
    DrawCube((Vector3){ 0, 1.7f, 0 }, 0.9f, 0.25f, 0.5f, clothes); // Shoulders
    rlPopMatrix();

    // --- ARMS & WEAPON ---
    if (type == EnemyType::SHOOTER) {
        // TACTICAL PISTOL STANCE (Both arms forward)
        // Right Arm
        rlPushMatrix();
        rlTranslatef(0.35f, 1.5f, 0.1f);
        rlRotatef(-80, 1, 0, 0); // Aiming forward
        rlRotatef(-15, 0, 1, 0); // Crossing in
        DrawCube((Vector3){0, 0, 0.3f}, 0.15f, 0.15f, 0.6f, skin);
        
        // Pistol
        rlPushMatrix();
        rlTranslatef(0, 0, 0.6f);
        DrawCube((Vector3){-0.1f, 0, 0.1f}, 0.1f, 0.15f, 0.4f, BLACK);
        rlPopMatrix();
        rlPopMatrix();

        // Left Arm supporting
        rlPushMatrix();
        rlTranslatef(-0.35f, 1.5f, 0.1f);
        rlRotatef(-75, 1, 0, 0);
        rlRotatef(25, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0.3f}, 0.15f, 0.15f, 0.6f, skin);
        rlPopMatrix();
    } else {
        // MELEE/TANK STANCE
        // Left Arm
        rlPushMatrix();
        rlTranslatef(-0.45f, 1.5f, 0);
        rlRotatef(-walkAnim * 45.0f, 1, 0, 0);
        rlTranslatef(0, -0.3f, 0);
        DrawCube((Vector3){0, 0, 0}, 0.18f, 0.7f, 0.18f, skin);
        rlPopMatrix();

        // Right Arm (Weapon)
        rlPushMatrix();
        rlTranslatef(0.45f, 1.5f, 0);
        if (attackAnim > 0) rlRotatef(attackAnim * -130.0f, 1, 0, 0);
        else rlRotatef(walkAnim * 45.0f, 1, 0, 0);
        rlTranslatef(0, -0.3f, 0);
        DrawCube((Vector3){0, 0, 0}, 0.18f, 0.7f, 0.18f, skin);

        // Weapon
        rlPushMatrix();
        rlTranslatef(0, -0.3f, 0.2f);
        if (weapon == WeaponType::KATANA) {
            rlRotatef(90, 1, 0, 0);
            DrawCube((Vector3){0, 0.8f, 0}, 0.05f, 1.8f, 0.05f, LIGHTGRAY);
            DrawCube((Vector3){0, 0, 0}, 0.15f, 0.05f, 0.15f, RED);
        } else if (weapon == WeaponType::MACHETE) {
            rlRotatef(90, 1, 0, 0);
            DrawCube((Vector3){0, 0.6f, 0}, 0.1f, 1.2f, 0.2f, GRAY);
        } else if (weapon == WeaponType::SHOTGUN) {
            rlRotatef(90, 1, 0, 0);
            DrawCube((Vector3){0, 0.5f, 0}, 0.15f, 1.0f, 0.15f, BLACK);
        } else if (weapon == WeaponType::EXPLOSIVE) {
            DrawSphere((Vector3){0, 0.3f, 0.2f}, 0.3f, RED);
        }
        rlPopMatrix();
        rlPopMatrix();
    }

    // --- HEAD ---
    rlPushMatrix();
    rlTranslatef(0, 1.9f, 0);
    DrawCube((Vector3){0, 0, 0}, 0.4f, 0.4f, 0.4f, skin);
    // Eyes
    DrawCube((Vector3){-0.12f, 0.1f, 0.2f}, 0.08f, 0.08f, 0.08f, eyeColor);
    DrawCube((Vector3){0.12f, 0.1f, 0.2f}, 0.08f, 0.08f, 0.08f, eyeColor);
    
    // Aesthetic variants per type
    if (type == EnemyType::TANK) DrawCube((Vector3){0, 0.22f, 0}, 0.45f, 0.2f, 0.45f, DARKGRAY); // Helmet
    else if (type == EnemyType::FAST) DrawCube((Vector3){0, 0.25f, 0}, 0.1f, 0.4f, 0.1f, YELLOW); // Mohawk
    else if (type == EnemyType::BOSS) DrawCube((Vector3){0, 0.3f, 0}, 0.5f, 0.6f, 0.5f, GOLD); // Crown
    rlPopMatrix();

    rlPopMatrix(); // Exit the body rotation/scaling group
}

void Enemy::DrawHUD(Camera3D camera) {
    if (!active || hp <= 0) return;

    // Show HP bar from a much further distance
    float viewDist = (type == EnemyType::BOSS) ? 400.0f : 150.0f;
    if (Vector3Distance(position, camera.position) > viewDist) return;

    float scale = 1.2f;
    if (type == EnemyType::BOSS) scale = 2.0f;
    else if (type == EnemyType::MINION) scale = 0.7f;
    else if (type == EnemyType::KAMIKAZE) scale = 0.9f;
    else if (type == EnemyType::GIGA_TANK) scale = 2.5f;
    else if (type == EnemyType::ELITE) scale = 1.5f;

    Vector3 barPos = { position.x, position.y + 2.5f * scale, position.z };
    
    // Prevent rendering HUD for enemies behind the camera
    Vector3 toEnemy = Vector3Subtract(barPos, camera.position);
    Vector3 camFwd = Vector3Subtract(camera.target, camera.position);
    if (Vector3DotProduct(toEnemy, camFwd) < 0) return;

    float hpPercent = std::max(0.0f, std::min(1.0f, (float)hp / maxHp));
    Vector2 screenPos = GetWorldToScreen(barPos, camera);
    
    // Smaller HP bar
    float bW = 25.0f * scale;
    float bH = 3.5f * scale;
    DrawRectangle((int)(screenPos.x - bW/2), (int)(screenPos.y - bH), (int)bW, (int)bH, BLACK);
    DrawRectangle((int)(screenPos.x - bW/2 + 1), (int)(screenPos.y - bH + 1), (int)((bW-2) * hpPercent), (int)(bH - 2), GREEN);
    
    const char* hpText = TextFormat("%d / %d", hp, maxHp);
    int fontSize = std::max(10, (int)(7 * scale));
    int textWidth = MeasureText(hpText, fontSize);
    DrawText(hpText, (int)(screenPos.x - textWidth/2), (int)(screenPos.y - bH - fontSize - 2), fontSize, RAYWHITE);
}

void Enemy::HandleCollision(BoundingBox box) {
    // Boss can step over small obstacles (like trash cans)
    if (type == EnemyType::BOSS && box.max.y < 3.0f) return;

    BoundingBox enemyBox = GetBoundingBox();
    if (CheckCollisionBoxes(enemyBox, box)) {
        float scale = 1.2f;
        if (type == EnemyType::BOSS) scale = 2.0f;
        else if (type == EnemyType::MINION) scale = 0.7f;
        else if (type == EnemyType::KAMIKAZE) scale = 0.9f;
        else if (type == EnemyType::GIGA_TANK) scale = 2.5f;
        else if (type == EnemyType::ELITE) scale = 1.5f;
        
        float pushDist = 0.72f * scale; // Adjust push distance based on enemy size

        float dx1 = enemyBox.max.x - box.min.x;
        float dx2 = box.max.x - enemyBox.min.x;
        float dz1 = enemyBox.max.z - box.min.z;
        float dz2 = box.max.z - enemyBox.min.z;
        
        float minOverlap = fmin(fmin(dx1, dx2), fmin(dz1, dz2));
        
        if (minOverlap == dx1) position.x = box.min.x - pushDist;
        else if (minOverlap == dx2) position.x = box.max.x + pushDist;
        else if (minOverlap == dz1) position.z = box.min.z - pushDist;
        else if (minOverlap == dz2) position.z = box.max.z + pushDist;
    }
}

BoundingBox Enemy::GetBoundingBox() {
    float scale = 1.2f;
    if (type == EnemyType::BOSS) scale = 2.0f;
    else if (type == EnemyType::MINION) scale = 0.7f;
    else if (type == EnemyType::KAMIKAZE) scale = 0.9f;
    else if (type == EnemyType::GIGA_TANK) scale = 2.5f;
    else if (type == EnemyType::ELITE) scale = 1.5f;
    
    float r = 0.75f * scale;
    float yMax = position.y + 2.45f * scale;
    yMax = std::max(yMax, position.y + 0.5f);
    
    return (BoundingBox){
        (Vector3){ position.x - r, position.y, position.z - r },
        (Vector3){ position.x + r, yMax, position.z + r }
    };
}

bool Enemy::RayHit(Ray ray, float& outDist) {
    RayCollision hit = GetRayCollisionBox(ray, GetBoundingBox());
    if (hit.hit) { outDist = hit.distance; return true; }
    return false;
}

void Enemy::TakeDamage(int amt) {
    hp -= amt;
}

void Enemy::TakeAOEDamage(Vector3 center, float radius, int maxDamage) {
    float dist = Vector3Distance(position, center);
    if (dist < radius) {
        float falloff = 1.0f - (dist / radius);
        TakeDamage((int)(maxDamage * (0.3f + 0.7f * falloff)));
    }
}

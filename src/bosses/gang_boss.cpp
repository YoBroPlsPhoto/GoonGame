#include "gang_boss.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

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

GangBoss::GangBoss(Vector3 spawnPos, int enemyId)
    : Enemy(spawnPos, EnemyType::GANG_BOSS, WeaponType::MACHETE, enemyId) {
    cutsceneState = GangCutscene::WARDROBE_FALLING;
    stateTimer = 0.0f;
    
    wardrobePos = spawnPos;
    wardrobePos.y = 400.0f; // Start high in sky
    wardrobeFallHeight = 400.0f;
    doorAngle = 0.0f;
    portalPulse = 0.0f;
    
    position = spawnPos;
    position.y = 0.0f;
    
    // Boss stats (aggregate - real HP is per member)
    hp = 125000;    // 5 * 25000
    maxHp = 125000;
    speed = 0.12f;
    radius = 5.0f;
    color = {50, 50, 50, 255};
    
    membersOut = 0;
    nextMemberTimer = 0.0f;
    
    // Initialize 5 gang members
    for (int i = 0; i < 5; i++) {
        members[i].position = spawnPos;
        members[i].position.y = 0.0f;
        members[i].angle = 0.0f;
        members[i].hp = 25000;
        members[i].maxHp = 25000;
        members[i].alive = true;
        members[i].attackTimer = 0.0f;
        members[i].walkTimer = 0.0f;
        members[i].isMoving = false;
        members[i].targetPlayerIdx = i % 2; // Alternate targets
        members[i].batSwingAngle = 0.0f;
        members[i].isSwinging = false;
        members[i].specialTimer = 5.0f + (float)(i * 2); // Stagger specials
    }
}

int GangBoss::GetTotalHP() const {
    int total = 0;
    for (const auto& m : members) {
        if (m.alive) total += m.hp;
    }
    return total;
}

int GangBoss::GetTotalMaxHP() const {
    return 125000;
}

bool GangBoss::AnyAlive() const {
    for (const auto& m : members) {
        if (m.alive && m.hp > 0) return true;
    }
    return false;
}

void GangBoss::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    
    float dt = GetFrameTime();
    stateTimer += dt;
    portalPulse += dt;
    
    // Update aggregate HP
    hp = GetTotalHP();
    if (!AnyAlive()) { active = false; return; }
    
    if (cutsceneState == GangCutscene::WARDROBE_FALLING) {
        // Wardrobe falls from sky
        float fallSpeed = 3.0f + stateTimer * 6.0f;
        wardrobePos.y -= fallSpeed * dt * 60.0f;
        
        // Wobble
        wardrobePos.x = position.x + sinf(stateTimer * 10.0f) * 1.5f;
        wardrobePos.z = position.z + cosf(stateTimer * 8.0f) * 1.5f;
        
        if (wardrobePos.y <= 0.0f) {
            wardrobePos.y = 0.0f;
            wardrobePos.x = position.x;
            wardrobePos.z = position.z;
            cutsceneState = GangCutscene::WARDROBE_LANDED;
            stateTimer = 0.0f;
        }
        
        isMoving = false;
        
    } else if (cutsceneState == GangCutscene::WARDROBE_LANDED) {
        // Dramatic pause then doors start opening
        isMoving = false;
        if (stateTimer > 2.0f) {
            cutsceneState = GangCutscene::PORTAL_OPENING;
            stateTimer = 0.0f;
        }
        
    } else if (cutsceneState == GangCutscene::PORTAL_OPENING) {
        // Doors swing open revealing portal
        doorAngle = std::min(110.0f, (stateTimer / 3.0f) * 110.0f);
        isMoving = false;
        
        if (stateTimer > 3.5f) {
            cutsceneState = GangCutscene::GUYS_WALKING_OUT;
            stateTimer = 0.0f;
            membersOut = 0;
            nextMemberTimer = 0.0f;
        }
        
    } else if (cutsceneState == GangCutscene::GUYS_WALKING_OUT) {
        doorAngle = 110.0f;
        
        // Pop out members one by one
        nextMemberTimer += dt;
        if (membersOut < 5 && nextMemberTimer > 1.2f) {
            // Position member at wardrobe exit and walk forward
            // INCREASED SPREAD to walk separately
            float spreadX = ((float)membersOut - 2.0f) * 12.0f; 
            members[membersOut].position = wardrobePos;
            members[membersOut].position.z += 2.0f; // Start just outside
            members[membersOut].position.x += spreadX;
            members[membersOut].isMoving = true;
            members[membersOut].angle = 0.0f; // Look forward
            membersOut++;
            nextMemberTimer = 0.0f;
        }
        
        // Move already-out members forward STRAIGHT and separately
        for (int i = 0; i < membersOut; i++) {
            float timeScale = dt * 60.0f;
            members[i].position.z += speed * 0.8f * timeScale; // Faster walk out
            members[i].walkTimer += dt;
        }
        
        if (membersOut >= 5 && stateTimer > 8.0f) {
            cutsceneState = GangCutscene::FIGHT;
            stateTimer = 0.0f;
        }
        
    } else if (cutsceneState == GangCutscene::FIGHT) {
        doorAngle = 110.0f;
        float timeScale = dt * 60.0f;
        
        // Each member acts independently
        for (int i = 0; i < 5; i++) {
            GangMember& m = members[i];
            if (!m.alive || m.hp <= 0) { m.alive = false; continue; }
            
            m.attackTimer -= dt;
            m.specialTimer -= dt;
            
            // Target the base exclusively
            Vector3 targetPos = basePos;
            bool targetingPlayer = false;
            
            Vector3 dir = Vector3Subtract(targetPos, m.position);
            dir.y = 0;
            float dist = Vector3Length(dir);
            
            // --- SEPARATION LOGIC (increased strength and distance) ---
            Vector3 separation = {0, 0, 0};
            for (int k = 0; k < 5; k++) {
                if (k == i || !members[k].alive) continue;
                float d = Vector3Distance(m.position, members[k].position);
                if (d < 7.0f) { // Increased min distance to 7.0 for big guys
                    Vector3 diff = Vector3Subtract(m.position, members[k].position);
                    diff.y = 0;
                    diff = Vector3Normalize(diff);
                    if (d < 0.1f) diff = (Vector3){(float)(i%2==0?1:-1), 0, 0};
                    float strength = (7.0f - d) * 1.8f; // Even stronger push
                    separation.x += diff.x * strength;
                    separation.z += diff.z * strength;
                }
            }
            
            float stopDist = 27.0f; // Distance from center of map (base)
            
            m.isMoving = false;
            // First 3 seconds of FIGHT: walk straight forward from wardrobe
            if (stateTimer < 3.0f) {
                m.isMoving = true;
                m.angle = 0.0f;
                m.position.z += speed * timeScale;
                m.position.x += separation.x * speed * timeScale; 
                m.walkTimer += dt;
            } else if (dist > stopDist) {
                m.isMoving = true;
                Vector3 moveDir = Vector3Normalize(dir);
                // Apply separation to moveDir
                moveDir.x += separation.x * 0.5f;
                moveDir.z += separation.z * 0.5f;
                moveDir = Vector3Normalize(moveDir);
                
                m.angle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
                m.position.x += moveDir.x * speed * timeScale;
                m.position.z += moveDir.z * speed * timeScale;
                m.walkTimer += dt;
            } else {
                m.angle = atan2f(dir.x, dir.z) * RAD2DEG;
                // Maintain personal space
                m.position.x += separation.x * speed * timeScale;
                m.position.z += separation.z * speed * timeScale;
            }
            
            // --- ATTACK LOGIC ---
            float closestPlayerDist = 999.0f;
            int* nearestHp = nullptr;
            bool atBaseEdge = DistanceToBaseFootprint(m.position, basePos) <= BASE_EDGE_AGGRO_DISTANCE;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (atBaseEdge && !p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
                float d = Vector3Distance(p.pos, m.position);
                if (d < closestPlayerDist) {
                    closestPlayerDist = d;
                    nearestHp = p.hp;
                }
            }
            
            if (m.attackTimer <= 0) {
                if (closestPlayerDist < 6.0f && nearestHp) {
                    // "Drive-by" melee attack on player
                    m.isSwinging = true;
                    m.batSwingAngle = 0.0f;
                    m.attackTimer = 1.0f; // Faster swing against players
                    *nearestHp -= 35;     // Damage to player
                } else {
                    // Check distance to base AABB (base is 50x50, so half-size is 25)
                    float dx = fmaxf(0.0f, fabsf(m.position.x - basePos.x) - 25.0f);
                    float dz = fmaxf(0.0f, fabsf(m.position.z - basePos.z) - 25.0f);
                    float distToBaseAABB = sqrtf(dx * dx + dz * dz);
                    
                    if (distToBaseAABB < 4.0f) {
                        // Melee attack on base
                        m.isSwinging = true;
                        m.batSwingAngle = 0.0f;
                        m.attackTimer = 1.5f;
                        
                        if (baseHp && *baseHp > 0) {
                            *baseHp -= 150.0f; // Stronger base damage
                            if (*baseHp < 0) *baseHp = 0;
                        }
                    }
                }
            }
            
            // Animate bat swing
            if (m.isSwinging) {
                m.batSwingAngle += dt * 400.0f;
                if (m.batSwingAngle > 180.0f) {
                    m.isSwinging = false;
                    m.batSwingAngle = 0.0f;
                }
            }
            
            // --- SPECIAL ATTACK: ground pound splash ---
            if (m.specialTimer <= 0 && dist < 32.0f) {
                m.specialTimer = 8.0f + (float)(rand() % 4);
                
                // AOE damage to players (collateral)
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0)) continue;
                    if (atBaseEdge && !p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
                    float d = Vector3Distance(p.pos, m.position);
                    if (d < 12.0f && p.hp) {
                        float falloff = 1.0f - (d / 12.0f);
                        *p.hp -= (int)(40 * falloff);
                    }
                }
                
                // AOE damage to base
                if (dist < 32.0f && baseHp) {
                    *baseHp -= 500.0f; // Big ground pound damage to building
                    if (*baseHp < 0) *baseHp = 0;
                }
            } // Close special attack if
        } // Close for loop over members
        
        // Update aggregate position to center of living members (for camera/HUD)
        Vector3 center = {0, 0, 0};
        int aliveCount = 0;
        for (const auto& m : members) {
            if (m.alive) {
                center.x += m.position.x;
                center.z += m.position.z;
                aliveCount++;
            }
        }
        if (aliveCount > 0) {
            position.x = center.x / (float)aliveCount;
            position.z = center.z / (float)aliveCount;
        }
    }
}

// Helper: Draw one gang member model
static void DrawGangMember(const GangMember& m, float pulseTime) {
    if (!m.alive) return;
    
    float scale = 4.0f; // Big thugs
    float animWalk = m.isMoving ? sinf(m.walkTimer * 8.0f) : 0.0f;
    
    rlPushMatrix();
    rlTranslatef(m.position.x, m.position.y, m.position.z);
    rlRotatef(m.angle, 0, 1, 0);
    rlScalef(scale, scale, scale);
    
    Color skin = {60, 40, 30, 255};
    Color pants = {30, 30, 40, 255};
    Color shirt = {20, 20, 20, 255};
    
    // --- LEGS ---
    rlPushMatrix();
    rlTranslatef(-0.15f, 0.35f, 0);
    rlRotatef(animWalk * 30.0f, 1, 0, 0);
    DrawCube({0, 0, 0}, 0.2f, 0.7f, 0.2f, pants);
    // Shoes
    DrawCube({0, -0.35f, 0.05f}, 0.22f, 0.1f, 0.3f, {40, 40, 40, 255});
    rlPopMatrix();
    
    rlPushMatrix();
    rlTranslatef(0.15f, 0.35f, 0);
    rlRotatef(-animWalk * 30.0f, 1, 0, 0);
    DrawCube({0, 0, 0}, 0.2f, 0.7f, 0.2f, pants);
    DrawCube({0, -0.35f, 0.05f}, 0.22f, 0.1f, 0.3f, {40, 40, 40, 255});
    rlPopMatrix();
    
    // --- TORSO (muscular) ---
    DrawCube({0, 1.1f, 0}, 0.7f, 0.75f, 0.4f, shirt);
    // Wide shoulders
    DrawCube({0, 1.5f, 0}, 0.9f, 0.2f, 0.45f, shirt);
    
    // --- LEFT ARM ---
    rlPushMatrix();
    rlTranslatef(-0.45f, 1.3f, 0);
    rlRotatef(-animWalk * 35.0f, 1, 0, 0);
    DrawCube({0, -0.3f, 0}, 0.2f, 0.6f, 0.2f, skin);
    // Fist
    DrawCube({0, -0.65f, 0}, 0.15f, 0.15f, 0.15f, skin);
    rlPopMatrix();
    
    // --- RIGHT ARM (with bat) ---
    rlPushMatrix();
    rlTranslatef(0.45f, 1.3f, 0);
    if (m.isSwinging) {
        rlRotatef(-m.batSwingAngle, 1, 0, 0);
    } else {
        rlRotatef(animWalk * 35.0f, 1, 0, 0);
    }
    DrawCube({0, -0.3f, 0}, 0.2f, 0.6f, 0.2f, skin);
    
    // Baseball bat
    rlPushMatrix();
    rlTranslatef(0, -0.6f, 0.15f);
    rlRotatef(70, 1, 0, 0);
    DrawCube({0, 0.5f, 0}, 0.08f, 1.2f, 0.08f, BROWN);       // Handle
    DrawCube({0, 1.15f, 0}, 0.14f, 0.3f, 0.14f, DARKBROWN);   // Bat head
    rlPopMatrix();
    rlPopMatrix();
    
    // --- HEAD ---
    rlPushMatrix();
    rlTranslatef(0, 1.85f, 0);
    DrawCube({0, 0, 0}, 0.35f, 0.35f, 0.35f, skin);
    // Sunglasses
    DrawCube({-0.1f, 0.05f, 0.18f}, 0.12f, 0.06f, 0.02f, BLACK);
    DrawCube({0.1f, 0.05f, 0.18f}, 0.12f, 0.06f, 0.02f, BLACK);
    DrawCube({0, 0.05f, 0.18f}, 0.05f, 0.02f, 0.02f, BLACK); // Bridge
    // Gold chain highlight
    DrawCube({0, -0.08f, 0.15f}, 0.04f, 0.04f, 0.04f, GOLD);
    rlPopMatrix();
    
    // Gold chain on chest
    for (int c = 0; c < 5; c++) {
        float ca = (float)c / 5.0f * 0.6f - 0.15f;
        DrawCube({sinf(ca) * 0.15f, 1.35f - ca * 0.5f, 0.21f}, 0.03f, 0.03f, 0.03f, GOLD);
    }
    
    rlPopMatrix();
    
    // Shadow
    DrawCircle3D({m.position.x, 0.05f, m.position.z}, 2.0f, {1, 0, 0}, 90.0f, Fade(BLACK, 0.4f));
}

void GangBoss::Draw() {
    if (!active) return;
    
    // --- DRAW WARDROBE (always, it stays on the field) ---
    if (cutsceneState != GangCutscene::FIGHT || true) {
        rlPushMatrix();
        rlTranslatef(wardrobePos.x, wardrobePos.y, wardrobePos.z);
        rlRotatef(180, 0, 1, 0);
        
        // Wardrobe structure (bigger than Adas Gooner's)
        DrawCube({0, 10.0f, 3.5f}, 14.0f, 20.0f, 1.0f, {30, 10, 40, 255}); // Back - dark purple
        DrawCube({-7.0f, 10.0f, 0}, 1.0f, 20.0f, 7.0f, {30, 10, 40, 255}); // Left
        DrawCube({7.0f, 10.0f, 0}, 1.0f, 20.0f, 7.0f, {30, 10, 40, 255});  // Right
        DrawCube({0, 20.5f, 0}, 15.0f, 1.0f, 7.0f, {20, 5, 30, 255});     // Top
        DrawCube({0, 0.5f, 0}, 15.0f, 1.0f, 7.0f, {20, 5, 30, 255});      // Base
        
        // Portal glow inside (always active after landing)
        if (cutsceneState != GangCutscene::WARDROBE_FALLING) {
            float glow = 0.5f + sinf(portalPulse * 3.0f) * 0.3f;
            DrawCube({0, 10.0f, 2.5f}, 10.0f, 16.0f, 0.5f, Fade({150, 50, 200, 255}, glow));
            // Swirl effect
            for (int s = 0; s < 6; s++) {
                float sa = portalPulse * 2.0f + (float)s * (PI * 2.0f / 6.0f);
                float sr = 3.0f + sinf(portalPulse + (float)s) * 1.5f;
                DrawSphere({cosf(sa) * sr, 10.0f + sinf(sa) * sr, 2.0f}, 0.6f, 
                          Fade({200, 100, 255, 255}, 0.6f));
            }
        }
        
        // Doors
        float currentDoorAngle = doorAngle;
        if (cutsceneState == GangCutscene::WARDROBE_FALLING) currentDoorAngle = 0;
        
        // Left door
        rlPushMatrix();
        rlTranslatef(-6.5f, 10.0f, -3.5f);
        rlRotatef(currentDoorAngle, 0, 1, 0);
        DrawCube({3.25f, 0, 0}, 6.5f, 19.0f, 0.5f, {40, 15, 50, 255});
        // Door handle
        DrawCube({5.5f, 0, -0.3f}, 0.3f, 0.6f, 0.3f, GOLD);
        rlPopMatrix();
        
        // Right door
        rlPushMatrix();
        rlTranslatef(6.5f, 10.0f, -3.5f);
        rlRotatef(-currentDoorAngle, 0, 1, 0);
        DrawCube({-3.25f, 0, 0}, 6.5f, 19.0f, 0.5f, {40, 15, 50, 255});
        DrawCube({-5.5f, 0, -0.3f}, 0.3f, 0.6f, 0.3f, GOLD);
        rlPopMatrix();
        
        // Text sign on top
        if (wardrobePos.y < 1.0f) {
            // "GANGBANG" sign won't render as text in 3D - instead a red glowing cube
            DrawCube({0, 21.5f, 0}, 10.0f, 1.5f, 2.0f, {180, 20, 20, 255});
        }
        
        rlPopMatrix();
        
        // Shadow while falling
        if (cutsceneState == GangCutscene::WARDROBE_FALLING) {
            float shadowAlpha = (400.0f - wardrobePos.y) / 400.0f;
            DrawCircle3D({position.x, 0.05f, position.z}, 10.0f * shadowAlpha, {1, 0, 0}, 90.0f, 
                        Fade(BLACK, 0.3f + shadowAlpha * 0.4f));
        }
    }
    
    // --- DRAW GANG MEMBERS ---
    for (int i = 0; i < 5; i++) {
        if (i < membersOut || cutsceneState == GangCutscene::FIGHT) {
            DrawGangMember(members[i], portalPulse);
        }
    }
    
    // Impact effect on landing
    if (cutsceneState == GangCutscene::WARDROBE_LANDED && stateTimer < 1.5f) {
        float shockRadius = stateTimer * 30.0f;
        float alpha = 1.0f - stateTimer / 1.5f;
        DrawCircle3D({wardrobePos.x, 0.1f, wardrobePos.z}, shockRadius, {1, 0, 0}, 90.0f,
                     Fade({150, 50, 200, 255}, alpha * 0.6f));
    }
    
    // Special attack visual (ground smash shockwave per member)
    for (int i = 0; i < 5; i++) {
        if (members[i].alive && members[i].specialTimer > 7.0f) {
            float t = 8.0f - members[i].specialTimer; // 0..1 range
            if (t > 0 && t < 1.0f) {
                DrawCircle3D({members[i].position.x, 0.1f, members[i].position.z}, 
                            t * 10.0f, {1, 0, 0}, 90.0f, Fade(RED, (1.0f - t) * 0.5f));
            }
        }
    }
}

void GangBoss::DrawHUD(Camera3D camera) {
    if (!active || !AnyAlive()) return;
    
    // Individual HP bars for each member
    for (int i = 0; i < 5; i++) {
        const GangMember& m = members[i];
        if (!m.alive || m.hp <= 0) continue;
        
        float viewDist = 200.0f;
        if (Vector3Distance(m.position, camera.position) > viewDist) continue;
        
        Vector3 barPos = {m.position.x, m.position.y + 20.0f, m.position.z};
        
        Vector3 toEnemy = Vector3Subtract(barPos, camera.position);
        Vector3 camFwd = Vector3Subtract(camera.target, camera.position);
        if (Vector3DotProduct(toEnemy, camFwd) < 0) continue;
        
        float hpPercent = std::max(0.0f, std::min(1.0f, (float)m.hp / (float)m.maxHp));
        Vector2 screenPos = GetWorldToScreen(barPos, camera);
        
        float bW = 120.0f;
        float bH = 12.0f;
        
        DrawRectangle((int)(screenPos.x - bW / 2.0f) - 1, (int)(screenPos.y - bH) - 1, (int)bW + 2, (int)bH + 2, BLACK);
        DrawRectangle((int)(screenPos.x - bW / 2.0f), (int)(screenPos.y - bH), (int)bW, (int)bH, DARKGRAY);
        DrawRectangle((int)(screenPos.x - bW / 2.0f), (int)(screenPos.y - bH), (int)(bW * hpPercent), (int)bH, RED);
        
        const char* name = TextFormat("TYP #%d", i + 1);
        int nameW = MeasureText(name, 18);
        DrawText(name, (int)(screenPos.x - nameW / 2.0f), (int)(screenPos.y - bH - 22), 18, RAYWHITE);
        
        const char* hpText = TextFormat("%d", m.hp);
        int textW = MeasureText(hpText, 10);
        DrawText(hpText, (int)(screenPos.x - textW / 2.0f), (int)(screenPos.y - bH + 2), 10, WHITE);
    }
}

BoundingBox GangBoss::GetBoundingBox() {
    // Use the wardrobe position for collision during cutscene, then spread area during fight
    float halfWidth = 15.0f;
    float height = 20.0f;
    return (BoundingBox){
        (Vector3){ wardrobePos.x - halfWidth, wardrobePos.y, wardrobePos.z - halfWidth },
        (Vector3){ wardrobePos.x + halfWidth, wardrobePos.y + height, wardrobePos.z + halfWidth }
    };
}

void GangBoss::HandleCollision(BoundingBox box) {
    if (cutsceneState != GangCutscene::FIGHT) return; // Wardrobe doesn't collide with items normally
    
    for (int i = 0; i < 5; i++) {
        if (!members[i].alive) continue;
        
        // Member radius for collision approx 1.5f
        float mRadius = 1.5f;
        Vector3 p = members[i].position;
        
        // Basic AABB vs Cylinder approximation
        if (p.x + mRadius > box.min.x && p.x - mRadius < box.max.x &&
            p.z + mRadius > box.min.z && p.z - mRadius < box.max.z &&
            p.y + 7.5f > box.min.y && p.y < box.max.y) 
        {
            // Push out of box
            float dx1 = p.x - box.min.x;
            float dx2 = box.max.x - p.x;
            float dz1 = p.z - box.min.z;
            float dz2 = box.max.z - p.z;
            
            float minDist = std::min({dx1, dx2, dz1, dz2});
            if (minDist == dx1) members[i].position.x = box.min.x - mRadius;
            else if (minDist == dx2) members[i].position.x = box.max.x + mRadius;
            else if (minDist == dz1) members[i].position.z = box.min.z - mRadius;
            else if (minDist == dz2) members[i].position.z = box.max.z + mRadius;
        }
    }
}

bool GangBoss::RayHit(Ray ray, float& outDist) {
    if (!active) return false;
    bool hit = false;
    outDist = 9999.0f;
    
    // During cutscene, hit wardrobe
    if (cutsceneState != GangCutscene::FIGHT) {
        RayCollision c = GetRayCollisionBox(ray, GetBoundingBox());
        if (c.hit && c.distance < outDist) {
            outDist = c.distance;
            hit = true;
            lastHitMemberIdx = -1;
        }
        return hit;
    }
    
    for (int i = 0; i < 5; i++) {
        if (!members[i].alive || members[i].hp <= 0) continue;
        BoundingBox mbrBox = {
            {members[i].position.x - 2.0f, members[i].position.y, members[i].position.z - 2.0f},
            {members[i].position.x + 2.0f, members[i].position.y + 7.5f, members[i].position.z + 2.0f}
        };
        RayCollision c = GetRayCollisionBox(ray, mbrBox);
        if (c.hit && c.distance < outDist) {
            outDist = c.distance;
            hit = true;
            lastHitMemberIdx = i; 
        }
    }
    return hit;
}

void GangBoss::TakeDamage(int amt) {
    if (lastHitMemberIdx >= 0 && lastHitMemberIdx < 5) {
        members[lastHitMemberIdx].hp -= amt;
        if (members[lastHitMemberIdx].hp < 0) members[lastHitMemberIdx].hp = 0;
        if (members[lastHitMemberIdx].hp == 0) members[lastHitMemberIdx].alive = false;
        lastHitMemberIdx = -1;
    }
    hp = GetTotalHP();
}

void GangBoss::TakeAOEDamage(Vector3 center, float splashRadius, int maxDamage) {
    for (int i = 0; i < 5; i++) {
        if (!members[i].alive || members[i].hp <= 0) continue;
        float d = Vector3Distance(members[i].position, center);
        if (d < splashRadius) {
            float f = 1.0f - (d / splashRadius);
            members[i].hp -= (int)(maxDamage * (0.3f + 0.7f * f));
            if (members[i].hp < 0) members[i].hp = 0;
            if (members[i].hp == 0) members[i].alive = false;
        }
    }
    hp = GetTotalHP();
}

#include "gibon_rzygacz.hpp"
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
    
    // Jump attack
    jumpCooldown = 5.0f + (float)(rand() % 1000) / 100.0f; // 5 to 15 seconds
    jumpStartPos = position;
    
    // Expanding shockwave attack
    shockwaveActive = false;
    shockwavePos = {0, 0, 0};
    shockwaveRadius = 0.0f;
    shockwaveMaxRadius = 60.0f;
    shockwaveSpeed = 40.0f;
    
    vomitCooldown = 0.0f;
    vomitOrbTriggered = false;
    lastRayHitVomitOrb = false;
    lastRayHitVomitShield = false;
    vomitOrbHp = 0;
    vomitOrbMaxHp = 10000;
    vomitOrbPosition = position;
    
    // Vomit orb state machine
    vomitOrbState = VomitOrbState::INACTIVE;
    vomitOrbChargeProgress = 0.0f;
    vomitOrbChargeTime = 5.0f;
    vomitOrbReadyTimer = 0.0f;
    vomitOrbExplosionTimer = 0.0f;
    vomitOrbFlyTimer = 0.0f;
    vomitOrbStartPos = {0, 0, 0};
    vomitOrbTargetPos = {0, 0, 0};
    vomitOrbVelocity = {0, 0, 0};
    vomitOrbGibonExpTimer = 0.0f;
    vomitOrbGibonExploding = false;
    
    stutterTimer = 0.0f;
    nextStutterTime = 0.5f + (float)(rand() % 200) / 100.0f;
    isStuttering = false;
    
    fpsLagAttackCooldown = 15.0f;
    fpsLagAttackTimer = 0.0f;
    isCastingFpsLag = false;
    fpsLagEffectTimer = 0.0f;
    fpsLagWaveRadius = 0.0f;
    
    directVomitCooldown = 3.0f;
    directVomitTimer = 0.0f;
    isDirectVomiting = false;
    directVomitSpawnTimer = 0.0f;
    directVomitTarget = {0, 0, 0};
    
    craterCreated = false;
    impactCrater = {{0, 0, 0}, 0};
    
    pulseTimer = 0.0f;
    bodyScale = 8.0f;
    toxicColor = {80, 200, 50, 255};
    
    jumpState = GibonJumpState::NONE;
    jumpAttackCooldown = 8.0f;
    jumpAttackTimer = 0.0f;
    jumpStartPos = {0, 0, 0};
    jumpTargetPos = {0, 0, 0};
}


void GibonRzygacz::Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) {
    if (!active) return;
    if (hp <= 0) { active = false; return; }

    float dt = GetFrameTime();
    stateTimer += dt;
    pulseTimer += dt;
    lastRayHitVomitOrb = false;
    
    if (fpsLagEffectTimer > 0.0f) {
        fpsLagEffectTimer -= dt;
        if (fpsLagEffectTimer < 0.0f) fpsLagEffectTimer = 0.0f;
        fpsLagWaveRadius += dt * 160.0f;
    } else if (!isCastingFpsLag) {
        fpsLagWaveRadius = 0.0f;
    }
    
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
        
    } else if (gibonState == GibonState::JUMPING) {
        // Parabolic jump towards landingTarget
        float jumpTime = 1.5f; // Jump takes 1.5 seconds
        float t = stateTimer / jumpTime;
        
        if (t >= 1.0f) {
            // Landed
            position.y = 0.5f;
            position.x = landingTarget.x;
            position.z = landingTarget.z;
            
            // Skip the cutscene pause (IMPACT), go straight back to combat!
            gibonState = GibonState::FINISHED_FALLING;
            stateTimer = 0.0f;
            
            craterCreated = true;
            impactCrater.position = landingTarget;
            impactCrater.position.y = 0.01f;
            impactCrater.radius = 20.0f;
            
            // Trigger traveling shockwave instead of instant damage
            shockwaveActive = true;
            shockwavePos = landingTarget;
            shockwavePos.y = 0.1f;
            shockwaveRadius = 0.0f;
            
            isMoving = false;
            walkTimer = 0;
            rollSpeed = 0.0f;
        } else {
            // Lerp x/z
            position.x = jumpStartPos.x + (landingTarget.x - jumpStartPos.x) * t;
            position.z = jumpStartPos.z + (landingTarget.z - jumpStartPos.z) * t;
            
            // Parabola for y
            float jumpHeight = 40.0f; // jump up 40 units
            position.y = jumpStartPos.y + 4.0f * jumpHeight * t * (1.0f - t);
            
            // Spin mid-air
            rollAngle += dt * 500.0f;
            
            isMoving = true;
            walkTimer += dt;
        }
        
    } else if (gibonState == GibonState::FINISHED_FALLING) {
        // --- NORMAL COMBAT BEHAVIOR ---
        float timeScale = dt * 60.0f;
        
        // Find nearest player (for attack targeting only, NOT for movement)
        Vector3 attackTargetPos = basePos;
        int* targetHp = nullptr;
        bool foundPlayer = false;
        bool atBaseEdge = DistanceToBaseFootprint(position, basePos) <= BASE_EDGE_AGGRO_DISTANCE;
        
        float closestDist = 1000.0f;
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            if (atBaseEdge && !p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
            float d = Vector3Distance(p.pos, position);
            if (d < 40.0f && d < closestDist) {
                closestDist = d;
                attackTargetPos = p.pos;
                targetHp = p.hp;
                foundPlayer = true;
            }
        }
        
        // MOVEMENT: Always go straight to the base
        Vector3 moveDirection = Vector3Subtract(basePos, position);
        moveDirection.y = 0;
        float distToBase = Vector3Length(moveDirection);
        
        // Calculate distance to base edge (AABB)
        float dx = fmaxf(0.0f, fabsf(position.x - basePos.x) - BASE_HALF_SIZE);
        float dz = fmaxf(0.0f, fabsf(position.z - basePos.z) - BASE_HALF_SIZE);
        float distToBaseEdge = sqrtf(dx * dx + dz * dz);
        
        float stopDist = 3.0f; // Stop right at the base edge
        
        bool orbAttackActive = vomitOrbState == VomitOrbState::CHARGING ||
                               vomitOrbState == VomitOrbState::READY ||
                               vomitOrbState == VomitOrbState::FLYING ||
                               vomitOrbState == VomitOrbState::EXPLODED_BASE;
        bool isJumpAttacking = (jumpState != GibonJumpState::NONE);
        bool attackInProgress = isDirectVomiting || isJumpAttacking || orbAttackActive ||
                                isCastingFpsLag || fpsLagEffectTimer > 0.0f;

        // Check if orb is in a state that forces gibon to stand still
        bool orbForcesStop = (vomitOrbState == VomitOrbState::CHARGING || 
                              vomitOrbState == VomitOrbState::READY);
        
        // --- JUMP ATTACK TRIGGER ---
        if (!orbForcesStop && foundPlayer) {
            jumpCooldown -= dt;
            if (jumpCooldown <= 0.0f) {
                gibonState = GibonState::JUMPING;
                stateTimer = 0.0f;
                jumpStartPos = position;
                landingTarget = attackTargetPos; // Jump to the nearest player
                // Random time between 5 and 15 seconds (max 15s)
                jumpCooldown = 5.0f + (float)(rand() % 1000) / 100.0f;
                return; // Stop processing this frame to start jump immediately
            }
        }
        
        isMoving = false;
        if (distToBaseEdge > stopDist && !attackInProgress) {
            isMoving = true;
            Vector3 moveDir = Vector3Normalize(moveDirection);
            angle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
            
            rollSpeed = speed;
            position.x += moveDir.x * rollSpeed * timeScale;
            position.z += moveDir.z * rollSpeed * timeScale;
            
            // Rolling animation
            rollAngle += rollSpeed * timeScale * 20.0f;
        } else {
            rollSpeed *= 0.95f;
            // Face nearest player when stopped at base
            if (foundPlayer && !isJumpAttacking) {
                Vector3 faceDir = Vector3Subtract(attackTargetPos, position);
                faceDir.y = 0;
                angle = atan2f(faceDir.x, faceDir.z) * RAD2DEG;
            }
        }
        
        if (isMoving) walkTimer += dt;
        else walkTimer = 0;

        if (!isDirectVomiting) directVomitCooldown -= dt;
        if (jumpState == GibonJumpState::NONE) jumpAttackCooldown -= dt;
        if (!isCastingFpsLag && fpsLagEffectTimer <= 0.0f) fpsLagAttackCooldown -= dt;

        // --- VOMIT ORB TRIGGER ---
        if (!vomitOrbTriggered && hp <= maxHp / 2 && !attackInProgress) {
            vomitOrbTriggered = true;
            vomitOrbState = VomitOrbState::CHARGING;
            vomitOrbChargeProgress = 0.0f;
            vomitOrbHp = vomitOrbMaxHp;
            vomitOrbReadyTimer = 25.0f;
            attackInProgress = true;
        }

        // --- 5 FPS LAG ATTACK ---
        if (!attackInProgress && fpsLagAttackCooldown <= 0.0f) {
            isCastingFpsLag = true;
            fpsLagAttackTimer = 0.0f;
            fpsLagWaveRadius = 0.0f;
            attackInProgress = true;
        }

        if (isCastingFpsLag) {
            isMoving = false;
            rollSpeed = 0.0f;
            fpsLagAttackTimer += dt;
            if (fpsLagAttackTimer >= 1.5f) {
                isCastingFpsLag = false;
                fpsLagEffectTimer = 5.0f;
                fpsLagWaveRadius = 1.0f;
                fpsLagAttackCooldown = 15.0f + (float)(rand() % 8);
            }
        }

        attackInProgress = isDirectVomiting || jumpState != GibonJumpState::NONE ||
                           vomitOrbState == VomitOrbState::CHARGING ||
                           vomitOrbState == VomitOrbState::READY ||
                           vomitOrbState == VomitOrbState::FLYING ||
                           vomitOrbState == VomitOrbState::EXPLODED_BASE ||
                           isCastingFpsLag || fpsLagEffectTimer > 0.0f;

        // --- TARGETED EXPLOSIVE VOMIT BALL SPAM ---
        if (!attackInProgress && directVomitCooldown <= 0.0f) {
            // Find nearest player strictly OUTSIDE the base
            float closestOutDist = 1000.0f;
            bool foundOutPlayer = false;
            Vector3 targetPosOut = {0, 0, 0};
            
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (p.isStructure) continue;
                if (IsInsideBaseFootprint(p.pos, basePos)) continue; // DO NOT TARGET PLAYERS IN BASE
                
                float d = Vector3Distance(p.pos, position);
                if (d < 120.0f && d < closestOutDist) {
                    closestOutDist = d;
                    targetPosOut = p.pos;
                    foundOutPlayer = true;
                }
            }
            
            if (foundOutPlayer) {
                isDirectVomiting = true;
                directVomitTimer = 2.4f;
                directVomitSpawnTimer = 0.0f;
                directVomitTarget = targetPosOut;
                directVomitCooldown = 4.0f + (float)(rand() % 3);
                attackInProgress = true;
            } else {
                directVomitCooldown = 0.5f;
            }
        }
        
        if (isDirectVomiting) {
            isMoving = false;
            rollSpeed = 0.0f;
            directVomitTimer -= dt;
            directVomitSpawnTimer -= dt;
            
            // TRACK the player continuously - re-find nearest player outside base
            float closestTrackDist = 1000.0f;
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (p.isStructure) continue;
                if (IsInsideBaseFootprint(p.pos, basePos)) continue;
                float d = Vector3Distance(p.pos, position);
                if (d < 120.0f && d < closestTrackDist) {
                    closestTrackDist = d;
                    directVomitTarget = p.pos;
                }
            }
            
            // Face target smoothly
            Vector3 faceDir = Vector3Subtract(directVomitTarget, position);
            faceDir.y = 0;
            if (Vector3Length(faceDir) > 0.1f) {
                angle = atan2f(faceDir.x, faceDir.z) * RAD2DEG;
            }
            
            // Big explosive balls aimed at the tracked player.
            if (directVomitSpawnTimer <= 0.0f) {
                directVomitSpawnTimer = 0.55f;
                
                int blobs = 1;
                for (int b = 0; b < blobs; b++) {
                    Vector3 mouthPos = position;
                    Vector3 fwd = {sinf(angle * DEG2RAD), 0.0f, cosf(angle * DEG2RAD)};
                    mouthPos = Vector3Add(mouthPos, Vector3Scale(fwd, bodyScale * 0.95f));
                    mouthPos.y += bodyScale * 0.6f;
                    
                    Vector3 aimTarget = directVomitTarget;
                    aimTarget.y += 1.0f;
                    aimTarget.x += (float)(rand() % 11 - 5);
                    aimTarget.z += (float)(rand() % 11 - 5);
                    
                    // Clamp out of base
                    if (IsInsideBaseFootprint(aimTarget, basePos)) {
                        if (aimTarget.x > basePos.x) aimTarget.x = basePos.x + BASE_HALF_SIZE + 2.0f;
                        else aimTarget.x = basePos.x - BASE_HALF_SIZE - 2.0f;
                    }
                    
                    Vector3 toTgt = Vector3Subtract(aimTarget, mouthPos);
                    float dist = Vector3Length(toTgt);
                    if (dist < 1.0f) dist = 1.0f;
                    
                    float projectileSpeed = 42.0f + (float)(rand() % 12);
                    Vector3 vel = Vector3Scale(Vector3Normalize(toTgt), projectileSpeed);
                    vel.y += 5.0f + (float)(rand() % 5);
                    
                    ToxicVomit v;
                    v.position = mouthPos;
                    v.velocity = vel;
                    v.radius = 2.2f;
                    v.lifetime = dist / projectileSpeed + 1.8f;
                    v.active = true;
                    vomitProjectiles.push_back(v);
                }
            }
            
            if (directVomitTimer <= 0.0f) {
                isDirectVomiting = false;
            }
        }

        // --- JUMP ATTACK LOGIC ---
        if (!attackInProgress && jumpState == GibonJumpState::NONE) {
            if (jumpAttackCooldown <= 0.0f) {
                jumpState = GibonJumpState::WINDUP;
                jumpAttackTimer = 0.0f;
                jumpAttackCooldown = 12.0f + (float)(rand() % 8); // 12-20s cooldown
                jumpTargetPos = position; // Jump in place
                jumpStartPos = position;
                attackInProgress = true;
            }
        } else if (jumpState == GibonJumpState::WINDUP) {
            jumpAttackTimer += dt;
            isMoving = false;
            rollSpeed = 0.0f;
            if (jumpAttackTimer >= 1.0f) {
                jumpState = GibonJumpState::AIR;
                jumpAttackTimer = 0.0f;
            }
        } else if (jumpState == GibonJumpState::AIR) {
            jumpAttackTimer += dt;
            float flightTime = 3.0f; // Slower jump
            float t = jumpAttackTimer / flightTime;
            if (t >= 1.0f) {
                t = 1.0f;
                jumpState = GibonJumpState::IMPACT;
                jumpAttackTimer = 0.0f;
                position.y = 0.5f;
                
                // CREATE SHOCKWAVE
                for (const auto& p : players) {
                    if (!p.active || (p.hp && *p.hp <= 0)) continue;
                    float d = Vector3Distance(p.pos, position);
                    if (d < 45.0f && p.hp) {
                        *p.hp -= 250; // Big shockwave damage
                    }
                }
            } else {
                float arcH = 200.0f; // Higher jump
                float arc = 4.0f * arcH * t * (1.0f - t);
                position.y = 0.5f + arc;
            }
        } else if (jumpState == GibonJumpState::IMPACT) {
            jumpAttackTimer += dt;
            isMoving = false;
            rollSpeed = 0.0f;
            if (jumpAttackTimer >= 3.0f) {
                jumpState = GibonJumpState::NONE;
                jumpAttackTimer = 0.0f;
            }
        }

        // --- VOMIT ORB STATE MACHINE ---
        if (vomitOrbState == VomitOrbState::CHARGING) {
            // Gibon stands still and charges the orb
            isMoving = false;
            rollSpeed = 0.0f;
            
            vomitOrbChargeProgress += dt / vomitOrbChargeTime;
            if (vomitOrbChargeProgress > 1.0f) vomitOrbChargeProgress = 1.0f;
            
            // Position the orb in front of gibon's mouth
            Vector3 forward = {sinf(angle * DEG2RAD), 0.0f, cosf(angle * DEG2RAD)};
            vomitOrbPosition = Vector3Add(position, Vector3Scale(forward, bodyScale + 7.0f));
            vomitOrbPosition.y = position.y + bodyScale * 0.9f;
            
            if (vomitOrbChargeProgress >= 1.0f) {
                vomitOrbState = VomitOrbState::READY;
            }
            
        } else if (vomitOrbState == VomitOrbState::READY) {
            // Orb is fully charged, players can shoot it
            isMoving = false;
            rollSpeed = 0.0f;
            
            Vector3 forward = {sinf(angle * DEG2RAD), 0.0f, cosf(angle * DEG2RAD)};
            vomitOrbPosition = Vector3Add(position, Vector3Scale(forward, bodyScale + 7.0f));
            vomitOrbPosition.y = position.y + bodyScale * 0.9f;
            
            vomitOrbReadyTimer -= dt;
            
            if (vomitOrbHp <= 0) {
                // Players destroyed the orb!
                vomitOrbState = VomitOrbState::DESTROYED;
                vomitOrbExplosionTimer = 0.0f;
                // Gibon takes feedback damage
                hp -= 10000;
                if (hp <= 0) {
                    hp = 0;
                    active = false;
                }
                // Start explosion at gibon too
                vomitOrbGibonExploding = true;
                vomitOrbGibonExpTimer = 0.0f;
            } else if (vomitOrbReadyTimer <= 0.0f) {
                // Time's up - orb launches toward the base!
                vomitOrbState = VomitOrbState::FLYING;
                vomitOrbFlyTimer = 0.0f;
                vomitOrbStartPos = vomitOrbPosition;
                vomitOrbTargetPos = basePos;
                vomitOrbTargetPos.y = 2.0f;
                
                // Calculate ballistic arc velocity
                Vector3 toBase = Vector3Subtract(vomitOrbTargetPos, vomitOrbStartPos);
                float flightTime = 2.5f; // 2.5 seconds flight
                vomitOrbVelocity = Vector3Scale(toBase, 1.0f / flightTime);
                vomitOrbVelocity.y = 0.0f; // We handle Y separately for arc
                
                // Start explosion at gibon (from the strain of launching)
                vomitOrbGibonExploding = true;
                vomitOrbGibonExpTimer = 0.0f;
            }
            
        } else if (vomitOrbState == VomitOrbState::DESTROYED) {
            // Explosion animation at orb position
            vomitOrbExplosionTimer += dt;
            
            if (vomitOrbExplosionTimer >= 1.5f) {
                vomitOrbState = VomitOrbState::INACTIVE;
            }
            
        } else if (vomitOrbState == VomitOrbState::FLYING) {
            // Orb flies in a ballistic arc toward the base
            vomitOrbFlyTimer += dt;
            float flightTime = 2.5f;
            float t = vomitOrbFlyTimer / flightTime;
            
            if (t >= 1.0f) {
                // Arrived at base!
                vomitOrbState = VomitOrbState::EXPLODED_BASE;
                vomitOrbExplosionTimer = 0.0f;
                vomitOrbPosition = vomitOrbTargetPos;
                
                // Deal massive damage to base
                if (baseHp && *baseHp > 0) {
                    *baseHp *= 0.5f;
                    if (*baseHp < 0) *baseHp = 0;
                }
            } else {
                // Lerp position with parabolic arc on Y
                Vector3 startPos = vomitOrbStartPos;
                Vector3 endPos = vomitOrbTargetPos;
                
                vomitOrbPosition.x = startPos.x + (endPos.x - startPos.x) * t;
                vomitOrbPosition.z = startPos.z + (endPos.z - startPos.z) * t;
                
                // Parabolic arc: peak at midpoint, height = 80 units
                float arcHeight = 80.0f;
                float baseY = startPos.y + (endPos.y - startPos.y) * t;
                float arc = 4.0f * arcHeight * t * (1.0f - t);
                vomitOrbPosition.y = baseY + arc;
            }
            
        } else if (vomitOrbState == VomitOrbState::EXPLODED_BASE) {
            // Explosion animation at base
            vomitOrbExplosionTimer += dt;
            
            if (vomitOrbExplosionTimer >= 2.0f) {
                vomitOrbState = VomitOrbState::INACTIVE;
            }
        }
        
        // Update gibon-side explosion (runs independently of orb state)
        if (vomitOrbGibonExploding) {
            vomitOrbGibonExpTimer += dt;
            if (vomitOrbGibonExpTimer >= 1.5f) {
                vomitOrbGibonExploding = false;
            }
        }
        
        // --- CRUSH ATTACK (rolling over players on the way) ---
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            if (atBaseEdge && !p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
            float d = Vector3Distance(p.pos, position);
            if (d < 5.0f && p.hp) {
                *p.hp -= (int)(150 * dt); // Constant crush damage while overlapping
            }
        }
        
        // --- CONTINUOUS TOXIC VOMIT RAIN (targets players outside the base only) ---
        vomitCooldown -= dt;
        if (vomitCooldown <= 0) {
            vomitCooldown = hp < maxHp * 0.3f ? 0.8f : 1.15f;
            
            std::vector<Vector3> outsidePlayerTargets;
            for (const auto& player : players) {
                if (!player.active || (player.hp && *player.hp <= 0)) continue;
                if (player.isStructure) continue;
                if (IsInsideBaseFootprint(player.pos, basePos)) continue;
                outsidePlayerTargets.push_back(player.pos);
            }

            if (!outsidePlayerTargets.empty()) {
                Vector3 target = outsidePlayerTargets[rand() % outsidePlayerTargets.size()];
                target.x += (float)(rand() % 18 - 9);
                target.z += (float)(rand() % 18 - 9);

                if (IsInsideBaseFootprint(target, basePos)) {
                    Vector3 awayFromBase = Vector3Subtract(target, basePos);
                    awayFromBase.y = 0.0f;
                    if (Vector3Length(awayFromBase) < 0.1f) awayFromBase = {1.0f, 0.0f, 0.0f};
                    awayFromBase = Vector3Normalize(awayFromBase);
                    target = Vector3Add(basePos, Vector3Scale(awayFromBase, BASE_HALF_SIZE + 4.0f));
                }

                ToxicVomit v;
                v.position = {target.x + (float)(rand() % 5 - 2), 85.0f + (float)(rand() % 20), target.z + (float)(rand() % 5 - 2)};
                v.velocity = {0, -18.0f - (float)(rand() % 9), 0};
                v.radius = 2.0f;
                v.lifetime = 7.0f;
                v.active = true;
                vomitProjectiles.push_back(v);
            }
        }
        
        // --- BASE DAMAGE (when at base edge) ---
        if (attackTimer > 0) attackTimer -= dt;
        if (distToBaseEdge < 6.0f && attackTimer <= 0) {
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
            vomitCooldown = std::min(vomitCooldown, 0.8f);
        }
    }
    
    // --- UPDATE VOMIT PROJECTILES ---
    for (auto& v : vomitProjectiles) {
        if (!v.active) continue;
        
        // Apply gravity so projectiles arc down naturally
        v.velocity.y -= 18.0f * dt; // Gravity pull
        
        v.position.x += v.velocity.x * dt;
        v.position.y += v.velocity.y * dt;
        v.position.z += v.velocity.z * dt;
        v.lifetime -= dt;
        
        // Check if projectile hits a player IN FLIGHT (direct hit = more damage)
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            if (p.isStructure) continue;
            if (IsInsideBaseFootprint(p.pos, basePos)) continue;
            float d = Vector3Distance(p.pos, v.position);
            if (d < v.radius + 1.1f && p.hp && v.position.y > 0.5f) {
                *p.hp -= 25;
                v.active = false;
                
                // Still spawn puddle at impact point
                if (!IsInsideBaseFootprint(v.position, basePos)) {
                    VomitPuddle puddle;
                    puddle.position = {v.position.x, 0.1f, v.position.z};
                    puddle.radius = 9.0f;
                    puddle.lifetime = 10.0f;
                    puddle.damageTimer = 0.0f;
                    puddle.active = true;
                    vomitPuddles.push_back(puddle);
                }
                break;
            }
        }
        if (!v.active) continue;
        
        if (v.position.y <= 0.5f || v.lifetime <= 0) {
            v.active = false;
            
            // Spawn BIG toxic puddle on ground impact
            if (v.position.y <= 0.5f && !IsInsideBaseFootprint(v.position, basePos)) {
                VomitPuddle puddle;
                puddle.position = {v.position.x, 0.1f, v.position.z};
                puddle.radius = 9.0f;
                puddle.lifetime = 10.0f;
                puddle.damageTimer = 0.0f;
                puddle.active = true;
                vomitPuddles.push_back(puddle);
            }
            
            // Splash damage on impact
            for (const auto& p : players) {
                if (!p.active || (p.hp && *p.hp <= 0)) continue;
                if (!p.isStructure && IsInsideBaseFootprint(p.pos, basePos)) continue;
                float d = Vector3Distance(p.pos, v.position);
                if (d < 7.0f && p.hp) {
                    float falloff = 1.0f - (d / 7.0f);
                    *p.hp -= (int)(35 * falloff);
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

    // --- UPDATE VOMIT PUDDLES ---
    for (auto& p : vomitPuddles) {
        if (!p.active) continue;
        p.lifetime -= dt;
        p.damageTimer -= dt;
        
        if (p.lifetime <= 0) {
            p.active = false;
            continue;
        }
        
        // Damage players standing in the puddle.
        if (p.damageTimer <= 0) {
            bool hitSomeone = false;
            for (const auto& player : players) {
                if (!player.active || (player.hp && *player.hp <= 0)) continue;
                if (player.isStructure) continue;
                if (IsInsideBaseFootprint(player.pos, basePos)) continue;
                
                float d = Vector3Distance(player.pos, p.position);
                if (d < p.radius && player.hp) {
                    *player.hp -= 20;
                    hitSomeone = true;
                }
            }
            if (hitSomeone) {
                p.damageTimer = 0.7f;
            }
        }
    }
    
    // Clean up inactive puddles
    vomitPuddles.erase(
        std::remove_if(vomitPuddles.begin(), vomitPuddles.end(), 
            [](const VomitPuddle& p) { return !p.active; }),
        vomitPuddles.end()
    );
    
    // --- UPDATE TRAVELING SHOCKWAVE ---
    if (shockwaveActive) {
        float prevRadius = shockwaveRadius;
        shockwaveRadius += shockwaveSpeed * dt;
        
        // Damage players that the wave just passed over
        for (const auto& p : players) {
            if (!p.active || (p.hp && *p.hp <= 0)) continue;
            if (p.isStructure) continue;
            
            float dist = Vector3Distance(p.pos, shockwavePos);
            if (dist <= shockwaveRadius && dist > prevRadius) {
                float damageRatio = 1.0f - (dist / shockwaveMaxRadius);
                if (damageRatio < 0.0f) damageRatio = 0.0f;
                int damage = (int)(400.0f * damageRatio); // Max 400 dmg, min 0
                if (damage > 0 && p.hp) {
                    *p.hp -= damage;
                }
            }
        }
        
        if (shockwaveRadius >= shockwaveMaxRadius) {
            shockwaveActive = false;
        }
    }
}

void GibonRzygacz::Draw() {
    if (!active) return;
    
    float pulse = sinf(pulseTimer * 3.0f) * 0.15f + 1.0f;
    float currentScale = bodyScale * pulse;

    // Shield rendering moved down for proper transparency sorting.
    
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
            
            // Cracks spreading outward on the road (deterministic, visible)
            if (i % 3 == 0) {
                float crackVariation = (float)((i * 7) % 10);
                Vector3 crackEnd = {
                    impactCrater.position.x + cosf(a) * (impactCrater.radius * 2.5f + crackVariation),
                    0.07f,
                    impactCrater.position.z + sinf(a) * (impactCrater.radius * 2.5f + crackVariation)
                };
                Vector3 crackStart = {cx, 0.07f, cz};
                // Make cracks thicker and visible
                DrawLine3D(crackStart, crackEnd, BLACK);
                
                Vector3 offset1 = {crackStart.x + 0.15f, crackStart.y, crackStart.z + 0.15f};
                Vector3 offsetEnd1 = {crackEnd.x + 0.15f, crackEnd.y, crackEnd.z + 0.15f};
                DrawLine3D(offset1, offsetEnd1, BLACK);

                Vector3 offset2 = {crackStart.x - 0.15f, crackStart.y, crackStart.z - 0.15f};
                Vector3 offsetEnd2 = {crackEnd.x - 0.15f, crackEnd.y, crackEnd.z - 0.15f};
                DrawLine3D(offset2, offsetEnd2, BLACK);
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
    
    // During charging, mouth opens wider and drool intensifies
    if (vomitOrbState == VomitOrbState::CHARGING) {
        float chargeIntensity = vomitOrbChargeProgress;
        float mouthScale = 2.0f + chargeIntensity * 2.0f;
        DrawSphere({0, -eyeOffset * 0.3f, currentScale * 0.85f}, mouthScale, {30, 80, 15, 255});
        droolLen = 3.0f + chargeIntensity * 5.0f + sinf(pulseTimer * 6.0f) * 2.0f;
        // Extra vomit streams
        for (int i = 0; i < 3; i++) {
            float offset = (float)(i - 1) * 0.8f;
            DrawCylinder({offset, -eyeOffset * 0.3f - droolLen * 0.5f, currentScale * 0.75f}, 
                         0.2f, 0.6f + chargeIntensity * 0.4f, droolLen * 0.7f, 6, 
                         Fade({100, 220, 30, 255}, 0.5f + chargeIntensity * 0.3f));
        }
    }
    
    // During direct vomiting, mouth opens wide while launching explosive balls
    if (isDirectVomiting) {
        // Wide open mouth
        DrawSphere({0, -eyeOffset * 0.3f, currentScale * 0.85f}, 4.0f, {20, 60, 10, 255});
        DrawSphere({0, -eyeOffset * 0.3f, currentScale * 0.88f}, 3.0f, {40, 120, 20, 255});
        
        // Short muzzle blast while balls are being launched
        float spewLen = 12.0f + sinf(pulseTimer * 15.0f) * 3.0f;
        for (int i = 0; i < 5; i++) {
            float offsetX = (float)(i - 2) * 0.5f;
            float offsetY = sinf(pulseTimer * 20.0f + (float)i * 1.5f) * 0.5f;
            DrawCylinder({offsetX, -eyeOffset * 0.3f + offsetY, currentScale * 0.85f}, 
                         0.3f, 1.5f + sinf(pulseTimer * 8.0f + (float)i) * 0.3f, spewLen, 8, 
                         Fade({100, 255, 30, 255}, 0.85f));
        }
        
        // Splatter drips falling from the stream
        for (int i = 0; i < 6; i++) {
            float t = (float)i / 6.0f;
            float dripX = sinf(pulseTimer * 6.0f + (float)i * 2.0f) * 1.5f;
            float dripZ = currentScale * 0.85f + t * spewLen;
            float dripY = -eyeOffset * 0.3f - t * 3.0f - sinf(pulseTimer * 4.0f + (float)i) * 1.0f;
            DrawSphere({dripX, dripY, dripZ}, 0.6f + t * 0.4f, Fade({80, 200, 20, 255}, 0.7f - t * 0.3f));
        }
    }
    
    DrawCylinder({0, -eyeOffset * 0.3f - droolLen, currentScale * 0.8f}, 0.3f, 0.8f, droolLen, 6, 
                 Fade({120, 200, 40, 255}, 0.6f));
    
    rlPopMatrix();
    
    

    // 5 FPS LAG ATTACK CHARGING VISUALS
    if (isCastingFpsLag) {
        float castProgress = fpsLagAttackTimer / 1.5f;
        float auraSize = currentScale * (1.1f + castProgress * 0.4f);
        DrawSphere({0, 0, 0}, auraSize, Fade({50, 255, 100, 255}, 0.35f + sinf(pulseTimer * 15.0f) * 0.15f));
        DrawSphereWires({0, 0, 0}, auraSize * 1.05f, 16, 16, Fade({150, 255, 50, 255}, 0.7f));
        
        for (int i = 0; i < 12; i++) {
            float angleRad = (float)i / 12.0f * PI * 2.0f + pulseTimer * 8.0f;
            float px = cosf(angleRad) * auraSize * 1.2f;
            float py = sinf((float)i + pulseTimer * 10.0f) * auraSize * 0.6f;
            float pz = sinf(angleRad) * auraSize * 1.2f;
            DrawCube({px, py, pz}, 1.2f, 1.2f, 1.2f, {100, 255, 100, 255});
            DrawCubeWires({px, py, pz}, 1.3f, 1.3f, 1.3f, LIME);
        }
    }

    rlPopMatrix();
    
    // 5 FPS LAG RELEASE SHOCKWAVE (WORLD SPACE)
    if (fpsLagWaveRadius > 0.0f && fpsLagWaveRadius < 160.0f) {
        float waveAlpha = 1.0f - (fpsLagWaveRadius / 160.0f);
        DrawCircle3D({position.x, 0.15f, position.z}, fpsLagWaveRadius, {1, 0, 0}, 90.0f, Fade({50, 255, 100, 255}, waveAlpha * 0.8f));
        DrawCircle3D({position.x, 0.18f, position.z}, fpsLagWaveRadius * 0.95f, {1, 0, 0}, 90.0f, Fade({200, 255, 50, 255}, waveAlpha * 0.9f));
        DrawCircle3D({position.x, 0.22f, position.z}, fpsLagWaveRadius * 0.90f, {1, 0, 0}, 90.0f, Fade(GREEN, waveAlpha * 0.6f));
    }
    
    // Impact shockwave and rocks (drawn in WORLD SPACE, independent of boss rotation)
    bool isSpawnImpact = (gibonState == GibonState::IMPACT && stateTimer < 3.0f);
    bool isJumpImpact = (jumpState == GibonJumpState::IMPACT && jumpAttackTimer < 3.0f);
    
    if (isSpawnImpact || isJumpImpact) {
        float t = isSpawnImpact ? stateTimer : jumpAttackTimer;
        float maxRadius = 60.0f;
        float shockRadius = (t < 0.3f) ? (t / 0.3f) * maxRadius : maxRadius;
        
        // Expanding wave - multiple layers for thickness
        if (t < 1.0f) {
            float waveAlpha = 1.0f - t;
            // 3 rings to make it thick
            DrawCircle3D({position.x, 0.1f, position.z}, shockRadius, {1, 0, 0}, 90.0f, 
                         Fade({255, 100, 50, 255}, waveAlpha * 0.9f));
            DrawCircle3D({position.x, 0.12f, position.z}, shockRadius * 0.9f, {1, 0, 0}, 90.0f, 
                         Fade({255, 150, 50, 255}, waveAlpha * 0.7f));
            DrawCircle3D({position.x, 0.15f, position.z}, shockRadius * 0.8f, {1, 0, 0}, 90.0f, 
                         Fade({255, 200, 100, 255}, waveAlpha * 0.5f));
        }

        // Jagged rocks forming a crater at max radius
        if (t > 0.15f) {
            float rockT = t - 0.15f; 
            float rockAlpha = (rockT > 2.0f) ? (1.0f - (rockT - 2.0f) / 0.85f) : 1.0f;
            if (rockAlpha < 0) rockAlpha = 0;
            
            float rockYOffset = (rockT < 0.2f) ? (rockT / 0.2f) * 6.0f : 6.0f;
            if (rockT > 2.0f) rockYOffset -= ((rockT - 2.0f) / 0.85f) * 6.0f;

            for (int i = 0; i < 45; i++) {
                float a = (float)i / 45.0f * PI * 2.0f;
                float jitter = sinf(a * 10.0f); // pseudo-random deterministic jitter
                float rockRadius = maxRadius + jitter * 2.0f;
                
                float cx = position.x + cosf(a) * rockRadius;
                float cz = position.z + sinf(a) * rockRadius;
                
                rlPushMatrix();
                rlTranslatef(cx, -4.0f + rockYOffset + jitter, cz); // Fixed to world ground Y
                rlRotatef(a * RAD2DEG, 0, 1, 0); // Face outward
                rlRotatef(-20.0f - jitter * 15.0f, 1, 0, 0); // Pitch up
                rlRotatef(jitter * 40.0f, 0, 0, 1); // Roll
                
                // Bigger rocks
                Vector3 size = {9.0f + jitter * 1.5f, 14.0f + jitter * 3.0f, 7.0f + jitter};
                Color rCol = {60, 55, 50, (unsigned char)(255 * rockAlpha)};
                Color rLine = {20, 15, 10, (unsigned char)(255 * rockAlpha)};
                
                DrawCube({0,0,0}, size.x, size.y, size.z, rCol);
                if (rockAlpha > 0.1f) {
                    DrawCubeWires({0,0,0}, size.x, size.y, size.z, rLine);
                }
                rlPopMatrix();
            }
        }
    }
    
    rlPopMatrix();
    
    // Impact shockwave and debris during IMPACT state (independent of gibon rotation)
    if (gibonState == GibonState::IMPACT && stateTimer < 2.0f) {
        float shockRadius = stateTimer * 40.0f;
        float alpha = 1.0f - stateTimer / 2.0f;
        DrawCircle3D({impactCrater.position.x, 0.1f, impactCrater.position.z}, shockRadius, {1, 0, 0}, 90.0f, 
                     Fade({200, 255, 50, 255}, alpha * 0.5f));
        
        // Pebbles and debris (kamyczki i odłamki) from the ground
        for (int i = 0; i < 30; i++) {
            float a = (float)i / 30.0f * PI * 2.0f;
            float speed = 20.0f + (float)((i * 17) % 20);
            float dist = speed * stateTimer;
            
            float height = 5.0f + (float)((i * 11) % 8) + sinf(a * 3.0f) * 3.0f - stateTimer * stateTimer * 25.0f;
            if (height < 0.1f) height = 0.1f;
            
            Vector3 debrisPos = {
                impactCrater.position.x + cosf(a) * dist,
                height,
                impactCrater.position.z + sinf(a) * dist
            };
            
            float dSize = 0.3f + (float)((i * 7) % 5) * 0.1f;
            // Appearance depends on ground (gray rubble), not the boss
            DrawCube(debrisPos, dSize, dSize, dSize, Fade({70, 70, 70, 255}, alpha));
            DrawCubeWires(debrisPos, dSize, dSize, dSize, Fade(BLACK, alpha));
        }
    }
    
    // --- SHADOW ---
    if (gibonState == GibonState::FALLING) {
        // Growing shadow on ground as ball falls
        float shadowScale = (500.0f - position.y) / 500.0f;
        DrawCircle3D({landingTarget.x, 0.05f, landingTarget.z}, currentScale * shadowScale * 1.5f, 
                     {1, 0, 0}, 90.0f, Fade(BLACK, 0.3f + shadowScale * 0.4f));
    } else {
        DrawCircle3D({position.x, 0.05f, position.z}, currentScale * 1.2f, {1, 0, 0}, 90.0f, Fade(BLACK, 0.5f));
    }

    // --- SHIELD ---
    if (hp <= 30000 && gibonState == GibonState::FINISHED_FALLING) {
        float shieldPulse = sinf(pulseTimer * 4.5f) * 1.8f;
        float shieldRadius = 55.0f + shieldPulse;
        Vector3 shieldCenter = {position.x, position.y + 10.0f, position.z};
        Color shieldFill = Fade({90, 255, 70, 255}, 0.16f + sinf(pulseTimer * 6.0f) * 0.04f);
        Color shieldWire = Fade({190, 255, 80, 255}, 0.62f);

        DrawSphere(shieldCenter, shieldRadius, shieldFill);
        DrawSphereWires(shieldCenter, shieldRadius, 32, 16, shieldWire);
        DrawCircle3D({position.x, 0.12f, position.z}, shieldRadius, {1, 0, 0}, 90.0f,
                     Fade({120, 255, 40, 255}, 0.45f));

        for (int i = 0; i < 10; i++) {
            float a = pulseTimer * 1.8f + (float)i * (PI * 2.0f / 10.0f);
            Vector3 dripPos = {
                shieldCenter.x + cosf(a) * shieldRadius * 0.72f,
                shieldCenter.y + sinf(pulseTimer * 2.5f + (float)i) * 4.0f,
                shieldCenter.z + sinf(a) * shieldRadius * 0.72f
            };
            DrawSphere(dripPos, 0.8f, Fade({130, 255, 45, 255}, 0.55f));
        }
    }
    
    // --- TOXIC VOMIT PROJECTILES ---
    for (const auto& v : vomitProjectiles) {
        if (!v.active) continue;
        float ballPulse = 1.0f + sinf(pulseTimer * 10.0f) * 0.08f;
        float ballRadius = v.radius * ballPulse;
        DrawSphere(v.position, ballRadius, {130, 235, 35, 255});
        DrawSphere(v.position, ballRadius * 0.55f, Fade({240, 255, 110, 255}, 0.65f));
        DrawSphere(v.position, ballRadius * 1.35f, Fade({80, 180, 20, 255}, 0.22f));
        DrawSphere({v.position.x, v.position.y + ballRadius * 1.15f, v.position.z}, ballRadius * 0.45f, Fade({150, 255, 50, 255}, 0.38f));
        DrawSphere({v.position.x, v.position.y + ballRadius * 2.0f, v.position.z}, ballRadius * 0.28f, Fade({150, 255, 50, 255}, 0.2f));
    }

    // --- TOXIC VOMIT PUDDLES ---
    for (const auto& p : vomitPuddles) {
        if (!p.active) continue;
        float alpha = (p.lifetime < 2.0f) ? (p.lifetime / 2.0f) : 1.0f;
        
        // --- EXPLOSION EFFECT ON IMPACT ---
        float timeSinceImpact = 10.0f - p.lifetime;
        if (timeSinceImpact < 0.5f) {
            float expProgress = timeSinceImpact / 0.5f;
            float expRadius = 3.0f + expProgress * 12.0f; // Made much larger
            float expAlpha = 1.0f - expProgress;
            
            // Toxic fireball explosion
            DrawSphere(p.position, expRadius, Fade({150, 255, 50, 255}, expAlpha * 0.8f));
            DrawSphere(p.position, expRadius * 0.7f, Fade({200, 255, 100, 255}, expAlpha * 0.9f));
            
            // Shockwave ring
            DrawCircle3D({p.position.x, 0.2f, p.position.z}, expRadius * 1.5f, {1, 0, 0}, 90.0f, Fade({255, 255, 100, 255}, expAlpha * 0.6f));
            
            // Splashes (flying vomit chunks)
            for(int i = 0; i < 12; i++) { // Increased number of chunks
                float a = (float)i / 12.0f * PI * 2.0f;
                float dist = expProgress * 16.0f; // Flying further
                float height = sinf(expProgress * PI) * 6.0f; // Flying higher
                Vector3 chunkPos = {p.position.x + cosf(a) * dist, 0.1f + height, p.position.z + sinf(a) * dist};
                DrawSphere(chunkPos, 1.2f, Fade({100, 220, 30, 255}, expAlpha)); // Larger chunks
            }
        }

        // Puddle decal (DrawCylinder)
        float pulse = sinf(pulseTimer * 8.0f) * 0.08f + 0.92f;
        float currentRadius = p.radius * pulse;
        Color puddleColor = Fade({100, 220, 30, 255}, alpha * 0.8f);
        Color coreColor = Fade({60, 150, 20, 255}, alpha * 0.9f);
        
        DrawCylinder({p.position.x, 0.08f, p.position.z}, currentRadius, currentRadius, 0.08f, 64, puddleColor);
        DrawCylinder({p.position.x, 0.10f, p.position.z}, currentRadius * 0.55f, currentRadius * 0.55f, 0.08f, 64, coreColor);
        
        // Bubbles in the puddle
        for (int i = 0; i < 5; i++) {
            float ba = (float)i / 5.0f * PI * 2.0f + pulseTimer * 2.0f;
            float br = p.radius * 0.5f * sinf(pulseTimer * 3.0f + i);
            float bx = p.position.x + cosf(ba) * br;
            float bz = p.position.z + sinf(ba) * br;
            float bSize = 0.5f + sinf(pulseTimer * 5.0f + i) * 0.3f;
            if (bSize > 0) {
                DrawSphere({bx, 0.2f, bz}, bSize, Fade({150, 255, 50, 255}, alpha * 0.6f));
            }
        }
    }

    // --- ROLL ATTACK MISS EXPLOSION ---
    if (rollMissExplosionTimer >= 0.0f && rollMissExplosionTimer < 0.6f) {
        float t = rollMissExplosionTimer / 0.6f;
        float expRadius = 3.0f + t * 15.0f;
        float expAlpha = 1.0f - t;
        Vector3 ep = rollMissExplosionPos;
        
        // Big shockwave sphere
        DrawSphere({ep.x, ep.y + expRadius * 0.3f, ep.z}, expRadius, Fade({80, 200, 50, 255}, expAlpha * 0.8f));
        DrawSphere({ep.x, ep.y + expRadius * 0.1f, ep.z}, expRadius * 0.7f, Fade({200, 255, 100, 255}, expAlpha * 0.9f));
        
        // Ground shockwave rings
        DrawCircle3D({ep.x, 0.15f, ep.z}, expRadius * 1.4f, {1, 0, 0}, 90.0f, Fade({150, 255, 50, 255}, expAlpha * 0.7f));
        DrawCircle3D({ep.x, 0.2f, ep.z}, expRadius * 1.1f, {1, 0, 0}, 90.0f, Fade({255, 255, 80, 255}, expAlpha * 0.5f));
        
        // Flying debris
        for (int i = 0; i < 10; i++) {
            float a = (float)i / 10.0f * PI * 2.0f;
            float dist = t * 18.0f;
            float height = sinf(t * PI) * 8.0f;
            DrawSphere({ep.x + cosf(a) * dist, ep.y + height, ep.z + sinf(a) * dist},
                       1.5f, Fade({80, 200, 30, 255}, expAlpha));
        }
    }

    // ============================================
    // --- VOMIT ORB RENDERING (STATE MACHINE) ---
    // ============================================
    
    if (vomitOrbState == VomitOrbState::CHARGING) {
        // Growing orb with charging effects
        float chargeSize = vomitOrbChargeProgress * 4.2f; // Grows to full size
        float chargePulse = sinf(pulseTimer * 10.0f * (0.5f + vomitOrbChargeProgress)) * 0.3f + 1.0f;
        float finalSize = chargeSize * chargePulse;
        
        // Core orb (growing)
        Color chargeColor = {
            (unsigned char)(150 + (int)(vomitOrbChargeProgress * 60)),
            (unsigned char)(200 + (int)(vomitOrbChargeProgress * 55)),
            (unsigned char)(80 + (int)(vomitOrbChargeProgress * 40)),
            255
        };
        DrawSphere(vomitOrbPosition, finalSize, chargeColor);
        
        // Inner glow (brighter as charge progresses)
        DrawSphere(vomitOrbPosition, finalSize * 0.6f, Fade(WHITE, 0.2f + vomitOrbChargeProgress * 0.3f));
        
        // Outer charging aura
        float auraSize = finalSize + 2.0f + vomitOrbChargeProgress * 3.0f;
        DrawSphere(vomitOrbPosition, auraSize, Fade({100, 255, 50, 255}, 0.08f + vomitOrbChargeProgress * 0.15f));
        
        // Particles swirling into the orb (from gibon's body)
        int numParticles = (int)(16 * vomitOrbChargeProgress) + 4;
        for (int i = 0; i < numParticles; i++) {
            float a = pulseTimer * 3.0f + (float)i * (PI * 2.0f / numParticles);
            float spiralT = fmodf(pulseTimer * 2.0f + (float)i * 0.3f, 1.0f);
            
            // Particles spiral from gibon body to orb
            float spiralR = (1.0f - spiralT) * (bodyScale * 1.5f);
            Vector3 particlePos = {
                vomitOrbPosition.x + cosf(a) * spiralR,
                vomitOrbPosition.y + sinf(a * 2.0f) * spiralR * 0.5f,
                vomitOrbPosition.z + sinf(a) * spiralR
            };
            float pSize = 0.3f + (1.0f - spiralT) * 0.5f;
            DrawSphere(particlePos, pSize, Fade({80, 255, 30, 255}, 0.4f + spiralT * 0.4f));
        }
        
        // Energy rings around the orb
        for (int ring = 0; ring < 3; ring++) {
            float ringAngle = pulseTimer * (60.0f + ring * 30.0f);
            float ringRadius = finalSize + 1.0f + ring * 0.8f;
            float ringAlpha = 0.3f + vomitOrbChargeProgress * 0.3f;
            Vector3 ringAxis = {sinf(ring * 1.2f), cosf(ring * 0.8f), sinf(ring * 2.1f)};
            DrawCircle3D(vomitOrbPosition, ringRadius, ringAxis, ringAngle, 
                         Fade({150, 255, 80, 255}, ringAlpha));
        }
        
        // Charging beam from gibon's mouth to orb
        Vector3 mouthPos = position;
        mouthPos.y += bodyScale * 0.7f;
        Vector3 forward = {sinf(angle * DEG2RAD), 0.0f, cosf(angle * DEG2RAD)};
        mouthPos = Vector3Add(mouthPos, Vector3Scale(forward, bodyScale * 0.85f));
        
        float beamAlpha = 0.3f + vomitOrbChargeProgress * 0.5f;
        DrawLine3D(mouthPos, vomitOrbPosition, Fade({120, 255, 40, 255}, beamAlpha));
        // Thicker beam effect
        for (int b = 0; b < 3; b++) {
            Vector3 offset = {
                sinf(pulseTimer * 8.0f + b * 2.0f) * 0.3f,
                cosf(pulseTimer * 6.0f + b * 1.5f) * 0.3f,
                sinf(pulseTimer * 7.0f + b * 1.8f) * 0.3f
            };
            DrawLine3D(Vector3Add(mouthPos, offset), Vector3Add(vomitOrbPosition, offset), 
                       Fade({80, 200, 30, 255}, beamAlpha * 0.5f));
        }
        
    } else if (vomitOrbState == VomitOrbState::READY) {
        // Fully charged orb - pulsing and dangerous
        float orbPulse = sinf(pulseTimer * 7.0f) * 0.35f + 1.0f;
        float orbSize = 4.2f * orbPulse;
        
        // Core
        DrawSphere(vomitOrbPosition, orbSize, {210, 255, 120, 255});
        
        // Outer glow
        DrawSphere(vomitOrbPosition, orbSize + 1.0f + orbPulse, Fade({120, 255, 40, 255}, 0.28f));
        
        // Inner bright core
        DrawSphere({vomitOrbPosition.x, vomitOrbPosition.y + 1.5f, vomitOrbPosition.z},
                   2.1f, Fade(WHITE, 0.35f));
        
        // Warning pulsing aura (gets faster as timer runs out)
        float urgency = 1.0f - (vomitOrbReadyTimer / 25.0f);
        float warningPulse = sinf(pulseTimer * (5.0f + urgency * 15.0f));
        if (warningPulse > 0.5f) {
            DrawSphere(vomitOrbPosition, orbSize + 3.0f, Fade({255, 100, 50, 255}, 0.15f * urgency));
        }
        
        // Floating toxic particles around orb
        for (int i = 0; i < 12; i++) {
            float a = pulseTimer * 2.0f + (float)i * (PI * 2.0f / 12.0f);
            float r = orbSize + 2.0f + sinf(a * 3.0f + pulseTimer) * 1.5f;
            Vector3 partPos = {
                vomitOrbPosition.x + cosf(a) * r,
                vomitOrbPosition.y + sinf(pulseTimer * 3.0f + (float)i) * 2.0f,
                vomitOrbPosition.z + sinf(a) * r
            };
            float pSize = 0.3f + sinf(pulseTimer * 4.0f + (float)i) * 0.2f;
            DrawSphere(partPos, pSize, Fade({100, 255, 50, 255}, 0.4f));
        }
        
    } else if (vomitOrbState == VomitOrbState::DESTROYED) {
        // Toxic explosion animation at orb position
        float t = vomitOrbExplosionTimer;
        float maxT = 1.5f;
        float progress = t / maxT;
        float alpha = 1.0f - progress;
        
        // Expanding toxic fireball
        float explodeRadius = 4.0f + progress * 20.0f;
        DrawSphere(vomitOrbPosition, explodeRadius * 0.5f, 
                   Fade({200, 255, 100, 255}, alpha));
        DrawSphere(vomitOrbPosition, explodeRadius * 0.75f, 
                   Fade({100, 200, 50, 255}, alpha * 0.6f));
        DrawSphere(vomitOrbPosition, explodeRadius, 
                   Fade({60, 150, 30, 255}, alpha * 0.3f));
        
        // Shockwave ring
        float shockRadius = progress * 30.0f;
        DrawCircle3D(vomitOrbPosition, shockRadius, {1, 0, 0}, 90.0f, 
                     Fade({150, 255, 50, 255}, alpha * 0.4f));
        
        // Flying debris chunks
        for (int i = 0; i < 16; i++) {
            float a = (float)i / 16.0f * PI * 2.0f;
            float debrisSpeed = 15.0f + sinf(a * 5.0f) * 8.0f;
            float debrisDist = debrisSpeed * t;
            Vector3 debrisPos = {
                vomitOrbPosition.x + cosf(a) * debrisDist,
                vomitOrbPosition.y + sinf(a * 3.0f) * debrisDist * 0.5f - t * 10.0f * t,
                vomitOrbPosition.z + sinf(a) * debrisDist
            };
            if (debrisPos.y < 0.1f) debrisPos.y = 0.1f;
            float debrisSize = 0.5f + sinf(a * 7.0f) * 0.3f;
            debrisSize *= alpha;
            DrawSphere(debrisPos, debrisSize, Fade({120, 220, 40, 255}, alpha * 0.7f));
        }
        
        // Toxic splash on ground
        if (progress > 0.2f) {
            float splashR = (progress - 0.2f) * 25.0f;
            DrawCircle3D({vomitOrbPosition.x, 0.08f, vomitOrbPosition.z}, splashR, {1, 0, 0}, 90.0f,
                         Fade({80, 200, 40, 255}, alpha * 0.5f));
        }
        
    } else if (vomitOrbState == VomitOrbState::FLYING) {
        // Orb flying in arc toward the base
        float orbPulse = sinf(pulseTimer * 12.0f) * 0.25f + 1.0f;
        float orbSize = 4.2f * orbPulse;
        
        // Core orb (angry red-green)
        DrawSphere(vomitOrbPosition, orbSize, {230, 255, 80, 255});
        
        // Burning outer layer
        DrawSphere(vomitOrbPosition, orbSize + 1.5f, Fade({255, 200, 50, 255}, 0.25f));
        DrawSphere(vomitOrbPosition, orbSize + 3.0f, Fade({255, 100, 30, 255}, 0.12f));
        
        // Inner core glow
        DrawSphere(vomitOrbPosition, orbSize * 0.4f, Fade(WHITE, 0.5f));
        
        // Toxic trail behind the orb
        float flightTime = 2.5f;
        float t = vomitOrbFlyTimer / flightTime;
        for (int i = 1; i <= 12; i++) {
            float trailT = t - (float)i * 0.015f;
            if (trailT < 0) continue;
            
            float tx = vomitOrbStartPos.x + (vomitOrbTargetPos.x - vomitOrbStartPos.x) * trailT;
            float tz = vomitOrbStartPos.z + (vomitOrbTargetPos.z - vomitOrbStartPos.z) * trailT;
            float arcH = 80.0f;
            float baseY = vomitOrbStartPos.y + (vomitOrbTargetPos.y - vomitOrbStartPos.y) * trailT;
            float arc = 4.0f * arcH * trailT * (1.0f - trailT);
            float ty = baseY + arc;
            
            float trailAlpha = 1.0f - (float)i / 12.0f;
            float trailSize = orbSize * (1.0f - (float)i / 15.0f);
            
            Vector3 trailPos = {tx, ty, tz};
            DrawSphere(trailPos, trailSize * 0.5f, Fade({100, 220, 40, 255}, trailAlpha * 0.4f));
            // Dripping particles below trail
            DrawSphere({tx, ty - 1.0f - (float)i * 0.3f, tz}, trailSize * 0.2f, 
                       Fade({80, 180, 30, 255}, trailAlpha * 0.3f));
        }
        
        // Shadow on the ground below the orb
        DrawCircle3D({vomitOrbPosition.x, 0.06f, vomitOrbPosition.z}, 
                     orbSize * 1.5f, {1, 0, 0}, 90.0f, 
                     Fade(BLACK, 0.3f * (1.0f - vomitOrbPosition.y / 100.0f)));
        
    } else if (vomitOrbState == VomitOrbState::EXPLODED_BASE) {
        // Massive toxic explosion at base
        float t = vomitOrbExplosionTimer;
        float maxT = 2.0f;
        float progress = t / maxT;
        float alpha = 1.0f - progress;
        
        // Giant expanding fireball (toxic green-yellow)
        float explodeRadius = 5.0f + progress * 35.0f;
        DrawSphere(vomitOrbPosition, explodeRadius * 0.4f, 
                   Fade({255, 255, 100, 255}, alpha));
        DrawSphere(vomitOrbPosition, explodeRadius * 0.65f, 
                   Fade({200, 255, 80, 255}, alpha * 0.7f));
        DrawSphere(vomitOrbPosition, explodeRadius * 0.85f, 
                   Fade({120, 200, 50, 255}, alpha * 0.4f));
        DrawSphere(vomitOrbPosition, explodeRadius, 
                   Fade({60, 150, 30, 255}, alpha * 0.2f));
        
        // Ground shockwave
        float shockRadius = progress * 50.0f;
        DrawCircle3D({vomitOrbPosition.x, 0.1f, vomitOrbPosition.z}, shockRadius, {1, 0, 0}, 90.0f,
                     Fade({200, 255, 50, 255}, alpha * 0.5f));
        DrawCircle3D({vomitOrbPosition.x, 0.12f, vomitOrbPosition.z}, shockRadius * 0.7f, {1, 0, 0}, 90.0f,
                     Fade({255, 255, 100, 255}, alpha * 0.3f));
        
        // Flying toxic chunks
        for (int i = 0; i < 24; i++) {
            float a = (float)i / 24.0f * PI * 2.0f;
            float chunkSpeed = 20.0f + sinf(a * 7.0f) * 12.0f;
            float chunkDist = chunkSpeed * t;
            float chunkY = vomitOrbPosition.y + sinf(a * 5.0f + t * 3.0f) * chunkDist * 0.3f - 5.0f * t * t;
            if (chunkY < 0.1f) chunkY = 0.1f;
            
            Vector3 chunkPos = {
                vomitOrbPosition.x + cosf(a) * chunkDist,
                chunkY,
                vomitOrbPosition.z + sinf(a) * chunkDist
            };
            float chunkSize = (0.6f + sinf(a * 3.0f) * 0.3f) * alpha;
            DrawSphere(chunkPos, chunkSize, Fade({150, 230, 50, 255}, alpha * 0.6f));
        }
        
        // Toxic mushroom cloud rising
        if (progress > 0.1f) {
            float cloudProgress = (progress - 0.1f) / 0.9f;
            float cloudY = vomitOrbPosition.y + cloudProgress * 40.0f;
            float cloudRadius = 5.0f + cloudProgress * 15.0f;
            DrawSphere({vomitOrbPosition.x, cloudY, vomitOrbPosition.z}, cloudRadius,
                       Fade({80, 150, 30, 255}, alpha * 0.3f));
            DrawSphere({vomitOrbPosition.x, cloudY + cloudRadius * 0.8f, vomitOrbPosition.z}, 
                       cloudRadius * 0.7f, Fade({100, 180, 40, 255}, alpha * 0.25f));
        }
    }
    
    // --- GIBON-SIDE EXPLOSION (runs when orb destroyed or launched) ---
    if (vomitOrbGibonExploding) {
        float t = vomitOrbGibonExpTimer;
        float maxT = 1.5f;
        float progress = t / maxT;
        float alpha = 1.0f - progress;
        
        Vector3 gibonExpPos = position;
        gibonExpPos.y += bodyScale;
        
        // Expanding toxic burst around gibon
        float burstRadius = 3.0f + progress * 15.0f;
        DrawSphere(gibonExpPos, burstRadius * 0.5f, Fade({200, 255, 80, 255}, alpha * 0.8f));
        DrawSphere(gibonExpPos, burstRadius * 0.75f, Fade({150, 220, 50, 255}, alpha * 0.5f));
        DrawSphere(gibonExpPos, burstRadius, Fade({80, 180, 30, 255}, alpha * 0.25f));
        
        // Shockwave ring around gibon
        float gibonShock = progress * 25.0f;
        DrawCircle3D({position.x, 0.08f, position.z}, gibonShock, {1, 0, 0}, 90.0f,
                     Fade({180, 255, 50, 255}, alpha * 0.4f));
        
        // Toxic spew chunks flying outward from gibon
        for (int i = 0; i < 10; i++) {
            float a = (float)i / 10.0f * PI * 2.0f;
            float chunkDist = 10.0f * t + sinf(a * 3.0f) * 3.0f;
            float chunkY = gibonExpPos.y + sinf(a * 4.0f) * t * 5.0f - 8.0f * t * t;
            if (chunkY < 0.1f) chunkY = 0.1f;
            Vector3 chunkPos = {
                position.x + cosf(a) * chunkDist,
                chunkY,
                position.z + sinf(a) * chunkDist
            };
            float chunkSize = 0.4f * alpha;
            DrawSphere(chunkPos, chunkSize, Fade({120, 200, 40, 255}, alpha * 0.6f));
        }
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
    
    // --- TRAVELING SHOCKWAVE VISUALS ---
    if (shockwaveActive) {
        float alpha = 1.0f - (shockwaveRadius / shockwaveMaxRadius);
        if (alpha < 0.0f) alpha = 0.0f;
        
        // Expanding rings
        DrawCircle3D(shockwavePos, shockwaveRadius, {1, 0, 0}, 90.0f, Fade({200, 255, 50, 255}, alpha * 0.8f));
        DrawCircle3D(shockwavePos, fmaxf(0.0f, shockwaveRadius - 3.0f), {1, 0, 0}, 90.0f, Fade({100, 200, 30, 255}, alpha * 0.4f));
        
        // Debris pushed by the wave edge
        for (int i = 0; i < 32; i++) {
            float a = (float)i / 32.0f * PI * 2.0f + shockwaveRadius * 0.05f; 
            float height = sinf(a * 8.0f + shockwaveRadius * 0.5f) * 2.0f + 1.5f;
            if (height < 0.1f) height = 0.1f;
            
            Vector3 debrisPos = {
                shockwavePos.x + cosf(a) * shockwaveRadius,
                height,
                shockwavePos.z + sinf(a) * shockwaveRadius
            };
            
            float dSize = 0.4f + (float)((i * 11) % 7) * 0.1f;
            DrawCube(debrisPos, dSize, dSize, dSize, Fade({70, 70, 70, 255}, alpha));
            DrawCubeWires(debrisPos, dSize, dSize, dSize, Fade(BLACK, alpha));
        }
    }
}

void GibonRzygacz::DrawHUD(Camera3D camera) {
    if (!active || hp <= 0) return;
    
    // --- 5 FPS LAG EFFECT OVERLAY FOR LOCAL PLAYER ---
    if (fpsLagEffectTimer > 0.0f) {
        int sw = GetScreenWidth();
        const char* lagMsg = TextFormat("⚠️ EFEKT 5 FPS! (VIRUS GIBONA): %.1fs ⚠️", fpsLagEffectTimer);
        int lagW = MeasureText(lagMsg, 26);
        float pulseAlpha = (sinf(pulseTimer * 14.0f) > 0) ? 1.0f : 0.6f;
        int jitterX = (rand() % 7) - 3;
        int jitterY = (rand() % 5) - 2;
        
        int boxX = sw / 2 - lagW / 2 - 20 + jitterX;
        int boxY = 160 + jitterY;
        int boxW = lagW + 40;
        int boxH = 45;
        
        DrawRectangle(boxX, boxY, boxW, boxH, Fade(BLACK, 0.85f));
        DrawRectangleLines(boxX, boxY, boxW, boxH, Fade({80, 255, 50, 255}, pulseAlpha));
        DrawText(lagMsg, sw / 2 - lagW / 2 + jitterX, boxY + 10, 26, Fade({100, 255, 80, 255}, pulseAlpha));
    }
    
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

    // Attack Status under Boss Bar
    if (isDirectVomiting) {
        const char* spewTag = "ATAK: STRUMIEŃ RZYGÓW!";
        int tagW = MeasureText(spewTag, 14);
        DrawText(spewTag, (int)(screenPos.x - tagW / 2.0f), (int)(screenPos.y + 6), 14, {120, 255, 40, 255});
    } else if (isCastingFpsLag) {
        const char* castTag = "ATAK: VIRUS 5 FPS (ŁADOWANIE...)";
        int tagW = MeasureText(castTag, 14);
        DrawText(castTag, (int)(screenPos.x - tagW / 2.0f), (int)(screenPos.y + 6), 14, {100, 255, 100, 255});
    } else if (fpsLagEffectTimer > 0.0f) {
        const char* activeTag = "ATAK: VIRUS 5 FPS AKTYWNY!";
        int tagW = MeasureText(activeTag, 14);
        DrawText(activeTag, (int)(screenPos.x - tagW / 2.0f), (int)(screenPos.y + 6), 14, {255, 200, 50, 255});
    }

    // Vomit orb HUD
    if (vomitOrbState == VomitOrbState::CHARGING || vomitOrbState == VomitOrbState::READY) {
        Vector3 orbBarPos = {vomitOrbPosition.x, vomitOrbPosition.y + 6.5f, vomitOrbPosition.z};
        Vector3 toOrb = Vector3Subtract(orbBarPos, camera.position);
        if (Vector3DotProduct(toOrb, camFwd) >= 0) {
            Vector2 orbScreen = GetWorldToScreen(orbBarPos, camera);
            int ow = 170;
            int oh = 12;
            
            if (vomitOrbState == VomitOrbState::CHARGING) {
                // Show charging progress bar
                DrawRectangle((int)(orbScreen.x - ow / 2) - 2, (int)orbScreen.y - 18, ow + 4, oh + 4, BLACK);
                DrawRectangle((int)(orbScreen.x - ow / 2), (int)orbScreen.y - 16, ow, oh, {40, 40, 40, 255});
                DrawRectangle((int)(orbScreen.x - ow / 2), (int)orbScreen.y - 16, 
                              (int)(ow * vomitOrbChargeProgress), oh, {255, 200, 50, 255});
                const char* chargeText = TextFormat("LADOWANIE... %.0f%%", vomitOrbChargeProgress * 100.0f);
                int chargeTextW = MeasureText(chargeText, 14);
                DrawText(chargeText, (int)(orbScreen.x - chargeTextW / 2), (int)orbScreen.y - 36, 14, {255, 200, 50, 255});
            } else {
                // Show orb HP and timer
                float orbRatio = std::max(0.0f, std::min(1.0f, vomitOrbHp / (float)vomitOrbMaxHp));
                DrawRectangle((int)(orbScreen.x - ow / 2) - 2, (int)orbScreen.y - 18, ow + 4, oh + 4, BLACK);
                DrawRectangle((int)(orbScreen.x - ow / 2), (int)orbScreen.y - 16, ow, oh, DARKGREEN);
                DrawRectangle((int)(orbScreen.x - ow / 2), (int)orbScreen.y - 16, (int)(ow * orbRatio), oh, LIME);
                
                // Flash warning when timer is low
                Color textColor = WHITE;
                if (vomitOrbReadyTimer < 10.0f) {
                    textColor = (sinf(pulseTimer * 8.0f) > 0) ? RED : WHITE;
                }
                const char* orbText = TextFormat("KULA RZYGOW %d  %.0fs", vomitOrbHp, std::max(0.0f, vomitOrbReadyTimer));
                int orbTextW = MeasureText(orbText, 14);
                DrawText(orbText, (int)(orbScreen.x - orbTextW / 2), (int)orbScreen.y - 36, 14, textColor);
            }
        }
    }
    
    // Flying orb warning
    if (vomitOrbState == VomitOrbState::FLYING) {
        int sw = GetScreenWidth();
        const char* warningText = "!!! KULA LECI NA BAZE !!!";
        int warnW = MeasureText(warningText, 30);
        float warningAlpha = sinf(pulseTimer * 10.0f) > 0 ? 1.0f : 0.3f;
        DrawText(warningText, sw / 2 - warnW / 2, 120, 30, Fade(RED, warningAlpha));
    }
}

BoundingBox GibonRzygacz::GetBoundingBox() {
    float s = bodyScale;
    return (BoundingBox){
        (Vector3){ position.x - s, position.y, position.z - s },
        (Vector3){ position.x + s, position.y + s * 2.0f, position.z + s }
    };
}

bool GibonRzygacz::RayHit(Ray ray, float& outDist) {
    lastRayHitVomitOrb = false;
    lastRayHitVomitShield = false;

    if (hp <= 30000 && gibonState == GibonState::FINISHED_FALLING) {
        Vector3 shieldCenter = {position.x, position.y + 10.0f, position.z};
        float shieldRadius = 34.0f;
        bool rayStartsInsideShield = Vector3Distance(ray.position, shieldCenter) < shieldRadius;
        if (!rayStartsInsideShield) {
            RayCollision shieldHit = GetRayCollisionSphere(ray, shieldCenter, shieldRadius);
            if (shieldHit.hit) {
                float bodyDist = 999999.0f;
                bool bodyHit = Enemy::RayHit(ray, bodyDist);
                if (!bodyHit || shieldHit.distance <= bodyDist) {
                    outDist = shieldHit.distance;
                    lastRayHitVomitShield = true;
                    return true;
                }
            }
        }
    }

    // Orb is hittable during CHARGING (partial) and READY states
    bool orbShootable = (vomitOrbState == VomitOrbState::CHARGING && vomitOrbChargeProgress > 0.3f) ||
                        (vomitOrbState == VomitOrbState::READY);
    if (orbShootable && vomitOrbHp > 0) {
        float orbHitRadius = (vomitOrbState == VomitOrbState::CHARGING) 
                             ? 2.0f + vomitOrbChargeProgress * 3.0f 
                             : 5.0f;
        RayCollision orbHit = GetRayCollisionSphere(ray, vomitOrbPosition, orbHitRadius);
        if (orbHit.hit) {
            float bodyDist = 999999.0f;
            bool bodyHit = Enemy::RayHit(ray, bodyDist);
            if (!bodyHit || orbHit.distance <= bodyDist) {
                outDist = orbHit.distance;
                lastRayHitVomitOrb = true;
                return true;
            }
        }
    }
    return Enemy::RayHit(ray, outDist);
}

void GibonRzygacz::TakeDamage(int damage) {
    if (lastRayHitVomitShield) {
        lastRayHitVomitShield = false;
        return;
    }

    if (lastRayHitVomitOrb && vomitOrbHp > 0 && 
        (vomitOrbState == VomitOrbState::CHARGING || vomitOrbState == VomitOrbState::READY)) {
        vomitOrbHp -= damage;
        if (vomitOrbHp < 0) vomitOrbHp = 0;
        lastRayHitVomitOrb = false;
        return;
    }
    Enemy::TakeDamage(damage);
}

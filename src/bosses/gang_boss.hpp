#pragma once
#include "../enemies/enemy.hpp"
#include <vector>
#include <array>

enum class GangCutscene { WARDROBE_FALLING, WARDROBE_LANDED, PORTAL_OPENING, GUYS_WALKING_OUT, FIGHT };

struct GangMember {
    Vector3 position;
    float angle;
    int hp;
    int maxHp;
    bool alive;
    float attackTimer;
    float walkTimer;
    bool isMoving;
    int targetPlayerIdx;     // Which player they're targeting
    float batSwingAngle;     // For bat swing animation
    bool isSwinging;
    float specialTimer;      // For special attack cooldown
};

class GangBoss : public Enemy {
public:
    GangBoss(Vector3 spawnPos, int enemyId);
    
    void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) override;
    void Draw() override;
    void DrawHUD(Camera3D camera) override;
    void HandleCollision(BoundingBox box) override;
    BoundingBox GetBoundingBox() override;
    
    bool RayHit(Ray ray, float& outDist) override;
    void TakeDamage(int amt) override;
    void TakeAOEDamage(Vector3 center, float splashRadius, int maxDamage) override;
    
    GangCutscene cutsceneState;
    float stateTimer;
    
    // Wardrobe
    Vector3 wardrobePos;
    float wardrobeFallHeight;
    float doorAngle;
    float portalPulse;
    
    // 5 gang members
    std::array<GangMember, 5> members;
    int membersOut;           // How many have walked out
    float nextMemberTimer;    // Timer for next member to walk out
    int lastHitMemberIdx = -1; // Added for collision tracking
    
    // Track total HP for the HUD bar
    int GetTotalHP() const;
    int GetTotalMaxHP() const;
    bool AnyAlive() const;
};

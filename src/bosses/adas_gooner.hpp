#pragma once
#include "../enemies/enemy.hpp"

enum class CutsceneState { WARDROBE_CLOSED, WARDROBE_OPENING, WALKING_OUT, FINISHED, RETREATING, WARDROBE_RETREAT_OPENING, WARDROBE_CLOSING, RETREATED };

class AdasGooner : public Enemy {
public:
    AdasGooner(Vector3 startPos, int enemyId);
    
    void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) override;
    void Draw() override;
    BoundingBox GetBoundingBox() override;

    static Model wafelModel;
    static bool wafelModelLoaded;
    static bool globalUseWafelModel;

    static void LoadSharedResources();
    static void UnloadSharedResources();
    
    CutsceneState cutsceneState;
    float cutsceneTimer;
    Vector3 wardrobePos;
    
    float shockwaveTimer = 0.0f;
    float summonTimer = 0.0f;
    bool isEnraged = false;
    float smokeDamageTimer = 0.0f;
    float smokePulseTimer = 0.0f;
    float smokeCooldown = 15.0f;
    float smokeActiveTimer = 0.0f;
    bool isSprayingSmoke = false;

    // White laser special attack
    float laserCooldown = 20.0f;   // Time until next laser (starts after 20s)
    float laserChargeTimer = 0.0f; // Charge-up before firing
    float laserFireTimer = 0.0f;   // How long the laser has been firing
    bool laserFiring = false;      // Is the laser currently active
    bool laserCharging = false;    // Is the laser charging up
    Vector3 laserTargetPos = {0, 0, 0}; // Where the laser is aimed

    // Explosive snus attack
    struct SnusProjectile {
        Vector3 position;
        Vector3 velocity;
        float radius;
        float lifetime;
        bool active;
    };
    std::vector<SnusProjectile> snusProjectiles;
    float snusCooldown = 12.0f;
    float snusShootTimer = 0.0f;
    int snusShotsFired = 0;
    bool isShootingSnus = false;
};

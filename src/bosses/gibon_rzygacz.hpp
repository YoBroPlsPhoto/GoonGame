#pragma once
#include "../enemies/enemy.hpp"
#include <vector>

enum class GibonState { FALLING, IMPACT, CRATER_PAUSE, ROLLING, FINISHED_FALLING };

struct ToxicVomit {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    bool active;
};

struct Crater {
    Vector3 position;
    float radius;
};

class GibonRzygacz : public Enemy {
public:
    GibonRzygacz(Vector3 landingPos, int enemyId);
    
    void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) override;
    void Draw() override;
    void DrawHUD(Camera3D camera) override;
    BoundingBox GetBoundingBox() override;
    
    GibonState gibonState;
    float stateTimer;
    Vector3 landingTarget;   // Where the ball will land
    float fallHeight;        // Current height during fall
    float rollAngle;         // Visual rotation while rolling
    float rollSpeed;         // Current rolling speed
    
    // Toxic vomit rain attack
    std::vector<ToxicVomit> vomitProjectiles;
    float vomitCooldown;
    
    // FPS stutter effect
    float stutterTimer;
    float nextStutterTime;
    bool isStuttering;
    
    // Crater
    Crater impactCrater;
    bool craterCreated;
    
    // Visual
    float pulseTimer;
    float bodyScale;
    Color toxicColor;
};

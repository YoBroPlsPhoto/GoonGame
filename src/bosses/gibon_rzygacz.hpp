#pragma once
#include "../enemies/enemy.hpp"
#include <vector>

enum class GibonState { FALLING, IMPACT, CRATER_PAUSE, ROLLING, FINISHED_FALLING };

enum class GibonJumpState { NONE, WINDUP, AIR, IMPACT };


enum class VomitOrbState {
    INACTIVE,        // Kula nie istnieje
    CHARGING,        // Gibon laduje kule
    READY,           // Kula naladowana, czeka na zniszczenie lub koniec timera
    DESTROYED,       // Kula zniszczona - animacja wybuchu
    FLYING,          // Kula leci do bazy
    EXPLODED_BASE    // Kula wybuchla przy bazie
};

struct ToxicVomit {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    bool active;
};

struct VomitPuddle {
    Vector3 position;
    float radius;
    float lifetime;
    float damageTimer;
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
    bool RayHit(Ray ray, float& outDist) override;
    void TakeDamage(int damage) override;
    
    GibonState gibonState;
    float stateTimer;
    Vector3 landingTarget;   // Where the ball will land
    float fallHeight;        // Current height during fall
    float rollAngle;         // Visual rotation while rolling
    float rollSpeed;         // Current rolling speed
    
    // Toxic vomit rain attack
    std::vector<ToxicVomit> vomitProjectiles;
    std::vector<VomitPuddle> vomitPuddles;
    float vomitCooldown;

    // Charged vomit orb attack below half HP
    bool vomitOrbTriggered;
    bool lastRayHitVomitOrb;
    int vomitOrbHp;
    int vomitOrbMaxHp;
    Vector3 vomitOrbPosition;
    
    // Vomit orb state machine
    VomitOrbState vomitOrbState;
    float vomitOrbChargeProgress;    // 0-1 charging progress
    float vomitOrbChargeTime;        // Total charge duration (5s)
    float vomitOrbReadyTimer;        // Countdown while READY (25s)
    float vomitOrbExplosionTimer;    // Timer for explosion animation
    float vomitOrbFlyTimer;          // Time spent flying
    Vector3 vomitOrbStartPos;        // Start position for flight arc
    Vector3 vomitOrbTargetPos;       // Target position (base)
    Vector3 vomitOrbVelocity;        // Current velocity during flight
    float vomitOrbGibonExpTimer;     // Timer for explosion at gibon
    bool vomitOrbGibonExploding;     // Is gibon-side explosion active
    
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

    // Jump Attack
    GibonJumpState jumpState;
    float jumpAttackCooldown;
    float jumpAttackTimer;
    Vector3 jumpStartPos;
    Vector3 jumpTargetPos;
};


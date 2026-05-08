#pragma once
#include "../enemies/enemy.hpp"

enum class PrimeCutscene { WARDROBE_CLOSED, WARDROBE_OPENING, WALKING_OUT, FIGHT };

class AdasPrime : public Enemy {
public:
    AdasPrime(Vector3 startPos, int enemyId);
    
    void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) override;
    void Draw() override;
    BoundingBox GetBoundingBox() override;
    
    PrimeCutscene cutsceneState;
    float cutsceneTimer;
    Vector3 wardrobePos;
    
    float shockwaveTimer = 0.0f;
    float summonTimer = 0.0f;
    float auraTimer = 0.0f;
    bool isEnraged = false;
};

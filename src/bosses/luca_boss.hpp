#pragma once
#include "../enemies/enemy.hpp"

class LucaBoss : public Enemy {
public:
    LucaBoss(Vector3 startPos, int enemyId);

    void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos) override;
    void Draw() override;
    BoundingBox GetBoundingBox() override;

    Vector3 portalPos;
    float portalTimer = 0.0f;
    float auraTimer = 0.0f;
};

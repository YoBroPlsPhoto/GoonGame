#pragma once
#include "../enemies/enemy.hpp"

enum class CutsceneState { WARDROBE_CLOSED, WARDROBE_OPENING, WALKING_OUT, FINISHED };

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
};

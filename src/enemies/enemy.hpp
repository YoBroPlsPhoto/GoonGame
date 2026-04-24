#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "raylib.h"
#include <vector>
#include "../player/player.hpp"

enum class EnemyType { MELEE, SHOOTER, FAST, TANK, BOSS, MINION, ELITE, GIGA_TANK, KAMIKAZE };
enum class WeaponType { KATANA, MACHETE, PISTOL, SHOTGUN, EXPLOSIVE };

struct TargetInfo {
    Vector3 pos;
    int* hp;
    bool active;
};

class Enemy {
public:
    Vector3 position;
    float speed;
    float radius;
    bool active;
    Color color;
    int hp;
    int maxHp;
    int id;
    EnemyType type;
    WeaponType weapon;
    float angle;
    bool isMoving;
    Vector3 velocity;

    Enemy(Vector3 startPos, EnemyType t, WeaponType w, int enemyId);
    virtual ~Enemy() = default;
    virtual void Update(const std::vector<TargetInfo>& players, float* baseHp, Vector3 basePos);
    virtual void Draw();
    virtual void DrawHUD(Camera3D camera);
    virtual void HandleCollision(BoundingBox box);
    virtual BoundingBox GetBoundingBox();

    float attackTimer;
    float walkTimer; // To keep track of movement phase
private:
    float attackCooldown;
};

#endif

#ifndef STRUCTURE_HPP
#define STRUCTURE_HPP

#include "raylib.h"
#include <vector>
#include <memory>

enum class StructureType { WALL, TURRET };

struct Structure {
    Vector3 position;
    int hp;
    int maxHp;
    StructureType type;
    float attackTimer = 0;
    bool active = true;
    Color color;
    float turretAngle = 0.0f;      // Smooth turret rotation
    float targetAngle = 0.0f;      // Desired angle toward enemy
    float damageFlash = 0.0f;      // Flash red when hit
    int lastHp = 0;                // Track HP changes for flash
    int ownerId = 0;               // Player who placed it (for kill rewards)
    float barrelRecoil = 0.0f;     // Visual recoil on fire

    BoundingBox GetBoundingBox() const;
    void Draw();
};

extern std::vector<std::shared_ptr<Structure>> structures;

#endif

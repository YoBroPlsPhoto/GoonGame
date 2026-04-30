#ifndef KNIFE_HPP
#define KNIFE_HPP

#include "weapon.hpp"

class Knife : public Weapon {
public:
    Knife();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    bool CanFire() override { return currentCooldown <= 0; }
    void Fire() override;
    bool IsAutomatic() override { return true; } // allow holding down for stabbing
};

#endif

#ifndef MINIGUN_HPP
#define MINIGUN_HPP

#include "weapon.hpp"

class Minigun : public Weapon {
public:
    Minigun();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;
    bool IsAutomatic() override { return true; }

private:
    float gunBob;
    float rotationAngle;
};

#endif

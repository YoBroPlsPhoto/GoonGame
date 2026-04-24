#ifndef GLOCK_HPP
#define GLOCK_HPP

#include "weapon.hpp"

class Glock : public Weapon {
public:
    Glock();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;

private:
    float gunBob;
};

#endif

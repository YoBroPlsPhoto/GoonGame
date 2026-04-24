#ifndef REVOLVER_HPP
#define REVOLVER_HPP

#include "weapon.hpp"

class Revolver : public Weapon {
public:
    Revolver();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;

private:
    float gunBob;
    float cylinderAngle; // rotating cylinder
};

#endif

#ifndef SHOTGUN_HPP
#define SHOTGUN_HPP

#include "weapon.hpp"

class Shotgun : public Weapon {
public:
    Shotgun();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;
    
    int GetPelletCount() const { return 8; }
    float GetSpreadAngle() const { return 5.0f; } // degrees

private:
    float gunBob;
    float pumpTimer; // pump-action animation
};

#endif

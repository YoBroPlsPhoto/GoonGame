#ifndef AK47_HPP
#define AK47_HPP

#include "weapon.hpp"

class AK47 : public Weapon {
public:
    AK47();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;
    bool IsAutomatic() override { return true; }

private:
    float gunBob;
};

#endif

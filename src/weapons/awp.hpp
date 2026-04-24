#ifndef AWP_HPP
#define AWP_HPP

#include "weapon.hpp"

class AWP : public Weapon {
public:
    AWP();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;
    float GetFOV(float preferredFOV) override;
    
    int scopeLevel = 0; // 0=none, 1=zoom1, 2=zoom2

private:
    float gunBob;
};

#endif

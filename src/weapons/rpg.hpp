#ifndef RPG_HPP
#define RPG_HPP

#include "weapon.hpp"

class RPG : public Weapon {
public:
    RPG();
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D camera) override;
    void Fire() override;
    
    float GetExplosionRadius() const { return 8.0f; }
    float GetProjectileSpeed() const { return 80.0f; }

private:
    float gunBob;
};

#endif

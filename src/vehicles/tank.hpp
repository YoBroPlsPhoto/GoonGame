#ifndef TANK_HPP
#define TANK_HPP

#include "vehicle.hpp"

class Tank : public Vehicle {
public:
    float turretRotation; // independent turret aim
    float barrelPitch;    // up/down barrel angle
    float cannonCooldown;
    float mgCooldown;
    int cannonDamage;
    int mgDamage;
    float muzzleFlash;   // timer for muzzle flash
    float recoilOffset;  // for barrel kickback animation
    
    Tank(Vector3 pos);
    
    void Update(float dt) override;
    void Draw() override;
    BoundingBox GetBoundingBox() override;
    
    bool CanFireCannon() const { return cannonCooldown <= 0; }
    bool CanFireMG() const { return mgCooldown <= 0; }
    void FireCannon();
    void FireMG();
    
    // Get the barrel tip position in world space for projectile spawning
    Vector3 GetBarrelTip() const;
    Vector3 GetTurretForward() const;
    Vector3 GetMGTip() const;
    
    // Interaction
    float explosionTimer; // for destruction?

private:
    float trackAnimation;
};

#endif

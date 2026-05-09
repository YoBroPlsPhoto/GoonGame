#ifndef WEAPON_HPP
#define WEAPON_HPP

#include "raylib.h"

class Weapon {
public:
    int damage = 0;
    float attackCooldown = 0.0f;
    float currentCooldown = 0.0f;
    float recoilTimer = 0.0f;
    const char* name = "Weapon";
    bool isMelee = false;

    // Ammo system
    int magSize = 30;
    int currentAmmo = 30;
    int reserveAmmo = 120;
    int maxReserve = 300;
    int ammoPrice = 500;
    float reloadTime = 2.0f;
    float reloadTimer = 0.0f;
    bool isReloading = false;

    virtual ~Weapon() = default;
    virtual void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) = 0;
    virtual void DrawViewModel(Camera3D camera) = 0;
    
    virtual bool CanFire() {
        return currentCooldown <= 0 && !isReloading && currentAmmo > 0;
    }
    
    virtual void Fire() {
        currentCooldown = attackCooldown;
        currentAmmo--;
    }

    virtual bool IsAutomatic() {
        return false;
    }

    virtual float GetFOV(float preferredFOV) {
        return preferredFOV;
    }

    void StartReload() {
        if (isReloading || currentAmmo == magSize || reserveAmmo <= 0) return;
        isReloading = true;
        reloadTimer = reloadTime;
    }

    void UpdateReload(float dt) {
        if (!isReloading) return;
        reloadTimer -= dt;
        if (reloadTimer <= 0) {
            isReloading = false;
            int needed = magSize - currentAmmo;
            int toLoad = (reserveAmmo >= needed) ? needed : reserveAmmo;
            currentAmmo += toLoad;
            reserveAmmo -= toLoad;
        }
    }

    // Auto-reload when empty
    void CheckAutoReload() {
        if (currentAmmo <= 0 && reserveAmmo > 0 && !isReloading) {
            StartReload();
        }
    }

    void RefillAmmo() {
        reserveAmmo = maxReserve;
    }
};

#endif

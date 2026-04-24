#include "shotgun.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Shotgun::Shotgun() {
    damage = 30; // per pellet, 8 pellets = 240 max
    attackCooldown = 0.8f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    pumpTimer = 0.0f;
    magSize = 8; currentAmmo = 8; reserveAmmo = 40; maxReserve = 120; ammoPrice = 600; reloadTime = 2.0f;
}

void Shotgun::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;
    if (pumpTimer > 0) pumpTimer -= dt;

    gunBob = sinf((float)GetTime() * 8.0f) * 0.01f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 2.0f;
    else if (!isMoving) gunBob = 0;
}

void Shotgun::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.25f;
    pumpTimer = 0.5f; // pump animation lasts half a second
    currentAmmo--;
}

void Shotgun::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.25f) * 0.3f : 0.0f;
    
    rlTranslatef(0.3f, -0.3f + gunBob, 0.4f - recoil);

    // Hand
    DrawCube((Vector3){0, -0.12f, -0.05f}, 0.14f, 0.14f, 0.18f, {255, 200, 150, 255});
    
    // Stock (wooden brown)
    DrawCube((Vector3){0, -0.08f, -0.3f}, 0.07f, 0.12f, 0.35f, {139, 90, 43, 255});
    
    // Receiver (dark metal)
    DrawCube((Vector3){0, 0.0f, 0.0f}, 0.08f, 0.1f, 0.25f, {40, 40, 45, 255});
    
    // Barrel (long tube)
    DrawCube((Vector3){0, 0.02f, 0.45f}, 0.05f, 0.05f, 0.7f, BLACK);
    
    // Second barrel (double-barrel look)
    DrawCube((Vector3){0, 0.07f, 0.45f}, 0.05f, 0.05f, 0.7f, BLACK);
    
    // Pump/Forend
    float pumpOffset = (pumpTimer > 0.25f) ? -0.15f * ((pumpTimer - 0.25f) / 0.25f) : 
                       (pumpTimer > 0) ? -0.15f * (1.0f - pumpTimer / 0.25f) : 0.0f;
    DrawCube((Vector3){0, 0.0f, 0.2f + pumpOffset}, 0.09f, 0.09f, 0.15f, {160, 110, 60, 255});
    
    // Muzzle
    DrawCube((Vector3){0, 0.045f, 0.81f}, 0.06f, 0.06f, 0.02f, DARKGRAY);

    rlPopMatrix();
}

#include "ak47.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

AK47::AK47() {
    damage = 45;
    attackCooldown = 0.1f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    magSize = 30; currentAmmo = 30; reserveAmmo = 180; maxReserve = 300; ammoPrice = 500; reloadTime = 2.5f;
}

void AK47::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;

    gunBob = sinf((float)GetTime() * 12.0f) * 0.015f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 2.5f;
    else if (!isMoving) gunBob = 0;
}

void AK47::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.08f;
    currentAmmo--;
}

void AK47::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.08f) * 0.12f : 0.0f;
    
    rlTranslatef(-0.35f, -0.3f + gunBob, 0.5f - recoil); 

    // Hand
    DrawCube((Vector3){0, -0.1f, -0.05f}, 0.12f, 0.12f, 0.15f, {255, 200, 150, 255});
    
    // Receiver (Main body)
    DrawCube((Vector3){0, 0, 0}, 0.1f, 0.15f, 0.6f, {60, 60, 60, 255});
    
    // Wooden Stock
    DrawCube((Vector3){0, -0.05f, -0.4f}, 0.08f, 0.12f, 0.35f, {100, 50, 20, 255});
    
    // Wooden Grip
    rlPushMatrix();
    rlTranslatef(0, -0.15f, 0);
    rlRotatef(-20, 1, 0, 0);
    DrawCube((Vector3){0, -0.05f, 0}, 0.07f, 0.15f, 0.07f, {100, 50, 20, 255});
    rlPopMatrix();
    
    // Wooden Handguard
    DrawCube((Vector3){0, -0.02f, 0.45f}, 0.09f, 0.1f, 0.3f, {100, 50, 20, 255});
    
    // Barrel
    DrawCube((Vector3){0, 0.03f, 0.7f}, 0.04f, 0.04f, 0.5f, BLACK);
    
    // Magazine (Curved look)
    rlPushMatrix();
    rlTranslatef(0, -0.2f, 0.3f);
    rlRotatef(15, 1, 0, 0);
    DrawCube((Vector3){0, -0.1f, 0}, 0.07f, 0.25f, 0.12f, {40, 40, 40, 255});
    rlPopMatrix();

    rlPopMatrix();
}

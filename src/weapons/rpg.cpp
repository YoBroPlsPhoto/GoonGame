#include "rpg.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

RPG::RPG() {
    damage = 300;
    attackCooldown = 3.0f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    magSize = 1; currentAmmo = 1; reserveAmmo = 5; maxReserve = 10; ammoPrice = 1000; reloadTime = 3.5f;
}

void RPG::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;

    gunBob = sinf((float)GetTime() * 5.0f) * 0.006f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 1.5f;
    else if (!isMoving) gunBob = 0;
}

void RPG::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.4f;
    currentAmmo--;
}

void RPG::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.4f) * 0.5f : 0.0f;
    
    // RPG sits on shoulder, offset to the right and up
    rlTranslatef(0.35f, -0.15f + gunBob, 0.3f - recoil);

    // Hand gripping the handle
    DrawCube((Vector3){-0.05f, -0.15f, -0.1f}, 0.14f, 0.14f, 0.16f, {255, 200, 150, 255});
    
    // Main tube (olive drab)
    DrawCube((Vector3){0, 0, 0}, 0.12f, 0.12f, 1.0f, {85, 107, 47, 255});
    
    // Tube front opening (dark)
    DrawCube((Vector3){0, 0, 0.51f}, 0.13f, 0.13f, 0.02f, {40, 40, 40, 255});
    DrawCube((Vector3){0, 0, 0.50f}, 0.09f, 0.09f, 0.02f, {20, 20, 20, 255});
    
    // Tube rear (exhaust port)
    DrawCube((Vector3){0, 0, -0.51f}, 0.13f, 0.13f, 0.02f, {60, 60, 60, 255});
    
    // Grip / trigger guard
    DrawCube((Vector3){-0.02f, -0.1f, -0.05f}, 0.05f, 0.1f, 0.06f, {60, 60, 60, 255});
    
    // Front sight
    DrawCube((Vector3){0, 0.1f, 0.4f}, 0.015f, 0.06f, 0.015f, {60, 60, 60, 255});
    
    // Rear sight
    DrawCube((Vector3){0, 0.1f, -0.2f}, 0.015f, 0.04f, 0.015f, {60, 60, 60, 255});
    
    // Warhead visible in tube (if not recently fired)
    if (recoilTimer <= 0) {
        DrawCube((Vector3){0, 0, 0.35f}, 0.08f, 0.08f, 0.2f, {60, 70, 60, 255});
        // Warhead tip (pointed)
        DrawCube((Vector3){0, 0, 0.47f}, 0.05f, 0.05f, 0.06f, {180, 50, 50, 255});
    }
    
    // Shoulder rest
    DrawCube((Vector3){0, -0.02f, -0.4f}, 0.14f, 0.08f, 0.1f, {70, 80, 50, 255});

    rlPopMatrix();
}

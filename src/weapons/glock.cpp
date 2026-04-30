#include "glock.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Glock::Glock() {
    damage = 35;
    attackCooldown = 0.3f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    magSize = 17; currentAmmo = 17; reserveAmmo = 120; maxReserve = 300; ammoPrice = 200; reloadTime = 1.5f;
}

void Glock::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;

    gunBob = sinf((float)GetTime() * 10.0f) * 0.01f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 3.0f;
    else if (!isMoving) gunBob = 0; // stop bobbing if completely still
}

void Glock::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.15f;
    currentAmmo--;
}

void Glock::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    // Calculate rotation to match camera angle
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.15f) * 0.15f : 0.0f;
    
    rlTranslatef(-0.3f, -0.25f + gunBob, 0.4f - recoil); 

    // Hand
    DrawCube((Vector3){0, -0.1f, -0.05f}, 0.12f, 0.12f, 0.15f, {255, 200, 150, 255});
    
    // Handle
    DrawCube((Vector3){0, -0.15f, 0}, 0.06f, 0.15f, 0.1f, DARKGRAY);
    // Barrel
    DrawCube((Vector3){0, 0, 0.1f}, 0.07f, 0.08f, 0.3f, BLACK);
    // Slide (Animates back when firing)
    if (recoilTimer > 0.07f) {
        DrawCube((Vector3){0, 0.03f, 0.05f}, 0.075f, 0.02f, 0.25f, GRAY); // Slide back
    } else {
        DrawCube((Vector3){0, 0.03f, 0.1f}, 0.075f, 0.02f, 0.3f, GRAY); // Slide forward
    }

    rlPopMatrix();
}

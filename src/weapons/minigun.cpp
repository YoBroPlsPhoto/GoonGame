#include "minigun.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Minigun::Minigun() {
    damage = 25;
    attackCooldown = 0.04f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    rotationAngle = 0.0f;
    magSize = 200; currentAmmo = 200; reserveAmmo = 600; maxReserve = 1000; ammoPrice = 800; reloadTime = 5.0f;
}

void Minigun::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) {
        recoilTimer -= dt;
        rotationAngle += dt * 1000.0f;
    }

    gunBob = sinf((float)GetTime() * 8.0f) * 0.005f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 2.0f;
    else if (!isMoving) gunBob = 0;
}

void Minigun::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.1f;
    currentAmmo--;
}

void Minigun::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? 0.05f : 0.0f;
    
    rlTranslatef(0.4f, -0.4f + gunBob, 0.4f - recoil); 

    // Hand (Holding from top)
    DrawCube((Vector3){0, 0.15f, 0.1f}, 0.12f, 0.12f, 0.15f, {255, 200, 150, 255});
    
    // Main Body
    DrawCube((Vector3){0, 0, 0}, 0.25f, 0.25f, 0.5f, DARKGRAY);
    
    // Rotating Barrels
    rlPushMatrix();
    rlTranslatef(0, 0, 0.5f);
    rlRotatef(rotationAngle, 0, 0, 1);
    
    for (int i = 0; i < 6; i++) {
        float bAngle = i * (360.0f / 6.0f) * DEG2RAD;
        float bx = cosf(bAngle) * 0.08f;
        float by = sinf(bAngle) * 0.08f;
        DrawCube((Vector3){bx, by, 0.35f}, 0.03f, 0.03f, 0.7f, GRAY);
    }
    rlPopMatrix();

    rlPopMatrix();
}

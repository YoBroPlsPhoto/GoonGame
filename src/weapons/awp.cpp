#include "awp.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

AWP::AWP() {
    damage = 500;
    attackCooldown = 1.5f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    magSize = 5; currentAmmo = 5; reserveAmmo = 25; maxReserve = 50; ammoPrice = 1500; reloadTime = 3.0f;
}

void AWP::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;

    // Toggle scope with right mouse button (3 stages: 0, 1, 2)
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        scopeLevel = (scopeLevel + 1) % 3;
    }

    gunBob = sinf((float)GetTime() * 6.0f) * 0.008f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 1.8f;
    else if (!isMoving) gunBob = 0;
    
    // Less bob when scoped
    // Less bob when scoped
    if (scopeLevel > 0) gunBob *= 0.1f;
}

void AWP::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.3f;
    currentAmmo--;
}

void AWP::DrawViewModel(Camera3D camera) {
    if (scopeLevel > 0) return; // Hide model when zoomed in
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.3f) * 0.4f : 0.0f;
    
    rlTranslatef(0.3f, -0.35f + gunBob, 0.4f - recoil); 

    // Hand
    DrawCube((Vector3){0, -0.1f, -0.1f}, 0.12f, 0.12f, 0.15f, {255, 200, 150, 255});
    
    // Main Body (Greenish)
    DrawCube((Vector3){0, 0, 0}, 0.08f, 0.15f, 0.8f, {50, 70, 50, 255});
    
    // Barrel (Long and thin)
    DrawCube((Vector3){0, 0.04f, 0.9f}, 0.03f, 0.03f, 1.0f, BLACK);
    
    // Scope
    rlPushMatrix();
    rlTranslatef(0, 0.12f, 0.1f);
    DrawCube((Vector3){0, 0, 0}, 0.06f, 0.06f, 0.4f, DARKGRAY);
    // Scope Lens
    DrawCube((Vector3){0, 0, 0.2f}, 0.07f, 0.07f, 0.02f, BLACK);
    DrawCube((Vector3){0, 0, -0.2f}, 0.07f, 0.07f, 0.02f, BLACK);
    rlPopMatrix();
    
    // Bolt handle
    rlPushMatrix();
    rlTranslatef(0.06f, 0.05f, -0.1f);
    if (recoilTimer > 0.15f) rlRotatef(-45, 0, 0, 1);
    DrawCube((Vector3){0.05f, 0, 0}, 0.1f, 0.02f, 0.02f, GRAY);
    rlPopMatrix();

    rlPopMatrix();
}

float AWP::GetFOV(float preferredFOV) {
    if (scopeLevel == 1) return 30.0f; // Zoom 1
    if (scopeLevel == 2) return 10.0f; // Zoom 2
    return preferredFOV;
}

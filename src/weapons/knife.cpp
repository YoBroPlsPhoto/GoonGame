#include "knife.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Knife::Knife() {
    damage = 50;
    attackCooldown = 0.4f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    magSize = 0; // indicates melee
    currentAmmo = 1;
    reserveAmmo = 0;
    maxReserve = 0;
    ammoPrice = 0;
    reloadTime = 0.0f;
}

void Knife::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;
}

void Knife::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.25f; // time for swing animation
}

void Knife::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float swingProgress = 0.0f;
    if (recoilTimer > 0) {
        swingProgress = sinf((recoilTimer / 0.25f) * PI);
    }
    
    // Right side is -0.3f
    rlTranslatef(-0.3f + swingProgress * 0.25f, -0.2f - swingProgress * 0.1f, 0.4f - swingProgress * 0.4f);
    rlRotatef(swingProgress * 80.0f, 0, 1, 0); // Slash rotation
    rlRotatef(-60.0f + swingProgress * 45.0f, 1, 0, 0); // Point blade forward like a karambit
    rlRotatef(15.0f, 0, 0, 1);

    // Hand
    DrawCube((Vector3){0, -0.05f, 0.02f}, 0.12f, 0.12f, 0.12f, {255, 200, 150, 255});
    
    // Tactical Handle
    DrawCube((Vector3){0, 0, 0}, 0.035f, 0.14f, 0.04f, BLACK);
    
    // Ring at the bottom
    DrawCube((Vector3){0, -0.08f, 0.01f}, 0.04f, 0.04f, 0.04f, DARKGRAY);
    DrawCube((Vector3){0, -0.08f, 0.01f}, 0.02f, 0.02f, 0.02f, BLACK); // Inner hole

    // Guard/Connector
    DrawCube((Vector3){0, 0.07f, 0.02f}, 0.05f, 0.03f, 0.06f, DARKGRAY);
    
    // Curved Red Blade (Karambit style)
    DrawCube((Vector3){0, 0.14f, 0.05f}, 0.015f, 0.12f, 0.06f, MAROON); // main blade
    DrawCube((Vector3){0, 0.20f, 0.07f}, 0.01f, 0.04f, 0.04f, RED);   // tip

    rlPopMatrix();
}

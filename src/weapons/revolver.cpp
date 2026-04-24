#include "revolver.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Revolver::Revolver() {
    damage = 120;
    attackCooldown = 0.6f;
    currentCooldown = 0.0f;
    recoilTimer = 0.0f;
    gunBob = 0.0f;
    cylinderAngle = 0.0f;
    magSize = 6; currentAmmo = 6; reserveAmmo = 36; maxReserve = 120; ammoPrice = 400; reloadTime = 2.0f;
}

void Revolver::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
    if (recoilTimer > 0) recoilTimer -= dt;

    gunBob = sinf((float)GetTime() * 9.0f) * 0.01f;
    if (!isGrounded || Vector2Length(mouseDelta) > 0 || (isSprinting && isMoving)) gunBob *= 2.5f;
    else if (!isMoving) gunBob = 0;
}

void Revolver::Fire() {
    currentCooldown = attackCooldown;
    recoilTimer = 0.2f;
    cylinderAngle += 60.0f; // 360/6 = 60 degrees per shot
    currentAmmo--;
}

void Revolver::DrawViewModel(Camera3D camera) {
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    rlPushMatrix();
    rlTranslatef(camera.position.x, camera.position.y, camera.position.z);
    
    float angleY = atan2f(lookDir.x, lookDir.z) * RAD2DEG;
    float angleX = asinf(lookDir.y) * RAD2DEG;

    rlRotatef(angleY, 0, 1, 0);
    rlRotatef(-angleX, 1, 0, 0);

    float recoil = (recoilTimer > 0) ? (recoilTimer / 0.2f) * 0.2f : 0.0f;
    // Strong upward kick for revolver
    float kickUp = (recoilTimer > 0) ? (recoilTimer / 0.2f) * 0.08f : 0.0f;
    
    rlTranslatef(0.3f, -0.25f + gunBob + kickUp, 0.4f - recoil);

    // Hand
    DrawCube((Vector3){0, -0.12f, -0.08f}, 0.14f, 0.14f, 0.16f, {255, 200, 150, 255});
    
    // Grip (dark wood)
    DrawCube((Vector3){0, -0.18f, -0.02f}, 0.06f, 0.16f, 0.08f, {80, 50, 30, 255});
    
    // Frame (silver/steel)
    DrawCube((Vector3){0, 0.0f, 0.05f}, 0.06f, 0.08f, 0.2f, {180, 180, 190, 255});
    
    // Cylinder (rotating)
    rlPushMatrix();
    rlTranslatef(0, 0.0f, 0.02f);
    rlRotatef(cylinderAngle, 0, 0, 1); // Rotate around barrel axis
    // Main cylinder body
    DrawCube((Vector3){0, 0, 0}, 0.1f, 0.1f, 0.08f, {160, 160, 170, 255});
    // Chamber holes (6 small dark spots around the cylinder)
    for (int i = 0; i < 6; i++) {
        float a = (float)i * 60.0f * DEG2RAD;
        float cx = cosf(a) * 0.035f;
        float cy = sinf(a) * 0.035f;
        DrawCube((Vector3){cx, cy, 0.04f}, 0.02f, 0.02f, 0.01f, {30, 30, 30, 255});
    }
    rlPopMatrix();
    
    // Barrel (long)
    DrawCube((Vector3){0, 0.02f, 0.2f}, 0.04f, 0.04f, 0.25f, {180, 180, 190, 255});
    
    // Front sight
    DrawCube((Vector3){0, 0.05f, 0.33f}, 0.015f, 0.03f, 0.015f, BLACK);
    
    // Hammer
    float hammerAngle = (recoilTimer > 0.1f) ? -30.0f : 0.0f;
    rlPushMatrix();
    rlTranslatef(0, 0.05f, -0.05f);
    rlRotatef(hammerAngle, 1, 0, 0);
    DrawCube((Vector3){0, 0.02f, 0}, 0.02f, 0.04f, 0.02f, {160, 160, 170, 255});
    rlPopMatrix();

    rlPopMatrix();
}

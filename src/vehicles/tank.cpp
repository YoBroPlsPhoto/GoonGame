#include "tank.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>
#include <initializer_list>

Tank::Tank(Vector3 pos) : Vehicle(pos, VehicleType::SUV) {
    // Override vehicle stats for tank
    maxSpeed = 0.85f; 
    acceleration = 0.015f;
    health = 50000;
    maxHealth = 50000;
    bodyColor = {45, 60, 35, 255}; // Darker military olive
    
    turretRotation = 0.0f;
    barrelPitch = 0.0f;
    cannonCooldown = 0.0f;
    mgCooldown = 0.0f;
    cannonDamage = 8500;
    mgDamage = 50;
    trackAnimation = 0.0f;
    muzzleFlash = 0.0f;
    recoilOffset = 0.0f;
}

void Tank::Update(float dt) {
    if (!isOccupied) {
        speed *= 0.95f;
        if (fabsf(speed) < 0.001f) speed = 0;
    } else {
        // --- NORMAL CONTROLS ---
        steerAngle = 0;
        if (IsKeyDown(KEY_A)) steerAngle = -3.5f; 
        if (IsKeyDown(KEY_D)) steerAngle = 3.5f; 
        
        if (fabsf(speed) > 0.01f) {
            rotation += steerAngle * (speed / maxSpeed);
        }
        
        if (IsKeyDown(KEY_W)) { 
            speed += acceleration;
            if (speed > maxSpeed) speed = maxSpeed;
        } else if (IsKeyDown(KEY_S)) { 
            speed -= acceleration * 1.5f;
            if (speed < -maxSpeed * 0.4f) speed = -maxSpeed * 0.4f;
        } else {
            speed *= 0.98f;
        }

        // Turret and Cannon are now controlled by the camera in main.cpp
        
        trackAnimation += speed * 40.0f * dt;
    }
    
    // Animation cooldowns
    if (cannonCooldown > 0) cannonCooldown -= dt;
    if (mgCooldown > 0) mgCooldown -= dt;
    if (muzzleFlash > 0) muzzleFlash -= dt;
    
    // Recoil recovery
    recoilOffset = Lerp(recoilOffset, 0, dt * 5.0f);
    
    // Move forward (Base movement)
    Vector3 forward = GetForward();
    position.x += forward.x * speed;
    position.z += forward.z * speed;
    position.y = 0.5f;
}

void Tank::FireCannon() {
    cannonCooldown = 1.2f;
    muzzleFlash = 0.15f;
    recoilOffset = 1.5f;
}

void Tank::FireMG() {
    mgCooldown = 0.05f;
}

Vector3 Tank::GetTurretForward() const {
    float totalRot = (rotation + turretRotation) * DEG2RAD;
    float pitchRad = barrelPitch * DEG2RAD;
    return {
        sinf(totalRot) * cosf(pitchRad),
        sinf(pitchRad),
        cosf(totalRot) * cosf(pitchRad)
    };
}

Vector3 Tank::GetBarrelTip() const {
    Vector3 forward = GetTurretForward();
    float barrelLen = 10.0f - recoilOffset; // Barrel gets shorter when firing
    return {
        position.x + forward.x * barrelLen,
        position.y + 4.2f + forward.y * barrelLen,
        position.z + forward.z * barrelLen
    };
}

Vector3 Tank::GetMGTip() const {
    Vector3 forward = GetTurretForward();
    return {
        position.x + forward.x * 5.0f,
        position.y + 5.0f + forward.y * 5.0f,
        position.z + forward.z * 5.0f
    };
}

BoundingBox Tank::GetBoundingBox() {
    return {
        {position.x - 6.0f, 0, position.z - 8.0f},
        {position.x + 6.0f, 7.0f, position.z + 8.0f}
    };
}

void Tank::Draw() {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(rotation, 0, 1, 0);
    
    // --- HULL (GIGA SIZE) ---
    // Main hull body (wider, taller)
    DrawCube({0, 1.2f, 0}, 9.0f, 2.5f, 13.0f, bodyColor);
    
    // Hull top plates
    DrawCube({0, 2.45f, 2.0f}, 8.0f, 0.5f, 6.0f, {50, 70, 35, 255});
    DrawCube({0, 2.45f, -3.0f}, 8.0f, 0.5f, 5.0f, {50, 70, 35, 255});
    
    // Front sloped armor
    rlPushMatrix();
    rlTranslatef(0, 1.5f, 6.3f);
    rlRotatef(-20, 1, 0, 0);
    DrawCube({0, 0, 0}, 9.0f, 3.5f, 0.8f, {65, 85, 50, 255});
    rlPopMatrix();
    
    // Side armored skirts
    DrawCube({-4.8f, 0.8f, 0}, 0.6f, 2.5f, 13.0f, {40, 50, 30, 255});
    DrawCube({4.8f, 0.8f, 0}, 0.6f, 2.5f, 13.0f, {40, 50, 30, 255});
    
    // --- TRACKS ---
    for (float tx : {-4.0f, 4.0f}) {
        DrawCube({tx, 0.2f, 0}, 1.6f, 1.0f, 13.5f, {25, 25, 25, 255});
        for (int i = -5; i <= 5; i++) {
            float wz = (float)i * 1.3f;
            rlPushMatrix();
            rlTranslatef(tx, 0.2f, wz);
            rlRotatef(90, 0, 0, 1);
            rlRotatef(trackAnimation, 0, 1, 0);
            DrawCylinder({0, 0, 0}, 0.6f, 0.6f, 0.4f, 12, {60, 60, 65, 255});
            rlPopMatrix();
        }
    }
    
    // --- TURRET ---
    rlPushMatrix();
    rlRotatef(turretRotation, 0, 1, 0);
    
    // Turret base (GIGA)
    DrawCube({0, 3.5f, 0}, 6.0f, 2.0f, 7.5f, {70, 90, 55, 255});
    DrawCube({0, 4.5f, -0.5f}, 5.0f, 0.5f, 5.0f, {60, 80, 45, 255});
    
    // --- MAIN CANNON ---
    rlPushMatrix();
    rlTranslatef(0, 3.8f, 3.5f);
    rlRotatef(-barrelPitch, 1, 0, 0);
    
    // Recoil kickback
    rlTranslatef(0, 0, -recoilOffset);
    
    // Barrel
    DrawCylinderEx({0, 0, 0}, {0, 0, 10.0f}, 0.45f, 0.35f, 16, {40, 40, 45, 255}); // Tapered barrel
    // Muzzle brake
    DrawCube({0, 0, 10.2f}, 0.8f, 0.8f, 0.6f, {35, 35, 40, 255});
    
    // Muzzle Flash Effect
    if (muzzleFlash > 0) {
        DrawSphere({0, 0, 11.0f}, 2.5f, Fade(ORANGE, muzzleFlash * 5.0f));
        DrawSphere({0, 0, 11.0f}, 1.5f, Fade(YELLOW, muzzleFlash * 5.0f));
    }
    rlPopMatrix();
    
    // MG and Detail
    DrawCube({1.5f, 5.2f, 1.0f}, 0.2f, 0.2f, 2.5f, {30, 30, 35, 255});
    DrawCylinder({-1.0f, 4.8f, -1.0f}, 0.8f, 0.8f, 0.2f, 12, {55, 75, 40, 255});
    
    rlPopMatrix(); // End turret
    
    // Huge shadow
    DrawCube({0, -0.48f, 0}, 12.0f, 0.05f, 16.0f, Fade(BLACK, 0.6f));
    
    rlPopMatrix(); // End hull
}

#include "tank.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>

Tank::Tank(Vector3 pos) : Vehicle(pos, VehicleType::SUV) {
    // Override vehicle stats for tank
    maxSpeed = 1.0f; // Slightly faster but with inertia
    acceleration = 0.012f;
    health = 50000;
    maxHealth = 50000;
    bodyColor = {45, 55, 35, 255}; // Dark military olive
    
    turretRotation = 0.0f;
    barrelPitch = 0.0f;
    cannonCooldown = 0.0f;
    mgCooldown = 0.0f;
    cannonDamage = 15000; // Buffed damage for a tank
    mgDamage = 65;
    trackLAnimation = 0.0f;
    trackRAnimation = 0.0f;
    muzzleFlash = 0.0f;
    recoilOffset = 0.0f;
    smokeTimer = 0.0f;
}

void Tank::Update(float dt) {
    if (!isOccupied) {
        speed *= 0.96f;
        if (fabsf(speed) < 0.001f) speed = 0;
    } else {
        float turnSpeed = 2.2f; // Neutral steering speed
        
        // --- NEUTRAL STEERING & TRACK ANIMATION ---
        float lMove = 0, rMove = 0;

        if (IsKeyDown(KEY_A)) {
            rotation += turnSpeed; // Inverted to follow standard player expectation
            lMove = 1.0f;          
            rMove = -1.0f;
        } else if (IsKeyDown(KEY_D)) {
            rotation -= turnSpeed; // Inverted
            lMove = -1.0f;         
            rMove = 1.0f;
        }
        
        if (IsKeyDown(KEY_W)) { 
            speed += acceleration;
            if (speed > maxSpeed) speed = maxSpeed;
            lMove += 1.0f;
            rMove += 1.0f;
        } else if (IsKeyDown(KEY_S)) { 
            speed -= acceleration * 1.5f;
            if (speed < -maxSpeed * 0.5f) speed = -maxSpeed * 0.5f;
            lMove -= 1.0f;
            rMove -= 1.0f;
        } else {
            speed *= 0.985f; // Heavy inertia
            if (fabsf(speed) < 0.005f) speed = 0;
            // Tracks should move with inertia speed if not pressing W/S
            if (speed != 0) {
                lMove += speed / maxSpeed;
                rMove += speed / maxSpeed;
            }
        }

        trackLAnimation += lMove * 40.0f * dt * 2.0f;
        trackRAnimation += rMove * 40.0f * dt * 2.0f;
    }
    
    // Animation cooldowns
    if (cannonCooldown > 0) cannonCooldown -= dt;
    if (mgCooldown > 0) mgCooldown -= dt;
    if (muzzleFlash > 0) muzzleFlash -= dt;
    if (smokeTimer > 0) smokeTimer -= dt;
    
    // Recoil recovery (snappier)
    recoilOffset = Lerp(recoilOffset, 0, dt * 8.0f);
    
    // Move forward (Base movement)
    Vector3 forward = GetForward();
    position.x += forward.x * speed;
    position.z += forward.z * speed;
    position.y = 0.5f;
}

void Tank::FireCannon() {
    cannonCooldown = 1.5f;
    muzzleFlash = 0.2f;
    recoilOffset = 2.0f;
}

void Tank::FireMG() {
    mgCooldown = 0.08f;
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
    float totalRot = (rotation + turretRotation) * DEG2RAD;
    float barrelLen = 12.0f - recoilOffset; 
    
    // Cannon pivot is at (0, 4.0, 4.2) in turret-local space
    // We rotate this offset by the turret's world rotation (totalRot)
    Vector3 pivotWorld = {
        position.x + sinf(totalRot) * 4.2f,
        position.y + 4.0f,
        position.z + cosf(totalRot) * 4.2f
    };
    
    return {
        pivotWorld.x + forward.x * barrelLen,
        pivotWorld.y + forward.y * barrelLen,
        pivotWorld.z + forward.z * barrelLen
    };
}

Vector3 Tank::GetMGTip() const {
    Vector3 forward = GetTurretForward(); // MG follows turret forward
    float totalRot = (rotation + turretRotation) * DEG2RAD;
    
    // MG is at (-1.5, 5.8, -1.0) in turret-local space
    // Rotate local (-1.5, 0, -1.0) by totalRot
    float localX = -1.5f;
    float localZ = -1.0f;
    float rx = localX * cosf(totalRot) + localZ * sinf(totalRot);
    float rz = -localX * sinf(totalRot) + localZ * cosf(totalRot);
    
    return {
        position.x + rx,
        position.y + 5.8f,
        position.z + rz + forward.z * 2.5f
    };
}


BoundingBox Tank::GetBoundingBox() {
    return {
        {position.x - 7.0f, 0, position.z - 9.0f},
        {position.x + 7.0f, 8.0f, position.z + 9.0f}
    };
}

void Tank::Draw() {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(rotation, 0, 1, 0);
    
    Color hullColor = bodyColor;
    Color detailColor = {35, 45, 25, 255};
    Color armorColor = {55, 70, 45, 255};
    
    // --- HULL (DETAILED) ---
    // Main base
    DrawCube({0, 1.2f, 0}, 9.5f, 2.8f, 15.0f, hullColor);
    
    // Front sloped armor Plate
    rlPushMatrix();
    rlTranslatef(0, 1.6f, 7.3f);
    rlRotatef(-22.0f, 1, 0, 0);
    DrawCube({0, 0, 0}, 9.5f, 4.0f, 0.8f, armorColor);
    rlPopMatrix();
    
    // Back engine deck
    DrawCube({0, 2.4f, -4.5f}, 8.0f, 0.6f, 6.0f, detailColor);
    // Exhausts
    DrawCube({-3.0f, 2.8f, -7.0f}, 0.8f, 0.4f, 1.5f, DARKGRAY);
    DrawCube({3.0f, 2.8f, -7.0f}, 0.8f, 0.4f, 1.5f, DARKGRAY);
    
    // Side skirts (Extra armor)
    DrawCube({-5.1f, 1.0f, 0}, 0.8f, 3.2f, 15.2f, detailColor);
    DrawCube({5.1f, 1.0f, 0}, 0.8f, 3.2f, 15.2f, detailColor);
    
    // Front headlights
    DrawCube({-3.5f, 2.0f, 7.4f}, 0.6f, 0.6f, 0.4f, YELLOW);
    DrawCube({3.5f, 2.0f, 7.4f}, 0.6f, 0.6f, 0.4f, YELLOW);

    // --- TRACKS ---
    for (int side = 0; side < 2; side++) {
        float tx = (side == 0) ? -4.0f : 4.0f;
        float trackAnim = (side == 0) ? trackLAnimation : trackRAnimation;
        
        DrawCube({tx, 0.3f, 0}, 1.8f, 1.2f, 15.0f, BLACK); // Track belt
        
        // Track wheels
        for (int i = -6; i <= 6; i++) {
            float wz = (float)i * 1.2f;
            rlPushMatrix();
            rlTranslatef(tx, 0.3f, wz);
            rlRotatef(90, 0, 0, 1);
            rlRotatef(trackAnim, 0, 1, 0);
            DrawCylinder({0, 0, 0}, 0.7f, 0.7f, 0.5f, 10, GRAY);
            rlPopMatrix();
        }
    }
    
    // --- TURRET ASSEMBLY ---
    rlPushMatrix();
    rlRotatef(turretRotation, 0, 1, 0);
    
    // Turret Basket / Ring
    DrawCylinder({0, 2.6f, 0}, 3.5f, 3.5f, 0.4f, 16, detailColor);
    
    // Main Turret Body (Angled frontal armor)
    DrawCube({0, 3.8f, 0}, 7.0f, 2.2f, 8.5f, hullColor);
    
    // Frontal Wedge Armor
    rlPushMatrix();
    rlTranslatef(-2.0f, 3.8f, 4.0f);
    rlRotatef(25.0f, 0, 1, 0);
    DrawCube({0, 0, 0}, 3.5f, 2.2f, 1.2f, armorColor);
    rlPopMatrix();
    
    rlPushMatrix();
    rlTranslatef(2.0f, 3.8f, 4.0f);
    rlRotatef(-25.0f, 0, 1, 0);
    DrawCube({0, 0, 0}, 3.5f, 2.2f, 1.2f, armorColor);
    rlPopMatrix();
    
    // Turret Top Plate & Hatches
    DrawCube({0, 4.9f, -0.5f}, 5.5f, 0.4f, 5.5f, detailColor);
    DrawCylinder({-1.5f, 5.1f, -1.0f}, 1.0f, 1.0f, 0.3f, 12, hullColor); // Cupola
    DrawCylinder({1.5f, 5.1f, 1.0f}, 0.8f, 0.8f, 0.2f, 12, hullColor);  // Loader hatch

    // --- MAIN CANNON ---
    rlPushMatrix();
    rlTranslatef(0, 4.0f, 4.2f);
    rlRotatef(-barrelPitch, 1, 0, 0);
    
    // Recoil kickback
    rlTranslatef(0, 0, -recoilOffset);
    
    // Gun Mantlet
    DrawCube({0, 0, 0}, 2.5f, 1.8f, 1.5f, detailColor);
    
    // Tapered Barrel
    DrawCylinderEx({0, 0, 0.5f}, {0, 0, 12.0f}, 0.55f, 0.4f, 20, {30, 30, 30, 255});
    
    // Thermal Sleeve / Detail
    DrawCylinderEx({0, 0, 3.0f}, {0, 0, 5.5f}, 0.65f, 0.65f, 16, DARKGRAY);
    
    // Muzzle Brake (Complex shape)
    DrawCube({0, 0, 12.3f}, 1.2f, 0.8f, 0.8f, {20, 20, 20, 255});
    DrawCube({0, 0, 12.3f}, 0.8f, 1.2f, 0.8f, {20, 20, 20, 255});
    
    // Muzzle Flash
    if (muzzleFlash > 0) {
        DrawSphere({0, 0, 13.5f}, 3.5f, Fade(ORANGE, muzzleFlash * 4.0f));
        DrawSphere({0, 0, 13.5f}, 2.0f, Fade(YELLOW, muzzleFlash * 4.0f));
    }
    rlPopMatrix();
    
    // Machine Gun (Commander's MG)
    rlPushMatrix();
    rlTranslatef(-1.5f, 5.8f, -1.0f);
    rlRotatef(-barrelPitch * 0.5f, 1, 0, 0); // Follows pitch slightly
    DrawCube({0, 0, 0}, 0.25f, 0.25f, 2.5f, BLACK);
    DrawCube({0, 0.1f, -0.6f}, 0.3f, 0.4f, 0.5f, detailColor); // MG Body
    rlPopMatrix();
    
    rlPopMatrix(); // End turret
    
    // Shadow (Better looking than a cube)
    DrawCircle3D({0, -0.48f, 0}, 12.0f, {1, 0, 0}, 90.0f, Fade(BLACK, 0.5f));
    
    rlPopMatrix(); // End hull
}



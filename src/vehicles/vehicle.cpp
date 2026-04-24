#include "vehicle.hpp"
#include "rlgl.h"
#include <cmath>

Vehicle::Vehicle(Vector3 pos, VehicleType vtype) : position(pos), type(vtype) {
    rotation = 0.0f;
    speed = 0.0f;
    isOccupied = false;
    wheelRotation = 0.0f;
    steerAngle = 0.0f;
    
    switch (vtype) {
        case VehicleType::SEDAN:
            maxSpeed = 0.35f;
            acceleration = 0.008f;
            health = 500;
            maxHealth = 500;
            bodyColor = {30, 60, 180, 255}; // Blue
            break;
        case VehicleType::SPORTS:
            maxSpeed = 0.55f;
            acceleration = 0.015f;
            health = 350;
            maxHealth = 350;
            bodyColor = {200, 30, 30, 255}; // Red
            break;
        case VehicleType::SUV:
            maxSpeed = 0.30f;
            acceleration = 0.006f;
            health = 800;
            maxHealth = 800;
            bodyColor = {40, 40, 40, 255}; // Black
            break;
    }
}

Vector3 Vehicle::GetForward() const {
    float rad = rotation * DEG2RAD;
    return {sinf(rad), 0, cosf(rad)};
}

Vector3 Vehicle::GetRight() const {
    float rad = rotation * DEG2RAD;
    return {cosf(rad), 0, -sinf(rad)};
}

float Vehicle::GetBodyLength() const {
    switch (type) {
        case VehicleType::SEDAN: return 5.0f;
        case VehicleType::SPORTS: return 5.5f;
        case VehicleType::SUV: return 5.5f;
        default: return 5.0f;
    }
}

float Vehicle::GetBodyWidth() const {
    switch (type) {
        case VehicleType::SEDAN: return 2.2f;
        case VehicleType::SPORTS: return 2.0f;
        case VehicleType::SUV: return 2.6f;
        default: return 2.2f;
    }
}

float Vehicle::GetBodyHeight() const {
    switch (type) {
        case VehicleType::SEDAN: return 1.4f;
        case VehicleType::SPORTS: return 1.1f;
        case VehicleType::SUV: return 2.0f;
        default: return 1.4f;
    }
}

void Vehicle::Update(float dt) {
    if (!isOccupied) {
        // Friction when unoccupied
        speed *= 0.95f;
        if (fabsf(speed) < 0.001f) speed = 0;
        return;
    }
    
    // Steering
    steerAngle = 0;
    if (IsKeyDown(KEY_A)) steerAngle = -2.5f;
    if (IsKeyDown(KEY_D)) steerAngle = 2.5f;
    
    // Only steer when moving
    if (fabsf(speed) > 0.01f) {
        rotation += steerAngle * (speed / maxSpeed);
    }
    
    // Acceleration / Braking
    if (IsKeyDown(KEY_W)) {
        speed += acceleration;
        if (speed > maxSpeed) speed = maxSpeed;
    } else if (IsKeyDown(KEY_S)) {
        speed -= acceleration * 1.5f;
        if (speed < -maxSpeed * 0.4f) speed = -maxSpeed * 0.4f;
    } else {
        // Natural deceleration
        speed *= 0.98f;
        if (fabsf(speed) < 0.001f) speed = 0;
    }
    
    // Handbrake
    if (IsKeyDown(KEY_SPACE)) {
        speed *= 0.92f;
    }
    
    // Move forward
    Vector3 forward = GetForward();
    position.x += forward.x * speed;
    position.z += forward.z * speed;
    
    // Keep on ground
    position.y = 0.5f;
    
    // Spin wheels
    wheelRotation += speed * 200.0f * dt;
}

void Vehicle::Enter() {
    isOccupied = true;
}

void Vehicle::Exit() {
    isOccupied = false;
    speed = 0;
}

Vector3 Vehicle::GetCameraPosition() const {
    Vector3 forward = GetForward();
    float camDist = 8.0f;
    float camHeight = 4.0f;
    return {
        position.x - forward.x * camDist,
        position.y + camHeight,
        position.z - forward.z * camDist
    };
}

Vector3 Vehicle::GetCameraTarget() const {
    Vector3 forward = GetForward();
    return {
        position.x + forward.x * 5.0f,
        position.y + 1.5f,
        position.z + forward.z * 5.0f
    };
}

BoundingBox Vehicle::GetBoundingBox() {
    float hw = GetBodyWidth() / 2.0f;
    float hl = GetBodyLength() / 2.0f;
    float h = GetBodyHeight();
    return {
        {position.x - hw - 0.5f, 0, position.z - hl - 0.5f},
        {position.x + hw + 0.5f, h + 0.5f, position.z + hl + 0.5f}
    };
}

void Vehicle::Draw() {
    float bw = GetBodyWidth();
    float bl = GetBodyLength();
    float bh = GetBodyHeight();
    
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(rotation, 0, 1, 0);
    
    // --- CHASSIS ---
    // Main body
    DrawCube({0, bh * 0.3f, 0}, bw, bh * 0.5f, bl, bodyColor);
    
    // Roof / cabin (smaller, higher)
    float roofW = bw * 0.9f;
    float roofH = bh * 0.45f;
    float roofL = bl * 0.5f;
    float roofZ = -bl * 0.05f;
    DrawCube({0, bh * 0.7f, roofZ}, roofW, roofH, roofL, bodyColor);
    
    // Windshield (front, angled - represented as a thin dark cube)
    DrawCube({0, bh * 0.65f, roofZ + roofL * 0.52f}, roofW * 0.95f, roofH * 0.9f, 0.05f, {50, 80, 120, 200});
    // Rear window
    DrawCube({0, bh * 0.65f, roofZ - roofL * 0.52f}, roofW * 0.95f, roofH * 0.9f, 0.05f, {50, 80, 120, 200});
    // Side windows
    DrawCube({bw * 0.46f, bh * 0.65f, roofZ}, 0.05f, roofH * 0.8f, roofL * 0.85f, {50, 80, 120, 180});
    DrawCube({-bw * 0.46f, bh * 0.65f, roofZ}, 0.05f, roofH * 0.8f, roofL * 0.85f, {50, 80, 120, 180});
    
    // Bumpers
    DrawCube({0, bh * 0.1f, bl * 0.52f}, bw * 1.02f, bh * 0.15f, 0.2f, {60, 60, 65, 255});
    DrawCube({0, bh * 0.1f, -bl * 0.52f}, bw * 1.02f, bh * 0.15f, 0.2f, {60, 60, 65, 255});
    
    // Headlights
    DrawSphere({-bw * 0.35f, bh * 0.25f, bl * 0.53f}, 0.15f, YELLOW);
    DrawSphere({bw * 0.35f, bh * 0.25f, bl * 0.53f}, 0.15f, YELLOW);
    // Headlight glow
    DrawSphere({-bw * 0.35f, bh * 0.25f, bl * 0.53f}, 0.3f, Fade(YELLOW, 0.2f));
    DrawSphere({bw * 0.35f, bh * 0.25f, bl * 0.53f}, 0.3f, Fade(YELLOW, 0.2f));
    
    // Taillights
    DrawSphere({-bw * 0.35f, bh * 0.25f, -bl * 0.53f}, 0.12f, RED);
    DrawSphere({bw * 0.35f, bh * 0.25f, -bl * 0.53f}, 0.12f, RED);
    
    // --- WHEELS ---
    float wheelR = 0.4f;
    float wheelW = 0.2f;
    float wheelY = -0.1f;
    float wheelXoff = bw * 0.5f + 0.05f;
    float wheelZfront = bl * 0.35f;
    float wheelZrear = -bl * 0.35f;
    
    // Front-left
    rlPushMatrix();
    rlTranslatef(-wheelXoff, wheelY, wheelZfront);
    rlRotatef(90, 0, 0, 1);
    rlRotatef(wheelRotation, 0, 1, 0);
    DrawCylinder({0, 0, 0}, wheelR, wheelR, wheelW, 8, DARKGRAY);
    DrawCylinderWires({0, 0, 0}, wheelR, wheelR, wheelW, 8, BLACK);
    rlPopMatrix();
    
    // Front-right
    rlPushMatrix();
    rlTranslatef(wheelXoff, wheelY, wheelZfront);
    rlRotatef(-90, 0, 0, 1);
    rlRotatef(wheelRotation, 0, 1, 0);
    DrawCylinder({0, 0, 0}, wheelR, wheelR, wheelW, 8, DARKGRAY);
    DrawCylinderWires({0, 0, 0}, wheelR, wheelR, wheelW, 8, BLACK);
    rlPopMatrix();
    
    // Rear-left
    rlPushMatrix();
    rlTranslatef(-wheelXoff, wheelY, wheelZrear);
    rlRotatef(90, 0, 0, 1);
    rlRotatef(wheelRotation, 0, 1, 0);
    DrawCylinder({0, 0, 0}, wheelR, wheelR, wheelW, 8, DARKGRAY);
    DrawCylinderWires({0, 0, 0}, wheelR, wheelR, wheelW, 8, BLACK);
    rlPopMatrix();
    
    // Rear-right
    rlPushMatrix();
    rlTranslatef(wheelXoff, wheelY, wheelZrear);
    rlRotatef(-90, 0, 0, 1);
    rlRotatef(wheelRotation, 0, 1, 0);
    DrawCylinder({0, 0, 0}, wheelR, wheelR, wheelW, 8, DARKGRAY);
    DrawCylinderWires({0, 0, 0}, wheelR, wheelR, wheelW, 8, BLACK);
    rlPopMatrix();
    
    // Undercarriage shadow
    DrawCube({0, -0.3f, 0}, bw + 0.5f, 0.01f, bl + 0.5f, Fade(BLACK, 0.5f));
    
    rlPopMatrix();
}

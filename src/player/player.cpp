#include "player.hpp"
#include "raymath.h"
#include "rcamera.h"
#include <cmath>
#include "../weapons/glock.hpp"

Player::Player(Vector3 startPos, int id) : position(startPos), hp(100), maxHp(100), money(0), currentWeapon(nullptr), playerId(id) {
    height = 2.0f;
    speed = 0.15f;
    verticalVelocity = 0.0f;
    gravity = -0.008f;
    jumpForce = 0.2f;
    isGrounded = false;
    isDead = false;

    camera = { 0 };
    camera.position = (Vector3){ position.x, position.y + 1.0f, position.z };
    camera.target = (Vector3){ position.x, position.y + 1.0f, position.z + 1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    currentWeapon = new Glock();
}

Player::~Player() {
    if (currentWeapon) {
        delete currentWeapon;
        currentWeapon = nullptr;
    }
}

void Player::Update() {
    if (hp <= 0) {
        isDead = true;
        if (!isRespawning) {
            isRespawning = true;
            respawnTimer = RESPAWN_TIME;
        }
        // Count down respawn timer
        respawnTimer -= GetFrameTime();
        if (respawnTimer <= 0) {
            Respawn({0, 2, 150}); // Spawn inside the base
        }
        // Apply only gravity when dead
        HandlePhysics();
        return;
    }
    isDead = false;
    
    // Skip movement if in vehicle
    if (inVehicle) return;

    HandleMovement();
    HandlePhysics();
    
    // Sync camera
    Vector3 camOffset = Vector3Subtract(camera.target, camera.position);
    camera.position = (Vector3){ position.x, position.y + 1.0f, position.z };
    camera.target = Vector3Add(camera.position, camOffset);

    if (currentWeapon) {
        bool isMoving = Vector2Length(GetMouseDelta()) > 0 || IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D);
        currentWeapon->Update(GetFrameTime(), isGrounded, GetMouseDelta(), isSprinting, isMoving);
        currentWeapon->UpdateReload(GetFrameTime());
        currentWeapon->CheckAutoReload();
        if (IsKeyPressed(KEY_R)) currentWeapon->StartReload();
    }
}

void Player::HandleMovement() {
    Vector2 mouseDelta = GetMouseDelta();
    UpdateCameraPro(&camera, 
        (Vector3){ 0, 0, 0 }, 
        (Vector3){ mouseDelta.x * 0.05f, mouseDelta.y * 0.05f, 0 }, 
        0.0f);

    Vector3 forward = Vector3Normalize((Vector3){ camera.target.x - camera.position.x, 0, camera.target.z - camera.position.z });
    Vector3 right = Vector3Normalize((Vector3){ -forward.z, 0, forward.x });

    Vector3 moveTarget = { 0, 0, 0 };
    
    isSprinting = IsKeyDown(KEY_LEFT_SHIFT);
    float currentSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);

    // Use WASD for local player
    if (IsKeyDown(KEY_W)) { moveTarget.x += forward.x; moveTarget.z += forward.z; }
    if (IsKeyDown(KEY_S)) { moveTarget.x -= forward.x; moveTarget.z -= forward.z; }
    if (IsKeyDown(KEY_A)) { moveTarget.x -= right.x; moveTarget.z -= right.z; }
    if (IsKeyDown(KEY_D)) { moveTarget.x += right.x; moveTarget.z += right.z; }

    float dt = GetFrameTime();
    float timeScale = dt * 60.0f;

    if (isFlying) {
        if (IsKeyDown(KEY_SPACE)) position.y += currentSpeed * timeScale;
        if (IsKeyDown(KEY_LEFT_CONTROL)) position.y -= currentSpeed * timeScale;
    }

    if (Vector3Length(moveTarget) > 0) {
        moveTarget = Vector3Normalize(moveTarget);
        position.x += moveTarget.x * currentSpeed * timeScale;
        position.z += moveTarget.z * currentSpeed * timeScale;
    }
}

void Player::HandlePhysics() {
    float dt = GetFrameTime();
    float timeScale = dt * 60.0f;

    if (isFlying) {
        verticalVelocity = 0;
        return;
    }

    if (!isDead && IsKeyPressed(KEY_SPACE) && isGrounded) {
        verticalVelocity = jumpForce;
        isGrounded = false;
    }

    // Reset grounding before checking collisions (floor and obstacles)
    isGrounded = false; 

    if (!isGrounded) {
        verticalVelocity += gravity * timeScale;
    }

    position.y += verticalVelocity * timeScale;

    // Floor collision
    if (position.y <= 1.0f) {
        position.y = 1.0f;
        verticalVelocity = 0;
        isGrounded = true;
    }
}

void Player::HandleCollision(BoundingBox box) {
    BoundingBox playerBox = {
        (Vector3){ position.x - 0.4f, position.y - 1.0f, position.z - 0.4f },
        (Vector3){ position.x + 0.4f, position.y + 1.0f, position.z + 0.4f }
    };

    if (CheckCollisionBoxes(playerBox, box)) {
        // Calculate overlap on each axis
        float dx1 = playerBox.max.x - box.min.x;
        float dx2 = box.max.x - playerBox.min.x;
        float dy1 = playerBox.max.y - box.min.y;
        float dy2 = box.max.y - playerBox.min.y;
        float dz1 = playerBox.max.z - box.min.z;
        float dz2 = box.max.z - playerBox.min.z;

        float minOverlap = fmin(fmin(fmin(dx1, dx2), fmin(dy1, dy2)), fmin(dz1, dz2));

        if (minOverlap == dy1) { // Hit from bottom
            position.y = box.min.y - 1.01f;
            if (verticalVelocity > 0) verticalVelocity = 0;
        }
        else if (minOverlap == dy2) { // Hit from top (Land)
            position.y = box.max.y + 1.0f;
            if (verticalVelocity < 0) {
                verticalVelocity = 0;
                isGrounded = true;
            }
        }
        else if (minOverlap == dx1) position.x = box.min.x - 0.41f;
        else if (minOverlap == dx2) position.x = box.max.x + 0.41f;
        else if (minOverlap == dz1) position.z = box.min.z - 0.41f;
        else if (minOverlap == dz2) position.z = box.max.z + 0.41f;
    }
}

void Player::TakeDamage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
}

void Player::AddMoney(int amount) {
    money += amount;
}

void Player::Respawn(Vector3 pos) {
    position = pos;
    hp = maxHp;
    isDead = false;
    isRespawning = false;
    respawnTimer = 0;
    verticalVelocity = 0;
}

void Player::Draw() {
    Color pColor = (playerId == 0) ? BLUE : DARKPURPLE;
    if (isDead) pColor = GRAY;

    // Draw player body (capsule/cylinder)
    DrawCapsule((Vector3){ position.x, position.y - 1.0f, position.z },
                (Vector3){ position.x, position.y + 1.0f, position.z },
                0.5f, 8, 4, pColor);

    // Draw look indicator (skip for local player viewing themselves, but fine for debug)
    Vector3 lookDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    DrawLine3D(position, Vector3Add(position, Vector3Scale(lookDir, 1.0f)), RED);
}

#include "rlgl.h"

void Player::DrawViewModel() {
    if (currentWeapon && !isDead) {
        currentWeapon->DrawViewModel(camera);
    }
}


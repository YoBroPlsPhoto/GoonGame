#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "raylib.h"
#include <vector>
#include "../weapons/weapon.hpp"

class Player {

public:
    Vector3 position;
    Vector3 target;
    Camera3D camera;

    
    float speed;
    float verticalVelocity;
    float gravity;
    float jumpForce;
    bool isGrounded;
    float height = 2.0f;
    int hp = 100;
    int maxHp = 100;
    int playerId; // 0 for Player 1, 1 for Player 2
    bool isDead = false;
    bool isFlying = false;
    bool isSprinting = false;
    float sprintMultiplier = 2.0f;
    
    // Respawn system
    float respawnTimer = 0.0f;
    bool isRespawning = false;
    static constexpr float RESPAWN_TIME = 10.0f;
    
    // Vehicle system
    bool inVehicle = false;
    int vehicleIndex = -1;
    
    int money = 0;
    
    // Weapon System
    Weapon* currentWeapon = nullptr;
    Weapon* previousWeapon = nullptr;
    std::vector<Weapon*> inventory;
    bool showInventory = false;
    bool isAdmin = false;
    bool showSettings = false;

    Player(Vector3 startPos, int id);
    ~Player();
    void Update();
    void Draw();
    void DrawViewModel(); // 1st person weapon render
    void TakeDamage(int amount);
    void AddMoney(int amount);
    void Respawn(Vector3 pos);
    void HandleCollision(BoundingBox box);

private:
    void HandleMovement();
    void HandlePhysics();
};

#endif

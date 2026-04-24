#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "raylib.h"
#include "raymath.h"

enum class VehicleType { SEDAN, SPORTS, SUV };

class Vehicle {
public:
    Vector3 position;
    float rotation; // Y-axis rotation in degrees
    float speed;
    float maxSpeed;
    float acceleration;
    int health;
    int maxHealth;
    bool isOccupied;
    Color bodyColor;
    VehicleType type;
    float wheelRotation; // spinning wheels
    float steerAngle;

    Vehicle(Vector3 pos, VehicleType vtype);
    virtual ~Vehicle() = default;
    
    virtual void Update(float dt);
    virtual void Draw();
    virtual BoundingBox GetBoundingBox();
    
    void Enter();
    void Exit();
    
    // Get the forward direction based on rotation
    Vector3 GetForward() const;
    Vector3 GetRight() const;
    
    // Get camera position for third-person view
    Vector3 GetCameraPosition() const;
    Vector3 GetCameraTarget() const;

protected:
    float GetBodyLength() const;
    float GetBodyWidth() const;
    float GetBodyHeight() const;
};

#endif

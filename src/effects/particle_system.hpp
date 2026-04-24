#ifndef PARTICLE_SYSTEM_HPP
#define PARTICLE_SYSTEM_HPP

#include "raylib.h"
#include "raymath.h"
#include <vector>

struct Particle {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    float maxLifetime;
    Color color;
    float size;
};

struct BulletTracer {
    Vector3 start;
    Vector3 end;
    float lifetime;
    float maxLifetime;
    Color color;
};

struct MuzzleFlash {
    Vector3 position;
    float lifetime;
    float size;
};

struct Explosion {
    Vector3 position;
    float timer;
    float maxTimer;
    float maxRadius;
};

struct RPGProjectile {
    Vector3 position;
    Vector3 direction;
    float speed;
    float lifetime;
    bool active;
    float trailTimer;
};

class ParticleSystem {
public:
    std::vector<Particle> particles;
    std::vector<BulletTracer> tracers;
    std::vector<MuzzleFlash> flashes;
    std::vector<Explosion> explosions;
    std::vector<RPGProjectile> projectiles;

    void SpawnMuzzleFlash(Vector3 pos);
    void SpawnBulletTracer(Vector3 start, Vector3 end, Color color = {255, 255, 100, 255});
    void SpawnHitSparks(Vector3 pos, int count = 8);
    void SpawnBloodSplash(Vector3 pos, int count = 6);
    void SpawnExplosion(Vector3 pos);
    void SpawnShotgunPelletTrails(Vector3 start, Vector3 end); // thinner, faster fade
    
    RPGProjectile& LaunchProjectile(Vector3 start, Vector3 direction, float speed);
    
    void Update(float dt);
    void Draw();
};

#endif

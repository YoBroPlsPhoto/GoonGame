#include "particle_system.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

void ParticleSystem::SpawnMuzzleFlash(Vector3 pos) {
    MuzzleFlash f;
    f.position = pos;
    f.lifetime = 0.06f;
    f.size = 0.15f;
    flashes.push_back(f);
    
    // Also spawn some sparks around muzzle
    for (int i = 0; i < 4; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {
            (float)(GetRandomValue(-100, 100)) / 200.0f,
            (float)(GetRandomValue(50, 150)) / 200.0f,
            (float)(GetRandomValue(-100, 100)) / 200.0f
        };
        p.lifetime = 0.15f;
        p.maxLifetime = 0.15f;
        p.color = {255, 200, 50, 255};
        p.size = 0.03f;
        particles.push_back(p);
    }
}

void ParticleSystem::SpawnBulletTracer(Vector3 start, Vector3 end, Color color) {
    BulletTracer t;
    t.start = start;
    t.end = end;
    t.lifetime = 0.08f;
    t.maxLifetime = 0.08f;
    t.color = color;
    tracers.push_back(t);
}

void ParticleSystem::SpawnHitSparks(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {
            (float)(GetRandomValue(-200, 200)) / 200.0f,
            (float)(GetRandomValue(0, 300)) / 200.0f,
            (float)(GetRandomValue(-200, 200)) / 200.0f
        };
        p.lifetime = 0.3f + (float)(GetRandomValue(0, 100)) / 500.0f;
        p.maxLifetime = p.lifetime;
        p.color = {255, (unsigned char)GetRandomValue(150, 255), 50, 255};
        p.size = 0.06f;
        particles.push_back(p);
    }
}

void ParticleSystem::SpawnBloodSplash(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {
            (float)(GetRandomValue(-150, 150)) / 200.0f,
            (float)(GetRandomValue(50, 250)) / 200.0f,
            (float)(GetRandomValue(-150, 150)) / 200.0f
        };
        p.lifetime = 0.4f + (float)(GetRandomValue(0, 200)) / 500.0f;
        p.maxLifetime = p.lifetime;
        p.color = {(unsigned char)GetRandomValue(150, 220), 0, 0, 255};
        p.size = 0.08f;
        particles.push_back(p);
    }
}

void ParticleSystem::SpawnExplosion(Vector3 pos) {
    Explosion e;
    e.position = pos;
    e.timer = 0.0f;
    e.maxTimer = 0.6f;
    e.maxRadius = 8.0f;
    explosions.push_back(e);
    
    // Spawn lots of fire/debris particles
    for (int i = 0; i < 40; i++) {
        Particle p;
        p.position = pos;
        float speed = (float)(GetRandomValue(200, 800)) / 200.0f;
        p.velocity = {
            (float)(GetRandomValue(-100, 100)) / 100.0f * speed,
            (float)(GetRandomValue(20, 100)) / 100.0f * speed,
            (float)(GetRandomValue(-100, 100)) / 100.0f * speed
        };
        p.lifetime = 0.5f + (float)(GetRandomValue(0, 500)) / 1000.0f;
        p.maxLifetime = p.lifetime;
        // Mix of orange, red, and yellow
        int colorType = GetRandomValue(0, 2);
        if (colorType == 0) p.color = {255, (unsigned char)GetRandomValue(80, 180), 0, 255};
        else if (colorType == 1) p.color = {255, 50, 0, 255};
        else p.color = {255, 255, (unsigned char)GetRandomValue(0, 100), 255};
        p.size = 0.1f + (float)(GetRandomValue(0, 100)) / 500.0f;
        particles.push_back(p);
    }
    
    // Smoke particles (slower, larger, gray)
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {
            (float)(GetRandomValue(-50, 50)) / 100.0f,
            (float)(GetRandomValue(100, 300)) / 200.0f,
            (float)(GetRandomValue(-50, 50)) / 100.0f
        };
        p.lifetime = 1.0f + (float)(GetRandomValue(0, 500)) / 500.0f;
        p.maxLifetime = p.lifetime;
        unsigned char gray = (unsigned char)GetRandomValue(40, 80);
        p.color = {gray, gray, gray, 200};
        p.size = 0.3f + (float)(GetRandomValue(0, 200)) / 500.0f;
        particles.push_back(p);
    }
}

void ParticleSystem::SpawnShotgunPelletTrails(Vector3 start, Vector3 end) {
    BulletTracer t;
    t.start = start;
    t.end = end;
    t.lifetime = 0.05f;
    t.maxLifetime = 0.05f;
    t.color = {255, 220, 100, 200};
    tracers.push_back(t);
}

RPGProjectile& ParticleSystem::LaunchProjectile(Vector3 start, Vector3 direction, float speed) {
    RPGProjectile proj;
    proj.position = start;
    proj.direction = Vector3Normalize(direction);
    proj.speed = speed;
    proj.lifetime = 5.0f; // 5 seconds max flight time
    proj.active = true;
    proj.trailTimer = 0.0f;
    projectiles.push_back(proj);
    return projectiles.back();
}

void ParticleSystem::Update(float dt) {
    // Update particles
    for (auto& p : particles) {
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.velocity.y -= 2.0f * dt; // gravity
        p.lifetime -= dt;
        p.size *= (1.0f - dt * 0.5f); // shrink slightly
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.lifetime <= 0; }), particles.end());
    
    // Update tracers
    for (auto& t : tracers) {
        t.lifetime -= dt;
    }
    tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
        [](const BulletTracer& t) { return t.lifetime <= 0; }), tracers.end());
    
    // Update muzzle flashes
    for (auto& f : flashes) {
        f.lifetime -= dt;
    }
    flashes.erase(std::remove_if(flashes.begin(), flashes.end(),
        [](const MuzzleFlash& f) { return f.lifetime <= 0; }), flashes.end());
    
    // Update explosions
    for (auto& e : explosions) {
        e.timer += dt;
    }
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
        [](const Explosion& e) { return e.timer >= e.maxTimer; }), explosions.end());
    
    // Update RPG projectiles
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        proj.position = Vector3Add(proj.position, Vector3Scale(proj.direction, proj.speed * dt));
        proj.lifetime -= dt;
        proj.trailTimer += dt;
        
        // Spawn trail particles
        if (proj.trailTimer > 0.02f) {
            proj.trailTimer = 0.0f;
            Particle p;
            p.position = proj.position;
            p.velocity = {
                (float)(GetRandomValue(-20, 20)) / 100.0f,
                (float)(GetRandomValue(10, 50)) / 100.0f,
                (float)(GetRandomValue(-20, 20)) / 100.0f
            };
            p.lifetime = 0.3f;
            p.maxLifetime = 0.3f;
            unsigned char gray = (unsigned char)GetRandomValue(100, 180);
            p.color = {gray, gray, gray, 180};
            p.size = 0.15f;
            particles.push_back(p);
        }
        
        if (proj.lifetime <= 0) proj.active = false;
        // Ground collision
        if (proj.position.y <= 0.1f) proj.active = false;
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const RPGProjectile& p) { return !p.active && p.lifetime <= -1.0f; }), projectiles.end());
}

void ParticleSystem::Draw() {
    // Draw particles as small spheres
    for (const auto& p : particles) {
        float alpha = p.lifetime / p.maxLifetime;
        Color c = p.color;
        c.a = (unsigned char)(alpha * c.a);
        DrawSphere(p.position, p.size, c);
        // Glow effect - slightly larger, more transparent sphere
        DrawSphere(p.position, p.size * 2.0f, Fade(c, alpha * 0.3f));
    }
    
    // Draw tracers as lines with glow
    for (const auto& t : tracers) {
        float alpha = t.lifetime / t.maxLifetime;
        Color c = t.color;
        c.a = (unsigned char)(alpha * 255);
        DrawLine3D(t.start, t.end, c);
        // Draw slightly offset lines for thickness
        Vector3 offset = {0, 0.01f, 0};
        DrawLine3D(Vector3Add(t.start, offset), Vector3Add(t.end, offset), Fade(c, alpha * 0.5f));
    }
    
    // Draw muzzle flashes
    for (const auto& f : flashes) {
        float alpha = f.lifetime / 0.06f;
        DrawSphere(f.position, f.size * alpha, Fade(YELLOW, alpha));
        DrawSphere(f.position, f.size * 1.5f * alpha, Fade(ORANGE, alpha * 0.4f));
        DrawSphere(f.position, f.size * 2.5f * alpha, Fade(RED, alpha * 0.15f));
    }
    
    // Draw explosions
    for (const auto& e : explosions) {
        float progress = e.timer / e.maxTimer;
        float radius = e.maxRadius * sinf(progress * PI * 0.5f); // Expand fast then slow
        float alpha = 1.0f - progress;
        
        // Core fireball
        DrawSphere(e.position, radius * 0.5f, Fade(YELLOW, alpha));
        // Mid ring
        DrawSphere(e.position, radius * 0.75f, Fade(ORANGE, alpha * 0.6f));
        // Outer blast
        DrawSphere(e.position, radius, Fade(RED, alpha * 0.3f));
        // Shockwave ring (flat disc)
        DrawCylinder(e.position, radius * 1.2f, radius * 1.3f, 0.1f, 16, Fade(WHITE, alpha * 0.2f));
    }
    
    // Draw RPG projectiles
    for (const auto& proj : projectiles) {
        if (!proj.active) continue;
        // Rocket body
        DrawCube(proj.position, 0.2f, 0.2f, 0.5f, {85, 107, 47, 255});
        // Warhead tip
        Vector3 tipPos = Vector3Add(proj.position, Vector3Scale(proj.direction, 0.3f));
        DrawSphere(tipPos, 0.12f, {180, 50, 50, 255});
        // Flame trail at the back
        Vector3 flamePos = Vector3Subtract(proj.position, Vector3Scale(proj.direction, 0.3f));
        DrawSphere(flamePos, 0.15f, Fade(ORANGE, 0.8f));
        DrawSphere(flamePos, 0.25f, Fade(YELLOW, 0.4f));
    }
}

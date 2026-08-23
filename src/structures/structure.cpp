#include "structure.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>

std::vector<std::shared_ptr<Structure>> structures;

BoundingBox Structure::GetBoundingBox() const {
    if (type == StructureType::WALL) {
        return { {position.x - 4.0f, position.y, position.z - 1.5f},
                 {position.x + 4.0f, position.y + 5.0f, position.z + 1.5f} };
    } else {
        return { {position.x - 2.0f, position.y, position.z - 2.0f},
                 {position.x + 2.0f, position.y + 5.0f, position.z + 2.0f} };
    }
}

void Structure::Draw() {
    // Damage-based color tinting
    float hpRatio = (float)hp / (float)maxHp;
    Color baseColor = color;
    // Darken color as HP drops
    if (hpRatio < 0.5f) {
        float darkFactor = 0.4f + 0.6f * (hpRatio / 0.5f);
        baseColor.r = (unsigned char)(baseColor.r * darkFactor);
        baseColor.g = (unsigned char)(baseColor.g * darkFactor);
        baseColor.b = (unsigned char)(baseColor.b * darkFactor);
    }
    // Flash red when hit
    if (damageFlash > 0) {
        float flashIntensity = damageFlash / 0.3f;
        baseColor.r = (unsigned char)std::min(255.0f, baseColor.r + 180.0f * flashIntensity);
        baseColor.g = (unsigned char)(baseColor.g * (1.0f - flashIntensity * 0.7f));
        baseColor.b = (unsigned char)(baseColor.b * (1.0f - flashIntensity * 0.7f));
    }
    
    if (type == StructureType::WALL) {
        // === REINFORCED WALL ===
        Vector3 p = position;
        Color gunMetal = {55, 60, 66, 255};
        Color weaponGlow = {255, 95, 60, 220};
        
        // Main body - reinforced concrete
        DrawCube({p.x, p.y + 2.0f, p.z}, 8.0f, 4.0f, 2.5f, baseColor);

        // Built-in weapon ports give every wall tier a defensive look.
        DrawCube({p.x, p.y + 2.35f, p.z + 1.46f}, 2.6f, 0.25f, 0.12f, {90, 95, 100, 255});
        DrawCube({p.x, p.y + 1.35f, p.z - 1.46f}, 2.2f, 0.2f, 0.12f, {70, 70, 76, 255});
        DrawCube({p.x - 2.8f, p.y + 2.35f, p.z + 1.46f}, 0.5f, 0.45f, 0.2f, gunMetal);
        DrawCube({p.x + 2.8f, p.y + 2.35f, p.z + 1.46f}, 0.5f, 0.45f, 0.2f, gunMetal);
        DrawSphere({p.x - 2.8f, p.y + 2.35f, p.z + 1.68f}, 0.12f, weaponGlow);
        DrawSphere({p.x + 2.8f, p.y + 2.35f, p.z + 1.68f}, 0.12f, weaponGlow);
        
        // Steel reinforcement plates on front and back
        Color steelCol = {90, 95, 100, 255};
        if (damageFlash > 0) steelCol = baseColor;
        DrawCube({p.x, p.y + 2.0f, p.z + 1.35f}, 7.5f, 3.5f, 0.25f, steelCol);
        DrawCube({p.x, p.y + 2.0f, p.z - 1.35f}, 7.5f, 3.5f, 0.25f, steelCol);
        
        // Top edge (concrete lip)
        Color topCol = {(unsigned char)(baseColor.r * 0.8f), (unsigned char)(baseColor.g * 0.8f), (unsigned char)(baseColor.b * 0.8f), 255};
        DrawCube({p.x, p.y + 4.1f, p.z}, 8.4f, 0.3f, 2.92f, topCol);
        
        // Vertical support pillars on edges
        Color pillarCol = {60, 60, 65, 255};
        DrawCube({p.x - 3.71f, p.y + 2.0f, p.z}, 0.62f, 4.02f, 2.62f, pillarCol);
        DrawCube({p.x + 3.71f, p.y + 2.0f, p.z}, 0.62f, 4.02f, 2.62f, pillarCol);
        
        // Center cross brace
        DrawCube({p.x, p.y + 2.0f, p.z}, 0.42f, 4.02f, 2.62f, pillarCol);
        
        // Bottom foundation
        DrawCube({p.x, p.y + 0.15f, p.z}, 8.6f, 0.3f, 3.02f, {50, 50, 55, 255});
        
        // Damage cracks (visible when < 60% HP)
        if (hpRatio < 0.6f) {
            Color crackCol = {30, 25, 20, 255};
            float crackSize = (1.0f - hpRatio) * 0.3f;
            DrawCube({p.x - 1.5f, p.y + 2.5f, p.z + 1.35f}, 0.8f * crackSize, 2.0f, 0.05f, crackCol);
            DrawCube({p.x + 2.0f, p.y + 1.5f, p.z + 1.35f}, 1.2f * crackSize, 1.5f, 0.05f, crackCol);
            if (hpRatio < 0.3f) {
                DrawCube({p.x + 0.5f, p.y + 3.0f, p.z - 1.35f}, 1.5f * crackSize, 2.5f, 0.05f, crackCol);
            }
        }
        
        // Wire outline
        DrawCubeWires({p.x, p.y + 2.0f, p.z}, 8.0f, 4.0f, 2.5f, Fade(BLACK, 0.3f));
        
        // Tier 2: Spikes on top
        if (tier >= 2) {
            for (int i = -3; i <= 3; i++) {
                DrawCylinder({p.x + i * 1.0f, p.y + 4.25f, p.z}, 0.2f, 0.0f, 0.6f, 8, DARKGRAY);
            }
            DrawCube({p.x - 2.4f, p.y + 4.15f, p.z + 1.45f}, 0.45f, 0.35f, 0.8f, gunMetal);
            DrawCube({p.x + 2.4f, p.y + 4.15f, p.z + 1.45f}, 0.45f, 0.35f, 0.8f, gunMetal);
            DrawSphere({p.x - 2.4f, p.y + 4.15f, p.z + 1.88f}, 0.10f, weaponGlow);
            DrawSphere({p.x + 2.4f, p.y + 4.15f, p.z + 1.88f}, 0.10f, weaponGlow);
        }
        // Tier 3: Metal plating on the front
        if (tier >= 3) {
            DrawCube({p.x, p.y + 2.0f, p.z + 1.4f}, 7.0f, 3.0f, 0.1f, {60, 60, 60, 255});
            DrawCube({p.x, p.y + 2.15f, p.z + 1.68f}, 3.1f, 0.45f, 0.35f, gunMetal);
            DrawCube({p.x, p.y + 2.15f, p.z + 2.20f}, 0.55f, 0.55f, 0.25f, {35, 35, 40, 255});
            DrawSphere({p.x, p.y + 2.15f, p.z + 2.34f}, 0.16f, weaponGlow);
        }
        
    } else { // === TURRET ===
        Vector3 p = position;
        Color weaponGlow = {255, 120, 70, 220};
        
        // Base platform (heavy concrete foundation)
        DrawCube({p.x, p.y + 0.3f, p.z}, 4.0f, 0.6f, 4.0f, {55, 55, 60, 255});
        DrawCubeWires({p.x, p.y + 0.3f, p.z}, 4.0f, 0.6f, 4.0f, Fade(BLACK, 0.2f));
        
        // Platform ring (beveled concrete)
        DrawCube({p.x, p.y + 0.7f, p.z}, 3.4f, 0.2f, 3.4f, {65, 65, 70, 255});
        
        // Central pedestal
        DrawCube({p.x, p.y + 1.4f, p.z}, 1.6f, 1.2f, 1.6f, baseColor);
        DrawCubeWires({p.x, p.y + 1.4f, p.z}, 1.6f, 1.2f, 1.6f, Fade(BLACK, 0.3f));
        
        // Rotating turret assembly
        rlPushMatrix();
        rlTranslatef(p.x, p.y + 2.0f, p.z);
        rlRotatef(turretAngle, 0, 1, 0);
        
        // Gun housing / shield
        Color housingCol = {
            (unsigned char)std::min(255, (int)baseColor.r + 20),
            (unsigned char)std::min(255, (int)baseColor.g + 15),
            (unsigned char)std::min(255, (int)baseColor.b + 15), 255};
        DrawCube({0, 0.4f, 0}, 2.0f, 1.2f, 1.8f, housingCol);
        DrawSphere({0.0f, 0.75f, 0.1f}, 0.16f, weaponGlow);

        // Front armor plate (angled)
        Color steelColT = {80, 85, 90, 255};
        DrawCube({0, 0.5f, 1.2f}, 2.2f, 1.4f, 0.3f, steelColT);
        DrawCube({-1.1f, 0.35f, 0.05f}, 0.15f, 0.55f, 0.95f, {65, 70, 75, 255});
        DrawCube({1.1f, 0.35f, 0.05f}, 0.15f, 0.55f, 0.95f, {65, 70, 75, 255});

        // Barrel recoil offset
        float recoilOffset = barrelRecoil * 0.4f;
        
        Color barrelCol = {45, 45, 50, 255};
        Color barrelGlowCol = {225, 120, 80, 220};
        
        // Tier 2 adds extra armor shielding
        if (tier >= 2) {
            DrawCube({0, 0.9f, 1.4f}, 2.4f, 0.6f, 0.2f, {70, 75, 80, 255});
            DrawCube({-1.1f, 0.5f, 1.4f}, 0.2f, 1.4f, 0.2f, {70, 75, 80, 255});
            DrawCube({1.1f, 0.5f, 1.4f}, 0.2f, 1.4f, 0.2f, {70, 75, 80, 255});
            DrawCube({-1.45f, 0.8f, 0.8f}, 0.35f, 0.35f, 1.0f, {60, 65, 70, 255});
            DrawCube({1.45f, 0.8f, 0.8f}, 0.35f, 0.35f, 1.0f, {60, 65, 70, 255});
        }

        // Dual barrels
        DrawCube({-0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, barrelCol);
        DrawCube({0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, barrelCol);
        DrawCubeWires({-0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, Fade(BLACK, 0.25f));
        DrawCubeWires({0.35f, 0.5f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, Fade(BLACK, 0.25f));
        DrawSphere({-0.35f, 0.5f, 3.32f - recoilOffset}, 0.12f, barrelGlowCol);
        DrawSphere({0.35f, 0.5f, 3.32f - recoilOffset}, 0.12f, barrelGlowCol);
        
        // Tier 3 adds a third central barrel
        if (tier >= 3) {
            DrawCube({0.0f, 0.8f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, barrelCol);
            DrawCubeWires({0.0f, 0.8f, 1.5f + 1.5f - recoilOffset}, 0.28f, 0.28f, 3.1f, Fade(BLACK, 0.25f));
            DrawCube({0.0f, 0.8f, 3.22f - recoilOffset}, 0.38f, 0.38f, 0.16f, {35, 35, 40, 255});
            DrawSphere({0.0f, 0.8f, 3.36f - recoilOffset}, 0.14f, barrelGlowCol);
            DrawCube({0.0f, 1.15f, 1.55f}, 0.55f, 0.08f, 0.55f, {90, 95, 100, 255});
        }
        
        // Barrel tips (flash guard)
        DrawCube({-0.35f, 0.5f, 3.2f - recoilOffset}, 0.35f, 0.35f, 0.15f, {35, 35, 40, 255});
        DrawCube({0.35f, 0.5f, 3.2f - recoilOffset}, 0.35f, 0.35f, 0.15f, {35, 35, 40, 255});
        
        // Ammo box on the side
        DrawCube({-1.2f, 0.0f, 0.3f}, 0.6f, 0.7f, 0.8f, {60, 70, 50, 255});
        DrawCubeWires({-1.2f, 0.0f, 0.3f}, 0.6f, 0.7f, 0.8f, Fade(BLACK, 0.2f));
        
        // Antenna
        DrawCube({0.8f, 1.4f, -0.5f}, 0.06f, 1.5f, 0.06f, DARKGRAY);
        DrawCube({0.8f, 2.15f, -0.5f}, 0.15f, 0.08f, 0.15f, RED);
        
        rlPopMatrix(); // End turret rotation
        
        // Range indicator ring (subtle, on ground)
        float pulse = sinf((float)GetTime() * 2.0f) * 0.1f + 0.15f;
        DrawCircle3D({p.x, 0.05f, p.z}, 60.0f, {1,0,0}, 90.0f, Fade(RED, pulse * 0.3f));
    }
}

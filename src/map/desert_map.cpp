#include "desert_map.hpp"
#include <cmath>
#include "raymath.h"
#include "rlgl.h"

DesertMap::DesertMap() {
    for (int x = 0; x <= terrainSize; x++) {
        for (int z = 0; z <= terrainSize; z++) {
            float nx = (float)x * 0.15f;
            float nz = (float)z * 0.15f;
            terrainHeights[x][z] = (sinf(nx * 0.5f) * cosf(nz * 0.5f) * 6.0f) + (sinf(nx * 0.2f) * 4.0f);
            if (terrainHeights[x][z] < -2.0f) terrainHeights[x][z] = -2.0f;
        }
    }

    // Add some rocks
    for (int i = 0; i < 15; i++) {
        float rx = (float)GetRandomValue(-150, 150);
        float rz = (float)GetRandomValue(-150, 150);
        float s = (float)GetRandomValue(2, 6);
        float h = GetHeight(rx, rz);
        obstacles.push_back((BoundingBox){ (Vector3){ rx - s/2, h, rz - s/2 }, (Vector3){ rx + s/2, h + s, rz + s/2 } });
    }
}

DesertMap::~DesertMap() {}

void DesertMap::Draw(int detailLevel, Vector3 viewPos, int currentWave) {
    rlBegin(RL_TRIANGLES);
    for (int x = 0; x < terrainSize; x++) {
        for (int z = 0; z < terrainSize; z++) {
            // Distance culling optimization
            float ox = -((float)terrainSize * terrainScale) / 2.0f;
            float oz = -((float)terrainSize * terrainScale) / 2.0f;
            float px = ox + (float)x * terrainScale;
            float pz = oz + (float)z * terrainScale;
            
            if (detailLevel > 0 && Vector2Distance({px, pz}, {viewPos.x, viewPos.z}) > 1000.0f) continue;

            Vector3 p1 = { px, terrainHeights[x][z], pz };
            Vector3 p2 = { ox + (float)(x + 1) * terrainScale, terrainHeights[x+1][z], pz };
            Vector3 p3 = { px, terrainHeights[x][z+1], oz + (float)(z + 1) * terrainScale };
            Vector3 p4 = { ox + (float)(x+1) * terrainScale, terrainHeights[x+1][z+1], oz + (float)(z + 1) * terrainScale };
            
            Color c = ((x + z) % 2 == 0) ? BEIGE : (Color){ 235, 200, 150, 255 };
            rlColor4ub(c.r, c.g, c.b, c.a);
            
            // Normal 1: p1 -> p3 -> p2  (CCW)
            Vector3 n1 = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p3, p1), Vector3Subtract(p2, p1)));
            rlNormal3f(n1.x, n1.y, n1.z);
            rlVertex3f(p1.x, p1.y, p1.z);
            rlVertex3f(p3.x, p3.y, p3.z);
            rlVertex3f(p2.x, p2.y, p2.z);

            // Normal 2: p2 -> p3 -> p4  (CCW)
            Vector3 n2 = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p3, p2), Vector3Subtract(p4, p2)));
            rlNormal3f(n2.x, n2.y, n2.z);
            rlVertex3f(p2.x, p2.y, p2.z);
            rlVertex3f(p3.x, p3.y, p3.z);
            rlVertex3f(p4.x, p4.y, p4.z);
        }
    }
    rlEnd();

    // Oasis
    DrawCircle3D((Vector3){ 0, -1.5f, 0 }, 15.0f, (Vector3){ 1, 0, 0 }, 90.0f, BLUE);

    // Obstacles
    for (const auto& box : obstacles) {
        DrawCubeV(Vector3Scale(Vector3Add(box.min, box.max), 0.5f), Vector3Subtract(box.max, box.min), DARKBROWN);
    }
}

float DesertMap::GetHeight(float worldX, float worldZ) {
    float gx = (worldX / terrainScale) + 50.0f;
    float gz = (worldZ / terrainScale) + 50.0f;
    int ix = (int)floorf(gx);
    int iz = (int)floorf(gz);
    float fx = gx - (float)ix;
    float fz = gz - (float)iz;

    if (ix >= 0 && ix < terrainSize && iz >= 0 && iz < terrainSize) {
        float h00 = terrainHeights[ix][iz];
        float h10 = terrainHeights[ix + 1][iz];
        float h01 = terrainHeights[ix][iz + 1];
        float h11 = terrainHeights[ix + 1][iz + 1];
        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;
        return h0 * (1.0f - fz) + h1 * fz;
    }
    return 0.0f;
}

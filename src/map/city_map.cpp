#include "city_map.hpp"
#include <cmath>
#include "rlgl.h"
#include "raymath.h"

CityMap::CityMap() {
  type = MapType::CITY;
  
  // Load models once
  trashBinModel = LoadModel("../models/bin.obj");
  modelLoaded = true;
  
  // Generate city
  for (int z = -40; z <= 40; z++) {
    for (int x : {-3, -2, 2, 3}) {
      float px = x * 60.0f;
      float pz = z * 55.0f;

      float w = (float)GetRandomValue(30, 45);
      float h = (float)GetRandomValue(120, 350);
      float d = (float)GetRandomValue(30, 45);

      unsigned char gray = GetRandomValue(60, 90);
      buildings.push_back({(Vector3){px, h / 2.0f, pz}, (Vector3){w, h, d},
                           (Color){gray, gray, gray, 255}});
      obstacles.push_back(
          (BoundingBox){(Vector3){px - w / 2.0f, 0, pz - d / 2.0f},
                        (Vector3){px + w / 2.0f, h, pz + d / 2.0f}});
    }
  }

  // --- MAIN BASE (30 FLOORS) ---
  float bw = 50.0f;
  float bd = 50.0f;
  float fh = 8.0f;
  int floors = 30;
  Vector3 bp = mainBasePos;

  for (int f = 0; f <= floors; f++) {
    float y = f * fh;
    // Floor slab with a clear 12x12 hole for the stairs in the center
    float sh = 12.0f;
    obstacles.push_back({{bp.x - bw / 2, y, bp.z - bd / 2},
                         {bp.x - sh / 2, y + 0.2f, bp.z + bd / 2}});
    obstacles.push_back({{bp.x + sh / 2, y, bp.z - bd / 2},
                         {bp.x + bw / 2, y + 0.2f, bp.z + bd / 2}});
    obstacles.push_back({{bp.x - sh / 2, y, bp.z - bd / 2},
                         {bp.x + sh / 2, y + 0.2f, bp.z - sh / 2}});
    obstacles.push_back({{bp.x - sh / 2, y, bp.z + sh / 2},
                         {bp.x + sh / 2, y + 0.2f, bp.z + bd / 2}});

    if (f < floors) {
      bool hasHoles = ((f + 1) % 3 == 0);

      for (int i = 0; i < 4; i++) {
        Vector3 wPos, wSize;
        if (i == 0) {
          wPos = {bp.x - bw / 2, y + fh / 2, bp.z};
          wSize = {0.5f, fh, bd};
        } // Left
        else if (i == 1) {
          wPos = {bp.x + bw / 2, y + fh / 2, bp.z};
          wSize = {0.5f, fh, bd};
        } // Right
        else if (i == 2) {
          wPos = {bp.x, y + fh / 2, bp.z - bd / 2};
          wSize = {bw, fh, 0.5f};
        } // Front
        else if (i == 3) {
          wPos = {bp.x, y + fh / 2, bp.z + bd / 2};
          wSize = {bw, fh, 0.5f};
        } // Back

        if (i == 2 && hasHoles) { // FRONT wall with holes on sides
          float gapY = 1.6f;
          float gapH = 2.0f;
          float middleSolidW = 20.0f;
          obstacles.push_back(
              {{bp.x - middleSolidW / 2, y, wPos.z - 0.25f},
               {bp.x + middleSolidW / 2, y + fh, wPos.z + 0.25f}});
          obstacles.push_back(
              {{bp.x - bw / 2, y, wPos.z - 0.25f},
               {bp.x - middleSolidW / 2, y + gapY, wPos.z + 0.25f}});
          obstacles.push_back(
              {{bp.x - bw / 2, y + gapY + gapH, wPos.z - 0.25f},
               {bp.x - middleSolidW / 2, y + fh, wPos.z + 0.25f}});
          obstacles.push_back({{bp.x + middleSolidW / 2, y, wPos.z - 0.25f},
                               {bp.x + bw / 2, y + gapY, wPos.z + 0.25f}});
          obstacles.push_back(
              {{bp.x + middleSolidW / 2, y + gapY + gapH, wPos.z - 0.25f},
               {bp.x + bw / 2, y + fh, wPos.z + 0.25f}});
        } else {
          if (f == 0 && (i == 2 || i == 3)) { // Entrances
            obstacles.push_back({{bp.x - bw / 2, y, wPos.z - 0.25f},
                                 {bp.x - 6, y + fh, wPos.z + 0.25f}});
            obstacles.push_back({{bp.x + 6, y, wPos.z - 0.25f},
                                 {bp.x + bw / 2, y + fh, wPos.z + 0.25f}});
          } else {
            obstacles.push_back(
                {{wPos.x - wSize.x / 2, y, wPos.z - wSize.z / 2},
                 {wPos.x + wSize.x / 2, y + fh, wPos.z + wSize.z / 2}});
          }
        }
      }

      // Stairs - SYNCED WITH DRAW
      int steps = 10;
      for (int s = 0; s < steps; s++) {
        float sy = y + (s * fh / steps);
        float sz = bp.z - sh / 2 + (s * sh / steps);
        obstacles.push_back(
            {{bp.x - sh / 2 + 1, sy, sz},
             {bp.x + sh / 2 - 1, sy + 0.8f, sz + (sh / steps)}});
      }
    }
  }

  // --- KEBAB STAND OBSTACLES (Ground Floor) ---
  // Stand box at (15, 0, bp.z + 10) width: 8 depth: 4 height: 2.5
  obstacles.push_back({{bp.x + 11, 0, bp.z + 8}, {bp.x + 19, 2.5f, bp.z + 12}});
  
  // Tank Terminal box at (15, 0, 120)
  obstacles.push_back({{14.0f, 0, 119.0f}, {16.0f, 3, 121.0f}});

  // Map Boundaries (Invisible Walls)
  obstacles.push_back((BoundingBox){{-500, 0, -4100}, {500, 100, -4000}});
  obstacles.push_back((BoundingBox){{-500, 0, 4000}, {500, 100, 4100}});
  obstacles.push_back((BoundingBox){{-300, 0, -4100}, {-290, 100, 4100}});
  obstacles.push_back((BoundingBox){{290, 0, -4100}, {300, 100, 4100}});

  // --- STREET PROPS COLLISIONS ---
  for (int i = -100; i <= 2; i++) {
    float pz = (float)i * 100.0f;
    if (i % 2 == 0) { // Lamps
      obstacles.push_back({{-24.5f, 0, pz - 0.5f}, {-23.5f, 8, pz + 0.5f}});
      obstacles.push_back({{23.5f, 0, pz - 0.5f}, {24.5f, 8, pz + 0.5f}});
    } else { // Trash cans
      obstacles.push_back({{-16.3f, 0, pz - 0.3f}, {-15.7f, 1.2f, pz + 0.3f}});
      obstacles.push_back({{15.7f, 0, pz - 0.3f}, {16.3f, 1.2f, pz + 0.3f}});
    }
  }
}

CityMap::~CityMap() {
    if (modelLoaded) UnloadModel(trashBinModel);
}

void CityMap::Draw(int detailLevel, Vector3 viewPos) {
  // Basic Ground & Road
  DrawPlane((Vector3){0, 0, 0}, (Vector2){5000, 5000},
            (Color){40, 40, 45, 255});

  // Sidewalks
  DrawCube((Vector3){-20.0f, 0.1f, 0}, 10, 0.2f, 8000,
           (Color){60, 60, 65, 255});
  DrawCube((Vector3){20.0f, 0.1f, 0}, 10, 0.2f, 8000, (Color){60, 60, 65, 255});

  DrawCube((Vector3){0, 0.05f, 0}, 30, 0.11f, 8000,
           (Color){30, 30, 35, 255}); // Road

  // Only draw props in high detail
  if (detailLevel > 0) {
    for (int i = -100; i <= 2; i++) {
      float pz = (float)i * 100.0f;
      DrawCube((Vector3){0, 0.13f, pz}, 1, 0.1f, 10, RAYWHITE);

      if (i % 2 == 0) {
        DrawCube((Vector3){-24.0f, 4, pz}, 0.5f, 8, 0.5f, DARKGRAY);
        DrawCube((Vector3){-24.0f, 8, pz + 1}, 0.5f, 0.5f, 2, DARKGRAY);
        DrawSphere((Vector3){-24.0f, 8, pz + 2}, 0.8f, YELLOW);

        DrawCube((Vector3){24.0f, 4, pz}, 0.5f, 8, 0.5f, DARKGRAY);
        DrawCube((Vector3){24.0f, 8, pz - 1}, 0.5f, 0.5f, 2, DARKGRAY);
        DrawSphere((Vector3){24.0f, 8, pz - 2}, 0.8f, YELLOW);
      } else {
        if (modelLoaded) {
            // Disable backface culling in case normals are inverted, and raise to y=0.7
            rlDisableBackfaceCulling();
            DrawModelEx(trashBinModel, (Vector3){-16.0f, 0.7f, pz}, (Vector3){0, 1, 0}, 0.0f, (Vector3){0.6f, 0.6f, 0.6f}, WHITE);
            DrawModelEx(trashBinModel, (Vector3){16.0f, 0.7f, pz}, (Vector3){0, 1, 0}, 0.0f, (Vector3){0.6f, 0.6f, 0.6f}, WHITE);
            rlEnableBackfaceCulling();
        } else {
            DrawCube((Vector3){-16.0f, 0.6f, pz}, 1.2f, 1.2f, 1.2f, (Color){50, 50, 55, 254});
            DrawCube((Vector3){16.0f, 0.6f, pz}, 1.2f, 1.2f, 1.2f, (Color){50, 50, 55, 254});
        }
      }
    }
  }

  // --- CLOUDS ---
  for (int c = 0; c < 30; c++) {
    float cx = sin(c * 0.5f) * 800.0f;
    float cz = cos(c * 0.3f) * 1200.0f;
    DrawCube((Vector3){cx, 300, cz}, 200, 2, 150, Fade(WHITE, 0.6f));
  }

  // Other Buildings
  for (const auto &b : buildings) {
    // OPTIMIZATION: Distance Culling
    if (detailLevel > 0 && Vector3Distance(b.pos, viewPos) > 1200.0f) continue;

    DrawPlane((Vector3){b.pos.x, 0.08f, b.pos.z},
              (Vector2){b.size.x + 10, b.size.z + 10}, Fade(BLACK, 0.6f));

    float hw = b.size.x / 2.0f;
    float hd = b.size.z / 2.0f;
    float cy = b.size.y / 2.0f;
    float h = b.size.y;

    DrawCube((Vector3){b.pos.x - hw, cy, b.pos.z}, 0.5f, h, b.size.z,
             b.color); // Left
    DrawCube((Vector3){b.pos.x + hw, cy, b.pos.z}, 0.5f, h, b.size.z,
             b.color); // Right
    DrawCube((Vector3){b.pos.x, cy, b.pos.z - hd}, b.size.x, h, 0.5f,
             b.color); // Front
    DrawCube((Vector3){b.pos.x, cy, b.pos.z + hd}, b.size.x, h, 0.5f,
             b.color); // Back
    DrawCube((Vector3){b.pos.x, h, b.pos.z}, b.size.x, 0.5f, b.size.z, b.color);
    DrawCube((Vector3){b.pos.x, b.size.y, b.pos.z}, b.size.x * 0.4f, 6.0f,
             b.size.z * 0.4f, (Color){40, 40, 45, 255});
  }

  // Horizon/Map Barriers (Buildings at ends)
  for (float x = -400; x <= 400; x += 150) {
    DrawCube((Vector3){x, 100, 3800}, 140, 200, 50, (Color){60, 60, 70, 255});
    DrawCube((Vector3){x, 100, -3800}, 140, 200, 50, (Color){60, 60, 70, 255});
  }

  DrawMainBase();
}

void CityMap::DrawMainBase() {
  float bw = 50.0f;
  float bd = 50.0f;
  float fh = 8.0f;
  int floors = 30;
  Vector3 bp = mainBasePos;
  Color baseCol = {70, 75, 90, 255};
  Color floorCol = {40, 40, 50, 255};
  Color stairsCol = {60, 60, 65, 255};
  float sh = 12.0f;

  // --- Ground Floor Slab (OVERLAP TO FIX GAPS) ---
  DrawCube((Vector3){bp.x, 0.1f, bp.z}, bw + 2.0f, 0.2f, bd + 2.0f,
           Fade(floorCol, 0.99f)); // Solid floor

  for (int f = 0; f <= floors; f++) {
    float y = f * fh;
    // Floor slab with smaller hole for stairs (ALIGNED WITH STAIRS)
    DrawCube((Vector3){bp.x - (bw + sh) / 4, y, bp.z}, (bw - sh) / 2 + 1.0f,
             0.2f, bd + 1.0f, floorCol); // Left
    DrawCube((Vector3){bp.x + (bw + sh) / 4, y, bp.z}, (bw - sh) / 2 + 1.0f,
             0.2f, bd + 1.0f, floorCol); // Right
    DrawCube((Vector3){bp.x, y, bp.z - (bd + 12.1f) / 4}, sh + 1.0f, 0.2f,
             (bd - 12.1f) / 2 + 1.0f, floorCol); // Front
    DrawCube((Vector3){bp.x, y, bp.z + (bd + 12.1f) / 4}, sh + 1.0f, 0.2f,
             (bd - 12.1f) / 2 + 1.0f, floorCol); // Back

    if (f < floors) {
      bool hasHoles = ((f + 1) % 3 == 0);

      for (int i = 0; i < 4; i++) {
        Vector3 wPos, wSize;
        if (i == 0) {
          wPos = {bp.x - bw / 2, y + fh / 2, bp.z};
          wSize = {0.5f, fh, bd};
        } // Left
        else if (i == 1) {
          wPos = {bp.x + bw / 2, y + fh / 2, bp.z};
          wSize = {0.5f, fh, bd};
        } // Right
        else if (i == 2) {
          wPos = {bp.x, y + fh / 2, bp.z - bd / 2};
          wSize = {bw, fh, 0.5f};
        } // Front
        else if (i == 3) {
          wPos = {bp.x, y + fh / 2, bp.z + bd / 2};
          wSize = {bw, fh, 0.5f};
        } // Back

        if (i == 2 && hasHoles) { // Holes only on sides of front wall
          float gapY = 1.6f;
          float gapH = 2.0f;
          float middleSolidW = 20.0f;
          float sideSegW = (bw - middleSolidW) / 2.0f;

          // Middle solid part
          DrawCube((Vector3){bp.x, y + fh / 2, wPos.z}, middleSolidW, fh, 0.5f,
                   baseCol);
          // Left and Right side segments (with gaps)
          for (float sx : {bp.x - middleSolidW / 2 - sideSegW / 2,
                           bp.x + middleSolidW / 2 + sideSegW / 2}) {
            DrawCube((Vector3){sx, y + gapY / 2, wPos.z}, sideSegW, gapY, 0.5f,
                     baseCol);
            DrawCube(
                (Vector3){sx, y + gapY + gapH + (fh - gapY - gapH) / 2, wPos.z},
                sideSegW, fh - gapY - gapH, 0.5f, baseCol);
            DrawCube(
                (Vector3){(sx < bp.x ? sx - sideSegW / 2 : sx + sideSegW / 2),
                          y + fh / 2, wPos.z},
                0.8f, fh, 0.8f, DARKGRAY);
          }
        } else {
          if (f == 0 && (i == 2 || i == 3)) { // Entrances
            DrawCube((Vector3){bp.x - bw / 4 - 3, y + fh / 2, wPos.z},
                     bw / 2 - 6, fh, 0.5f, baseCol);
            DrawCube((Vector3){bp.x + bw / 4 + 3, y + fh / 2, wPos.z},
                     bw / 2 - 6, fh, 0.5f, baseCol);
          } else {
            DrawCube(wPos, wSize.x, wSize.y, wSize.z, baseCol);
          }
        }
      }

      // Stairs (Lowered to be flush with floor)
      int steps = 15;
      for (int s = 0; s < steps; s++) {
        float sy = y + (s * fh / (steps - 1));
        float sz = bp.z - sh / 2 + (s * sh / (steps - 1));
        // Shifted down slightly to be perfectly flush with floor slab
        DrawCube((Vector3){bp.x, sy - 0.1f, sz}, sh - 2, 0.8f,
                 sh / (steps - 1) + 0.1f,
                 (Color){stairsCol.r, stairsCol.g, stairsCol.b, 254});
      }
    }
  }

  // --- KEBAB STAND (Ground Floor) ---
  Vector3 standPos = {bp.x + 15, 0, bp.z + 10};
  
  // Base counter
  DrawCube((Vector3){standPos.x, 1.0f, standPos.z}, 8.0f, 2.0f, 4.0f, (Color){128, 0, 0, 254}); 
  // Counter top
  DrawCube((Vector3){standPos.x, 2.05f, standPos.z}, 8.2f, 0.1f, 4.2f, LIGHTGRAY);
  
  // Roof supporting poles
  DrawCylinder((Vector3){standPos.x - 3.8f, 2.0f, standPos.z - 1.8f}, 0.05f, 0.05f, 2.0f, 4, DARKGRAY);
  DrawCylinder((Vector3){standPos.x + 3.8f, 2.0f, standPos.z - 1.8f}, 0.05f, 0.05f, 2.0f, 4, DARKGRAY);
  DrawCylinder((Vector3){standPos.x - 3.8f, 2.0f, standPos.z + 1.8f}, 0.05f, 0.05f, 2.0f, 4, DARKGRAY);
  DrawCylinder((Vector3){standPos.x + 3.8f, 2.0f, standPos.z + 1.8f}, 0.05f, 0.05f, 2.0f, 4, DARKGRAY);
  
  // Roof
  DrawCube((Vector3){standPos.x, 4.0f, standPos.z}, 9.0f, 0.2f, 5.0f, (Color){255, 200, 0, 254}); 
  DrawText3D("TURKISH KEBAB", (Vector3){standPos.x - 2.5f, 4.2f, standPos.z - 2.6f}, 0.4f, 0.1f, GOLD);

  // The kebabs (vertical meat spits)
  DrawCylinderEx((Vector3){standPos.x - 2.0f, 2.1f, standPos.z}, (Vector3){standPos.x - 2.0f, 3.3f, standPos.z}, 0.3f, 0.3f, 16, (Color){200, 100, 0, 254});
  DrawCylinderEx((Vector3){standPos.x, 2.1f, standPos.z}, (Vector3){standPos.x, 3.6f, standPos.z}, 0.45f, 0.45f, 16, (Color){210, 110, 0, 254});
  DrawCylinderEx((Vector3){standPos.x + 2.0f, 2.1f, standPos.z}, (Vector3){standPos.x + 2.0f, 3.9f, standPos.z}, 0.6f, 0.6f, 16, (Color){220, 120, 0, 254});

  // --- WEAPON SCREEN (ULTRA WIDE GRID) ---
  Vector3 screenPos = {bp.x - 24.5f, 5, bp.z};
  DrawCube(screenPos, 0.5f, 10, 18, (Color){10, 10, 15, 255}); // Darker background - wider
  DrawCube((Vector3){screenPos.x + 0.1f, screenPos.y, screenPos.z}, 0.1f, 9.5f,
           17.5f, Fade(SKYBLUE, 0.3f)); // Screen glow
  
  // Pulsing border glow
  float pulse = sinf((float)GetTime() * 3.0f) * 0.15f + 0.35f;
  DrawCube((Vector3){screenPos.x + 0.15f, screenPos.y, screenPos.z}, 0.05f, 9.8f,
           17.8f, Fade(GOLD, pulse));
           
  EndShaderMode(); // Text shouldn't use the lighting shader
  DrawText3D("=== WEAPON DEPOT ===",
             (Vector3){screenPos.x + 0.3f, screenPos.y + 4.0f, screenPos.z},
             0.6f, 0.1f, GOLD);
             
  // Column 1 - Left
  float col1Z = screenPos.z - 4.5f;
  DrawText3D("REVOLVER: $1000",
             (Vector3){screenPos.x + 0.3f, screenPos.y + 2.5f, col1Z},
             0.35f, 0.1f, WHITE);
  DrawText3D("AK-47: $1500",
             (Vector3){screenPos.x + 0.3f, screenPos.y + 1.0f, col1Z},
             0.35f, 0.1f, WHITE);
  DrawText3D("SHOTGUN: $2000",
             (Vector3){screenPos.x + 0.3f, screenPos.y - 0.5f, col1Z},
             0.35f, 0.1f, WHITE);
  DrawText3D("MINIGUN: $5000",
             (Vector3){screenPos.x + 0.3f, screenPos.y - 2.0f, col1Z},
             0.35f, 0.1f, ORANGE);
             
  // Column 2 - Right
  float col2Z = screenPos.z + 4.5f;
  DrawText3D("AWP: $8000",
             (Vector3){screenPos.x + 0.3f, screenPos.y + 2.5f, col2Z},
             0.35f, 0.1f, SKYBLUE);
  DrawText3D("RPG: $12000",
             (Vector3){screenPos.x + 0.3f, screenPos.y + 1.0f, col2Z},
             0.35f, 0.1f, RED);
  DrawText3D("REPLENISH: $500",
             (Vector3){screenPos.x + 0.3f, screenPos.y - 0.5f, col2Z},
             0.35f, 0.1f, {255, 150, 50, 255});
  DrawText3D("AMMO: $500",
             (Vector3){screenPos.x + 0.3f, screenPos.y - 2.0f, col2Z},
             0.35f, 0.1f, GREEN);

  DrawText3D(">>> SHOOT THE ITEM YOU WANT <<<",
             (Vector3){screenPos.x + 0.3f, screenPos.y - 3.8f, screenPos.z},
             0.25f, 0.1f, LIME);

  // Roof access structure
  DrawCube((Vector3){bp.x, floors * fh + 2, bp.z}, 12, 4, 12, baseCol);
  DrawCube((Vector3){bp.x, floors * fh + 20, bp.z}, 1, 40, 1, MAROON);
  DrawSphere((Vector3){bp.x, floors * fh + 40, bp.z}, 3, RED);

  // --- TANK TERMINAL (In front of base) ---
  Vector3 terminalPos = {15, 0, 120};
  DrawCube((Vector3){terminalPos.x, 1, terminalPos.z}, 1.5f, 2, 1.5f, DARKGRAY); // Stand
  DrawCube((Vector3){terminalPos.x, 2.1f, terminalPos.z}, 2.5f, 0.2f, 2.0f, GRAY); // Screen base
  
  // Terminal Screen
  rlPushMatrix();
  rlTranslatef(terminalPos.x, 2.5f, terminalPos.z);
  rlRotatef(-30, 1, 0, 0); // Angle screen
  DrawCube({0, 0, 0}, 2.2f, 1.5f, 0.1f, {20, 20, 30, 255});
  DrawCube({0, 0, 0.06f}, 2.0f, 1.3f, 0.05f, Fade(LIME, 0.4f));
  
  EndShaderMode();
  DrawText3D("TANK DEPOT", {0.1f, 0.3f, 0.1f}, 0.2f, 0.05f, GOLD);
  DrawText3D("BUY: $100k", {0.1f, -0.2f, 0.1f}, 0.15f, 0.05f, WHITE);
  rlPopMatrix();
}

float CityMap::GetHeight(float x, float z) { return 0.0f; }
#include "rlgl.h"

void CityMap::DrawText3D(const char *text, Vector3 pos, float size,
                         float spacing, Color col) {
    // Basic 3D Text using Raylib's default font rendered in 3D space
    Font font = GetFontDefault();
    Vector2 textSize = MeasureTextEx(font, text, size * 100, spacing * 100);
    
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    
    // Rotate to face the player (aligned with the shop screen which is on YZ plane)
    rlRotatef(90, 0, 1, 0); 
    
    rlTranslatef(-textSize.x * 0.005f, 0, 0); // Center text
    rlScalef(0.01f, -0.01f, 0.01f); // Scale down to world units and flip Y for correct orientation
    
    DrawTextEx(font, text, (Vector2){0, 0}, size * 100, spacing * 100, col);
    
    rlPopMatrix();
}

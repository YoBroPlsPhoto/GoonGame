#include "builder_tool.hpp"
#include "rlgl.h"

BuilderTool::BuilderTool(int t) {
    buildType = t;
    name = (t == 0) ? "WALL SCH." : "TURRET SCH.";
    magSize = 1; currentAmmo = 1; reserveAmmo = 0; maxReserve = 0;
    attackCooldown = 0.3f;
}

void BuilderTool::Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) {
    if (currentCooldown > 0) currentCooldown -= dt;
}

void BuilderTool::DrawViewModel(Camera3D c) {
    rlPushMatrix();
    rlTranslatef(0.6f, -0.8f, -1.5f);
    rlRotatef(-15, 0, 1, 0);
    if (buildType == 0) {
        // Blueprint clipboard for wall
        DrawCube({0,0,0}, 0.4f, 0.55f, 0.05f, (Color){60, 40, 20, 255});
        DrawCube({0,0.02f,0.03f}, 0.35f, 0.48f, 0.02f, (Color){200, 200, 180, 255});
        // Blueprint lines
        for (int i = 0; i < 4; i++) {
            DrawCube({0, -0.15f + i * 0.1f, 0.045f}, 0.28f, 0.015f, 0.005f, (Color){50, 80, 180, 255});
        }
        // Wall icon
        DrawCube({0, 0.08f, 0.045f}, 0.2f, 0.12f, 0.005f, DARKBROWN);
    } else {
        // Blueprint clipboard for turret
        DrawCube({0,0,0}, 0.4f, 0.55f, 0.05f, (Color){60, 40, 20, 255});
        DrawCube({0,0.02f,0.03f}, 0.35f, 0.48f, 0.02f, (Color){200, 200, 180, 255});
        for (int i = 0; i < 4; i++) {
            DrawCube({0, -0.15f + i * 0.1f, 0.045f}, 0.28f, 0.015f, 0.005f, (Color){180, 50, 50, 255});
        }
        // Turret icon
        DrawCube({0, 0.1f, 0.045f}, 0.12f, 0.1f, 0.005f, MAROON);
        DrawCube({0.1f, 0.12f, 0.045f}, 0.15f, 0.04f, 0.005f, DARKGRAY);
    }
    rlPopMatrix();
}

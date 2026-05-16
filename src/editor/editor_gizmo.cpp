#include "editor/editor.hpp"
#include <cstring>
#include <cmath>
#include <cstdio>

// ── Gizmo Constants ──
static const float GIZMO_LEN   = 8.0f;
static const float GIZMO_THICK = 0.25f;
static const float GIZMO_HEAD  = 1.2f;
static const float GIZMO_HIT   = 2.5f; // hit detection radius

// ── Draw Arrow (axis line + cone head) ──
static void DrawArrow(Vector3 from, Vector3 dir, float len, Color col) {
    Vector3 to = Vector3Add(from, Vector3Scale(dir, len));
    Vector3 headBase = Vector3Add(from, Vector3Scale(dir, len - GIZMO_HEAD));
    DrawLine3D(from, to, col);
    DrawCylinderEx(from, headBase, GIZMO_THICK, GIZMO_THICK, 6, col);
    DrawCylinderEx(headBase, to, GIZMO_THICK*2.5f, 0.0f, 8, col);
}

// ── Scale Cube (for scale gizmo) ──
static void DrawScaleHandle(Vector3 from, Vector3 dir, float len, Color col) {
    Vector3 to = Vector3Add(from, Vector3Scale(dir, len));
    DrawLine3D(from, to, col);
    DrawCube(to, 1.0f, 1.0f, 1.0f, col);
}

// ── Rotate Ring ──
static void DrawRotateRing(Vector3 center, Vector3 axis, float radius, Color col) {
    DrawCircle3D(center, radius, axis, 0, col);
}

// ── Gizmo Rendering ──
void Editor::DrawGizmo() {
    auto* sel = GetSelected();
    if (!sel) return;

    Vector3 p = sel->position;
    float dist = Vector3Distance(p, cam.position);
    float s = dist * 0.04f; // scale gizmo with distance
    s = Clamp(s, 2.0f, 30.0f);

    Color xCol = (hoveredAxis == GizmoAxis::X) ? YELLOW : RED;
    Color yCol = (hoveredAxis == GizmoAxis::Y) ? YELLOW : GREEN;
    Color zCol = (hoveredAxis == GizmoAxis::Z) ? YELLOW : BLUE;

    if (activeAxis == GizmoAxis::X) xCol = WHITE;
    if (activeAxis == GizmoAxis::Y) yCol = WHITE;
    if (activeAxis == GizmoAxis::Z) zCol = WHITE;

    if (gizmoMode == GizmoMode::TRANSLATE) {
        DrawArrow(p, {1,0,0}, s, xCol);
        DrawArrow(p, {0,1,0}, s, yCol);
        DrawArrow(p, {0,0,1}, s, zCol);
    } else if (gizmoMode == GizmoMode::SCALE) {
        DrawScaleHandle(p, {1,0,0}, s, xCol);
        DrawScaleHandle(p, {0,1,0}, s, yCol);
        DrawScaleHandle(p, {0,0,1}, s, zCol);
    } else if (gizmoMode == GizmoMode::ROTATE) {
        DrawRotateRing(p, {1,0,0}, s, xCol);
        DrawRotateRing(p, {0,1,0}, s, yCol);
        DrawRotateRing(p, {0,0,1}, s, zCol);
    }

    // Center sphere
    DrawSphere(p, s * 0.08f, RAYWHITE);
}

// ── Axis closest approach test ──
static float PointToLineDistance(Vector3 point, Vector3 lineStart, Vector3 lineDir, float lineLen) {
    Vector3 diff = Vector3Subtract(point, lineStart);
    float t = Vector3DotProduct(diff, lineDir);
    t = Clamp(t, 0, lineLen);
    Vector3 closest = Vector3Add(lineStart, Vector3Scale(lineDir, t));
    return Vector3Distance(point, closest);
}

// ── Gizmo Update ──
void Editor::UpdateGizmo() {
    auto* sel = GetSelected();
    if (!sel) { hoveredAxis = GizmoAxis::NONE; activeAxis = GizmoAxis::NONE; dragging = false; return; }

    Vector2 mouse = GetMousePosition();
    Vector3 p = sel->position;
    float dist = Vector3Distance(p, cam.position);
    float s = Clamp(dist * 0.04f, 2.0f, 30.0f);

    // Hover detection — check which axis the mouse is closest to
    if (!dragging) {
        hoveredAxis = GizmoAxis::NONE;
        if (IsInViewport(mouse)) {
            Ray ray = GetScreenToWorldRay(mouse, cam);
            // Project ray onto each axis and check proximity
            Vector3 axes[3] = {{1,0,0},{0,1,0},{0,0,1}};
            GizmoAxis axisIds[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
            float bestDist = GIZMO_HIT;

            for (int i = 0; i < 3; i++) {
                // Check several points along the axis
                for (float t = 0.2f; t <= 1.0f; t += 0.1f) {
                    Vector3 axisPoint = Vector3Add(p, Vector3Scale(axes[i], s * t));
                    Vector2 screenPt = GetWorldToScreen(axisPoint, cam);
                    float d = Vector2Distance(mouse, screenPt);
                    if (d < bestDist) {
                        bestDist = d;
                        hoveredAxis = axisIds[i];
                    }
                }
            }
        }
    }

    // Start dragging
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredAxis != GizmoAxis::NONE && IsInViewport(mouse)) {
        activeAxis = hoveredAxis;
        dragging = true;
        dragPrev = mouse;
    }

    // Dragging
    if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouseDelta = Vector2Subtract(mouse, dragPrev);
        dragPrev = mouse;

        // Get screen-space direction of the active axis
        Vector3 axisDir = {0,0,0};
        if (activeAxis == GizmoAxis::X) axisDir = {1,0,0};
        else if (activeAxis == GizmoAxis::Y) axisDir = {0,1,0};
        else if (activeAxis == GizmoAxis::Z) axisDir = {0,0,1};

        Vector2 axisSS0 = GetWorldToScreen(p, cam);
        Vector2 axisSS1 = GetWorldToScreen(Vector3Add(p, axisDir), cam);
        Vector2 axisScreen = Vector2Subtract(axisSS1, axisSS0);
        float axisScreenLen = Vector2Length(axisScreen);
        if (axisScreenLen < 0.001f) return;
        axisScreen = Vector2Scale(axisScreen, 1.0f / axisScreenLen);

        float projection = Vector2DotProduct(mouseDelta, axisScreen);
        float sensitivity = dist * 0.003f;

        if (gizmoMode == GizmoMode::TRANSLATE) {
            sel->position = Vector3Add(sel->position, Vector3Scale(axisDir, projection * sensitivity));
        } else if (gizmoMode == GizmoMode::SCALE) {
            Vector3 scaleDelta = Vector3Scale(axisDir, projection * sensitivity * 0.5f);
            sel->scale = Vector3Add(sel->scale, scaleDelta);
            sel->scale.x = fmaxf(sel->scale.x, 0.1f);
            sel->scale.y = fmaxf(sel->scale.y, 0.1f);
            sel->scale.z = fmaxf(sel->scale.z, 0.1f);
        } else if (gizmoMode == GizmoMode::ROTATE) {
            float angle = projection * 0.5f;
            if (activeAxis == GizmoAxis::X) sel->rotation.x += angle;
            else if (activeAxis == GizmoAxis::Y) sel->rotation.y += angle;
            else if (activeAxis == GizmoAxis::Z) sel->rotation.z += angle;
        }
    }

    // Stop dragging
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && dragging) {
        dragging = false;
        activeAxis = GizmoAxis::NONE;
    }
}

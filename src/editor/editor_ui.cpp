#include "editor/editor.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>

// ── UI Colors ──
static const Color COL_PANEL    = {30, 30, 38, 245};
static const Color COL_PANEL2   = {38, 38, 48, 245};
static const Color COL_BORDER   = {60, 60, 75, 255};
static const Color COL_HOVER    = {55, 55, 70, 255};
static const Color COL_SELECTED = {40, 80, 130, 255};
static const Color COL_ACCENT   = {220, 170, 40, 255};
static const Color COL_BTN      = {50, 50, 65, 255};
static const Color COL_BTN_HOV  = {70, 70, 90, 255};
static const Color COL_TEXT     = {220, 220, 230, 255};
static const Color COL_DIM      = {140, 140, 155, 255};

// ── Button Helper ──
static bool EdButton(Rectangle r, const char* text, Color bg, Color hover, int fontSize=16) {
    Vector2 m = GetMousePosition();
    bool hov = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, hov ? hover : bg);
    DrawRectangleLinesEx(r, 1, COL_BORDER);
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height/2 - fontSize/2), fontSize, COL_TEXT);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── Toolbar ──
void Editor::DrawToolbar() {
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, ED_TOOLBAR_H, COL_PANEL);
    DrawLine(0, ED_TOOLBAR_H, sw, ED_TOOLBAR_H, COL_BORDER);

    float x = 8;
    float bw = 70, bh = 30, gap = 4;
    float y = 7;

    // File ops
    if (EdButton({x,y,bw,bh}, "Save", COL_BTN, COL_BTN_HOV, 14)) SaveScene("../maps/editor_scene.txt");
    x += bw + gap;
    if (EdButton({x,y,bw,bh}, "Load", COL_BTN, COL_BTN_HOV, 14)) LoadScene("../maps/editor_scene.txt");
    x += bw + gap;
    if (EdButton({x,y,bw,bh}, "Reset", COL_BTN, COL_BTN_HOV, 14)) LoadFromMap(mapPtr);
    x += bw + gap + 10;

    // Separator
    DrawLine((int)x, 4, (int)x, ED_TOOLBAR_H-4, COL_BORDER);
    x += 10;

    // Gizmo mode buttons
    Color gCol = (gizmoMode == GizmoMode::TRANSLATE) ? COL_SELECTED : COL_BTN;
    if (EdButton({x,y,50,bh}, "Move", gCol, COL_BTN_HOV, 13)) gizmoMode = GizmoMode::TRANSLATE;
    x += 54;
    gCol = (gizmoMode == GizmoMode::ROTATE) ? COL_SELECTED : COL_BTN;
    if (EdButton({x,y,50,bh}, "Rot", gCol, COL_BTN_HOV, 13)) gizmoMode = GizmoMode::ROTATE;
    x += 54;
    gCol = (gizmoMode == GizmoMode::SCALE) ? COL_SELECTED : COL_BTN;
    if (EdButton({x,y,50,bh}, "Scale", gCol, COL_BTN_HOV, 13)) gizmoMode = GizmoMode::SCALE;
    x += 54 + 10;

    DrawLine((int)x, 4, (int)x, ED_TOOLBAR_H-4, COL_BORDER);
    x += 10;

    // Add entity dropdown
    if (EdButton({x,y,80,bh}, "+ Add", {60,100,60,255}, {80,130,80,255}, 14)) {
        addMenuOpen = addMenuOpen ? 0 : 1;
    }
    if (addMenuOpen) {
        float dy = y + bh + 2;
        EntityType types[] = {EntityType::BUILDING, EntityType::ENEMY_SPAWN, EntityType::STRUCTURE,
                              EntityType::VEHICLE_SPAWN, EntityType::SPAWN_POINT, EntityType::PROP,
                              EntityType::OBSTACLE, EntityType::LIGHT};
        int count = 8;
        DrawRectangle((int)x, (int)dy, 160, count*28+4, COL_PANEL);
        DrawRectangleLinesEx({x, dy, 160, (float)(count*28+4)}, 1, COL_BORDER);
        for (int i = 0; i < count; i++) {
            Rectangle item = {x+2, dy+2+(float)(i*28), 156, 26};
            if (EdButton(item, EntityTypeName(types[i]), COL_BTN, COL_BTN_HOV, 13)) {
                Vector3 gp = GetGroundPoint();
                AddEntity(types[i], gp);
                addMenuOpen = 0;
            }
        }
    }
    x += 84;

    // Delete / Duplicate
    if (selIdx >= 0) {
        if (EdButton({x,y,65,bh}, "Delete", {100,40,40,255}, {140,50,50,255}, 13)) DeleteSelected();
        x += 69;
        if (EdButton({x,y,65,bh}, "Dupe", COL_BTN, COL_BTN_HOV, 13)) DuplicateSelected();
        x += 69;
    }

    // View toggles
    x = sw - 190.0f;
    Color mapC = showMapGeometry ? COL_SELECTED : COL_BTN;
    if (EdButton({x,y,80,bh}, "Map", mapC, COL_BTN_HOV, 13)) showMapGeometry = !showMapGeometry;
    x += 84;
    Color gridC = showGrid ? COL_SELECTED : COL_BTN;
    if (EdButton({x,y,80,bh}, "Grid", gridC, COL_BTN_HOV, 13)) showGrid = !showGrid;
}

// ── Entity List (Left Panel) ──
void Editor::DrawEntityList() {
    int sh = GetScreenHeight();
    int panelH = sh - ED_TOOLBAR_H - ED_STATUS_H;
    DrawRectangle(0, ED_TOOLBAR_H, ED_LEFT_PANEL, panelH, COL_PANEL);
    DrawLine(ED_LEFT_PANEL, ED_TOOLBAR_H, ED_LEFT_PANEL, sh - ED_STATUS_H, COL_BORDER);

    // Header
    DrawRectangle(0, ED_TOOLBAR_H, ED_LEFT_PANEL, 28, COL_PANEL2);
    DrawText(TextFormat("ENTITIES (%d)", (int)entities.size()), 8, ED_TOOLBAR_H+6, 14, COL_ACCENT);
    DrawLine(0, ED_TOOLBAR_H+28, ED_LEFT_PANEL, ED_TOOLBAR_H+28, COL_BORDER);

    // Scroll with mouse wheel when hovering the panel
    Vector2 m = GetMousePosition();
    if (m.x < ED_LEFT_PANEL && m.y > ED_TOOLBAR_H) {
        entityListScroll -= GetMouseWheelMove() * 24;
        if (entityListScroll < 0) entityListScroll = 0;
        float maxScroll = fmaxf(0, (int)entities.size() * 26.0f - panelH + 40);
        if (entityListScroll > maxScroll) entityListScroll = maxScroll;
    }

    int startY = ED_TOOLBAR_H + 30 - (int)entityListScroll;
    int itemH = 26;
    BeginScissorMode(0, ED_TOOLBAR_H + 29, ED_LEFT_PANEL, panelH - 29);
    for (int i = 0; i < (int)entities.size(); i++) {
        int y = startY + i * itemH;
        if (y + itemH < ED_TOOLBAR_H || y > sh) continue;

        Rectangle r = {0, (float)y, (float)ED_LEFT_PANEL-1, (float)itemH};
        bool hov = CheckCollisionPointRec(m, r);
        Color bg = (i == selIdx) ? COL_SELECTED : (hov ? COL_HOVER : (Color){0,0,0,0});
        if (bg.a > 0) DrawRectangleRec(r, bg);

        // Type color indicator
        Color tc = GRAY;
        switch(entities[i].type) {
            case EntityType::BUILDING: tc = {100,100,110,255}; break;
            case EntityType::ENEMY_SPAWN: tc = RED; break;
            case EntityType::STRUCTURE: tc = BROWN; break;
            case EntityType::VEHICLE_SPAWN: tc = BLUE; break;
            case EntityType::LIGHT: tc = YELLOW; break;
            case EntityType::SPAWN_POINT: tc = GREEN; break;
            default: break;
        }
        DrawRectangle(4, y+4, 4, itemH-8, tc);

        const char* label = entities[i].name.c_str();
        if (MeasureText(label, 12) > ED_LEFT_PANEL - 20) {
            // Truncate
            char buf[32];
            snprintf(buf, sizeof(buf), "%.18s..", label);
            DrawText(buf, 14, y+7, 12, COL_TEXT);
        } else {
            DrawText(label, 14, y+7, 12, COL_TEXT);
        }

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selIdx = i;
            editingField = -1;
        }
    }
    EndScissorMode();
}

// ── Inspector Field Helper ──
static void DrawFieldLabel(int x, int y, const char* label) {
    DrawText(label, x, y, 13, COL_DIM);
}

static bool DrawFieldFloat(int x, int y, int w, const char* label, float* val,
                           int fieldId, int& editingField, char* editBuf, int& editCursor) {
    DrawFieldLabel(x, y, label);
    Rectangle r = {(float)(x+80), (float)y-2, (float)w, 20};
    Vector2 m = GetMousePosition();
    bool hov = CheckCollisionPointRec(m, r);
    bool editing = (editingField == fieldId);

    DrawRectangleRec(r, editing ? (Color){50,50,70,255} : (hov ? COL_HOVER : COL_BTN));
    DrawRectangleLinesEx(r, 1, editing ? COL_ACCENT : COL_BORDER);

    if (editing) {
        // Text input
        int key = GetCharPressed();
        while (key > 0) {
            if (editCursor < 60 && ((key >= '0' && key <= '9') || key == '.' || key == '-')) {
                editBuf[editCursor++] = (char)key;
                editBuf[editCursor] = 0;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && editCursor > 0) {
            editBuf[--editCursor] = 0;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_TAB)) {
            *val = (float)atof(editBuf);
            editingField = -1;
            return true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) { editingField = -1; }
        // Blinking cursor
        const char* display = TextFormat("%s|", editBuf);
        DrawText(display, (int)r.x+4, (int)r.y+3, 13, WHITE);
    } else {
        DrawText(TextFormat("%.2f", *val), (int)r.x+4, (int)r.y+3, 13, COL_TEXT);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            editingField = fieldId;
            snprintf(editBuf, 64, "%.2f", *val);
            editCursor = (int)strlen(editBuf);
        }
    }
    return false;
}

// ── Inspector ──
void Editor::DrawInspector() {
    auto* sel = GetSelected();
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int panelX = sw - ED_RIGHT_PANEL;
    int panelH = sh - ED_TOOLBAR_H - ED_STATUS_H;

    DrawRectangle(panelX, ED_TOOLBAR_H, ED_RIGHT_PANEL, panelH, COL_PANEL);
    DrawLine(panelX, ED_TOOLBAR_H, panelX, sh - ED_STATUS_H, COL_BORDER);

    // Header
    DrawRectangle(panelX, ED_TOOLBAR_H, ED_RIGHT_PANEL, 28, COL_PANEL2);
    DrawText("INSPECTOR", panelX+8, ED_TOOLBAR_H+6, 14, COL_ACCENT);
    DrawLine(panelX, ED_TOOLBAR_H+28, sw, ED_TOOLBAR_H+28, COL_BORDER);

    if (!sel) {
        DrawText("No entity selected", panelX+15, ED_TOOLBAR_H+50, 14, COL_DIM);
        return;
    }

    int x = panelX + 10;
    int y = ED_TOOLBAR_H + 38;
    int fw = ED_RIGHT_PANEL - 100;

    // Name
    DrawFieldLabel(x, y, "Name:");
    DrawText(sel->name.c_str(), x+80, y, 13, COL_TEXT);
    y += 24;

    // Type
    DrawFieldLabel(x, y, "Type:");
    DrawText(EntityTypeName(sel->type), x+80, y, 13, COL_ACCENT);
    y += 24;

    // ID
    DrawFieldLabel(x, y, "ID:");
    DrawText(TextFormat("%d", sel->id), x+80, y, 13, COL_DIM);
    y += 30;

    // Separator
    DrawLine(x, y, sw-10, y, COL_BORDER);
    y += 8;
    DrawText("TRANSFORM", x, y, 12, COL_DIM);
    y += 20;

    // Position
    DrawText("Position", x, y, 13, {180,180,200,255});
    y += 18;
    DrawFieldFloat(x, y, fw, "X:", &sel->position.x, 10, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "Y:", &sel->position.y, 11, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "Z:", &sel->position.z, 12, editingField, editBuf, editCursor); y += 28;

    // Scale
    DrawText("Scale", x, y, 13, {180,180,200,255});
    y += 18;
    DrawFieldFloat(x, y, fw, "W:", &sel->scale.x, 20, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "H:", &sel->scale.y, 21, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "D:", &sel->scale.z, 22, editingField, editBuf, editCursor); y += 28;

    // Rotation
    DrawText("Rotation", x, y, 13, {180,180,200,255});
    y += 18;
    DrawFieldFloat(x, y, fw, "RX:", &sel->rotation.x, 30, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "RY:", &sel->rotation.y, 31, editingField, editBuf, editCursor); y += 24;
    DrawFieldFloat(x, y, fw, "RZ:", &sel->rotation.z, 32, editingField, editBuf, editCursor); y += 28;

    // Separator
    DrawLine(x, y, sw-10, y, COL_BORDER);
    y += 8;
    DrawText("APPEARANCE", x, y, 12, COL_DIM);
    y += 20;

    // Color preview
    DrawFieldLabel(x, y, "Color:");
    DrawRectangle(x+80, y-2, 30, 18, sel->color);
    DrawRectangleLinesEx({(float)(x+80), (float)(y-2), 30, 18}, 1, COL_BORDER);
    DrawText(TextFormat("(%d,%d,%d)", sel->color.r, sel->color.g, sel->color.b), x+116, y, 12, COL_DIM);
    y += 26;

    // Color sliders
    {
        float r_f = sel->color.r / 255.0f;
        float g_f = sel->color.g / 255.0f;
        float b_f = sel->color.b / 255.0f;

        auto drawSlider = [&](const char* label, float* val, Color sliderCol, int yPos) {
            DrawText(label, x, yPos, 12, sliderCol);
            Rectangle bar = {(float)(x+20), (float)yPos, (float)(fw+60), 14};
            DrawRectangleRec(bar, COL_BTN);
            DrawRectangle((int)bar.x, yPos, (int)(bar.width * (*val)), 14, sliderCol);
            DrawRectangleLinesEx(bar, 1, COL_BORDER);
            Vector2 mo = GetMousePosition();
            if (CheckCollisionPointRec(mo, bar) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                *val = Clamp((mo.x - bar.x) / bar.width, 0, 1);
            }
        };
        drawSlider("R", &r_f, RED, y); y += 18;
        drawSlider("G", &g_f, GREEN, y); y += 18;
        drawSlider("B", &b_f, BLUE, y); y += 22;

        sel->color.r = (unsigned char)(r_f * 255);
        sel->color.g = (unsigned char)(g_f * 255);
        sel->color.b = (unsigned char)(b_f * 255);
    }

    // Visible toggle
    DrawFieldLabel(x, y, "Visible:");
    Rectangle vr = {(float)(x+80), (float)y-2, 20, 18};
    DrawRectangleRec(vr, sel->visible ? GREEN : COL_BTN);
    DrawRectangleLinesEx(vr, 1, COL_BORDER);
    if (sel->visible) DrawText("v", x+84, y-1, 14, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), vr) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        sel->visible = !sel->visible;
    }
}

// ── Status Bar ──
void Editor::DrawStatusBar() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int y = sh - ED_STATUS_H;
    DrawRectangle(0, y, sw, ED_STATUS_H, COL_PANEL2);
    DrawLine(0, y, sw, y, COL_BORDER);

    // Camera position
    DrawText(TextFormat("Cam: (%.0f, %.0f, %.0f)  Speed: %.0f",
        cam.position.x, cam.position.y, cam.position.z, camSpeed),
        10, y+7, 12, COL_DIM);

    // Gizmo mode
    const char* modeStr = (gizmoMode == GizmoMode::TRANSLATE) ? "TRANSLATE [G]" :
                          (gizmoMode == GizmoMode::ROTATE) ? "ROTATE [R]" : "SCALE [T]";
    DrawText(modeStr, sw/2 - 40, y+7, 12, COL_ACCENT);

    // Selection info
    if (selIdx >= 0) {
        auto* s = GetSelected();
        if (s) DrawText(TextFormat("Selected: %s (ID:%d)", s->name.c_str(), s->id), sw-300, y+7, 12, COL_TEXT);
    }

    // Shortcuts hint
    DrawText("RMB=Camera  LMB=Select  G/R/T=Mode  Del=Delete  Ctrl+D=Dupe  Esc=Exit", 10, y+7, 11, Fade(COL_DIM, 0.6f));
}

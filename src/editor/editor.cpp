#include "editor/editor.hpp"
#include "map/city_map.hpp"
#include "map/desert_map.hpp"
#include <cstring>
#include <cmath>

// ── EntityType names ──
const char* EntityTypeName(EntityType t) {
    switch(t) {
        case EntityType::BUILDING:      return "Building";
        case EntityType::ENEMY_SPAWN:   return "Enemy Spawn";
        case EntityType::STRUCTURE:     return "Structure";
        case EntityType::VEHICLE_SPAWN: return "Vehicle Spawn";
        case EntityType::LIGHT:         return "Light";
        case EntityType::SHOP:          return "Shop";
        case EntityType::OBSTACLE:      return "Obstacle";
        case EntityType::PROP:          return "Prop";
        case EntityType::SPAWN_POINT:   return "Spawn Point";
    }
    return "Unknown";
}

// ── EditorEntity ──
BoundingBox EditorEntity::GetBoundingBox() const {
    Vector3 half = {scale.x/2, scale.y/2, scale.z/2};
    return {
        {position.x - half.x, position.y - half.y, position.z - half.z},
        {position.x + half.x, position.y + half.y, position.z + half.z}
    };
}

void EditorEntity::Draw() const {
    if (!visible) return;
    switch(type) {
        case EntityType::BUILDING:
            DrawCube(position, scale.x, scale.y, scale.z, color);
            DrawCubeWires(position, scale.x, scale.y, scale.z, Fade(BLACK, 0.3f));
            break;
        case EntityType::ENEMY_SPAWN:
            DrawSphere(position, 1.5f, Fade(RED, 0.7f));
            DrawCircle3D(position, 4.0f, {1,0,0}, 90, RED);
            DrawCube({position.x, position.y+2.5f, position.z}, 0.2f, 2.0f, 0.2f, RED);
            break;
        case EntityType::STRUCTURE:
            DrawCube(position, scale.x, scale.y, scale.z, color);
            DrawCubeWires(position, scale.x, scale.y, scale.z, Fade(BLACK, 0.4f));
            break;
        case EntityType::VEHICLE_SPAWN:
            DrawCube(position, 4,2,8, Fade(BLUE, 0.6f));
            DrawCubeWires(position, 4,2,8, SKYBLUE);
            break;
        case EntityType::LIGHT:
            DrawSphere(position, 1.0f, YELLOW);
            DrawCircle3D(position, 10.0f, {0,1,0}, 0, Fade(YELLOW, 0.15f));
            break;
        case EntityType::SHOP:
            DrawCube(position, 3,5,3, Fade(PURPLE, 0.7f));
            DrawCubeWires(position, 3,5,3, VIOLET);
            break;
        case EntityType::OBSTACLE:
            DrawCubeWires(position, scale.x, scale.y, scale.z, YELLOW);
            break;
        case EntityType::PROP:
            DrawCube(position, scale.x, scale.y, scale.z, color);
            DrawCubeWires(position, scale.x, scale.y, scale.z, Fade(BLACK, 0.3f));
            break;
        case EntityType::SPAWN_POINT:
            DrawSphere(position, 1.2f, Fade(GREEN, 0.7f));
            DrawCircle3D(position, 3.0f, {1,0,0}, 90, GREEN);
            DrawCube({position.x, position.y+2.0f, position.z}, 0.15f, 2.0f, 0.15f, GREEN);
            break;
    }
}

void EditorEntity::DrawWireframe() const {
    BoundingBox bb = GetBoundingBox();
    Vector3 sz = {bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z};
    Vector3 cen = {(bb.min.x+bb.max.x)/2, (bb.min.y+bb.max.y)/2, (bb.min.z+bb.max.z)/2};
    DrawCubeWires(cen, sz.x+0.2f, sz.y+0.2f, sz.z+0.2f, ORANGE);
}

// ── Editor Constructor / Destructor ──
Editor::Editor() {}
Editor::~Editor() {}

void Editor::Init(Map* gameMap) {
    cam.position = {0, 80, -100};
    cam.target   = {0, 0, 50};
    cam.up       = {0, 1, 0};
    cam.fovy     = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    mapPtr = gameMap;
    LoadFromMap(gameMap);
}

// ── Helpers ──
Rectangle Editor::GetViewportRect() {
    return {(float)ED_LEFT_PANEL, (float)ED_TOOLBAR_H,
            (float)(GetScreenWidth()-ED_LEFT_PANEL-ED_RIGHT_PANEL),
            (float)(GetScreenHeight()-ED_TOOLBAR_H-ED_STATUS_H)};
}

bool Editor::IsInViewport(Vector2 p) {
    return CheckCollisionPointRec(p, GetViewportRect());
}

Vector3 Editor::GetGroundPoint() {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    if (fwd.y >= -0.01f) return {cam.target.x, 0, cam.target.z};
    float t = -cam.position.y / fwd.y;
    return Vector3Add(cam.position, Vector3Scale(fwd, t));
}

EditorEntity* Editor::GetSelected() {
    if (selIdx >= 0 && selIdx < (int)entities.size()) return &entities[selIdx];
    return nullptr;
}

// ── Camera ──
void Editor::UpdateCamera() {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsInViewport(GetMousePosition())) {
        Vector2 delta = GetMouseDelta();
        Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));

        float yaw = -delta.x * camSens;
        float pitch = -delta.y * camSens;

        Matrix yawM = MatrixRotate(cam.up, yaw);
        fwd = Vector3Transform(fwd, yawM);
        right = Vector3Transform(right, yawM);

        Matrix pitchM = MatrixRotate(right, pitch);
        Vector3 newFwd = Vector3Transform(fwd, pitchM);
        if (fabsf(Vector3DotProduct(newFwd, cam.up)) < 0.98f) fwd = newFwd;

        cam.target = Vector3Add(cam.position, fwd);
        if (!IsCursorHidden()) DisableCursor();
    } else {
        if (IsCursorHidden()) EnableCursor();
    }

    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
    float spd = camSpeed * GetFrameTime();
    if (IsKeyDown(KEY_LEFT_SHIFT)) spd *= 3.0f;

    Vector3 move = {0,0,0};
    if (IsKeyDown(KEY_W)) move = Vector3Add(move, Vector3Scale(fwd, spd));
    if (IsKeyDown(KEY_S)) move = Vector3Add(move, Vector3Scale(fwd, -spd));
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, Vector3Scale(right, spd));
    if (IsKeyDown(KEY_A)) move = Vector3Add(move, Vector3Scale(right, -spd));
    if (IsKeyDown(KEY_SPACE))       move.y += spd;
    if (IsKeyDown(KEY_LEFT_CONTROL)) move.y -= spd;

    cam.position = Vector3Add(cam.position, move);
    cam.target   = Vector3Add(cam.target, move);

    float scroll = GetMouseWheelMove();
    if (scroll != 0) { camSpeed += scroll * 8.0f; camSpeed = Clamp(camSpeed, 5, 500); }
}

// ── Selection (Raycasting) ──
void Editor::UpdateSelection() {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    Vector2 mouse = GetMousePosition();
    if (!IsInViewport(mouse)) return;
    if (dragging) return;

    // Check gizmo first (handled in UpdateGizmo)
    Ray ray = GetScreenToWorldRay(mouse, cam);
    float closest = 99999.0f;
    int hit = -1;

    for (int i = 0; i < (int)entities.size(); i++) {
        if (!entities[i].visible) continue;
        RayCollision rc = GetRayCollisionBox(ray, entities[i].GetBoundingBox());
        if (rc.hit && rc.distance < closest) {
            closest = rc.distance;
            hit = i;
        }
    }
    selIdx = hit;
    editingField = -1;
}

// ── Input ──
void Editor::UpdateInput() {
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)) DeleteSelected();
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D)) DuplicateSelected();
    if (IsKeyPressed(KEY_G)) gizmoMode = GizmoMode::TRANSLATE;
    if (IsKeyPressed(KEY_R)) gizmoMode = GizmoMode::ROTATE;
    if (IsKeyPressed(KEY_T)) gizmoMode = GizmoMode::SCALE;
    if (IsKeyPressed(KEY_H)) showGrid = !showGrid;
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (selIdx >= 0) { selIdx = -1; editingField = -1; }
        else shouldExit = true;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) SaveScene("../maps/editor_scene.txt");
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) LoadScene("../maps/editor_scene.txt");
}

// ── Entity Management ──
int Editor::AddEntity(EntityType type, Vector3 pos) {
    EditorEntity e;
    e.id = nextId++;
    e.type = type;
    e.position = pos;
    e.name = std::string(EntityTypeName(type)) + "_" + std::to_string(e.id);
    switch(type) {
        case EntityType::BUILDING:
            e.scale = {30, 100, 30}; e.color = {80,80,85,255};
            e.position.y = e.scale.y / 2;
            break;
        case EntityType::STRUCTURE:
            e.scale = {8,4,2.5f}; e.color = {80,65,45,255};
            e.position.y = 2;
            break;
        case EntityType::OBSTACLE:
            e.scale = {10,10,10}; e.color = {255,255,0,255};
            break;
        case EntityType::PROP:
            e.scale = {3,3,3}; e.color = {120,120,130,255};
            break;
        default:
            e.scale = {1,1,1}; e.color = WHITE;
            break;
    }
    entities.push_back(e);
    selIdx = (int)entities.size()-1;
    return e.id;
}

void Editor::DeleteSelected() {
    if (selIdx < 0 || selIdx >= (int)entities.size()) return;
    entities.erase(entities.begin() + selIdx);
    selIdx = -1;
    editingField = -1;
}

void Editor::DuplicateSelected() {
    auto* s = GetSelected();
    if (!s) return;
    EditorEntity dup = *s;
    dup.id = nextId++;
    dup.name = dup.name + "_copy";
    dup.position.x += 5;
    entities.push_back(dup);
    selIdx = (int)entities.size()-1;
}

// ── Load From Actual Map ──
void Editor::LoadFromMap(Map* map) {
    entities.clear();
    nextId = 1;
    selIdx = -1;

    if (!map) return;

    CityMap* city = dynamic_cast<CityMap*>(map);
    if (city) {
        // Import all real buildings
        for (auto& b : city->GetBuildings()) {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::BUILDING;
            e.position = b.pos;
            e.scale = b.size;
            e.color = b.color;
            e.name = "Building_" + std::to_string(e.id);
            entities.push_back(e);
        }

        // Import obstacles that aren't from buildings (base, props, etc.)
        // We skip building-derived obstacles since they'll be auto-generated
        // Import the base position as a special entity
        Vector3 bp = city->GetBasePos();
        {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::STRUCTURE;
            e.position = {bp.x, 120, bp.z};
            e.scale = {50, 240, 50};
            e.color = {70, 75, 90, 255};
            e.name = "MainBase";
            e.properties["baseFloors"] = "30";
            entities.push_back(e);
        }

        // Kebab stand
        {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::SHOP;
            e.position = {bp.x + 15, 1.0f, bp.z + 10};
            e.scale = {8, 4, 4};
            e.color = {128, 0, 0, 254};
            e.name = "KebabStand";
            entities.push_back(e);
        }

        // Tank terminal
        {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::SHOP;
            e.position = {15, 1, 120};
            e.scale = {2.5f, 3, 2};
            e.color = {80, 80, 90, 255};
            e.name = "TankTerminal";
            e.properties["cost"] = "100000";
            entities.push_back(e);
        }

        // Player spawn
        {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::SPAWN_POINT;
            e.position = {0, 2, 0};
            e.scale = {1,1,1};
            e.color = GREEN;
            e.name = "PlayerSpawn";
            entities.push_back(e);
        }

        // Enemy spawn area
        {
            EditorEntity e;
            e.id = nextId++;
            e.type = EntityType::ENEMY_SPAWN;
            e.position = {0, 0, -200};
            e.scale = {1,1,1};
            e.color = RED;
            e.name = "EnemySpawnZone";
            entities.push_back(e);
        }
    }
}

// ── Save — writes directly to map data file ──
void Editor::SaveScene(const char* path) {
    // Sync editor entities to the live map and save the map data file
    ApplyToMap();
    CityMap* city = dynamic_cast<CityMap*>(mapPtr);
    if (city) {
        city->SaveBuildingsToFile("../maps/city_buildings.dat");
        city->RebuildObstaclesFromBuildings();
    }
    // Also save full editor scene for re-opening later
    FILE* f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "ADASGOONER_SCENE\n");
    fprintf(f, "COUNT %d\n", (int)entities.size());
    for (auto& e : entities) {
        fprintf(f, "ENTITY\n");
        fprintf(f, "ID %d\n", e.id);
        fprintf(f, "NAME %s\n", e.name.c_str());
        fprintf(f, "TYPE %d\n", (int)e.type);
        fprintf(f, "POS %.2f %.2f %.2f\n", e.position.x, e.position.y, e.position.z);
        fprintf(f, "ROT %.2f %.2f %.2f\n", e.rotation.x, e.rotation.y, e.rotation.z);
        fprintf(f, "SCALE %.2f %.2f %.2f\n", e.scale.x, e.scale.y, e.scale.z);
        fprintf(f, "COLOR %d %d %d %d\n", e.color.r, e.color.g, e.color.b, e.color.a);
        fprintf(f, "VISIBLE %d\n", e.visible ? 1 : 0);
        for (auto& [k,v] : e.properties) {
            fprintf(f, "PROP %s %s\n", k.c_str(), v.c_str());
        }
        fprintf(f, "END\n");
    }
    fclose(f);
}

void Editor::LoadScene(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    entities.clear();
    selIdx = -1;
    char line[256];
    EditorEntity cur;
    bool inEntity = false;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "ENTITY", 6) == 0) { cur = {}; inEntity = true; }
        else if (strncmp(line, "END", 3) == 0 && inEntity) {
            entities.push_back(cur);
            if (cur.id >= nextId) nextId = cur.id + 1;
            inEntity = false;
        }
        else if (inEntity) {
            if (strncmp(line,"ID ",3)==0) sscanf(line,"ID %d",&cur.id);
            else if (strncmp(line,"NAME ",5)==0) { char n[128]; sscanf(line,"NAME %127[^\n]",n); cur.name=n; }
            else if (strncmp(line,"TYPE ",5)==0) { int t; sscanf(line,"TYPE %d",&t); cur.type=(EntityType)t; }
            else if (strncmp(line,"POS ",4)==0) sscanf(line,"POS %f %f %f",&cur.position.x,&cur.position.y,&cur.position.z);
            else if (strncmp(line,"ROT ",4)==0) sscanf(line,"ROT %f %f %f",&cur.rotation.x,&cur.rotation.y,&cur.rotation.z);
            else if (strncmp(line,"SCALE ",6)==0) sscanf(line,"SCALE %f %f %f",&cur.scale.x,&cur.scale.y,&cur.scale.z);
            else if (strncmp(line,"COLOR ",6)==0) {
                int r,g,b,a; sscanf(line,"COLOR %d %d %d %d",&r,&g,&b,&a);
                cur.color = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            }
            else if (strncmp(line,"VISIBLE ",8)==0) { int v; sscanf(line,"VISIBLE %d",&v); cur.visible=v; }
            else if (strncmp(line,"PROP ",5)==0) { char k[64],v[128]; sscanf(line,"PROP %63s %127[^\n]",k,v); cur.properties[k]=v; }
        }
    }
    fclose(f);
}

// ── Main Update ──
void Editor::Update() {
    UpdateCamera();
    if (editingField < 0) {
        UpdateInput();
        UpdateGizmo();
        if (!dragging) UpdateSelection();
    }
}

// ── Grid ──
void Editor::DrawGrid3D() {
    if (!showGrid) return;
    int range = 500;
    int step = 20;
    for (int i = -range; i <= range; i += step) {
        Color c = (i == 0) ? Fade(WHITE, 0.3f) : Fade(GRAY, 0.1f);
        DrawLine3D({(float)i, 0, (float)-range}, {(float)i, 0, (float)range}, c);
        DrawLine3D({(float)-range, 0, (float)i}, {(float)range, 0, (float)i}, c);
    }
    // Axis lines
    DrawLine3D({0,0,0}, {50,0,0}, RED);
    DrawLine3D({0,0,0}, {0,50,0}, GREEN);
    DrawLine3D({0,0,0}, {0,0,50}, BLUE);
}

// ── Draw Entities (overlays only when map renders buildings) ──
void Editor::DrawEntities() {
    for (int i = 0; i < (int)entities.size(); i++) {
        float dist = Vector3Distance(entities[i].position, cam.position);
        if (dist > 1500.0f && entities[i].type == EntityType::BUILDING) continue;

        // When map geometry is rendered, buildings are already drawn by the map
        // Only draw non-building entities + selection wireframe
        if (showMapGeometry && entities[i].type == EntityType::BUILDING) {
            // Just draw selection highlight, map already rendered the building
            if (i == selIdx) entities[i].DrawWireframe();
            continue;
        }
        entities[i].Draw();
        if (i == selIdx) entities[i].DrawWireframe();
    }
}

// ── Main Render ──
void Editor::Render() {
    // Sync building positions to map before rendering so changes show immediately
    ApplyToMap();

    BeginDrawing();
    ClearBackground(SKYBLUE);

    // 3D viewport
    BeginMode3D(cam);
    DrawGrid3D();

    // Render the actual game map
    if (showMapGeometry && mapPtr) {
        mapPtr->Draw(1, cam.position, 1);
    }

    // Draw editor-only entities (spawns, shops, etc.) + selection highlights
    DrawEntities();
    DrawGizmo();
    EndMode3D();

    // 2D UI overlays
    DrawToolbar();
    DrawEntityList();
    DrawInspector();
    DrawStatusBar();

    EndDrawing();
}

// ── Apply editor changes back to the live map ──
void Editor::ApplyToMap() {
    if (!mapPtr) return;

    CityMap* city = dynamic_cast<CityMap*>(mapPtr);
    if (city) {
        auto& buildings = city->GetBuildings();
        // Rebuild buildings vector from editor entities
        buildings.clear();
        for (auto& e : entities) {
            if (e.type == EntityType::BUILDING) {
                CityMap::Building b;
                b.pos = e.position;
                b.size = e.scale;
                b.color = e.color;
                buildings.push_back(b);
            }
        }
        // Rebuild obstacles from buildings
        auto& obstacles = city->GetObstaclesMut();
        obstacles.clear();
        for (auto& b : buildings) {
            float hw = b.size.x/2, hh = b.size.y, hd = b.size.z/2;
            obstacles.push_back({
                {b.pos.x - hw, 0, b.pos.z - hd},
                {b.pos.x + hw, hh, b.pos.z + hd}
            });
        }
    }
}

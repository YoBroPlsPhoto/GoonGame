#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <cstdio>
#include <algorithm>

// Forward declarations
class Map;
class CityMap;

// ── Entity Types ──
enum class EntityType {
    BUILDING, ENEMY_SPAWN, STRUCTURE, VEHICLE_SPAWN,
    LIGHT, SHOP, OBSTACLE, PROP, SPAWN_POINT
};

const char* EntityTypeName(EntityType t);

// ── Gizmo ──
enum class GizmoMode { TRANSLATE, ROTATE, SCALE };
enum class GizmoAxis { NONE, X, Y, Z };

// ── Editor Entity ──
struct EditorEntity {
    int id = 0;
    std::string name;
    EntityType type = EntityType::PROP;
    Vector3 position = {0,0,0};
    Vector3 rotation = {0,0,0};
    Vector3 scale = {1,1,1};
    Color color = {128,128,128,255};
    bool visible = true;
    std::map<std::string, std::string> properties;

    BoundingBox GetBoundingBox() const;
    void Draw() const;
    void DrawWireframe() const;
};

// ── UI Constants ──
static const int ED_LEFT_PANEL  = 220;
static const int ED_RIGHT_PANEL = 300;
static const int ED_TOOLBAR_H   = 44;
static const int ED_STATUS_H    = 28;

// ── Editor Class ──
class Editor {
public:
    Editor();
    ~Editor();

    void Init(Map* gameMap);
    void Update();
    void Render();

    bool shouldExit = false;

private:
    // Camera
    Camera3D cam;
    float camSpeed = 60.0f;
    float camSens  = 0.003f;
    Map* mapPtr = nullptr;

    // Entities
    std::vector<EditorEntity> entities;
    int nextId = 1;
    int selIdx = -1; // selected index, -1 = none

    // Gizmo
    GizmoMode gizmoMode = GizmoMode::TRANSLATE;
    GizmoAxis hoveredAxis = GizmoAxis::NONE;
    GizmoAxis activeAxis  = GizmoAxis::NONE;
    bool dragging = false;
    Vector2 dragPrev;

    // UI state
    bool showGrid = true;
    bool showMapGeometry = true; // render actual map
    int addMenuOpen = 0; // 0=closed, 1=open
    float entityListScroll = 0;

    // Inspector editing
    int editingField = -1; // which field is being text-edited
    char editBuf[64] = {0};
    int editCursor = 0;

    // Core methods
    void UpdateCamera();
    void UpdateSelection();
    void UpdateGizmo();
    void UpdateInput();

    // Rendering
    void DrawGrid3D();
    void DrawEntities();
    void DrawGizmo();
    void DrawToolbar();
    void DrawEntityList();
    void DrawInspector();
    void DrawStatusBar();

    // Entity management
    int AddEntity(EntityType type, Vector3 pos);
    void DeleteSelected();
    void DuplicateSelected();
    void LoadFromMap(Map* map);
    void ApplyToMap();  // sync editor entities back to the map
    EditorEntity* GetSelected();

    // Save / Load
    void SaveScene(const char* path);
    void LoadScene(const char* path);

    // Helpers
    bool IsInViewport(Vector2 pos);
    Rectangle GetViewportRect();
    Vector3 GetGroundPoint(); // where camera looks at ground
};

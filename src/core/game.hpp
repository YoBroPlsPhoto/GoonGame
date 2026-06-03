#pragma once
#include "editor/editor.hpp"
#include "bosses/adas_gooner.hpp"
#include "bosses/adas_prime.hpp"
#include "bosses/gang_boss.hpp"
#include "bosses/gibon_rzygacz.hpp"
#include "effects/particle_system.hpp"
#include "enemies/enemy.hpp"
#include "hud/menu.hpp"
#include "map/city_map.hpp"
#include "map/desert_map.hpp"
#include "map/map.hpp"
#include "network/network_manager.hpp"
#include "player/player.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "vehicles/tank.hpp"
#include "weapons/ak47.hpp"
#include "weapons/awp.hpp"
#include "weapons/minigun.hpp"
#include "weapons/revolver.hpp"
#include "weapons/rpg.hpp"
#include "weapons/shotgun.hpp"
#include "structures/structure.hpp"
#include "weapons/builder_tool.hpp"
#include <algorithm>
#include <memory>
#include <time.h>
#include <vector>
#include <map>

enum class GameState { MENU, LOBBY, GAME, PAUSED, EDITOR };

class Game {
public:
    Game();
    ~Game();
    void Init();
    void Run();

private:
    struct AdasDrop {
        Vector3 position;
        float radius;
        float fallSpeed;
    };

    struct AdasStain {
        Vector3 position;
        float radius;
    };

    void UpdateCore();
    void HandleInput();
    void UpdateNetworkAndEnemies();
    void Render();
    void UpdateAdasHazards();
    void DrawAdasHazards();
    bool IsLocalPlayerInAdasStain() const;

    NetworkManager net;
    GameState state;
    Menu menu;
    Player localPlayer;
    Map *currentMap;
    ParticleSystem effects;
    Editor editor;

    Shader lighting;
    Texture2D sunTex;
    int viewPosLoc, lightPosLoc, lightColorLoc, reflTexLoc, shadowTexLoc, shadowVPLoc, matViewLoc;
    Vector3 lightPos;
    Vector4 lightCol;
    RenderTexture2D reflectionTarget, shadowTarget, worldTarget;
    Camera3D shadowCam;
    bool shouldExit;

    float maxBaseHP;
    float baseHP;
    std::vector<std::shared_ptr<Enemy>> enemies;
    float spawnTimer;
    Vector3 basePosition;
    int currentWave;
    int enemiesSpawnedSinceStartOfWave;
    std::vector<std::shared_ptr<Vehicle>> vehicles;
    bool waveActive;
    float waveWaitTimer;

    Music goonMusic;
    Music gibonMusic;
    bool goonMusicPlaying;
    bool gibonMusicPlaying;

    int enemyIdCounter;
    std::map<int, float> remoteFireCooldowns;
    float preferredFOV;
    int worldDetailLevel;
    
    int cheatIdx;
    char cheatCode[5];
    int moneyIdx;
    char moneyCheat[5];
    int gibonIdx;
    char gibonCheat[6];
    int gangIdx;
    char gangCheat[9];
    int primeIdx;
    char primeCheat[6];

    float vehicleCamOrbitH;
    float vehicleCamOrbitV;
    float firingStickyTimer;
    
    bool shopNearby;
    bool inCutscene;
    Vector3 bossPos;
    std::shared_ptr<Enemy> activeBoss;
    std::vector<AdasDrop> adasDrops;
    std::vector<AdasStain> adasStains;
    float adasDropTimer;
};

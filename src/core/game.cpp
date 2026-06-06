#include "core/game.hpp"
#include <cstring>

Game::Game() : localPlayer((Vector3){0.0f, 2.0f, 0.0f}, 0), currentMap(new CityMap()) {
    state = GameState::MENU;
    shouldExit = false;
    maxBaseHP = 60000.0f;
    baseHP = 60000.0f;
    spawnTimer = 0.0f;
    basePosition = {0, 0, 150};
    currentWave = 1;
    enemiesSpawnedSinceStartOfWave = 0;
    waveActive = true;
    waveWaitTimer = 0.0f;
    goonMusicPlaying = false;
    gibonMusicPlaying = false;
    enemyIdCounter = 0;
    preferredFOV = 70.0f;
    worldDetailLevel = 1;
    cheatIdx = 0;
    strcpy(cheatCode, "goon");
    moneyIdx = 0;
    strcpy(moneyCheat, "cash");
    gibonIdx = 0;
    strcpy(gibonCheat, "gibon");
    gangIdx = 0;
    strcpy(gangCheat, "gangbang");
    primeIdx = 0;
    strcpy(primeCheat, "prime");
    vehicleCamOrbitH = 0.0f;
    vehicleCamOrbitV = 0.3f;
    firingStickyTimer = 0.0f;
    shopNearby = false;
    inCutscene = false;
    bossPos = {0};
    activeBoss = nullptr;
    adasDropTimer = 0.0f;
}
Game::~Game() {}

void Game::Init() {

  goonMusic = LoadMusicStream("../audio/goon.mp3");
  gibonMusic = LoadMusicStream("../audio/GibonUlaniecPart3 (1).mp3");
  
  
  SetExitKey(KEY_NULL);

  
  net.StartDiscovery();

  
  
  
  localPlayer.playerId = 0;

  
  

  // SHADER LOAD
  lighting = LoadShader("../resources/shaders/lighting.vs",
                               "../resources/shaders/lighting.fs");

  Image sunImg = LoadImage("../textures/sun.png");
  ImageColorReplace(&sunImg, (Color){102, 191, 255, 255},
                    BLANK); // Replace skyblue with transparency
  sunTex = LoadTextureFromImage(sunImg);
  UnloadImage(sunImg);

  AdasGooner::LoadSharedResources();

  viewPosLoc = GetShaderLocation(lighting, "viewPos");
  lightPosLoc = GetShaderLocation(lighting, "lightPos");
  lightColorLoc = GetShaderLocation(lighting, "lightColor");
  reflTexLoc = GetShaderLocation(lighting, "reflectionTexture");
  shadowTexLoc = GetShaderLocation(lighting, "shadowMap");
  shadowVPLoc = GetShaderLocation(lighting, "shadowVP");
  matViewLoc = GetShaderLocation(lighting, "matView");

  lightPos = {-150.0f, 450.0f, 350.0f};
  lightCol = {1.5f, 1.5f, 1.4f, 1.0f};
  if (lightPosLoc >= 0)
    SetShaderValue(lighting, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
  if (lightColorLoc >= 0)
    SetShaderValue(lighting, lightColorLoc, &lightCol, SHADER_UNIFORM_VEC4);

  // RENDER TARGETS
  reflectionTarget = LoadRenderTexture(512, 512); // Optimized
  shadowTarget = LoadRenderTexture(1024, 1024);   // Optimized

  // RENDER RESOLUTION TARGET
  worldTarget =
      LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  SetTextureFilter(reflectionTarget.texture, TEXTURE_FILTER_BILINEAR);
  SetTextureFilter(shadowTarget.texture, TEXTURE_FILTER_BILINEAR);

  // SHADOW CAMERA (ORTHO for Sun)
  shadowCam = {0};
  shadowCam.position = lightPos;
  shadowCam.target = (Vector3){0, 0, 0};
  shadowCam.up = (Vector3){0, 1, 0};
  shadowCam.fovy = 800.0f; // Adjusted
  shadowCam.projection = CAMERA_ORTHOGRAPHIC;

  SetTargetFPS(menu.vsync ? 60 : 144);

  

  // --- GAMEPLAY STATS ---
  
  
  
  
  

  // Wave System
  
  
  
  
  

                    
  }

void Game::Run() {
    while (!shouldExit && !WindowShouldClose()) {
        // F12 toggles editor from any state
        if (IsKeyPressed(KEY_F12)) {
            if (state == GameState::EDITOR) {
                state = GameState::MENU;
                if (IsCursorHidden()) EnableCursor();
            } else {
                state = GameState::EDITOR;
                editor.Init(currentMap);
            }
        }

        if (state == GameState::EDITOR) {
            editor.Update();
            editor.Render();
            if (editor.shouldExit) {
                state = GameState::MENU;
                editor.shouldExit = false;
                if (IsCursorHidden()) EnableCursor();
            }
            if (editor.shouldPlay) {
                editor.shouldPlay = false;
                // Reload map from saved buildings data — CityMap constructor
                // loads from file and rebuilds ALL obstacles correctly
                delete currentMap;
                currentMap = new CityMap();
                // Reset game state for play testing
                baseHP = maxBaseHP;
                currentWave = 1;
                enemiesSpawnedSinceStartOfWave = 0;
                waveActive = true;
                waveWaitTimer = 0;
                enemies.clear();
                vehicles.clear();
                structures.clear();
                spawnTimer = 0;
                inCutscene = false;
                activeBoss = nullptr;
                adasDrops.clear();
                adasStains.clear();
                adasDropTimer = 0.0f;
                state = GameState::GAME;
                if (IsCursorHidden()) EnableCursor();
                DisableCursor();
                printf("[EDITOR] Play testing map...\n");
            }
        } else {
            UpdateCore();
            HandleInput();
            UpdateNetworkAndEnemies();
            Render();
        }
    }
    
    UnloadRenderTexture(reflectionTarget);
    UnloadRenderTexture(shadowTarget);
    UnloadRenderTexture(worldTarget);
    UnloadShader(lighting);
    UnloadTexture(sunTex);
    
    CloseAudioDevice();
    CloseWindow();
}

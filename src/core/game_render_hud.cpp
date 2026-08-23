#include "core/game.hpp"
#include "../weapons/glock.hpp"

// Helper: draw a rounded bar with background
static void DrawStatBar(int x, int y, int w, int h, float ratio, Color fillCol, Color bgCol) {
    DrawRectangleRounded({(float)x, (float)y, (float)w, (float)h}, 0.5f, 8, bgCol);
    if (ratio > 0.0f) {
        float fillW = w * ratio;
        if (fillW < h) fillW = h; // min width so rounded doesn't glitch
        DrawRectangleRounded({(float)x, (float)y, fillW, (float)h}, 0.5f, 8, fillCol);
    }
    DrawRectangleRoundedLines({(float)x, (float)y, (float)w, (float)h}, 0.5f, 8, 1, Fade(WHITE, 0.15f));
}

void Game::RenderHUD() {

    // ──────────────────────────────────────────────────────────
    // WORLD-SPACE: Enemy health bars over heads
    // ──────────────────────────────────────────────────────────
    if (net.mode == NetworkManager::Mode::SERVER) {
        for (auto &e : enemies) {
            if (e->type != EnemyType::BOSS && e->type != EnemyType::GIBON_BOSS &&
                e->type != EnemyType::GANG_BOSS &&
                e->type != EnemyType::ADAS_PRIME &&
                e->type != EnemyType::LUCA_BOSS) {
                e->DrawHUD(activeCam);
            }
        }
    } else {
        for (auto const &[id, e] : net.syncedEnemies) {
            std::shared_ptr<Enemy> temp;
            if (e.type == (int)EnemyType::BOSS) {
                auto adas = std::make_shared<AdasGooner>(e.pos, e.id);
                adas->cutsceneState = (CutsceneState)(int)e.attackTimer;
                adas->cutsceneTimer = e.walkTimer;
                temp = adas;
            } else if (e.type == (int)EnemyType::GIBON_BOSS) {
                auto gibon = std::make_shared<GibonRzygacz>(e.pos, e.id);
                gibon->gibonState = (GibonState)(int)e.attackTimer;
                gibon->stateTimer = e.walkTimer;
                temp = gibon;
            } else if (e.type == (int)EnemyType::GANG_BOSS) {
                auto gang = std::make_shared<GangBoss>(e.pos, e.id);
                gang->cutsceneState = (GangCutscene)(int)e.attackTimer;
                gang->stateTimer = e.walkTimer;
                temp = gang;
            } else if (e.type == (int)EnemyType::LUCA_BOSS) {
                auto luca = std::make_shared<LucaBoss>(e.pos, e.id);
                luca->position = e.pos;
                luca->portalTimer = e.walkTimer;
                luca->angle = e.angle;
                luca->isMoving = e.isMoving;
                luca->walkTimer = e.isMoving ? e.walkTimer : 0.0f;
                luca->attackTimer = e.attackTimer;
                temp = luca;
            } else {
                temp = std::make_shared<Enemy>(e.pos, (EnemyType)e.type,
                                               (WeaponType)e.weapon, e.id);
                temp->angle = e.angle;
                temp->isMoving = e.isMoving;
                temp->walkTimer = e.walkTimer;
                temp->attackTimer = e.attackTimer;
            }
            temp->hp = e.hp;
            temp->maxHp = e.maxHp;

            if (temp->type != EnemyType::BOSS &&
                temp->type != EnemyType::GIBON_BOSS &&
                temp->type != EnemyType::GANG_BOSS &&
                temp->type != EnemyType::ADAS_PRIME &&
                temp->type != EnemyType::LUCA_BOSS) {
                temp->DrawHUD(activeCam);
            }
        }
    }

    // ──────────────────────────────────────────────────────────
    // WORLD-SPACE: Structure health bars
    // ──────────────────────────────────────────────────────────
    for (auto &s : structures) {
        if (!s->active || s->hp <= 0)
            continue;
        float viewDist = Vector3Distance(s->position, activeCam.position);
        if (viewDist > 80.0f)
            continue;

        float barHeight = (s->type == StructureType::TURRET) ? 5.5f : 5.0f;
        Vector3 barPos = {s->position.x, s->position.y + barHeight, s->position.z};

        Vector3 toStruct = Vector3Subtract(barPos, activeCam.position);
        Vector3 camFwd = Vector3Subtract(activeCam.target, activeCam.position);
        if (Vector3DotProduct(toStruct, camFwd) < 0)
            continue;

        Vector2 screenPos = GetWorldToScreen(barPos, activeCam);
        float hpRatio = std::max(0.0f, std::min(1.0f, (float)s->hp / (float)s->maxHp));

        float distScale = std::max(0.5f, 1.0f - (viewDist - 20.0f) / 100.0f);
        float bW = 50.0f * distScale;
        float bH = 6.0f * distScale;

        DrawRectangle((int)(screenPos.x - bW / 2 - 1),
                      (int)(screenPos.y - bH - 1), (int)(bW + 2), (int)(bH + 2),
                      Fade(BLACK, 0.8f));

        Color barColor;
        if (s->type == StructureType::WALL) {
            barColor = hpRatio > 0.5f ? (Color){50, 180, 80, 255}
                                      : (hpRatio > 0.25f ? ORANGE : RED);
        } else {
            barColor = hpRatio > 0.5f
                           ? (Color){220, 150, 30, 255}
                           : (hpRatio > 0.25f ? (Color){220, 100, 30, 255} : RED);
        }
        DrawRectangle((int)(screenPos.x - bW / 2), (int)(screenPos.y - bH),
                      (int)(bW * hpRatio), (int)bH, barColor);

        const char *label = (s->type == StructureType::WALL) ? "WALL" : "TURRET";
        int fontSize = std::max(8, (int)(12 * distScale));
        int labelWidth = MeasureText(label, fontSize);
        DrawText(label, (int)(screenPos.x - labelWidth / 2),
                 (int)(screenPos.y - bH - fontSize - 2), fontSize,
                 s->type == StructureType::WALL ? (Color){180, 150, 100, 255}
                                                : (Color){220, 160, 50, 255});

        if (viewDist < 40.0f) {
            const char *hpText = TextFormat("%d/%d", s->hp, s->maxHp);
            int hpFontSize = std::max(7, (int)(9 * distScale));
            int hpWidth = MeasureText(hpText, hpFontSize);
            DrawText(hpText, (int)(screenPos.x - hpWidth / 2),
                     (int)(screenPos.y + 2), hpFontSize, RAYWHITE);
        }
    }

    // ══════════════════════════════════════════════════════════
    // SCREEN-SPACE HUD
    // ══════════════════════════════════════════════════════════
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // ──────────────────────────────────────────────────────────
    // TOP LEFT: FPS counter (optional)
    // ──────────────────────────────────────────────────────────
    if (showFPS) {
        int fps = GetFPS();
        Color fpsColor = fps >= 60 ? (Color){80, 220, 100, 220}
                       : fps >= 30 ? (Color){240, 180, 40, 220}
                                   : (Color){240, 60, 60, 220};
        const char *fpsText = TextFormat("FPS %d", fps);
        int fpsW = MeasureText(fpsText, 14);
        DrawRectangleRounded({10, 10, (float)(fpsW + 16), 24}, 0.4f, 6, Fade(BLACK, 0.55f));
        DrawText(fpsText, 18, 15, 14, fpsColor);
    }

    // ──────────────────────────────────────────────────────────
    // TOP RIGHT: Money + Wave info
    // ──────────────────────────────────────────────────────────
    {
        int panelW = 190;
        int panelH = 60;
        int px = sw - panelW - 12;
        int py = 12;

        DrawRectangleRounded({(float)px, (float)py, (float)panelW, (float)panelH},
                             0.15f, 8, Fade(BLACK, 0.60f));
        DrawRectangleRoundedLines({(float)px, (float)py, (float)panelW, (float)panelH},
                                  0.15f, 8, 1, Fade(WHITE, 0.10f));

        // Money
        const char *moneyStr = TextFormat("$%d", localPlayer.money);
        int moneyW = MeasureText(moneyStr, 22);
        DrawText(moneyStr, px + panelW - moneyW - 10, py + 8, 22, (Color){80, 220, 80, 255});

        // Wave
        const char *waveStr = TextFormat("WAVE %d", currentWave);
        DrawText(waveStr, px + 10, py + 8, 16, (Color){240, 160, 40, 255});

        // Enemies / countdown
        if (!waveActive) {
            const char *nextStr = TextFormat("%.0fs", waveWaitTimer);
            DrawText(nextStr, px + 10, py + 34, 14, (Color){100, 180, 240, 255});
        } else {
            const char *enemStr = TextFormat("%d enemies", (int)enemies.size());
            DrawText(enemStr, px + 10, py + 34, 14, (Color){200, 200, 200, 220});
        }

        // Structures count
        if (!structures.empty()) {
            const char *defStr = TextFormat("DEF %d", (int)structures.size());
            int defW = MeasureText(defStr, 13);
            DrawText(defStr, px + panelW - defW - 10, py + 36, 13, (Color){180, 150, 100, 200});
        }
    }

    // ──────────────────────────────────────────────────────────
    // TOP CENTER: Boss HP bar
    // ──────────────────────────────────────────────────────────
    {
        std::shared_ptr<Enemy> activeBossLocal = nullptr;
        if (net.mode == NetworkManager::Mode::SERVER) {
            for (auto &e : enemies) {
                if (e->type == EnemyType::BOSS || e->type == EnemyType::GIBON_BOSS ||
                    e->type == EnemyType::GANG_BOSS ||
                    e->type == EnemyType::ADAS_PRIME ||
                    e->type == EnemyType::LUCA_BOSS) {
                    activeBossLocal = e;
                    break;
                }
            }
        } else {
            for (auto const &[id, e] : net.syncedEnemies) {
                if (e.type == (int)EnemyType::BOSS ||
                    e.type == (int)EnemyType::GIBON_BOSS ||
                    e.type == (int)EnemyType::GANG_BOSS ||
                    e.type == (int)EnemyType::ADAS_PRIME ||
                    e.type == (int)EnemyType::LUCA_BOSS) {
                    activeBossLocal = std::make_shared<Enemy>(e.pos, (EnemyType)e.type,
                                                             WeaponType::KATANA, e.id);
                    activeBossLocal->hp = e.hp;
                    activeBossLocal->maxHp = e.maxHp;
                    break;
                }
            }
        }

        if (activeBossLocal && activeBossLocal->hp > 0) {
            const char *bossName = "BOSS";
            if (activeBossLocal->type == EnemyType::BOSS)
                bossName = "ADAS GOONER";
            else if (activeBossLocal->type == EnemyType::GIBON_BOSS)
                bossName = "GIBON RZYGACZ";
            else if (activeBossLocal->type == EnemyType::GANG_BOSS)
                bossName = "THE GANG";
            else if (activeBossLocal->type == EnemyType::ADAS_PRIME)
                bossName = "ADAS PRIME";
            else if (activeBossLocal->type == EnemyType::LUCA_BOSS)
                bossName = "GOON LORD-LUCA";

            float bossRatio = (float)activeBossLocal->hp / (float)activeBossLocal->maxHp;
            int bw = 440;
            int bh = 18;
            int bx = sw / 2 - bw / 2;
            int by = 16;

            // Name
            int nameW = MeasureText(bossName, 18);
            DrawText(bossName, sw / 2 - nameW / 2, by - 2, 18, (Color){255, 210, 50, 255});

            // Bar background
            int barY = by + 22;
            DrawRectangleRounded({(float)bx, (float)barY, (float)bw, (float)bh}, 0.5f, 8,
                                 Fade(BLACK, 0.7f));
            // Bar fill (color shifts red -> gold at low hp)
            Color bossFill = bossRatio > 0.5f
                ? (Color){220, 50, 50, 255}
                : (bossRatio > 0.2f ? (Color){220, 110, 30, 255} : (Color){255, 40, 40, 255});
            if (bossRatio > 0.0f) {
                DrawRectangleRounded({(float)bx, (float)barY, bw * bossRatio, (float)bh},
                                     0.5f, 8, bossFill);
            }
            DrawRectangleRoundedLines({(float)bx, (float)barY, (float)bw, (float)bh},
                                      0.5f, 8, 1, Fade(WHITE, 0.2f));

            // HP text
            const char *hpTxt = TextFormat("%d / %d", activeBossLocal->hp, activeBossLocal->maxHp);
            int htw = MeasureText(hpTxt, 12);
            DrawText(hpTxt, sw / 2 - htw / 2, barY + 3, 12, Fade(WHITE, 0.85f));
        }
    }

    // ──────────────────────────────────────────────────────────
    // TOP CENTER (below boss bar area): Base HP bar
    // ──────────────────────────────────────────────────────────
    {
        float baseRatio = std::max(0.0f, std::min(1.0f, baseHP / maxBaseHP));
        int bw = 260;
        int bh = 14;
        int bx = sw / 2 - bw / 2;
        int by = 60; // sits just below the boss bar area

        // Label
        const char *baseLabel = "NYC BASE";
        int lblW = MeasureText(baseLabel, 13);
        DrawText(baseLabel, sw / 2 - lblW / 2, by - 2, 13, (Color){255, 205, 50, 200});

        // Background
        DrawRectangleRounded({(float)bx, (float)(by + 16), (float)bw, (float)bh},
                             0.5f, 8, Fade(BLACK, 0.60f));
        // Fill
        Color baseColor = baseRatio > 0.5f
            ? (Color){220, 190, 30, 255}
            : (baseRatio > 0.25f ? (Color){230, 120, 20, 255} : (Color){220, 40, 40, 255});
        if (baseRatio > 0.0f) {
            DrawRectangleRounded({(float)bx, (float)(by + 16), bw * baseRatio, (float)bh},
                                 0.5f, 8, baseColor);
        }
        DrawRectangleRoundedLines({(float)bx, (float)(by + 16), (float)bw, (float)bh},
                                  0.5f, 8, 1, Fade(WHITE, 0.15f));

        // HP numbers
        const char *baseHpTxt = TextFormat("%d / %d", (int)std::max(0.0f, baseHP), (int)maxBaseHP);
        int hpW2 = MeasureText(baseHpTxt, 11);
        DrawText(baseHpTxt, sw / 2 - hpW2 / 2, by + 18, 11, Fade(WHITE, 0.75f));
    }

    // ──────────────────────────────────────────────────────────
    // BOTTOM LEFT: Player HP (+ Vehicle HP if in vehicle)
    // ──────────────────────────────────────────────────────────
    {
        int panelX = 14;
        int panelY = sh - 70;
        int barW = 220;
        int barH = 14;

        bool inVeh = localPlayer.inVehicle && localPlayer.vehicleIndex >= 0;
        int panelH = inVeh ? 120 : 72;
        panelY = sh - panelH - 14;

        // Panel background
        DrawRectangleRounded({(float)panelX, (float)panelY, (float)(barW + 20), (float)panelH},
                             0.12f, 8, Fade(BLACK, 0.58f));
        DrawRectangleRoundedLines({(float)panelX, (float)panelY, (float)(barW + 20), (float)panelH},
                                  0.12f, 8, 1, Fade(WHITE, 0.08f));

        int curY = panelY + 10;

        // Player HP
        {
            float ratio = std::max(0.0f, std::min(1.0f, localPlayer.hp / (float)localPlayer.maxHp));
            Color hpColor = ratio > 0.5f ? (Color){50, 210, 80, 255}
                          : ratio > 0.25f ? (Color){240, 170, 30, 255}
                                          : (Color){230, 50, 50, 255};
            DrawText("HP", panelX + 10, curY, 13, Fade(WHITE, 0.70f));
            const char *hpNum = TextFormat("%d/%d", std::max(0, localPlayer.hp), localPlayer.maxHp);
            int hpNumW = MeasureText(hpNum, 13);
            DrawText(hpNum, panelX + barW + 10 - hpNumW, curY, 13, hpColor);
            curY += 18;
            DrawStatBar(panelX + 10, curY, barW, barH, ratio, hpColor, Fade(BLACK, 0.5f));
            curY += barH + 10;
        }

        // Vehicle HP
        if (inVeh) {
            auto v = vehicles[localPlayer.vehicleIndex];
            float vRatio = std::max(0.0f, std::min(1.0f, v->health / (float)v->maxHealth));
            Color vColor = vRatio > 0.5f ? (Color){60, 160, 240, 255}
                         : vRatio > 0.25f ? ORANGE : RED;
            DrawText("VEHICLE", panelX + 10, curY, 13, Fade(SKYBLUE, 0.80f));
            const char *vNum = TextFormat("%d/%d", std::max(0, v->health), v->maxHealth);
            int vNumW = MeasureText(vNum, 13);
            DrawText(vNum, panelX + barW + 10 - vNumW, curY, 13, vColor);
            curY += 18;
            DrawStatBar(panelX + 10, curY, barW, barH, vRatio, vColor, Fade(BLACK, 0.5f));
        }
    }

    // ──────────────────────────────────────────────────────────
    // BOTTOM RIGHT: Ammo & Weapon list
    // ──────────────────────────────────────────────────────────

    // Ammo display
    if (localPlayer.currentWeapon && localPlayer.currentWeapon->magSize > 0) {
        auto *w = localPlayer.currentWeapon;

        int ammoX = sw - 180;
        int ammoY = sh - 80;

        DrawRectangleRounded({(float)(ammoX - 10), (float)(ammoY - 8), 175, 68},
                             0.15f, 8, Fade(BLACK, 0.58f));

        Color ammoColor = (w->currentAmmo > 0) ? WHITE : (Color){230, 60, 60, 255};
        DrawText(TextFormat("%d", w->currentAmmo), ammoX, ammoY, 36, ammoColor);

        const char *slash = TextFormat("/ %d", w->magSize);
        DrawText(slash, ammoX + MeasureText(TextFormat("%d", w->currentAmmo), 36) + 4,
                 ammoY + 14, 18, Fade(LIGHTGRAY, 0.75f));

        const char *resText = TextFormat("RES %d", w->reserveAmmo);
        DrawText(resText, ammoX, ammoY + 42, 14, Fade(LIGHTGRAY, 0.60f));

        // Reload bar
        if (w->isReloading) {
            float prog = 1.0f - (w->reloadTimer / w->reloadTime);
            int rBarX = sw - 180 - 10;
            int rBarY = sh - 90;
            DrawRectangleRounded({(float)rBarX, (float)rBarY, 175, 8}, 0.5f, 6,
                                 Fade(BLACK, 0.5f));
            DrawRectangleRounded({(float)rBarX, (float)rBarY, 175 * prog, 8}, 0.5f, 6,
                                 (Color){240, 200, 50, 255});
            // Center screen reload text
            const char *relTxt = "RELOADING";
            int rtw = MeasureText(relTxt, 18);
            DrawText(relTxt, sw / 2 - rtw / 2, sh / 2 + 60, 18, (Color){240, 200, 50, 200});
        }
    } else if (localPlayer.currentWeapon &&
               (strncmp(localPlayer.currentWeapon->name, "TURRET", 6) == 0 ||
                strncmp(localPlayer.currentWeapon->name, "WALL", 4) == 0)) {
        auto *w = localPlayer.currentWeapon;
        bool isTurret = (strncmp(w->name, "TURRET", 6) == 0);
        int total = w->currentAmmo + w->reserveAmmo;
        int bx2 = sw - 180;
        int by2 = sh - 80;
        DrawRectangleRounded({(float)(bx2 - 10), (float)(by2 - 8), 175, 68},
                             0.15f, 8, Fade(BLACK, 0.58f));
        Color placeColor = (total > 0)
            ? (isTurret ? (Color){255, 150, 50, 255} : (Color){200, 170, 120, 255})
            : RED;
        DrawText(TextFormat("%d", total), bx2, by2, 36, placeColor);
        DrawText(isTurret ? "TURRET" : "WALL", bx2, by2 + 40, 14, Fade(LIGHTGRAY, 0.65f));
        DrawText("LMB PLACE", bx2, by2 + 54, 12, Fade(LIME, 0.75f));
    }

    // Weapon slot list (CS-style right side)
    if (!localPlayer.showInventory) {
        int eqWidth = 148;
        int eqHeight = 32;
        int spacing = 4;

        std::vector<Weapon *> categories[5];
        for (auto *w : localPlayer.inventory) {
            if (w->magSize == 30 || w->magSize == 200)
                categories[1].push_back(w);
            else if (w->magSize == 17 || w->magSize == 6)
                categories[2].push_back(w);
            else if (w->magSize == 0)
                categories[3].push_back(w);
            else if (w->magSize == 8 || w->magSize == 5 || w->magSize == 1)
                categories[4].push_back(w);
        }

        int numActive = 0;
        for (int c = 1; c <= 4; c++)
            if (!categories[c].empty()) numActive++;

        int startY = sh / 2 - (numActive * (eqHeight + spacing)) / 2;
        int curY = startY;

        for (int c = 1; c <= 4; c++) {
            if (categories[c].empty()) continue;
            for (size_t i = 0; i < categories[c].size(); i++) {
                auto *w = categories[c][i];
                bool equipped = (w == localPlayer.currentWeapon);

                Rectangle slot = {
                    (float)(sw - eqWidth - 16 - (int)i * (eqWidth + spacing)),
                    (float)curY, (float)eqWidth, (float)eqHeight};

                Color bg = equipped ? Fade((Color){50, 50, 50, 255}, 0.92f)
                                    : Fade(BLACK, 0.42f);
                DrawRectangleRounded(slot, 0.25f, 8, bg);
                if (equipped)
                    DrawRectangleRoundedLines(slot, 0.25f, 8, 2, (Color){80, 220, 100, 200});

                const char *wName = "WEAPON";
                if (strncmp(w->name, "TURRET", 6) == 0 || strncmp(w->name, "WALL", 4) == 0)
                    wName = w->name;
                else if (w->magSize == 17)   wName = "GLOCK";
                else if (w->magSize == 30)   wName = "AK47";
                else if (w->magSize == 6)    wName = "REVOLVER";
                else if (w->magSize == 8)    wName = "SHOTGUN";
                else if (w->magSize == 5)    wName = "AWP";
                else if (w->magSize == 200)  wName = "MINIGUN";
                else if (w->magSize == 1)    wName = "RPG";
                else if (w->magSize == 0)    wName = "KNIFE";

                Color txtColor = equipped ? WHITE : Fade(LIGHTGRAY, 0.70f);
                DrawText(TextFormat("[%d] %s", c, wName), (int)slot.x + 8, (int)slot.y + 9,
                         15, txtColor);
            }
            curY += eqHeight + spacing;
        }
    }

    // ──────────────────────────────────────────────────────────
    // INTERACTION PROMPTS
    // ──────────────────────────────────────────────────────────
    if (shopNearby) {
        const char *buyTxt = "[E] BUY";
        int btw = MeasureText(buyTxt, 20);
        DrawText(buyTxt, sw / 2 - btw / 2, sh / 2 + 40, 20, (Color){240, 195, 40, 220});
    }

    // Kebab stand interaction
    {
        Vector3 bp = {0, 0, 150};
        Vector3 standPos = {bp.x + 15, 0, bp.z + 10};

        struct KebabOffer { Vector3 pos; int cost; int hp; const char *label; };
        KebabOffer offers[] = {
            {{standPos.x - 3, 1, standPos.z - 4}, 50,  10, "[E] SMALL KEBAB  $50  +10 HP"},
            {{standPos.x,     1, standPos.z - 4}, 100, 25, "[E] MEDIUM KEBAB $100 +25 HP"},
            {{standPos.x + 3, 1, standPos.z - 4}, 150, 50, "[E] LARGE KEBAB  $150 +50 HP"},
        };
        for (auto &o : offers) {
            if (Vector3Distance(localPlayer.position, o.pos) < 4.0f) {
                int tw2 = MeasureText(o.label, 18);
                DrawText(o.label, sw / 2 - tw2 / 2, sh / 2 + 70, 18, (Color){240, 195, 40, 220});
                if (IsKeyPressed(KEY_E) && localPlayer.money >= o.cost &&
                    localPlayer.hp < localPlayer.maxHp) {
                    localPlayer.money -= o.cost;
                    localPlayer.hp = std::min(localPlayer.maxHp, localPlayer.hp + o.hp);
                }
            }
        }
    }

    // ──────────────────────────────────────────────────────────
    // BASE LOST
    // ──────────────────────────────────────────────────────────
    if (baseHP <= 0) {
        const char *lostTxt = "NYC HAS FALLEN";
        int ltw = MeasureText(lostTxt, 40);
        DrawText(lostTxt, sw / 2 - ltw / 2, sh / 2, 40, (Color){220, 40, 40, 240});
    }

    // ──────────────────────────────────────────────────────────
    // DEATH SCREEN
    // ──────────────────────────────────────────────────────────
    if (localPlayer.isDead) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.50f));
        const char *deathMsg = "YOU DIED";
        int dw = MeasureText(deathMsg, 60);
        DrawText(deathMsg, sw / 2 - dw / 2, sh / 2 - 60, 60, (Color){220, 40, 40, 255});

        const char *respMsg = TextFormat("RESPAWNING IN %.1fs", localPlayer.respawnTimer);
        int rw = MeasureText(respMsg, 26);
        DrawText(respMsg, sw / 2 - rw / 2, sh / 2 + 20, 26, RAYWHITE);
    }

    // ──────────────────────────────────────────────────────────
    // INVENTORY UI
    // ──────────────────────────────────────────────────────────
    if (state == GameState::GAME && localPlayer.showInventory) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.80f));
        DrawText("INVENTORY", sw / 2 - MeasureText("INVENTORY", 40) / 2, 50, 40, GOLD);

        int startX = sw / 2 - 400;
        int startY = 150;
        for (size_t i = 0; i < localPlayer.inventory.size(); i++) {
            int row = i / 4;
            int col = i % 4;
            Rectangle slot = {(float)(startX + col * 200),
                              (float)(startY + row * 150), 180, 120};
            bool isHovered = CheckCollisionPointRec(GetMousePosition(), slot);
            bool isEquipped = (localPlayer.currentWeapon == localPlayer.inventory[i]);

            DrawRectangleRec(slot, isEquipped
                                     ? Fade(DARKGREEN, 0.6f)
                                     : (isHovered ? Fade(GRAY, 0.6f)
                                                  : Fade(DARKGRAY, 0.6f)));
            DrawRectangleLinesEx(slot, 2,
                                 isEquipped ? LIME : (isHovered ? WHITE : BLACK));

            const char *wName = "WEAPON";
            if (strncmp(localPlayer.inventory[i]->name, "TURRET", 6) == 0 ||
                strncmp(localPlayer.inventory[i]->name, "WALL", 4) == 0)
                wName = localPlayer.inventory[i]->name;
            else if (localPlayer.inventory[i]->magSize == 17) wName = "GLOCK";
            else if (localPlayer.inventory[i]->magSize == 30) wName = "AK47";
            else if (localPlayer.inventory[i]->magSize == 6)  wName = "REVOLVER";
            else if (localPlayer.inventory[i]->magSize == 8)  wName = "SHOTGUN";
            else if (localPlayer.inventory[i]->magSize == 10) wName = "AWP";
            else if (localPlayer.inventory[i]->magSize == 100) wName = "MINIGUN";
            else if (localPlayer.inventory[i]->magSize == 1)  wName = "RPG";
            else if (localPlayer.inventory[i]->magSize == 0)  wName = "KNIFE";

            DrawText(wName, slot.x + 10, slot.y + 10, 20, WHITE);
            if (localPlayer.inventory[i]->magSize > 0) {
                DrawText(TextFormat("AMMO: %d", localPlayer.inventory[i]->currentAmmo),
                         slot.x + 10, slot.y + 40, 15, RAYWHITE);
                DrawText(TextFormat("RES: %d", localPlayer.inventory[i]->reserveAmmo),
                         slot.x + 10, slot.y + 60, 15, LIGHTGRAY);
            } else {
                DrawText("MELEE", slot.x + 10, slot.y + 40, 15, GRAY);
            }

            if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (localPlayer.currentWeapon != localPlayer.inventory[i])
                    localPlayer.previousWeapon = localPlayer.currentWeapon;
                localPlayer.currentWeapon = localPlayer.inventory[i];
            }
        }
        DrawText("PRESS ~ TO CLOSE",
                 sw / 2 - MeasureText("PRESS ~ TO CLOSE", 20) / 2, sh - 50, 20, LIGHTGRAY);
    }

    // ──────────────────────────────────────────────────────────
    // PAUSE MENU
    // ──────────────────────────────────────────────────────────
    if (state == GameState::PAUSED) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.60f));

        int boxH = localPlayer.showSettings ? 580 : (localPlayer.showAdminSettings ? 700 : 460);
        Rectangle menuBox = {(float)sw / 2 - 200, (float)sh / 2 - (float)boxH / 2, 400, (float)boxH};
        DrawRectangleRounded(menuBox, 0.10f, 8, Fade((Color){18, 18, 22, 255}, 0.96f));
        DrawRectangleLinesEx(menuBox, 2,
                             localPlayer.showAdminSettings ? Fade(ORANGE, 0.9f) : Fade(GOLD, 0.7f));

        const char *menuTitle = localPlayer.showAdminSettings ? "ADMIN TERMINAL" : "CONTROL TERMINAL";
        DrawText(menuTitle,
                 (int)(menuBox.x + 200 - MeasureText(menuTitle, 22) / 2),
                 (int)(menuBox.y + 28), 22,
                 localPlayer.showAdminSettings ? ORANGE : GOLD);

        // thin separator
        DrawLine((int)menuBox.x + 30, (int)menuBox.y + 62,
                 (int)menuBox.x + 370, (int)menuBox.y + 62,
                 Fade(WHITE, 0.12f));

        float menuY = menuBox.y + 80;
        auto menuBtn = [&](const char *text, Color col) -> bool {
            Rectangle r = {menuBox.x + 40, menuY, 320, 46};
            bool hov = CheckCollisionPointRec(GetMousePosition(), r);
            Color bg = hov ? Fade(col, 0.55f) : Fade(col, 0.18f);
            DrawRectangleRounded(r, 0.22f, 8, bg);
            if (hov) DrawRectangleRoundedLines(r, 0.22f, 8, 1, Fade(col, 0.7f));
            DrawText(text, (int)(r.x + 160 - MeasureText(text, 18) / 2), (int)(r.y + 14), 18, WHITE);
            menuY += 56;
            return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        };

        if (menuBtn("RESUME MISSION", (Color){30, 160, 60, 255}))
            state = GameState::GAME;

        if (localPlayer.showSettings) {
            // ── OPTIONS SUB-PANEL ──
            DrawText("OPTIONS", (int)(menuBox.x + 40), (int)(menuY - 30), 16,
                     Fade(SKYBLUE, 0.80f));

            // FOV slider
            DrawText(TextFormat("FOV: %d", (int)preferredFOV),
                     (int)(menuBox.x + 40), (int)menuY, 16, RAYWHITE);
            menuY += 22;
            Rectangle fovBar = {menuBox.x + 40, menuY, 320, 12};
            DrawRectangleRounded(fovBar, 0.5f, 6, Fade(BLACK, 0.5f));
            float fovT = (preferredFOV - 40.0f) / 80.0f;
            DrawRectangleRounded({fovBar.x, fovBar.y, fovBar.width * fovT, fovBar.height},
                                 0.5f, 6, SKYBLUE);
            DrawRectangleLinesEx(fovBar, 1, Fade(SKYBLUE, 0.3f));
            if (CheckCollisionPointRec(GetMousePosition(), fovBar) &&
                IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                preferredFOV = 40.0f + ((GetMouseX() - fovBar.x) / 320.0f) * 80.0f;
                preferredFOV = std::max(40.0f, std::min(120.0f, preferredFOV));
            }
            menuY += 28;

            // FPS toggle
            {
                Rectangle fpsTogRect = {menuBox.x + 40, menuY, 320, 40};
                bool hov2 = CheckCollisionPointRec(GetMousePosition(), fpsTogRect);
                DrawRectangleRounded(fpsTogRect, 0.22f, 8,
                                     hov2 ? Fade(WHITE, 0.10f) : Fade(BLACK, 0.25f));
                DrawText("SHOW FPS", (int)(fpsTogRect.x + 10), (int)(fpsTogRect.y + 11), 16, WHITE);

                // Toggle pill
                float pillX = fpsTogRect.x + 220;
                float pillY = fpsTogRect.y + 8;
                Color pillBg = showFPS ? (Color){50, 200, 90, 255} : Fade(DARKGRAY, 0.8f);
                DrawRectangleRounded({pillX, pillY, 60, 24}, 0.5f, 8, pillBg);
                float knobX = showFPS ? pillX + 36 : pillX + 4;
                DrawCircle((int)(knobX + 10), (int)(pillY + 12), 9, WHITE);
                DrawText(showFPS ? "ON" : "OFF", (int)(pillX + (showFPS ? 6 : 20)),
                         (int)(pillY + 6), 12, WHITE);

                if (hov2 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    showFPS = !showFPS;
                menuY += 52;
            }

            if (menuBtn("BACK", Fade(DARKGRAY, 0.8f)))
                localPlayer.showSettings = false;

        } else if (localPlayer.showAdminSettings) {
            // ── ADMIN SUB-PANEL ──
            Color flyCol = localPlayer.isFlying ? (Color){0, 200, 80, 255} : MAROON;
            if (menuBtn(localPlayer.isFlying ? "FLY MODE: ON" : "FLY MODE: OFF", flyCol)) {
                localPlayer.isAdmin = true;
                localPlayer.isFlying = !localPlayer.isFlying;
            }

            DrawText("GLOCK DAMAGE:", (int)(menuBox.x + 40), (int)(menuY - 5), 16, RAYWHITE);
            DrawText(TextFormat("%d", Glock::globalGlockDamage),
                     (int)(menuBox.x + 270), (int)(menuY - 5), 20, ORANGE);
            menuY += 26;

            Rectangle dmgBar = {menuBox.x + 40, menuY, 320, 18};
            DrawRectangleRounded(dmgBar, 0.5f, 8, Fade(BLACK, 0.7f));
            float dmgT = (float)(Glock::globalGlockDamage - 1) / (9999.0f - 1.0f);
            DrawRectangleRounded({dmgBar.x, dmgBar.y, dmgBar.width * dmgT, dmgBar.height},
                                 0.5f, 8, ORANGE);
            DrawRectangleLinesEx(dmgBar, 1, Fade(ORANGE, 0.4f));
            if (CheckCollisionPointRec(GetMousePosition(), dmgBar) &&
                IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                float t = (GetMouseX() - dmgBar.x) / dmgBar.width;
                t = std::max(0.0f, std::min(1.0f, t));
                Glock::globalGlockDamage = 1 + (int)(t * 9998.0f);
            }
            menuY += 36;

            const int presets[] = {35, 100, 500, 9999};
            const char *presetLabels[] = {"35 (DEF)", "100", "500", "9999"};
            float btnW = 68.0f;
            for (int p = 0; p < 4; p++) {
                Rectangle pr = {menuBox.x + 40 + p * (btnW + 8.0f), menuY, btnW, 34};
                bool hov3 = CheckCollisionPointRec(GetMousePosition(), pr);
                bool act = (Glock::globalGlockDamage == presets[p]);
                DrawRectangleRounded(pr, 0.3f, 8,
                                     act ? Fade(ORANGE, 0.9f) : (hov3 ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f)));
                DrawText(presetLabels[p],
                         (int)(pr.x + pr.width / 2 - MeasureText(presetLabels[p], 12) / 2),
                         (int)(pr.y + 11), 12, WHITE);
                if (hov3 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    Glock::globalGlockDamage = presets[p];
            }
            menuY += 50;

            DrawText("MAX HP:", (int)(menuBox.x + 40), (int)(menuY - 5), 16, RAYWHITE);
            DrawText(TextFormat("%d", localPlayer.maxHp),
                     (int)(menuBox.x + 270), (int)(menuY - 5), 20, GREEN);
            menuY += 26;

            Rectangle hpBar = {menuBox.x + 40, menuY, 320, 18};
            DrawRectangleRounded(hpBar, 0.5f, 8, Fade(BLACK, 0.7f));
            float hpT = (float)(localPlayer.maxHp - 100) / (9999.0f - 100.0f);
            hpT = std::max(0.0f, std::min(1.0f, hpT));
            DrawRectangleRounded({hpBar.x, hpBar.y, hpBar.width * hpT, hpBar.height},
                                 0.5f, 8, GREEN);
            DrawRectangleLinesEx(hpBar, 1, Fade(GREEN, 0.4f));
            if (CheckCollisionPointRec(GetMousePosition(), hpBar) &&
                IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                float t = (GetMouseX() - hpBar.x) / hpBar.width;
                t = std::max(0.0f, std::min(1.0f, t));
                localPlayer.maxHp = 100 + (int)(t * 9899.0f);
                localPlayer.hp = localPlayer.maxHp;
            }
            menuY += 36;

            const int hpPresets[] = {100, 1000, 5000, 9999};
            const char *hpPresetLabels[] = {"100 (DEF)", "1000", "5000", "9999"};
            float hpBtnW = 68.0f;
            for (int p = 0; p < 4; p++) {
                Rectangle pr = {menuBox.x + 40 + p * (hpBtnW + 8.0f), menuY, hpBtnW, 34};
                bool hov3 = CheckCollisionPointRec(GetMousePosition(), pr);
                bool act = (localPlayer.maxHp == hpPresets[p]);
                DrawRectangleRounded(pr, 0.3f, 8,
                                     act ? Fade(GREEN, 0.9f) : (hov3 ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f)));
                DrawText(hpPresetLabels[p],
                         (int)(pr.x + pr.width / 2 - MeasureText(hpPresetLabels[p], 12) / 2),
                         (int)(pr.y + 11), 12, WHITE);
                if (hov3 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    localPlayer.maxHp = hpPresets[p];
                    localPlayer.hp = localPlayer.maxHp;
                }
            }
            menuY += 50;

            if (menuBtn("BACK", Fade(DARKGRAY, 0.8f)))
                localPlayer.showAdminSettings = false;

        } else {
            // ── MAIN PAUSE BUTTONS ──
            if (menuBtn("OPTIONS", (Color){30, 80, 200, 255}))
                localPlayer.showSettings = true;

            if (net.mode == NetworkManager::Mode::SERVER)
                if (menuBtn("ADMIN SETTINGS", MAROON))
                    localPlayer.showAdminSettings = true;

            if (menuBtn("DISCONNECT", (Color){180, 30, 30, 255})) {
                net.SendDisconnect(localPlayer.playerId);
                net.shouldQuit = true;
                menu.currentState = MenuState::MAIN;
            }
        }
    }

    EndTextureMode();
}

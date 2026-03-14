#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "../input/Input.h"

// ═══════════════════════════════════════════════════════════════════════════════
// RunnerGame – Endless side-scroller
// PLAY=Jump, FEED=Duck, SLEEP=Quit
// ═══════════════════════════════════════════════════════════════════════════════

#define RUNNER_FLOOR_Y      48
#define RUNNER_PLAYER_X     20
#define RUNNER_PLAYER_W      8
#define RUNNER_PLAYER_H      8
#define RUNNER_MAX_OBSTACLES 4
#define RUNNER_GRAVITY       0.5f
#define RUNNER_JUMP_VEL     -5.0f

struct Obstacle {
    float    x;
    uint8_t  w, h;
    bool     active;
    bool     isTall;  // tall = must jump, short = can duck
};

struct RunnerState {
    float     playerY;
    float     playerVY;
    bool      onGround;
    bool      ducking;
    uint32_t  distance;    // pixels scrolled
    float     speed;       // pixels per frame
    Obstacle  obstacles[RUNNER_MAX_OBSTACLES];
    uint32_t  spawnDist;
    uint32_t  nextSpawn;
    bool      running;
    bool      dead;
};

class RunnerGame {
public:
    RunnerState gs;

    void init();
    // Returns distance; -1 if quit
    int16_t run(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);

private:
    void spawnObstacle();
    void update();
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    bool checkCollision();
    void drawPlayer(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawObstacles(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawBackground(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawHUD(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
};

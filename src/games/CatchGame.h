#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "../input/Input.h"
#include "../display/Sprites.h"

// ═══════════════════════════════════════════════════════════════════════════════
// CatchGame – Falling items to catch
// FEED=move left, PLAY=move right, SLEEP=pause/quit
// ═══════════════════════════════════════════════════════════════════════════════

#define CATCH_MAX_ITEMS    6
#define CATCH_PLAYER_W    12
#define CATCH_PLAYER_H     6
#define CATCH_ITEM_SIZE    6
#define CATCH_PLAYER_Y    (64 - 14 - CATCH_PLAYER_H - 2)  // above status bar
#define CATCH_GROUND_Y    (64 - 14)

enum class ItemType : uint8_t { FOOD = 0, POISON = 1, STAR = 2 };

struct FallingItem {
    float    x, y;
    float    speed;
    ItemType type;
    bool     active;
};

struct CatchGameState {
    float       playerX;
    FallingItem items[CATCH_MAX_ITEMS];
    uint16_t    score;
    uint8_t     lives;
    uint32_t    spawnTimer;
    uint32_t    spawnInterval;  // ms between spawns (decreases over time)
    bool        running;
    bool        paused;
};

class CatchGame {
public:
    CatchGameState gs;

    void init();
    // Returns score; -1 if quit
    int16_t run(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);

private:
    void spawnItem();
    void update(uint32_t nowMs);
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawPlayer(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawItems(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    void drawHUD(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2);
    bool checkCollision(const FallingItem& item);
};

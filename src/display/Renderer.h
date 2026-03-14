#pragma once
#include <U8g2lib.h>
#include <Wire.h>
#include "../pet/Pet.h"
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Renderer – U8g2 SH1106 128×64, full-buffer mode
// Runs on Core 1 at 30fps
// ═══════════════════════════════════════════════════════════════════════════════

enum class Screen : uint8_t {
    PET       = 0,  // normal pet view
    EVOLUTION = 1,  // evolution animation
    DEAD      = 2,  // game-over screen
    GAME      = 3,  // mini-game (handled by GameManager)
    STATS     = 4,  // detailed stats screen
};

class Renderer {
public:
    // U8g2: AZ-Delivery 1.3" = SH1106 128x64, HW I2C
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

    Renderer() : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE,
                       /* clock=*/ PIN_SCL, /* data=*/ PIN_SDA) {}

    void init();

    // Main render call – call every frame
    void renderFrame(Pet& pet, Screen screen = Screen::PET);

    // FPS counter (updated internally)
    float fps;

private:
    uint32_t _frameCount;
    uint32_t _fpsTimer;
    uint32_t _frameTimer;
    uint8_t  _animFrame;    // current sprite animation frame index
    uint32_t _animTimer;    // ms since last frame advance

    // ── Screen renderers ──────────────────────────────────────────────────
    void renderPetScreen(Pet& pet);
    void renderEvolutionScreen(Pet& pet);
    void renderDeadScreen(Pet& pet);
    void renderStatsScreen(Pet& pet);

    // ── Sub-renderers ──────────────────────────────────────────────────────
    void drawPetSprite(Pet& pet, int16_t x, int16_t y);
    void drawParticles(ParticleSystem& ps);
    void drawStatusBar(Pet& pet);
    void drawStatBar(int16_t x, int16_t y, uint8_t value, uint8_t w, uint8_t h,
                     const uint8_t* icon);
    void drawBackground(Pet& pet);
    void drawMoodText(Pet& pet);
    void drawWeatherOverlay(Pet& pet);

    // ── Animation frame management ────────────────────────────────────────
    void advanceAnimFrame(Pet& pet, uint32_t nowMs);
    uint8_t getFrameCount(Pet& pet);

    // ── Helpers ───────────────────────────────────────────────────────────
    static const char* moodName(PetMood mood);
};

extern Renderer Display;

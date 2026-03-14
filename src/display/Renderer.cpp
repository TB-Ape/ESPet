#include "Renderer.h"
#include "Sprites.h"
#include "../pet/Physics.h"
#include <Arduino.h>

Renderer Display;

static const char* MOOD_NAMES[] = {
    "Idle", "Happy", "Sad", "Sleepy", "Eating", "Playing", "Sick", "Dead"
};

void Renderer::init() {
    Wire.begin(PIN_SDA, PIN_SCL);
    u8g2.begin();
    u8g2.setContrast(200);
    fps         = 0.0f;
    _frameCount = 0;
    _fpsTimer   = millis();
    _frameTimer = millis();
    _animFrame  = 0;
    _animTimer  = millis();
}

void Renderer::renderFrame(Pet& pet, Screen screen) {
    uint32_t nowMs = millis();

    // FPS calculation
    _frameCount++;
    if (nowMs - _fpsTimer >= 1000) {
        fps       = _frameCount * 1000.0f / (nowMs - _fpsTimer);
        _frameCount = 0;
        _fpsTimer   = nowMs;
    }

    // Advance animation frame
    advanceAnimFrame(pet, nowMs);
    pet.frameCount++;  // used by Physics for bobbing

    // Render selected screen
    u8g2.clearBuffer();

    switch (screen) {
    case Screen::EVOLUTION:
        renderEvolutionScreen(pet);
        break;
    case Screen::DEAD:
        renderDeadScreen(pet);
        break;
    case Screen::STATS:
        renderStatsScreen(pet);
        break;
    case Screen::PET:
    default:
        renderPetScreen(pet);
        break;
    }

    u8g2.sendBuffer();
}

// ─── Pet Screen ──────────────────────────────────────────────────────────────

void Renderer::renderPetScreen(Pet& pet) {
    // Background (floor line + decorations)
    drawBackground(pet);

    // Weather overlay (snowflakes, etc.)
    drawWeatherOverlay(pet);

    // Pet sprite with bobbing
    float bobY = Physics::bobbingOffset(pet.frameCount);
    float shakeX = 0.0f;
    if (pet.state.mood == PetMood::SICK) {
        shakeX = Physics::shakeOffset(pet.frameCount);
    }

    int16_t px = (int16_t)(pet.state.posX + shakeX);
    int16_t py = (int16_t)(pet.state.posY + bobY);

    // Clamp to screen
    const EvoStageInfo& si = Evolution::stageInfo(pet.state.stage);
    px = max((int16_t)0, min(px, (int16_t)(DISPLAY_WIDTH  - si.spriteW)));
    py = max((int16_t)0, min(py, (int16_t)(DISPLAY_HEIGHT - si.spriteH - 14)));

    drawPetSprite(pet, px, py);

    // Particles
    drawParticles(pet.particles);

    // Status bar (bottom 14 pixels)
    drawStatusBar(pet);

    // Mood text
    drawMoodText(pet);

    // Exploration indicator
    if (pet.state.exploreTimer > 0) {
        pet.state.exploreTimer--;
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(px + si.spriteW + 1, py + 4, "?");
    }

    // Night indicator (moon icon top-right)
    if (pet.state.isNight) {
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(120, 8, "~");
    }
}

// ─── Evolution Screen ─────────────────────────────────────────────────────────

void Renderer::renderEvolutionScreen(Pet& pet) {
    const EvoStageInfo& si = Evolution::stageInfo(pet.state.stage);
    uint8_t timer = pet.state.evolutionAnimTimer;

    // Flash effect: alternate full white/black
    if (timer > 80 && (timer % 4 < 2)) {
        u8g2.drawBox(0, 0, 128, 64);
        return;
    }

    // Draw large starburst
    int cx = 64, cy = 28;
    if (timer > 40) {
        // expanding circles
        uint8_t r = (120 - timer) / 2;
        u8g2.drawCircle(cx, cy, r);
        u8g2.drawCircle(cx, cy, r > 4 ? r - 4 : 0);
    }

    // Draw pet sprite centered
    int16_t px = cx - si.spriteW / 2;
    int16_t py = cy - si.spriteH / 2;
    drawPetSprite(pet, px, py);

    // Draw particles
    drawParticles(pet.particles);

    // Stage name
    u8g2.setFont(u8g2_font_6x10_tr);
    const char* stageName = si.name;
    int sw = u8g2.getStrWidth(stageName);
    u8g2.drawStr(64 - sw / 2, 58, stageName);

    // "Level Up!" text
    if (timer < 60) {
        u8g2.setFont(u8g2_font_7x13B_tr);
        u8g2.drawStr(28, 12, "EVOLVED!");
    }
}

// ─── Dead Screen ──────────────────────────────────────────────────────────────

void Renderer::renderDeadScreen(Pet& pet) {
    // Somber dead screen
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(30, 12, "R.I.P.");

    // Pet name
    int sw = u8g2.getStrWidth(pet.state.name);
    u8g2.drawStr(64 - sw / 2, 24, pet.state.name);

    // Draw dead sprite centered
    const EvoStageInfo& si = Evolution::stageInfo(pet.state.stage);
    int16_t px = 64 - si.spriteW / 2;
    int16_t py = 30;
    drawPetSprite(pet, px, py);

    // Stats summary
    u8g2.setFont(u8g2_font_4x6_tr);
    char buf[24];
    snprintf(buf, sizeof(buf), "Age: %lud", pet.state.ageSeconds / 86400);
    u8g2.drawStr(4, 58, buf);
    snprintf(buf, sizeof(buf), "Fed: %d", pet.state.feedCount);
    u8g2.drawStr(60, 58, buf);
}

// ─── Stats Screen ─────────────────────────────────────────────────────────────

void Renderer::renderStatsScreen(Pet& pet) {
    u8g2.setFont(u8g2_font_5x7_tr);

    // Title: pet name + personality
    char title[20];
    snprintf(title, sizeof(title), "%s [%s]",
             pet.state.name,
             getTraits(pet.state.personality).symbol);
    u8g2.drawStr(2, 8, title);

    u8g2.drawHLine(0, 10, 128);

    // Stats grid
    u8g2.setFont(u8g2_font_4x6_tr);
    char buf[20];
    snprintf(buf, sizeof(buf), "Hunger: %3d%%", pet.state.hunger);
    u8g2.drawStr(2, 19, buf);
    snprintf(buf, sizeof(buf), "Happy:  %3d%%", pet.state.happiness);
    u8g2.drawStr(2, 27, buf);
    snprintf(buf, sizeof(buf), "Energy: %3d%%", pet.state.energy);
    u8g2.drawStr(2, 35, buf);
    snprintf(buf, sizeof(buf), "Health: %3d%%", pet.state.health);
    u8g2.drawStr(2, 43, buf);

    snprintf(buf, sizeof(buf), "Stage: %s", Evolution::stageInfo(pet.state.stage).name);
    u8g2.drawStr(2, 53, buf);
    snprintf(buf, sizeof(buf), "Age: %lud %luh",
             pet.state.ageSeconds / 86400,
             (pet.state.ageSeconds % 86400) / 3600);
    u8g2.drawStr(64, 53, buf);
}

// ─── Sub-renderers ────────────────────────────────────────────────────────────

void Renderer::drawPetSprite(Pet& pet, int16_t x, int16_t y) {
    uint8_t moodIdx = pet.moodIndex();
    bool adult = pet.isAdult();

    const SpriteFrame* frames;
    if (adult) {
        frames = ADULT_SPRITES[moodIdx];
    } else {
        frames = BABY_SPRITES[moodIdx];
    }

    // Get current frame (clamp to available)
    uint8_t fc = getSpriteFrameCount(moodIdx, adult);
    uint8_t fi = _animFrame % fc;

    const SpriteFrame& frame = frames[fi];
    if (frame.data == nullptr) return;

    u8g2.drawXBMP(x, y, frame.w, frame.h, frame.data);
}

void Renderer::drawParticles(ParticleSystem& ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = ps.pool[i];
        if (p.life == 0) continue;

        int16_t px = (int16_t)p.x;
        int16_t py = (int16_t)p.y;

        // Fade: use life ratio for size modulation
        const uint8_t* icon = nullptr;
        switch (p.type) {
        case ParticleType::HEART:      icon = spr_heart;     break;
        case ParticleType::ZZZ:        icon = spr_zzz;       break;
        case ParticleType::SPARKLE:    icon = spr_sparkle;   break;
        case ParticleType::TEAR:       icon = spr_tear;      break;
        case ParticleType::STAR:       icon = spr_star;      break;
        case ParticleType::SNOWFLAKE:  icon = spr_snowflake; break;
        case ParticleType::SWEAT:      icon = spr_sweat;     break;
        default: break;
        }
        if (icon) {
            // Draw 8x8 particle icon
            // Partial fade: skip drawing if life < 30 and it's on odd frame
            if (p.life < 50 && (p.life % 2 == 0)) continue;
            u8g2.drawXBMP(px, py, 8, 8, icon);
        }
    }
}

void Renderer::drawStatusBar(Pet& pet) {
    // Black bar at bottom 14px
    u8g2.drawBox(0, 50, 128, 14);
    u8g2.setDrawColor(0);  // draw in black (inverted)

    // 4 stat bars: hunger, happy, energy, health
    // Each bar: 8px icon + 22px fill bar, with 2px gap
    struct BarDef { const uint8_t* icon; uint8_t val; };
    BarDef bars[4] = {
        { icon_hunger, pet.state.hunger    },
        { icon_happy,  pet.state.happiness },
        { icon_energy, pet.state.energy    },
        { icon_health, pet.state.health    },
    };

    int16_t xOff = 2;
    for (int i = 0; i < 4; i++) {
        // Icon
        u8g2.drawXBMP(xOff, 52, 8, 8, bars[i].icon);
        xOff += 9;
        // Bar background (outline)
        u8g2.setDrawColor(0);
        u8g2.drawFrame(xOff, 54, 20, 5);
        // Bar fill
        uint8_t fillW = (uint8_t)(bars[i].val * 18 / 100);
        if (fillW > 0) {
            u8g2.drawBox(xOff + 1, 55, fillW, 3);
        }
        xOff += 22;
    }

    u8g2.setDrawColor(1);  // restore
}

void Renderer::drawBackground(Pet& pet) {
    // Ground line
    u8g2.drawHLine(0, 49, 128);

    // Small grass tufts
    for (int x = 4; x < 128; x += 16) {
        u8g2.drawLine(x, 49, x - 2, 46);
        u8g2.drawLine(x, 49, x,     46);
        u8g2.drawLine(x, 49, x + 2, 46);
    }

    // Stars at night
    if (pet.state.isNight) {
        uint8_t starX[] = { 10, 30, 50, 70, 90, 110, 20, 60, 100 };
        uint8_t starY[] = {  4,  8,  3,  6,  2,   5,  12, 10,  14 };
        for (int i = 0; i < 9; i++) {
            // Twinkle: alternate based on frameCount
            if ((pet.frameCount / 15 + i) % 3 != 0) {
                u8g2.drawPixel(starX[i], starY[i]);
            }
        }
    }
}

void Renderer::drawMoodText(Pet& pet) {
    if (pet.state.mood == PetMood::IDLE) return;  // no text for idle
    u8g2.setFont(u8g2_font_4x6_tr);
    const char* txt = MOOD_NAMES[pet.moodIndex()];
    u8g2.drawStr(1, 8, txt);
}

void Renderer::drawWeatherOverlay(Pet& pet) {
    // Snowflakes: code 71-77
    if (pet.state.weatherCode >= 71 && pet.state.weatherCode <= 77) {
        // Draw 3 pseudo-random snowflakes using frameCount as seed
        for (int i = 0; i < 3; i++) {
            uint8_t sx = (uint8_t)((pet.frameCount * (7 + i * 13) + i * 43) % 120);
            uint8_t sy = (uint8_t)((pet.frameCount * (3 + i * 7)  + i * 19) % 48);
            u8g2.drawPixel(sx, sy);
            u8g2.drawPixel(sx + 1, sy);
        }
    }

    // Rain lines: code 61-67
    if (pet.state.weatherCode >= 61 && pet.state.weatherCode <= 67) {
        for (int i = 0; i < 5; i++) {
            uint8_t rx = (uint8_t)((pet.frameCount * (5 + i * 11) + i * 31) % 124);
            uint8_t ry = (uint8_t)((pet.frameCount * (2 + i * 9)  + i * 17) % 44);
            u8g2.drawLine(rx, ry, rx - 1, ry + 3);
        }
    }
}

void Renderer::advanceAnimFrame(Pet& pet, uint32_t nowMs) {
    // Frame speed per mood (ms per frame)
    static const uint16_t FRAME_SPEED[] = {
        500, // IDLE: slow blink
        250, // HAPPY: lively
        600, // SAD: slow
        800, // SLEEPING: very slow
        150, // EATING: fast chomp
        200, // PLAYING: fast
        400, // SICK
        0,   // DEAD: frozen
    };
    uint16_t speed = FRAME_SPEED[pet.moodIndex()];
    if (speed == 0) { _animFrame = 0; return; }

    if (nowMs - _animTimer >= speed) {
        _animTimer = nowMs;
        uint8_t fc = getFrameCount(pet);
        _animFrame = (_animFrame + 1) % fc;
    }
}

uint8_t Renderer::getFrameCount(Pet& pet) {
    return getSpriteFrameCount(pet.moodIndex(), pet.isAdult());
}

const char* Renderer::moodName(PetMood mood) {
    return MOOD_NAMES[(uint8_t)mood];
}

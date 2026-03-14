#pragma once
#include <Arduino.h>
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Audio – PWM Buzzer via ledcWrite
// Melody sequences stored in PROGMEM
// Non-blocking playback via FreeRTOS task
// ═══════════════════════════════════════════════════════════════════════════════

struct Note {
    uint16_t freq;   // Hz; 0 = rest
    uint16_t durMs;  // duration in milliseconds
};

enum class MelodyID : uint8_t {
    NONE        = 0,
    FEED        = 1,
    PLAY        = 2,
    SLEEP       = 3,
    SAD         = 4,
    DEAD        = 5,
    LEVEL_UP    = 6,
    GAME_OVER   = 7,
    EVOLUTION   = 8,
    BUTTON_TICK = 9,
};

class AudioManager {
public:
    QueueHandle_t queue;  // MelodyID queue

    void init();
    void play(MelodyID id);
    void stop();

    // Called from audioTask – blocking playback
    void runTask();

private:
    void playMelody(const Note* notes, uint8_t len);
    void playNote(uint16_t freq, uint16_t durMs);
    bool _stopRequested;
};

extern AudioManager Audio;

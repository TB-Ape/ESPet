#pragma once
#include <stdint.h>
#include <Arduino.h>
#include "Personality.h"
#include "Evolution.h"
#include "Particles.h"
#include "Physics.h"
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Pet Core – State Machine, Stats, Behavior Tree
// ═══════════════════════════════════════════════════════════════════════════════

enum class PetMood : uint8_t {
    IDLE     = 0,
    HAPPY    = 1,
    SAD      = 2,
    SLEEPING = 3,
    EATING   = 4,
    PLAYING  = 5,
    SICK     = 6,
    DEAD     = 7,
    COUNT    = 8,
};

// Events that can be sent to the pet from buttons or world
enum class PetEvent : uint8_t {
    NONE      = 0,
    FEED      = 1,
    PLAY      = 2,
    SLEEP     = 3,
    WAKE      = 4,
    TIME_TICK = 5,  // every second
};

struct PetState {
    // Core stats (0–100)
    uint8_t hunger;
    uint8_t happiness;
    uint8_t energy;
    uint8_t health;

    // Identity
    Personality    personality;
    EvolutionStage stage;
    char           name[PET_NAME_MAX_LEN + 1];

    // Time / Growth
    uint32_t ageSeconds;
    uint16_t careScore;

    // Mood / behavior
    PetMood  mood;
    uint32_t moodTimerMs;       // how long pet has been in current mood
    uint32_t lastActionMs;      // last time player interacted

    // Stats tracking
    uint32_t totalInteractions;
    uint16_t feedCount;
    uint16_t playCount;
    uint16_t highscoreCatch;
    uint16_t highscoreRunner;

    // Physics
    float posX, posY;
    float velX, velY;

    // World state (set by external managers)
    bool    isNight;
    int8_t  ambientTemp;      // °C, -40..+80
    uint8_t weatherCode;      // Open-Meteo WMO code
    bool    isEvolutionAnim;  // true during evolution animation
    uint8_t evolutionAnimTimer;

    // Exploration/curiosity animation timer
    uint8_t exploreTimer;
};

class Pet {
public:
    PetState       state;
    ParticleSystem particles;

    Pet();
    void init();

    // Load/save via Preferences (NVS)
    void load();
    void save();
    void resetToDefaults();

    // ── Actions (called from input/web handlers) ─────────────────────────────
    void feed();
    void play();
    void forceSleep();
    void wake();
    void rename(const char* newName);

    // ── Main AI tick (called from Core 0, every 100ms) ───────────────────────
    void tick(uint32_t nowMs);

    // ── Accessors ─────────────────────────────────────────────────────────────
    uint8_t moodIndex() const { return (uint8_t)state.mood; }
    bool    isDead()    const { return state.mood == PetMood::DEAD; }
    bool    isAdult()   const {
        return state.stage == EvolutionStage::ADULT ||
               state.stage == EvolutionStage::SENIOR;
    }

    // Frame count for animation (incremented every render frame on Core 1)
    volatile uint32_t frameCount;

private:
    uint32_t _lastTickMs;
    uint32_t _lastSaveMs;
    uint32_t _lastSecondMs;
    uint32_t _secondCounter;  // seconds since last stat decay

    // Internal behavior tree
    void runBehaviorTree(uint32_t nowMs);
    void updateStats(uint32_t elapsedMs);
    void checkMoodTransitions();
    void checkEvolution();
    void checkRandomEvents(uint32_t nowMs);
    void setMood(PetMood newMood, uint32_t nowMs);
    void applyWorldEffects();
    uint8_t clampStat(int16_t val);
};

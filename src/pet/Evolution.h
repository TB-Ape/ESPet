#pragma once
#include <stdint.h>
#include "Personality.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Evolution System
// 4 growth stages: BABY → CHILD → ADULT → SENIOR
// Progression driven by age + careScore
// ═══════════════════════════════════════════════════════════════════════════════

enum class EvolutionStage : uint8_t {
    BABY   = 0,
    CHILD  = 1,
    ADULT  = 2,
    SENIOR = 3,
};

struct EvoStageInfo {
    const char*    name;
    uint32_t       minAgeHours;   // minimum real-time hours to advance
    uint8_t        minCareScore;  // minimum careScore required
    uint8_t        spriteW;       // sprite width
    uint8_t        spriteH;       // sprite height
};

static const EvoStageInfo EVO_STAGES[] = {
    { "Baby",   0,   0,  16, 16 },
    { "Child",  24,  50, 20, 20 },
    { "Adult",  72,  70, 24, 24 },
    { "Senior", 168,  0, 24, 24 },
};

class Evolution {
public:
    // Compute careScore increment for this tick
    // Called every second by petAITask
    // hunger, happy, energy, health: current stats (0–100)
    static uint8_t computeCareIncrement(uint8_t hunger, uint8_t happy,
                                        uint8_t energy, uint8_t health);

    // Check if stage can advance; returns true if it did
    // ageSeconds: total age in seconds
    // careScore: cumulative care score
    // stage: in/out parameter
    static bool checkEvolution(uint32_t ageSeconds, uint16_t careScore,
                               EvolutionStage& stage);

    // True if the pet should "evolve up" to next stage
    static bool canEvolve(EvolutionStage current, uint32_t ageSeconds,
                          uint16_t careScore);

    // Get sprite info for current stage
    static const EvoStageInfo& stageInfo(EvolutionStage stage) {
        return EVO_STAGES[(uint8_t)stage];
    }

    // Influence personality based on care quality at end of BABY stage
    // Returns modified personality (may shift toward GRUMPY or CHEERFUL)
    static Personality resolvePersonality(Personality base, uint16_t careScore);
};

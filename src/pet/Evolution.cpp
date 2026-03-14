#include "Evolution.h"
#include "../Config.h"

uint8_t Evolution::computeCareIncrement(uint8_t hunger, uint8_t happy,
                                         uint8_t energy, uint8_t health) {
    // Average of all stats, capped at 2 points per second
    uint16_t avg = (hunger + happy + energy + health) / 4;
    if (avg >= 80) return 2;
    if (avg >= 60) return 1;
    if (avg >= 40) return 0;  // neutral – no progress
    return 0;  // poor care: no increment (but could be negative in extended version)
}

bool Evolution::canEvolve(EvolutionStage current, uint32_t ageSeconds,
                           uint16_t careScore) {
    if (current == EvolutionStage::SENIOR) return false;

    uint8_t next = (uint8_t)current + 1;
    const EvoStageInfo& info = EVO_STAGES[next];

    uint32_t ageHours = ageSeconds / 3600;
    return (ageHours >= info.minAgeHours) && (careScore >= info.minCareScore);
}

bool Evolution::checkEvolution(uint32_t ageSeconds, uint16_t careScore,
                                EvolutionStage& stage) {
    if (canEvolve(stage, ageSeconds, careScore)) {
        stage = (EvolutionStage)((uint8_t)stage + 1);
        return true;
    }
    return false;
}

Personality Evolution::resolvePersonality(Personality base, uint16_t careScore) {
    // If baby was well cared for, shift toward CHEERFUL
    // If poorly cared for, shift toward GRUMPY
    if (careScore > 80 && base == Personality::LAZY) {
        return Personality::CHEERFUL;  // lazy but loved → cheerful
    }
    if (careScore < 30 && base == Personality::CURIOUS) {
        return Personality::GRUMPY;   // neglected curious → grumpy
    }
    if (careScore > 90) {
        // Excellent care: improve any personality
        if (base == Personality::GRUMPY)    return Personality::CURIOUS;
        if (base == Personality::LAZY)      return Personality::CHEERFUL;
    }
    return base;  // most personalities stay the same
}

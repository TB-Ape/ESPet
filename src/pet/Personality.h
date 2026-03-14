#pragma once
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════════
// Personality System
// Each pet gets a random personality at birth (stored in NVS).
// Traits modify stat decay rates and reaction behaviors.
// ═══════════════════════════════════════════════════════════════════════════════

enum class Personality : uint8_t {
    LAZY      = 0,  // Energy decays slow, happiness fast; loves sleep
    ENERGETIC = 1,  // Hunger/energy decays fast; bonus from play
    CURIOUS   = 2,  // Balanced; random explore animations
    GRUMPY    = 3,  // Happiness decays very fast; hard to please
    CHEERFUL  = 4,  // All decays slow; positive bias; recovers happiness
    COUNT     = 5
};

struct PersonalityTraits {
    // Multipliers applied to decay tick intervals (higher = slower decay)
    float hungerDecayMult;   // 1.0 = normal; 1.3 = 30% slower
    float happyDecayMult;
    float energyDecayMult;

    // Multipliers for action effects (1.0 = normal)
    float feedEffectMult;
    float playEffectMult;
    float sleepEffectMult;

    // Random event probabilities (0.0–1.0)
    float chanceRandomHappy;   // Spontaneous happiness gain per minute
    float chanceRandomExplore; // Spontaneous explore/curiosity animation

    // Happiness recovery: if >0, happiness slowly regenerates
    float happyRegenPerMin;

    // Display name
    const char* name;
    // Emoji-like symbol for UI
    const char* symbol;
};

// Trait table indexed by Personality enum
static const PersonalityTraits PERSONALITY_TRAITS[(uint8_t)Personality::COUNT] = {
    // LAZY
    {
        .hungerDecayMult   = 1.0f,
        .happyDecayMult    = 0.7f,  // happiness decays slower (content)
        .energyDecayMult   = 1.6f,  // energy decays faster (lazy but drains)
        .feedEffectMult    = 1.0f,
        .playEffectMult    = 0.7f,  // doesn't enjoy play as much
        .sleepEffectMult   = 1.4f,  // loves sleep
        .chanceRandomHappy = 0.05f,
        .chanceRandomExplore = 0.01f,
        .happyRegenPerMin  = 0.0f,
        .name   = "Lazy",
        .symbol = "ZZZ",
    },
    // ENERGETIC
    {
        .hungerDecayMult   = 0.7f,  // gets hungry fast
        .happyDecayMult    = 1.1f,
        .energyDecayMult   = 0.7f,  // burns energy fast
        .feedEffectMult    = 0.9f,
        .playEffectMult    = 1.4f,  // loves playing
        .sleepEffectMult   = 0.8f,
        .chanceRandomHappy = 0.08f,
        .chanceRandomExplore = 0.03f,
        .happyRegenPerMin  = 0.5f,
        .name   = "Energetic",
        .symbol = "!!",
    },
    // CURIOUS
    {
        .hungerDecayMult   = 1.0f,
        .happyDecayMult    = 1.0f,
        .energyDecayMult   = 1.0f,
        .feedEffectMult    = 1.0f,
        .playEffectMult    = 1.1f,
        .sleepEffectMult   = 1.0f,
        .chanceRandomHappy = 0.10f,
        .chanceRandomExplore = 0.15f,  // often curious
        .happyRegenPerMin  = 0.2f,
        .name   = "Curious",
        .symbol = "?",
    },
    // GRUMPY
    {
        .hungerDecayMult   = 0.9f,
        .happyDecayMult    = 0.6f,  // happiness drains very fast
        .energyDecayMult   = 1.0f,
        .feedEffectMult    = 0.8f,
        .playEffectMult    = 0.8f,  // hard to make happy
        .sleepEffectMult   = 1.1f,
        .chanceRandomHappy = 0.01f,
        .chanceRandomExplore = 0.02f,
        .happyRegenPerMin  = 0.0f,
        .name   = "Grumpy",
        .symbol = ">_<",
    },
    // CHEERFUL
    {
        .hungerDecayMult   = 1.1f,
        .happyDecayMult    = 1.4f,  // happiness drains slowly
        .energyDecayMult   = 1.1f,
        .feedEffectMult    = 1.1f,
        .playEffectMult    = 1.2f,
        .sleepEffectMult   = 1.1f,
        .chanceRandomHappy = 0.15f,
        .chanceRandomExplore = 0.05f,
        .happyRegenPerMin  = 1.0f,  // happiness regenerates
        .name   = "Cheerful",
        .symbol = ":D",
    },
};

// Helper: get traits for a personality
inline const PersonalityTraits& getTraits(Personality p) {
    return PERSONALITY_TRAITS[(uint8_t)p];
}

// Helper: pick a random personality
inline Personality randomPersonality() {
    return (Personality)(esp_random() % (uint8_t)Personality::COUNT);
}

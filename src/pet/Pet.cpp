#include "Pet.h"
#include <Preferences.h>
#include <Arduino.h>

static Preferences prefs;

Pet::Pet() : frameCount(0), _lastTickMs(0), _lastSaveMs(0),
             _lastSecondMs(0), _secondCounter(0) {
}

void Pet::init() {
    Physics::init();
    particles.reset();
    load();
}

void Pet::resetToDefaults() {
    state.hunger      = 80;
    state.happiness   = 80;
    state.energy      = 80;
    state.health      = 80;
    state.personality = randomPersonality();
    state.stage       = EvolutionStage::BABY;
    strlcpy(state.name, PET_NAME_DEFAULT, sizeof(state.name));
    state.ageSeconds  = 0;
    state.careScore   = 0;
    state.mood        = PetMood::IDLE;
    state.moodTimerMs = 0;
    state.lastActionMs = 0;
    state.totalInteractions = 0;
    state.feedCount   = 0;
    state.playCount   = 0;
    state.highscoreCatch  = 0;
    state.highscoreRunner = 0;
    state.posX        = 56.0f;
    state.posY        = 48.0f;
    state.velX        = 0.0f;
    state.velY        = 0.0f;
    state.isNight     = false;
    state.ambientTemp = 20;
    state.weatherCode = 0;
    state.isEvolutionAnim   = false;
    state.evolutionAnimTimer = 0;
    state.exploreTimer = 0;
}

void Pet::load() {
    prefs.begin(NVS_NAMESPACE, false);
    bool hasSave = prefs.getBool("saved", false);
    if (!hasSave) {
        prefs.end();
        resetToDefaults();
        return;
    }
    state.hunger      = prefs.getUChar("hunger",  80);
    state.happiness   = prefs.getUChar("happy",   80);
    state.energy      = prefs.getUChar("energy",  80);
    state.health      = prefs.getUChar("health",  80);
    state.personality = (Personality)prefs.getUChar("pers",  (uint8_t)Personality::CURIOUS);
    state.stage       = (EvolutionStage)prefs.getUChar("stage", (uint8_t)EvolutionStage::BABY);
    prefs.getString("name", state.name, sizeof(state.name));
    if (state.name[0] == '\0') strlcpy(state.name, PET_NAME_DEFAULT, sizeof(state.name));
    state.ageSeconds  = prefs.getULong("age",    0);
    state.careScore   = prefs.getUShort("care",  0);
    state.totalInteractions = prefs.getULong("inter",  0);
    state.feedCount   = prefs.getUShort("feeds", 0);
    state.playCount   = prefs.getUShort("plays", 0);
    state.highscoreCatch   = prefs.getUShort("hsCatch",  0);
    state.highscoreRunner  = prefs.getUShort("hsRun",   0);
    state.mood        = PetMood::IDLE;
    state.moodTimerMs = 0;
    state.lastActionMs = 0;
    state.posX        = 56.0f;
    state.posY        = 48.0f;
    state.velX        = 0.0f;
    state.velY        = 0.0f;
    state.isNight     = false;
    state.ambientTemp = 20;
    state.weatherCode = 0;
    state.isEvolutionAnim   = false;
    state.evolutionAnimTimer = 0;
    state.exploreTimer = 0;
    prefs.end();
}

void Pet::save() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("saved",   true);
    prefs.putUChar("hunger", state.hunger);
    prefs.putUChar("happy",  state.happiness);
    prefs.putUChar("energy", state.energy);
    prefs.putUChar("health", state.health);
    prefs.putUChar("pers",   (uint8_t)state.personality);
    prefs.putUChar("stage",  (uint8_t)state.stage);
    prefs.putString("name",  state.name);
    prefs.putULong("age",    state.ageSeconds);
    prefs.putUShort("care",  state.careScore);
    prefs.putULong("inter",  state.totalInteractions);
    prefs.putUShort("feeds", state.feedCount);
    prefs.putUShort("plays", state.playCount);
    prefs.putUShort("hsCatch", state.highscoreCatch);
    prefs.putUShort("hsRun",   state.highscoreRunner);
    prefs.end();
}

void Pet::rename(const char* newName) {
    strlcpy(state.name, newName, sizeof(state.name));
}

// ─── Actions ─────────────────────────────────────────────────────────────────

void Pet::feed() {
    if (state.mood == PetMood::DEAD) return;
    if (state.mood == PetMood::SLEEPING) return;

    const PersonalityTraits& t = getTraits(state.personality);
    int16_t dHunger = (int16_t)(FEED_HUNGER * t.feedEffectMult);
    int16_t dHealth = (int16_t)(FEED_HEALTH  * t.feedEffectMult);

    state.hunger     = clampStat((int16_t)state.hunger + dHunger);
    state.health     = clampStat((int16_t)state.health + dHealth);
    state.feedCount++;
    state.totalInteractions++;
    state.lastActionMs = millis();

    setMood(PetMood::EATING, millis());
    state.moodTimerMs = millis();

    // Spawn hearts if well-fed
    particles.burst(state.posX, state.posY - 10, ParticleType::HEART, 4);
}

void Pet::play() {
    if (state.mood == PetMood::DEAD) return;
    if (state.mood == PetMood::SLEEPING) return;
    if (state.energy < 10) return;  // too tired

    const PersonalityTraits& t = getTraits(state.personality);
    int16_t dHappy  = (int16_t)(PLAY_HAPPINESS * t.playEffectMult);
    int16_t dEnergy = PLAY_ENERGY;

    state.happiness  = clampStat((int16_t)state.happiness + dHappy);
    state.energy     = clampStat((int16_t)state.energy + dEnergy);
    state.playCount++;
    state.totalInteractions++;
    state.lastActionMs = millis();

    setMood(PetMood::PLAYING, millis());

    // Launch pet with velocity for bounce animation
    state.velX = (esp_random() & 1) ? 2.0f : -2.0f;
    state.velY = -4.0f;

    particles.burst(state.posX, state.posY, ParticleType::SPARKLE, 5);
}

void Pet::forceSleep() {
    if (state.mood == PetMood::DEAD) return;
    setMood(PetMood::SLEEPING, millis());
    particles.spawn(state.posX + 8, state.posY - 8, ParticleType::ZZZ, 2);
}

void Pet::wake() {
    if (state.mood == PetMood::SLEEPING) {
        setMood(PetMood::IDLE, millis());
    }
}

// ─── Main Tick ───────────────────────────────────────────────────────────────

void Pet::tick(uint32_t nowMs) {
    if (_lastTickMs == 0) {
        _lastTickMs   = nowMs;
        _lastSaveMs   = nowMs;
        _lastSecondMs = nowMs;
        return;
    }
    uint32_t elapsedMs = nowMs - _lastTickMs;
    _lastTickMs = nowMs;

    if (state.mood == PetMood::DEAD) {
        // Nothing to do when dead except periodic save
        if (nowMs - _lastSaveMs >= (SAVE_INTERVAL_S * 1000UL)) {
            save();
            _lastSaveMs = nowMs;
        }
        return;
    }

    // ── Per-second logic ───────────────────────────────────────────────────
    if (nowMs - _lastSecondMs >= 1000) {
        uint32_t seconds = (nowMs - _lastSecondMs) / 1000;
        _lastSecondMs += seconds * 1000;
        _secondCounter += seconds;
        state.ageSeconds += seconds;

        // careScore
        uint8_t inc = Evolution::computeCareIncrement(
            state.hunger, state.happiness, state.energy, state.health);
        state.careScore = (uint16_t)min(9999, (int)state.careScore + inc * seconds);

        // Happiness regen for CHEERFUL
        const PersonalityTraits& t = getTraits(state.personality);
        if (t.happyRegenPerMin > 0.0f && state.happiness < 95) {
            float regenPerSec = t.happyRegenPerMin / 60.0f;
            state.happiness = clampStat((int16_t)state.happiness +
                                         (int16_t)(regenPerSec * seconds));
        }
    }

    // ── Stat decay ────────────────────────────────────────────────────────
    updateStats(elapsedMs);

    // ── World effects ─────────────────────────────────────────────────────
    applyWorldEffects();

    // ── Behavior Tree ─────────────────────────────────────────────────────
    runBehaviorTree(nowMs);

    // ── Evolution check ───────────────────────────────────────────────────
    checkEvolution();

    // ── Evolution animation timer ─────────────────────────────────────────
    if (state.isEvolutionAnim) {
        if (state.evolutionAnimTimer > 0) {
            state.evolutionAnimTimer--;
        } else {
            state.isEvolutionAnim = false;
        }
    }

    // ── Physics update (playing / idle bounce) ────────────────────────────
    if (state.mood == PetMood::PLAYING ||
        (state.velX != 0.0f || state.velY != 0.0f)) {

        bool bounced = Physics::stepBounce(
            state.posX, state.posY, state.velX, state.velY,
            0.3f,  // gravity
            0.6f,  // damping
            8.0f,  // xMin
            120.0f,// xMax
            48.0f  // yFloor (above status bar)
        );
        if (bounced && fabsf(state.velY) < 0.2f && fabsf(state.velX) < 0.2f) {
            // Settle
            state.velX = 0;
            state.velY = 0;
        }
    }

    // ── Particle update ───────────────────────────────────────────────────
    particles.update(1.0f / TARGET_FPS);

    // ── Random events ─────────────────────────────────────────────────────
    checkRandomEvents(nowMs);

    // ── Auto-save ─────────────────────────────────────────────────────────
    if (nowMs - _lastSaveMs >= (SAVE_INTERVAL_S * 1000UL)) {
        save();
        _lastSaveMs = nowMs;
    }
}

// ─── Private Methods ──────────────────────────────────────────────────────────

void Pet::updateStats(uint32_t elapsedMs) {
    if (state.mood == PetMood::SLEEPING) {
        // Only energy recovers while sleeping
        const PersonalityTraits& t = getTraits(state.personality);
        float energyPerMs = (float)SLEEP_ENERGY / (60.0f * 1000.0f) * t.sleepEffectMult;
        state.energy = clampStat((int16_t)state.energy + (int16_t)(energyPerMs * elapsedMs));
        return;
    }

    const PersonalityTraits& t = getTraits(state.personality);

    // Hunger decay
    float hungerDecayPerMs = 1.0f / ((float)HUNGER_DECAY_S * 1000.0f * t.hungerDecayMult);
    float hdelta = hungerDecayPerMs * elapsedMs;
    if (hdelta >= 1.0f) {
        state.hunger = clampStat((int16_t)state.hunger - (int16_t)hdelta);
    }

    // Happiness decay
    float happyDecayPerMs = 1.0f / ((float)HAPPY_DECAY_S * 1000.0f * t.happyDecayMult);
    float apdelta = happyDecayPerMs * elapsedMs;
    if (apdelta >= 0.5f) {
        state.happiness = clampStat((int16_t)state.happiness - (int16_t)apdelta);
    }

    // Energy decay
    float energyDecayPerMs = 1.0f / ((float)ENERGY_DECAY_S * 1000.0f * t.energyDecayMult);
    float edelta = energyDecayPerMs * elapsedMs;
    if (edelta >= 0.5f) {
        state.energy = clampStat((int16_t)state.energy - (int16_t)edelta);
    }

    // Health decays when hunger AND happiness are low
    if (state.hunger < HUNGER_CRIT_THRESH && state.happiness < HAPPY_SAD_THRESH) {
        float healthDecayPerMs = 2.0f / (60.0f * 1000.0f);  // -2 per minute
        float hhdelta = healthDecayPerMs * elapsedMs;
        if (hhdelta >= 0.5f) {
            state.health = clampStat((int16_t)state.health - (int16_t)hhdelta);
        }
    }
    // Small health regen when stats are good
    else if (state.hunger > 60 && state.happiness > 60 && state.energy > 40) {
        float regenPerMs = 0.5f / (60.0f * 1000.0f);
        float rgdelta = regenPerMs * elapsedMs;
        if (rgdelta >= 0.2f && state.health < 100) {
            state.health = clampStat((int16_t)state.health + (int16_t)rgdelta);
        }
    }
}

void Pet::checkMoodTransitions() {
    if (state.mood == PetMood::DEAD) return;

    // Death check
    if (state.health == 0) {
        setMood(PetMood::DEAD, millis());
        return;
    }

    // Sick check
    if (state.health < HEALTH_SICK_THRESH && state.mood != PetMood::SICK) {
        setMood(PetMood::SICK, millis());
        return;
    }

    // Recovery from sick
    if (state.mood == PetMood::SICK && state.health >= HEALTH_SICK_THRESH + 10) {
        setMood(PetMood::IDLE, millis());
        return;
    }

    // Timed moods: EATING, PLAYING → auto-return to IDLE/SAD
    uint32_t nowMs = millis();
    if (state.mood == PetMood::EATING && (nowMs - state.moodTimerMs) > 3000) {
        setMood(state.happiness < HAPPY_SAD_THRESH ? PetMood::SAD : PetMood::HAPPY, nowMs);
        return;
    }
    if (state.mood == PetMood::PLAYING && (nowMs - state.moodTimerMs) > 5000) {
        setMood(PetMood::HAPPY, nowMs);
        return;
    }
    if (state.mood == PetMood::HAPPY && (nowMs - state.moodTimerMs) > 8000) {
        setMood(PetMood::IDLE, nowMs);
        return;
    }

    // Sleep from exhaustion
    if (state.energy <= ENERGY_SLEEP_THRESH && state.mood != PetMood::SLEEPING) {
        setMood(PetMood::SLEEPING, millis());
        particles.spawn(state.posX + 8, state.posY - 8, ParticleType::ZZZ, 2);
        return;
    }

    // Auto-wake after rest
    if (state.mood == PetMood::SLEEPING && state.energy >= 80) {
        if (!state.isNight) {  // Don't wake if it's night
            setMood(PetMood::IDLE, millis());
        }
        return;
    }

    // Sadness from hunger
    if (state.hunger < HUNGER_SAD_THRESH &&
        state.mood != PetMood::SAD && state.mood != PetMood::SLEEPING &&
        state.mood != PetMood::SICK) {
        setMood(PetMood::SAD, millis());
        particles.spawn(state.posX, state.posY, ParticleType::TEAR, 2);
        return;
    }

    // Recovery to IDLE from SAD if stats improved
    if (state.mood == PetMood::SAD &&
        state.hunger >= HUNGER_SAD_THRESH + 10 &&
        state.happiness >= HAPPY_SAD_THRESH + 10) {
        setMood(PetMood::IDLE, millis());
        return;
    }
}

void Pet::runBehaviorTree(uint32_t nowMs) {
    checkMoodTransitions();

    // Night time: encourage sleeping
    if (state.isNight && state.mood == PetMood::IDLE && state.energy < 60) {
        setMood(PetMood::SLEEPING, nowMs);
        return;
    }
}

void Pet::checkEvolution() {
    EvolutionStage prev = state.stage;
    if (Evolution::checkEvolution(state.ageSeconds, state.careScore, state.stage)) {
        // Stage advanced!
        if (prev == EvolutionStage::BABY) {
            // Resolve personality based on care
            state.personality = Evolution::resolvePersonality(
                state.personality, state.careScore);
        }
        // Trigger evolution animation
        state.isEvolutionAnim = true;
        state.evolutionAnimTimer = 120;  // ~4 seconds at 30fps
        particles.burst(state.posX, state.posY, ParticleType::SPARKLE, 12);
        particles.burst(state.posX, state.posY, ParticleType::STAR, 6);
    }
}

void Pet::checkRandomEvents(uint32_t nowMs) {
    // Every 5 seconds, check for random events
    static uint32_t lastEventCheck = 0;
    if (nowMs - lastEventCheck < 5000) return;
    lastEventCheck = nowMs;

    const PersonalityTraits& t = getTraits(state.personality);

    // Random happiness boost
    float roll = (float)(esp_random() % 1000) / 1000.0f;
    if (roll < t.chanceRandomHappy / 12.0f) {  // per 5s fraction
        state.happiness = clampStat((int16_t)state.happiness + 3);
    }

    // Random curious explore animation
    roll = (float)(esp_random() % 1000) / 1000.0f;
    if (state.personality == Personality::CURIOUS && roll < 0.08f &&
        state.mood == PetMood::IDLE) {
        state.exploreTimer = 60;  // 2s of explore anim
        // Tiny sparkle
        particles.spawn(state.posX + 12, state.posY - 5, ParticleType::SPARKLE, 1);
    }

    // Spontaneous attention-seeking if neglected (no interaction for 10+ min)
    if (nowMs - state.lastActionMs > 600000UL && state.mood == PetMood::IDLE) {
        state.happiness = clampStat((int16_t)state.happiness - 2);
        if (state.happiness < HAPPY_SAD_THRESH) {
            setMood(PetMood::SAD, nowMs);
            particles.spawn(state.posX, state.posY, ParticleType::TEAR, 1);
        }
    }
}

void Pet::setMood(PetMood newMood, uint32_t nowMs) {
    if (state.mood == newMood) return;
    state.mood        = newMood;
    state.moodTimerMs = nowMs;
}

void Pet::applyWorldEffects() {
    static uint32_t lastWorldTick = 0;
    uint32_t nowMs = millis();
    if (nowMs - lastWorldTick < 10000) return;  // every 10s
    lastWorldTick = nowMs;

    // Temperature effects
    if (state.ambientTemp < 10) {
        state.hunger  = clampStat((int16_t)state.hunger - 2);  // cold uses energy
        if (state.happiness > 5) state.happiness--;
    } else if (state.ambientTemp > 35) {
        state.energy  = clampStat((int16_t)state.energy - 3);  // hot drains energy
        state.health  = clampStat((int16_t)state.health - 1);
        particles.spawn(state.posX + 10, state.posY - 5, ParticleType::SWEAT, 1);
    }

    // Weather effects (WMO codes: 61-67=rain, 71-77=snow, 95-99=storm)
    if (state.weatherCode >= 95) {
        // Storm: pet gets scared
        if (state.mood == PetMood::IDLE) {
            state.happiness = clampStat((int16_t)state.happiness - 3);
        }
    }
    if (state.weatherCode >= 71 && state.weatherCode <= 77) {
        // Snow: particle effect added in renderer
    }
}

uint8_t Pet::clampStat(int16_t val) {
    if (val < 0)   return 0;
    if (val > 100) return 100;
    return (uint8_t)val;
}

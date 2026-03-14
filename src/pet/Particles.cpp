#include "Particles.h"
#include <Arduino.h>
#include <stdlib.h>

static inline float frand() {
    return (float)esp_random() / (float)UINT32_MAX;
}
static inline float frandRange(float lo, float hi) {
    return lo + frand() * (hi - lo);
}

ParticleSystem::ParticleSystem() {
    reset();
}

void ParticleSystem::reset() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        pool[i].life = 0;
        pool[i].type = ParticleType::NONE;
    }
}

Particle* ParticleSystem::getFree() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (pool[i].life == 0) return &pool[i];
    }
    // Pool full: evict oldest (smallest life)
    Particle* oldest = &pool[0];
    for (int i = 1; i < MAX_PARTICLES; i++) {
        if (pool[i].life < oldest->life) oldest = &pool[i];
    }
    return oldest;
}

void ParticleSystem::initParticle(Particle& p, float x, float y, ParticleType type) {
    p.x    = x;
    p.y    = y;
    p.type = type;
    p.size = 1;

    switch (type) {
    case ParticleType::HEART:
        p.vx     = frandRange(-0.4f, 0.4f);
        p.vy     = frandRange(-1.2f, -0.6f);  // float up
        p.life   = 200;
        p.maxLife= 200;
        p.size   = 1 + (esp_random() % 2);
        break;
    case ParticleType::ZZZ:
        p.vx     = frandRange(0.2f, 0.6f);    // drift right-up
        p.vy     = frandRange(-0.8f, -0.4f);
        p.life   = 180;
        p.maxLife= 180;
        p.size   = 1;
        break;
    case ParticleType::SPARKLE:
        p.vx     = frandRange(-1.0f, 1.0f);
        p.vy     = frandRange(-1.0f, 1.0f);
        p.life   = 120;
        p.maxLife= 120;
        p.size   = 1 + (esp_random() % 2);
        break;
    case ParticleType::TEAR:
        p.vx     = frandRange(-0.1f, 0.1f);
        p.vy     = frandRange(0.4f, 0.8f);    // fall down
        p.life   = 150;
        p.maxLife= 150;
        p.size   = 1;
        break;
    case ParticleType::STAR:
        p.vx     = frandRange(-0.8f, 0.8f);
        p.vy     = frandRange(-1.0f, -0.2f);
        p.life   = 160;
        p.maxLife= 160;
        p.size   = 1;
        break;
    case ParticleType::SNOWFLAKE:
        p.vx     = frandRange(-0.3f, 0.3f);
        p.vy     = frandRange(0.3f, 0.7f);    // fall down slowly
        p.life   = 220;
        p.maxLife= 220;
        p.size   = 1;
        break;
    case ParticleType::SWEAT:
        p.vx     = frandRange(-0.2f, 0.2f);
        p.vy     = frandRange(0.5f, 1.0f);
        p.life   = 100;
        p.maxLife= 100;
        p.size   = 1;
        break;
    default:
        p.life   = 0;
        break;
    }
}

void ParticleSystem::spawn(float x, float y, ParticleType type, uint8_t count) {
    for (int i = 0; i < count; i++) {
        Particle* p = getFree();
        if (p) initParticle(*p, x, y, type);
    }
}

void ParticleSystem::burst(float x, float y, ParticleType type, uint8_t count) {
    // Distribute across a small area
    for (int i = 0; i < count; i++) {
        float bx = x + frandRange(-8.0f, 8.0f);
        float by = y + frandRange(-4.0f, 4.0f);
        Particle* p = getFree();
        if (p) initParticle(*p, bx, by, type);
    }
}

void ParticleSystem::update(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = pool[i];
        if (p.life == 0) continue;

        // Apply velocity
        p.x += p.vx;
        p.y += p.vy;

        // Apply gravity for falling particles
        if (p.type == ParticleType::TEAR ||
            p.type == ParticleType::SNOWFLAKE ||
            p.type == ParticleType::SWEAT) {
            p.vy += 0.05f;  // gentle gravity
        }

        // Slight gravity opposition for rising particles
        if (p.type == ParticleType::HEART ||
            p.type == ParticleType::ZZZ ||
            p.type == ParticleType::STAR) {
            p.vy += 0.02f;  // drag slows rise
        }

        // Decay life
        uint8_t decay = (p.type == ParticleType::SNOWFLAKE) ? 1 : 2;
        if (p.life <= decay) {
            p.life = 0;
            p.type = ParticleType::NONE;
        } else {
            p.life -= decay;
        }

        // Kill if off-screen
        if (p.x < -8 || p.x > 136 || p.y < -8 || p.y > 72) {
            p.life = 0;
            p.type = ParticleType::NONE;
        }
    }
}

uint8_t ParticleSystem::activeCount() const {
    uint8_t count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (pool[i].life > 0) count++;
    }
    return count;
}

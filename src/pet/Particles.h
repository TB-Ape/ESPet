#pragma once
#include <stdint.h>
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Particle System
// Fixed-size pool (MAX_PARTICLES), no dynamic allocation
// Updated on Core 0 (petAITask), rendered on Core 1 (renderTask) — but
// the render only reads position/life/type so no mutex needed for rendering
// ═══════════════════════════════════════════════════════════════════════════════

enum class ParticleType : uint8_t {
    NONE = 0,
    HEART,
    ZZZ,
    SPARKLE,
    TEAR,
    STAR,
    SNOWFLAKE,
    SWEAT,
};

struct Particle {
    float         x, y;
    float         vx, vy;
    uint8_t       life;      // 255 = fresh, 0 = dead
    uint8_t       maxLife;
    ParticleType  type;
    uint8_t       size;      // 1–3 (render scale)
};

class ParticleSystem {
public:
    Particle pool[MAX_PARTICLES];

    ParticleSystem();
    void reset();

    // Spawn particle at (x,y) with given type
    void spawn(float x, float y, ParticleType type, uint8_t count = 1);

    // Spawn burst (multiple particles with random velocities)
    void burst(float x, float y, ParticleType type, uint8_t count = 5);

    // Update all particles (call every frame, dt in seconds)
    void update(float dt);

    // Count active particles
    uint8_t activeCount() const;

private:
    Particle* getFree();
    void initParticle(Particle& p, float x, float y, ParticleType type);
};

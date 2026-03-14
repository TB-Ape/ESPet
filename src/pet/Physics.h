#pragma once
#include <stdint.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════════
// Physics & Animation Math
// Sin/cos lookup table (256 entries, 0–255 → 0–2π)
// Easing functions for smooth animations
// ═══════════════════════════════════════════════════════════════════════════════

class Physics {
public:
    // ── Sin lookup table (Q8.8 fixed-point: -128..127) ──────────────────────
    static int8_t sinTable[256];

    static void init();

    // ── Sin/Cos via lookup (angle: 0–255) ───────────────────────────────────
    static inline int8_t sinLUT(uint8_t angle) {
        return sinTable[angle];
    }
    static inline int8_t cosLUT(uint8_t angle) {
        return sinTable[(angle + 64) & 0xFF];
    }

    // ── Easing functions (input: 0.0–1.0, output: 0.0–1.0) ─────────────────
    static float easeInOut(float t);
    static float easeOut(float t);
    static float easeIn(float t);
    static float easeElastic(float t);
    static float easeBounce(float t);

    // ── Bobbing offset for idle animation ───────────────────────────────────
    // Returns a Y offset (-2..+2) for smooth up/down bobbing
    static float bobbingOffset(uint32_t frameCount, float amplitude = 2.0f, float speed = 0.08f);

    // ── Breathing scale (0.95–1.05) ─────────────────────────────────────────
    static float breathingScale(uint32_t frameCount);

    // ── Lerp ─────────────────────────────────────────────────────────────────
    static inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    static inline float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    // ── Physics step for bouncing pet ───────────────────────────────────────
    // Call every frame; updates pos/vel in-place; returns true if bounced
    static bool stepBounce(float& x, float& y, float& vx, float& vy,
                           float gravity, float damping,
                           float xMin, float xMax, float yFloor);

    // ── Simple shake offset for sick/scared animation ───────────────────────
    static float shakeOffset(uint32_t frameCount, float amplitude = 1.5f);
};

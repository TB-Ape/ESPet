#include "Physics.h"
#include <Arduino.h>

// Pre-computed sin table (256 entries)
int8_t Physics::sinTable[256];

void Physics::init() {
    for (int i = 0; i < 256; i++) {
        sinTable[i] = (int8_t)(127.0f * sinf(2.0f * M_PI * i / 256.0f));
    }
}

float Physics::easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float Physics::easeOut(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float Physics::easeIn(float t) {
    return t * t;
}

float Physics::easeElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * (2.0f * M_PI / 3.0f)) + 1.0f;
}

float Physics::easeBounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

float Physics::bobbingOffset(uint32_t frameCount, float amplitude, float speed) {
    float angle = frameCount * speed;
    uint8_t idx = (uint8_t)(angle * (256.0f / (2.0f * M_PI)));
    return (sinTable[idx] / 127.0f) * amplitude;
}

float Physics::breathingScale(uint32_t frameCount) {
    float angle = frameCount * 0.04f;
    uint8_t idx = (uint8_t)(angle * (256.0f / (2.0f * M_PI)));
    return 1.0f + (sinTable[idx] / 127.0f) * 0.04f;  // 0.96 - 1.04
}

bool Physics::stepBounce(float& x, float& y, float& vx, float& vy,
                         float gravity, float damping,
                         float xMin, float xMax, float yFloor) {
    bool bounced = false;

    vx *= 0.99f;  // air friction
    vy += gravity;

    x += vx;
    y += vy;

    if (x < xMin) {
        x = xMin;
        vx = -vx * damping;
        bounced = true;
    }
    if (x > xMax) {
        x = xMax;
        vx = -vx * damping;
        bounced = true;
    }
    if (y >= yFloor) {
        y = yFloor;
        vy = -vy * damping;
        if (fabsf(vy) < 0.5f) vy = 0.0f;
        bounced = true;
    }
    return bounced;
}

float Physics::shakeOffset(uint32_t frameCount, float amplitude) {
    // Quick pseudo-random shake using sin at prime frequency
    uint8_t idx1 = (uint8_t)(frameCount * 23);
    uint8_t idx2 = (uint8_t)(frameCount * 37);
    return (sinTable[idx1] / 127.0f + sinTable[idx2] / 127.0f) * 0.5f * amplitude;
}

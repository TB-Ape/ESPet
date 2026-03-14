#pragma once
#include <Arduino.h>
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Button Input – ISR-driven with debounce, FreeRTOS queue
// ═══════════════════════════════════════════════════════════════════════════════

enum class ButtonID : uint8_t {
    FEED  = 0,
    PLAY  = 1,
    SLEEP = 2,
};

enum class ButtonEvent : uint8_t {
    PRESS        = 0,
    LONG_PRESS   = 1,   // held > 1s
};

struct ButtonMsg {
    ButtonID    btn;
    ButtonEvent evt;
};

class InputManager {
public:
    QueueHandle_t queue;  // ButtonMsg queue, capacity 8

    void init();

    // Call from inputTask on Core 1
    void poll(uint32_t nowMs);

    // Peek without consuming (used for games)
    bool peek(ButtonMsg& out);

    // Consume next message (blocks up to ticksToWait)
    bool receive(ButtonMsg& out, TickType_t ticksToWait = 0);

private:
    struct BtnState {
        bool     lastRaw;
        bool     debounced;
        uint32_t lastChangeMs;
        uint32_t pressStartMs;
        bool     longFired;
    };

    BtnState _states[3];
    uint8_t  _pins[3];

    void processButton(uint8_t idx, uint32_t nowMs);
};

extern InputManager Input;

#include "Input.h"

InputManager Input;

void InputManager::init() {
    queue = xQueueCreate(8, sizeof(ButtonMsg));
    configASSERT(queue);

    _pins[0] = PIN_BTN_FEED;
    _pins[1] = PIN_BTN_PLAY;
    _pins[2] = PIN_BTN_SLEEP;

    // GPIO 0 (BOOT) is active LOW, others depend on wiring
    // We use INPUT_PULLUP for all; buttons connect pin to GND
    pinMode(PIN_BTN_FEED,  INPUT_PULLUP);
    pinMode(PIN_BTN_PLAY,  INPUT_PULLUP);
    pinMode(PIN_BTN_SLEEP, INPUT_PULLUP);

    for (int i = 0; i < 3; i++) {
        _states[i] = { true, true, 0, 0, false };
    }
}

void InputManager::poll(uint32_t nowMs) {
    for (int i = 0; i < 3; i++) {
        processButton(i, nowMs);
    }
}

void InputManager::processButton(uint8_t idx, uint32_t nowMs) {
    BtnState& s  = _states[idx];
    bool rawState = digitalRead(_pins[idx]) == LOW;  // active LOW

    // Debounce: state must be stable for 50ms
    if (rawState != s.lastRaw) {
        s.lastRaw      = rawState;
        s.lastChangeMs = nowMs;
    }

    if ((nowMs - s.lastChangeMs) < 50) return;  // not yet stable

    bool prev    = s.debounced;
    s.debounced  = rawState;

    if (!prev && s.debounced) {
        // Button just pressed
        s.pressStartMs = nowMs;
        s.longFired    = false;
    }
    else if (prev && !s.debounced) {
        // Button released
        if (!s.longFired) {
            ButtonMsg msg = { (ButtonID)idx, ButtonEvent::PRESS };
            xQueueSendToBack(queue, &msg, 0);
        }
    }
    else if (s.debounced && !s.longFired) {
        // Still held: check for long press (1000ms)
        if ((nowMs - s.pressStartMs) >= 1000) {
            s.longFired = true;
            ButtonMsg msg = { (ButtonID)idx, ButtonEvent::LONG_PRESS };
            xQueueSendToBack(queue, &msg, 0);
        }
    }
}

bool InputManager::peek(ButtonMsg& out) {
    return xQueuePeek(queue, &out, 0) == pdTRUE;
}

bool InputManager::receive(ButtonMsg& out, TickType_t ticksToWait) {
    return xQueueReceive(queue, &out, ticksToWait) == pdTRUE;
}

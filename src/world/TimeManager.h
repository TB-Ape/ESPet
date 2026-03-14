#pragma once
#include <Arduino.h>
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// TimeManager – NTP sync, day/night cycle
// Uses configTime() from Arduino ESP32 framework
// Falls back to millis()-based counter when WiFi is unavailable
// ═══════════════════════════════════════════════════════════════════════════════

struct TimeInfo {
    uint8_t hour;   // 0–23
    uint8_t minute; // 0–59
    uint8_t second; // 0–59
    bool    valid;  // true if NTP-synced
    bool    isNight; // 22:00–07:00
};

class TimeManager {
public:
    void init(const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3");
    void sync();

    TimeInfo getTime() const;
    bool     isNight() const;

    // Check if a new hour started since last call
    bool     hourChanged();

    // Timezone string (POSIX format), configurable via Web UI
    char timezone[48];

private:
    uint8_t _lastHour;
    bool    _synced;
};

extern TimeManager TimeMan;

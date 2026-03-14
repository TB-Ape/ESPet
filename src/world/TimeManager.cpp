#include "TimeManager.h"
#include <time.h>
#include <WiFi.h>

TimeManager TimeMan;

void TimeManager::init(const char* tz) {
    strlcpy(timezone, tz, sizeof(timezone));
    _lastHour = 255;
    _synced   = false;
}

void TimeManager::sync() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", timezone, 1);
    tzset();

    // Wait up to 3s for sync
    struct tm ti;
    uint32_t start = millis();
    while (!getLocalTime(&ti) && (millis() - start) < 3000) {
        delay(100);
    }
    _synced = getLocalTime(&ti);
}

TimeInfo TimeManager::getTime() const {
    TimeInfo info = { 12, 0, 0, false, false };
    struct tm ti;
    if (getLocalTime(&ti)) {
        info.hour   = (uint8_t)ti.tm_hour;
        info.minute = (uint8_t)ti.tm_min;
        info.second = (uint8_t)ti.tm_sec;
        info.valid  = true;
        info.isNight = (ti.tm_hour >= 22 || ti.tm_hour < 7);
    }
    return info;
}

bool TimeManager::isNight() const {
    TimeInfo t = getTime();
    if (!t.valid) return false;
    return t.isNight;
}

bool TimeManager::hourChanged() {
    TimeInfo t = getTime();
    if (!t.valid) return false;
    if (t.hour != _lastHour) {
        _lastHour = t.hour;
        return true;
    }
    return false;
}

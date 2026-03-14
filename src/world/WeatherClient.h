#pragma once
#include <Arduino.h>
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// WeatherClient – Open-Meteo API (free, no API key)
// GET https://api.open-meteo.com/v1/forecast?latitude=XX&longitude=XX
//     &current=temperature_2m,weathercode
// ═══════════════════════════════════════════════════════════════════════════════

struct WeatherData {
    int8_t  temperature;  // °C
    uint8_t weatherCode;  // WMO code (0=clear, 61=rain, 71=snow, 95=storm)
    bool    valid;
};

class WeatherClient {
public:
    WeatherData current;
    float       latitude;
    float       longitude;

    WeatherClient() : latitude(48.14f), longitude(11.58f) {  // Default: Munich
        current = { 20, 0, false };
    }

    void setLocation(float lat, float lon);

    // Fetch weather from Open-Meteo (blocking, ~500ms)
    bool fetch();

    // Parse JSON response (minimal parser, no library needed)
    bool parseResponse(const char* json);

    // WMO code description
    static const char* weatherDescription(uint8_t code);

private:
    char _url[256];
    void buildURL();
};

extern WeatherClient Weather;

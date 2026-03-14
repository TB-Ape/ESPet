#include "WeatherClient.h"
#include <WiFiClient.h>
#include <HTTPClient.h>

WeatherClient Weather;

void WeatherClient::setLocation(float lat, float lon) {
    latitude  = lat;
    longitude = lon;
}

void WeatherClient::buildURL() {
    snprintf(_url, sizeof(_url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,weathercode"
        "&forecast_days=1",
        latitude, longitude);
}

bool WeatherClient::fetch() {
    buildURL();

    HTTPClient http;
    http.begin(_url);
    http.setTimeout(5000);

    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    return parseResponse(body.c_str());
}

bool WeatherClient::parseResponse(const char* json) {
    // Minimal JSON parsing without a library
    // Look for "temperature_2m":<value> and "weathercode":<value>

    const char* tempKey = "\"temperature_2m\":";
    const char* wcode   = "\"weathercode\":";

    const char* p = strstr(json, tempKey);
    if (p) {
        p += strlen(tempKey);
        current.temperature = (int8_t)atof(p);
    }

    p = strstr(json, wcode);
    if (p) {
        p += strlen(wcode);
        current.weatherCode = (uint8_t)atoi(p);
    }

    current.valid = (p != nullptr);
    return current.valid;
}

const char* WeatherClient::weatherDescription(uint8_t code) {
    if (code == 0)           return "Clear";
    if (code <= 3)           return "Partly Cloudy";
    if (code <= 48)          return "Foggy";
    if (code <= 55)          return "Drizzle";
    if (code <= 57)          return "Frz Drizzle";
    if (code <= 65)          return "Rain";
    if (code <= 67)          return "Frz Rain";
    if (code <= 77)          return "Snow";
    if (code <= 82)          return "Showers";
    if (code <= 86)          return "Snow Shower";
    if (code == 95)          return "Thunderstorm";
    if (code <= 99)          return "Heavy Storm";
    return "Unknown";
}

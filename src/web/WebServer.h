#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "../pet/Pet.h"
#include "../world/WeatherClient.h"
#include "../world/TimeManager.h"
#include "../Config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// WebServer – ESPAsyncWebServer
// Access Point: "ESPet-XXXX" (last 4 of MAC)
// URL: http://192.168.4.1
// ═══════════════════════════════════════════════════════════════════════════════

class PetWebServer {
public:
    AsyncWebServer server;
    Pet*           pet;

    // WiFi credentials (stored in NVS)
    char ssid[33];
    char pass[65];

    // Location for weather
    float latitude;
    float longitude;

    PetWebServer() : server(80), pet(nullptr),
                     latitude(48.14f), longitude(11.58f) {
        ssid[0] = '\0';
        pass[0] = '\0';
    }

    void init(Pet* p);
    void startAP();
    bool connectWiFi(uint32_t timeoutMs = WIFI_CONNECT_TIMEOUT_MS);

    void loadConfig();
    void saveConfig();

private:
    void setupRoutes();

    // Route handlers
    void handleRoot(AsyncWebServerRequest* req);
    void handleApiStatus(AsyncWebServerRequest* req);
    void handleApiFeed(AsyncWebServerRequest* req);
    void handleApiPlay(AsyncWebServerRequest* req);
    void handleApiSleep(AsyncWebServerRequest* req);
    void handleApiRename(AsyncWebServerRequest* req);
    void handleConfig(AsyncWebServerRequest* req);
    void handleConfigSave(AsyncWebServerRequest* req);

    // HTML builder
    String buildDashboardHTML();
    String buildConfigHTML();
    String buildStatusJSON();
};

extern PetWebServer WebSrv;

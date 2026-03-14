#include "WebServer.h"
#include <Preferences.h>

PetWebServer WebSrv;

static Preferences webPrefs;

void PetWebServer::loadConfig() {
    webPrefs.begin("espet-web", true);
    webPrefs.getString("ssid", ssid, sizeof(ssid));
    webPrefs.getString("pass", pass, sizeof(pass));
    latitude  = webPrefs.getFloat("lat",  48.14f);
    longitude = webPrefs.getFloat("lon",  11.58f);
    // Timezone
    char tz[48];
    webPrefs.getString("tz", tz, sizeof(tz));
    if (tz[0] != '\0') strlcpy(TimeMan.timezone, tz, sizeof(TimeMan.timezone));
    webPrefs.end();
}

void PetWebServer::saveConfig() {
    webPrefs.begin("espet-web", false);
    webPrefs.putString("ssid", ssid);
    webPrefs.putString("pass", pass);
    webPrefs.putFloat("lat",   latitude);
    webPrefs.putFloat("lon",   longitude);
    webPrefs.putString("tz",   TimeMan.timezone);
    webPrefs.end();
}

void PetWebServer::init(Pet* p) {
    pet = p;
    loadConfig();
    setupRoutes();
}

void PetWebServer::startAP() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[20];
    snprintf(apName, sizeof(apName), "%s%02X%02X", WIFI_AP_PREFIX, mac[4], mac[5]);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName);
    Serial.printf("[WiFi] AP started: %s, IP: %s\n", apName, WiFi.softAPIP().toString().c_str());
    server.begin();
}

bool PetWebServer::connectWiFi(uint32_t timeoutMs) {
    if (ssid[0] == '\0') return false;

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, pass);
    Serial.printf("[WiFi] Connecting to %s...\n", ssid);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.println("[WiFi] Connection failed, staying in AP mode.");
    return false;
}

void PetWebServer::setupRoutes() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleRoot(req);
    });
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleApiStatus(req);
    });
    server.on("/api/feed", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleApiFeed(req);
    });
    server.on("/api/play", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleApiPlay(req);
    });
    server.on("/api/sleep", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleApiSleep(req);
    });
    server.on("/api/rename", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleApiRename(req);
    });
    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleConfig(req);
    });
    server.on("/config/save", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleConfigSave(req);
    });
    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });
}

// ─── Route Handlers ───────────────────────────────────────────────────────────

void PetWebServer::handleRoot(AsyncWebServerRequest* req) {
    req->send(200, "text/html", buildDashboardHTML());
}

void PetWebServer::handleApiStatus(AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStatusJSON());
}

void PetWebServer::handleApiFeed(AsyncWebServerRequest* req) {
    if (pet) pet->feed();
    req->send(200, "application/json", "{\"ok\":true}");
}

void PetWebServer::handleApiPlay(AsyncWebServerRequest* req) {
    if (pet) pet->play();
    req->send(200, "application/json", "{\"ok\":true}");
}

void PetWebServer::handleApiSleep(AsyncWebServerRequest* req) {
    if (pet) pet->forceSleep();
    req->send(200, "application/json", "{\"ok\":true}");
}

void PetWebServer::handleApiRename(AsyncWebServerRequest* req) {
    if (req->hasParam("name", true)) {
        String name = req->getParam("name", true)->value();
        if (pet) pet->rename(name.c_str());
        pet->save();
        req->send(200, "application/json", "{\"ok\":true}");
    } else {
        req->send(400, "application/json", "{\"error\":\"Missing name\"}");
    }
}

void PetWebServer::handleConfig(AsyncWebServerRequest* req) {
    req->send(200, "text/html", buildConfigHTML());
}

void PetWebServer::handleConfigSave(AsyncWebServerRequest* req) {
    if (req->hasParam("ssid", true))
        strlcpy(ssid, req->getParam("ssid", true)->value().c_str(), sizeof(ssid));
    if (req->hasParam("pass", true))
        strlcpy(pass, req->getParam("pass", true)->value().c_str(), sizeof(pass));
    if (req->hasParam("lat", true))
        latitude = req->getParam("lat", true)->value().toFloat();
    if (req->hasParam("lon", true))
        longitude = req->getParam("lon", true)->value().toFloat();
    if (req->hasParam("tz", true))
        strlcpy(TimeMan.timezone, req->getParam("tz", true)->value().c_str(),
                sizeof(TimeMan.timezone));

    Weather.setLocation(latitude, longitude);
    saveConfig();

    req->redirect("/");
}

// ─── HTML / JSON Builders ─────────────────────────────────────────────────────

String PetWebServer::buildStatusJSON() {
    if (!pet) return "{}";
    PetState& s = pet->state;

    char buf[512];
    snprintf(buf, sizeof(buf),
        "{"
        "\"name\":\"%s\","
        "\"hunger\":%d,"
        "\"happiness\":%d,"
        "\"energy\":%d,"
        "\"health\":%d,"
        "\"mood\":\"%s\","
        "\"personality\":\"%s\","
        "\"stage\":\"%s\","
        "\"age\":%lu,"
        "\"interactions\":%lu,"
        "\"highscoreCatch\":%d,"
        "\"highscoreRunner\":%d,"
        "\"isNight\":%s,"
        "\"temp\":%d,"
        "\"weather\":\"%s\""
        "}",
        s.name,
        s.hunger, s.happiness, s.energy, s.health,
        // mood name
        (s.mood == PetMood::IDLE)     ? "Idle"    :
        (s.mood == PetMood::HAPPY)    ? "Happy"   :
        (s.mood == PetMood::SAD)      ? "Sad"     :
        (s.mood == PetMood::SLEEPING) ? "Sleeping":
        (s.mood == PetMood::EATING)   ? "Eating"  :
        (s.mood == PetMood::PLAYING)  ? "Playing" :
        (s.mood == PetMood::SICK)     ? "Sick"    : "Dead",
        getTraits(s.personality).name,
        Evolution::stageInfo(s.stage).name,
        s.ageSeconds,
        s.totalInteractions,
        s.highscoreCatch, s.highscoreRunner,
        s.isNight ? "true" : "false",
        (int)s.ambientTemp,
        WeatherClient::weatherDescription(s.weatherCode)
    );
    return String(buf);
}

String PetWebServer::buildDashboardHTML() {
    if (!pet) return "<html><body>No pet</body></html>";
    PetState& s = pet->state;

    String h = R"(<!DOCTYPE html>
<html><head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta http-equiv='refresh' content='3'>
<title>ESPet Dashboard</title>
<style>
body{font-family:monospace;background:#111;color:#0f0;margin:20px}
h1{color:#0ff;text-align:center}
.card{background:#222;border:1px solid #0f0;border-radius:8px;padding:12px;margin:10px 0}
.bar-bg{background:#333;height:12px;border-radius:6px;overflow:hidden;margin:4px 0}
.bar-fill{height:100%;border-radius:6px;transition:width .3s}
.label{font-size:12px;color:#888}
.val{float:right;color:#fff}
.btn{background:#0a3;color:#fff;border:none;padding:8px 16px;border-radius:4px;
     cursor:pointer;font-size:14px;margin:4px}
.btn:hover{background:#0c5}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.mood-badge{display:inline-block;background:#003;padding:4px 10px;
            border-radius:12px;color:#0ff;font-size:13px}
.stat-row{margin:6px 0}
</style></head><body>
<h1>&#127760; ESPet</h1>)";

    // Pet info card
    h += "<div class='card'>";
    h += "<b style='color:#ff0;font-size:18px'>&#128149; ";
    h += s.name;
    h += "</b> &nbsp; <span class='mood-badge'>";
    // mood
    const char* moodNames[] = {"Idle","Happy","Sad","Sleeping","Eating","Playing","Sick","Dead"};
    h += moodNames[(uint8_t)s.mood];
    h += "</span><br><span class='label'>Personality: ";
    h += getTraits(s.personality).name;
    h += " &bull; Stage: ";
    h += Evolution::stageInfo(s.stage).name;
    h += " &bull; Age: ";
    h += String(s.ageSeconds / 86400);
    h += "d</span></div>";

    // Stats card
    h += "<div class='card'>";
    struct StatRow { const char* name; uint8_t val; const char* color; };
    StatRow stats[] = {
        {"&#127860; Hunger",    s.hunger,    "#f80"},
        {"&#128512; Happiness", s.happiness, "#ff0"},
        {"&#9889; Energy",      s.energy,    "#0af"},
        {"&#10084; Health",     s.health,    "#f44"},
    };
    for (auto& st : stats) {
        h += "<div class='stat-row'><span class='label'>";
        h += st.name;
        h += "<span class='val'>";
        h += String(st.val);
        h += "%</span></span>";
        h += "<div class='bar-bg'><div class='bar-fill' style='width:";
        h += String(st.val);
        h += "%;background:";
        h += st.color;
        h += "'></div></div></div>";
    }
    h += "</div>";

    // Action buttons
    h += "<div class='card'><b>Actions</b><br>";
    h += "<form method='POST' action='/api/feed' style='display:inline'>"
         "<button class='btn'>&#127860; Feed</button></form> ";
    h += "<form method='POST' action='/api/play' style='display:inline'>"
         "<button class='btn' style='background:#038'>&#127923; Play</button></form> ";
    h += "<form method='POST' action='/api/sleep' style='display:inline'>"
         "<button class='btn' style='background:#555'>&#128564; Sleep</button></form>";
    h += "</div>";

    // World + Highscores
    h += "<div class='grid'>";
    h += "<div class='card'><b>&#127777; World</b><br>"
         "<span class='label'>Temp: ";
    h += String((int)s.ambientTemp);
    h += "°C &bull; Weather: ";
    h += WeatherClient::weatherDescription(s.weatherCode);
    h += "<br>";
    h += s.isNight ? "&#127761; Night" : "&#9728; Day";
    h += "</span></div>";

    h += "<div class='card'><b>&#127942; Highscores</b><br>"
         "<span class='label'>Catch: ";
    h += String(s.highscoreCatch);
    h += "<br>Runner: ";
    h += String(s.highscoreRunner);
    h += "m</span></div></div>";

    // Footer link
    h += "<p style='text-align:center'><a href='/config' style='color:#888'>&#9881; Settings</a></p>";
    h += "</body></html>";
    return h;
}

String PetWebServer::buildConfigHTML() {
    String h = R"(<!DOCTYPE html>
<html><head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ESPet Settings</title>
<style>
body{font-family:monospace;background:#111;color:#0f0;margin:20px}
h1{color:#0ff}
input{background:#222;color:#0f0;border:1px solid #0f0;padding:6px;
      border-radius:4px;width:100%;box-sizing:border-box;margin:4px 0}
label{font-size:13px;color:#888}
.btn{background:#0a3;color:#fff;border:none;padding:10px 20px;
     border-radius:4px;cursor:pointer;width:100%;margin-top:12px}
</style></head>
<body><h1>&#9881; Settings</h1>
<form method='POST' action='/config/save'>)";

    h += "<label>WiFi SSID</label>"
         "<input name='ssid' value='"; h += ssid; h += "'><br>";
    h += "<label>WiFi Password</label>"
         "<input name='pass' type='password' value=''><br>";
    h += "<label>Latitude</label>"
         "<input name='lat' value='"; h += String(latitude,4); h += "'><br>";
    h += "<label>Longitude</label>"
         "<input name='lon' value='"; h += String(longitude,4); h += "'><br>";
    h += "<label>Timezone (POSIX, e.g. CET-1CEST,M3.5.0,M10.5.0/3)</label>"
         "<input name='tz' value='"; h += TimeMan.timezone; h += "'><br>";
    h += "<button class='btn' type='submit'>Save &amp; Restart</button>"
         "</form><br><a href='/' style='color:#888'>&#8592; Back</a>"
         "</body></html>";
    return h;
}

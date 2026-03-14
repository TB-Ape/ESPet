#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "Config.h"
#include "pet/Pet.h"
#include "display/Renderer.h"
#include "input/Input.h"
#include "audio/Audio.h"
#include "web/WebServer.h"
#include "world/TimeManager.h"
#include "world/WeatherClient.h"
#include "games/GameManager.h"

#ifdef ENABLE_SENSORS
#include "sensors/Sensors.h"
#endif

// ─── Global Objects ───────────────────────────────────────────────────────────
Pet   pet;
SemaphoreHandle_t petMutex;

// Current screen state (written by Core 1, only Screen enum fits in uint8_t)
volatile Screen currentScreen = Screen::PET;

// Game selection menu state
volatile bool  showGameMenu   = false;
volatile uint8_t gameMenuSel  = 0;

// Stats screen toggle
volatile bool showStatsScreen = false;

// ─── FreeRTOS Task Declarations ───────────────────────────────────────────────
void petAITask(void* param);
void renderTask(void* param);
void inputTask(void* param);
void audioTask(void* param);
void wifiTask(void* param);
void timeTask(void* param);

#ifdef ENABLE_WEATHER
void weatherTask(void* param);
#endif

#ifdef ENABLE_SENSORS
void sensorTask(void* param);
#endif

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n╔══════════════════════╗");
    Serial.println("║    ESPet Starting    ║");
    Serial.println("╚══════════════════════╝");

    // Create mutex
    petMutex = xSemaphoreCreateMutex();
    configASSERT(petMutex);

    // Initialize subsystems
    pet.init();
    Input.init();
    Audio.init();
    Display.init();
    TimeMan.init();

    Serial.printf("[Pet] Name: %s | Stage: %s | Personality: %s\n",
        pet.state.name,
        Evolution::stageInfo(pet.state.stage).name,
        getTraits(pet.state.personality).name);
    Serial.printf("[Pet] Stats: H=%d A=%d E=%d HP=%d\n",
        pet.state.hunger, pet.state.happiness,
        pet.state.energy, pet.state.health);

    // ── Core 0 Tasks ──────────────────────────────────────────────────────
    xTaskCreatePinnedToCore(petAITask,  "PetAI",  TASK_AI_STACK,
                            nullptr, TASK_AI_PRIORITY,  nullptr, TASK_AI_CORE);

    xTaskCreatePinnedToCore(wifiTask,   "WiFi",   TASK_WIFI_STACK,
                            nullptr, TASK_WIFI_PRIORITY, nullptr, TASK_WIFI_CORE);

    xTaskCreatePinnedToCore(timeTask,   "Time",   TASK_TIME_STACK,
                            nullptr, TASK_TIME_PRIORITY, nullptr, TASK_TIME_CORE);

#ifdef ENABLE_WEATHER
    xTaskCreatePinnedToCore(weatherTask, "Weather", TASK_WEATHER_STACK,
                            nullptr, TASK_WEATHER_PRIORITY, nullptr, TASK_WEATHER_CORE);
#endif

#ifdef ENABLE_SENSORS
    Sensors.init();
    xTaskCreatePinnedToCore(sensorTask, "Sensor", TASK_SENSOR_STACK,
                            nullptr, TASK_SENSOR_PRIORITY, nullptr, TASK_SENSOR_CORE);
#endif

    // ── Core 1 Tasks ──────────────────────────────────────────────────────
    xTaskCreatePinnedToCore(renderTask, "Render", TASK_RENDER_STACK,
                            nullptr, TASK_RENDER_PRIORITY, nullptr, TASK_RENDER_CORE);

    xTaskCreatePinnedToCore(inputTask,  "Input",  TASK_INPUT_STACK,
                            nullptr, TASK_INPUT_PRIORITY,  nullptr, TASK_INPUT_CORE);

    xTaskCreatePinnedToCore(audioTask,  "Audio",  TASK_AUDIO_STACK,
                            nullptr, TASK_AUDIO_PRIORITY,  nullptr, TASK_AUDIO_CORE);

    Serial.println("[Setup] All tasks started.");

    // Startup melody
    Audio.play(MelodyID::BUTTON_TICK);
}

void loop() {
    // Print debug stats every SERIAL_STATS_INTERVAL_S
    static uint32_t lastStats = 0;
    uint32_t now = millis();
    if (now - lastStats >= SERIAL_STATS_INTERVAL_S * 1000UL) {
        lastStats = now;
        if (xSemaphoreTake(petMutex, 10 / portTICK_PERIOD_MS)) {
            Serial.printf("[Stats] H=%d A=%d E=%d HP=%d Mood=%d Stage=%d Age=%lus FPS=%.1f\n",
                pet.state.hunger, pet.state.happiness,
                pet.state.energy, pet.state.health,
                (int)pet.state.mood,
                (int)pet.state.stage,
                pet.state.ageSeconds,
                Display.fps);
            // Stack watermarks
            Serial.printf("[Stack] Free heap: %d\n", esp_get_free_heap_size());
            xSemaphoreGive(petMutex);
        }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 0: petAITask – Pet simulation, behavior tree, particle update
// ═══════════════════════════════════════════════════════════════════════════════
void petAITask(void* param) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        uint32_t nowMs = millis();

        if (xSemaphoreTake(petMutex, 5 / portTICK_PERIOD_MS)) {
            pet.tick(nowMs);
            xSemaphoreGive(petMutex);
        }

        // Determine screen
        if (xSemaphoreTake(petMutex, 5 / portTICK_PERIOD_MS)) {
            if (pet.state.isEvolutionAnim) {
                currentScreen = Screen::EVOLUTION;
            } else if (pet.isDead()) {
                currentScreen = Screen::DEAD;
            } else if (showStatsScreen) {
                currentScreen = Screen::STATS;
            } else if (Games.active) {
                currentScreen = Screen::GAME;
            } else {
                currentScreen = Screen::PET;
            }
            xSemaphoreGive(petMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, AI_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 1: renderTask – 30fps rendering
// ═══════════════════════════════════════════════════════════════════════════════
void renderTask(void* param) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t period = FRAME_US / (1000 * portTICK_PERIOD_MS);

    for (;;) {
        // Game screen is handled by gameTask/inputTask while game runs
        if (currentScreen == Screen::GAME) {
            vTaskDelay(33 / portTICK_PERIOD_MS);
            continue;
        }

        // Show game menu overlay
        if (showGameMenu) {
            Display.u8g2.clearBuffer();
            Display.u8g2.setFont(u8g2_font_6x10_tr);
            Display.u8g2.drawStr(20, 14, "CHOOSE GAME");
            Display.u8g2.drawHLine(0, 16, 128);

            const char* games[] = {"1. Catch Game", "2. Runner Game", "3. Cancel"};
            for (int i = 0; i < 3; i++) {
                if (gameMenuSel == i) {
                    Display.u8g2.drawBox(0, 18 + i * 14, 128, 13);
                    Display.u8g2.setDrawColor(0);
                }
                Display.u8g2.drawStr(6, 28 + i * 14, games[i]);
                Display.u8g2.setDrawColor(1);
            }
            Display.u8g2.sendBuffer();
            vTaskDelay(33 / portTICK_PERIOD_MS);
            continue;
        }

        if (xSemaphoreTake(petMutex, 5 / portTICK_PERIOD_MS)) {
            Display.renderFrame(pet, (Screen)currentScreen);
            xSemaphoreGive(petMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, period);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 1: inputTask – Button handling and action dispatching
// ═══════════════════════════════════════════════════════════════════════════════
void inputTask(void* param) {
    static uint32_t lastPoll = 0;

    for (;;) {
        uint32_t nowMs = millis();

        // Poll buttons at ~100Hz
        if (nowMs - lastPoll >= 10) {
            lastPoll = nowMs;
            Input.poll(nowMs);
        }

        ButtonMsg msg;
        if (Input.receive(msg, 5 / portTICK_PERIOD_MS)) {

            // ── Game menu navigation ───────────────────────────────────────
            if (showGameMenu) {
                if (msg.btn == ButtonID::FEED && msg.evt == ButtonEvent::PRESS) {
                    gameMenuSel = (gameMenuSel + 2) % 3;  // up
                }
                if (msg.btn == ButtonID::PLAY && msg.evt == ButtonEvent::PRESS) {
                    gameMenuSel = (gameMenuSel + 1) % 3;  // down
                }
                if (msg.btn == ButtonID::SLEEP && msg.evt == ButtonEvent::PRESS) {
                    // Confirm selection
                    showGameMenu = false;
                    if (gameMenuSel == 0) {
                        // Launch Catch Game (blocks renderTask via mutex)
                        Games.active = true;
                        currentScreen = Screen::GAME;
                        if (xSemaphoreTake(petMutex, 100 / portTICK_PERIOD_MS)) {
                            Games.runGame(GameID::CATCH, pet);
                            xSemaphoreGive(petMutex);
                        }
                        Games.active = false;
                    } else if (gameMenuSel == 1) {
                        Games.active = true;
                        currentScreen = Screen::GAME;
                        if (xSemaphoreTake(petMutex, 100 / portTICK_PERIOD_MS)) {
                            Games.runGame(GameID::RUNNER, pet);
                            xSemaphoreGive(petMutex);
                        }
                        Games.active = false;
                    }
                }
                continue;
            }

            // ── Stats screen toggle ────────────────────────────────────────
            if (msg.evt == ButtonEvent::LONG_PRESS && msg.btn == ButtonID::SLEEP) {
                showStatsScreen = !showStatsScreen;
                Audio.play(MelodyID::BUTTON_TICK);
                continue;
            }

            // ── Dead state: any press restarts ────────────────────────────
            if (pet.isDead()) {
                if (msg.evt == ButtonEvent::PRESS) {
                    if (xSemaphoreTake(petMutex, 20 / portTICK_PERIOD_MS)) {
                        pet.resetToDefaults();
                        pet.save();
                        xSemaphoreGive(petMutex);
                        Audio.play(MelodyID::LEVEL_UP);
                    }
                }
                continue;
            }

            // ── Normal gameplay ────────────────────────────────────────────
            if (msg.evt == ButtonEvent::PRESS) {
                switch (msg.btn) {
                case ButtonID::FEED:
                    if (xSemaphoreTake(petMutex, 20 / portTICK_PERIOD_MS)) {
                        pet.feed();
                        Audio.play(MelodyID::FEED);
                        xSemaphoreGive(petMutex);
                    }
                    break;

                case ButtonID::PLAY:
                    // Show game selection menu
                    gameMenuSel  = 0;
                    showGameMenu = true;
                    Audio.play(MelodyID::BUTTON_TICK);
                    break;

                case ButtonID::SLEEP:
                    if (xSemaphoreTake(petMutex, 20 / portTICK_PERIOD_MS)) {
                        if (pet.state.mood == PetMood::SLEEPING) {
                            pet.wake();
                        } else {
                            pet.forceSleep();
                            Audio.play(MelodyID::SLEEP);
                        }
                        xSemaphoreGive(petMutex);
                    }
                    break;
                }
            }

            // PLAY short press (not in menu): open game selection
            // PLAY long press: direct play (no game menu)
            if (msg.evt == ButtonEvent::LONG_PRESS && msg.btn == ButtonID::PLAY) {
                showGameMenu = false;
                if (xSemaphoreTake(petMutex, 20 / portTICK_PERIOD_MS)) {
                    pet.play();
                    Audio.play(MelodyID::PLAY);
                    xSemaphoreGive(petMutex);
                }
            }
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 1: audioTask – Non-blocking audio playback
// ═══════════════════════════════════════════════════════════════════════════════
void audioTask(void* param) {
    Audio.runTask();  // blocking loop inside
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 0: wifiTask – WiFi init, AP + optional station mode
// ═══════════════════════════════════════════════════════════════════════════════
void wifiTask(void* param) {
    WebSrv.init(&pet);

    // Try to connect to configured WiFi
    bool connected = WebSrv.connectWiFi();

    // Always start AP mode (even alongside STA)
    WebSrv.startAP();

    if (connected) {
        // Sync NTP time
        TimeMan.sync();
        Serial.println("[Time] NTP synced.");
    }

    // The web server is async – no task loop needed
    // Just keep this task alive for periodic WiFi checks
    for (;;) {
        // Check if STA disconnected; if so, reconnect
        if (WebSrv.ssid[0] != '\0' && WiFi.status() != WL_CONNECTED) {
            WebSrv.connectWiFi(5000);
        }
        vTaskDelay(30000 / portTICK_PERIOD_MS);  // check every 30s
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 0: timeTask – NTP sync, day/night update
// ═══════════════════════════════════════════════════════════════════════════════
void timeTask(void* param) {
    for (;;) {
        TimeInfo t = TimeMan.getTime();
        if (xSemaphoreTake(petMutex, 10 / portTICK_PERIOD_MS)) {
            pet.state.isNight = t.isNight;
            xSemaphoreGive(petMutex);
        }

        // Re-sync NTP every hour
        static uint32_t lastSync = 0;
        if (millis() - lastSync >= NTP_INTERVAL_S * 1000UL) {
            lastSync = millis();
            TimeMan.sync();
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS);  // update every 10s
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 0: weatherTask (optional)
// ═══════════════════════════════════════════════════════════════════════════════
#ifdef ENABLE_WEATHER
void weatherTask(void* param) {
    // Wait for WiFi
    vTaskDelay(8000 / portTICK_PERIOD_MS);

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            Weather.setLocation(WebSrv.latitude, WebSrv.longitude);
            bool ok = Weather.fetch();
            if (ok) {
                if (xSemaphoreTake(petMutex, 10 / portTICK_PERIOD_MS)) {
                    pet.state.ambientTemp  = Weather.current.temperature;
                    pet.state.weatherCode  = Weather.current.weatherCode;
                    xSemaphoreGive(petMutex);
                }
                Serial.printf("[Weather] %d°C, code %d (%s)\n",
                    (int)Weather.current.temperature,
                    Weather.current.weatherCode,
                    WeatherClient::weatherDescription(Weather.current.weatherCode));
            }
        }
        vTaskDelay(WEATHER_INTERVAL_S * 1000UL / portTICK_PERIOD_MS);
    }
}
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// CORE 0: sensorTask (optional)
// ═══════════════════════════════════════════════════════════════════════════════
#ifdef ENABLE_SENSORS
void sensorTask(void* param) {
    for (;;) {
        bool ok = Sensors.read();
        if (ok) {
            if (xSemaphoreTake(petMutex, 10 / portTICK_PERIOD_MS)) {
                pet.state.ambientTemp = (int8_t)Sensors.latest.temperature;
                // Humidity effects
                if (Sensors.latest.humidity < 30.0f) {
                    pet.state.happiness = max(0, (int)pet.state.happiness - 1);
                }
                xSemaphoreGive(petMutex);
            }
            Serial.printf("[Sensor] %.1f°C %.1f%%\n",
                Sensors.latest.temperature, Sensors.latest.humidity);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
#endif

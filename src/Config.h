#pragma once
#include <stdint.h>

// ── Pin Definitions ──────────────────────────────────────────────────────────
#define PIN_SDA          21
#define PIN_SCL          22
#define PIN_BTN_FEED      0   // BOOT button (active LOW)
#define PIN_BTN_PLAY     35
#define PIN_BTN_SLEEP    34
#define PIN_BUZZER       25
#define PIN_DHT          4

// ── Display ───────────────────────────────────────────────────────────────────
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT   64

// ── Timing ────────────────────────────────────────────────────────────────────
#define TARGET_FPS       30
#define FRAME_US         (1000000 / TARGET_FPS)   // 33333 µs
#define AI_INTERVAL_MS  100
#define SAVE_INTERVAL_S  60
#define WEATHER_INTERVAL_S (15 * 60)
#define NTP_INTERVAL_S   (60 * 60)

// ── Pet Stats ─────────────────────────────────────────────────────────────────
#define STAT_MAX         100
#define STAT_MIN           0

// Stat decay periods (seconds per -1 point)
#define HUNGER_DECAY_S   30
#define HAPPY_DECAY_S    60
#define ENERGY_DECAY_S   45

// Action effects
#define FEED_HUNGER      +30
#define FEED_HEALTH      +5
#define PLAY_HAPPINESS   +25
#define PLAY_ENERGY      -10
#define SLEEP_ENERGY     +50

// Thresholds for mood triggers
#define HUNGER_SAD_THRESH     30
#define HAPPY_SAD_THRESH      20
#define ENERGY_SLEEP_THRESH   15
#define HEALTH_SICK_THRESH    30
#define HUNGER_CRIT_THRESH    20

// ── Particles ─────────────────────────────────────────────────────────────────
#define MAX_PARTICLES    32

// ── Evolution ────────────────────────────────────────────────────────────────
#define STAGE_BABY_HOURS    24
#define STAGE_CHILD_HOURS   72
#define STAGE_ADULT_HOURS  168
#define CARESCORE_CHILD_MIN  50
#define CARESCORE_ADULT_MIN  70

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_AP_PREFIX   "ESPet-"
#define WIFI_AP_IP       "192.168.4.1"
#define WIFI_CONNECT_TIMEOUT_MS 10000

// ── NVS Namespace ─────────────────────────────────────────────────────────────
#define NVS_NAMESPACE    "espet"

// ── Audio ────────────────────────────────────────────────────────────────────
#define BUZZER_CHANNEL   0
#define BUZZER_RESOLUTION 8    // bits
#define BUZZER_DUTY      128   // 50% duty cycle

// ── Pet Name ──────────────────────────────────────────────────────────────────
#define PET_NAME_MAX_LEN 12
#define PET_NAME_DEFAULT "Pixel"

// ── Sprite Sizes per Stage ────────────────────────────────────────────────────
#define SPRITE_BABY_W    16
#define SPRITE_BABY_H    16
#define SPRITE_CHILD_W   20
#define SPRITE_CHILD_H   20
#define SPRITE_ADULT_W   24
#define SPRITE_ADULT_H   24

// ── FreeRTOS Task Config ──────────────────────────────────────────────────────
#define TASK_AI_STACK     4096
#define TASK_RENDER_STACK 6144
#define TASK_INPUT_STACK  2048
#define TASK_AUDIO_STACK  2048
#define TASK_WIFI_STACK   8192
#define TASK_TIME_STACK   3072
#define TASK_WEATHER_STACK 6144
#define TASK_SENSOR_STACK 2048

#define TASK_AI_PRIORITY      3
#define TASK_RENDER_PRIORITY  4
#define TASK_INPUT_PRIORITY   5
#define TASK_AUDIO_PRIORITY   2
#define TASK_WIFI_PRIORITY    1
#define TASK_TIME_PRIORITY    1
#define TASK_WEATHER_PRIORITY 1
#define TASK_SENSOR_PRIORITY  1

#define TASK_AI_CORE      0
#define TASK_RENDER_CORE  1
#define TASK_INPUT_CORE   1
#define TASK_AUDIO_CORE   1
#define TASK_WIFI_CORE    0
#define TASK_TIME_CORE    0
#define TASK_WEATHER_CORE 0
#define TASK_SENSOR_CORE  0

// ── Debug ─────────────────────────────────────────────────────────────────────
#define SERIAL_STATS_INTERVAL_S 5

#include "Audio.h"
#include <pgmspace.h>

AudioManager Audio;

// ─── Melody Definitions ──────────────────────────────────────────────────────
// Notes: C4=262, D4=294, E4=330, F4=349, G4=392, A4=440, B4=494, C5=523

static const Note PROGMEM mel_feed[] = {
    {523, 80}, {659, 80}, {784, 120}, {0, 40}, {1046,160},
};
static const Note PROGMEM mel_play[] = {
    {262, 60}, {330, 60}, {392, 60}, {523, 60}, {659, 60}, {784, 120},
};
static const Note PROGMEM mel_sleep[] = {
    {523, 150}, {494, 150}, {440, 200}, {392, 300},
};
static const Note PROGMEM mel_sad[] = {
    {330, 200}, {294, 200}, {262, 400},
};
static const Note PROGMEM mel_dead[] = {
    {262, 200}, {247, 200}, {233, 200}, {220, 400}, {0, 100}, {196, 600},
};
static const Note PROGMEM mel_levelup[] = {
    {523, 80}, {659, 80}, {784, 80}, {1047, 80},
    {1318, 80}, {1568, 160}, {0, 40}, {1568, 80},
};
static const Note PROGMEM mel_gameover[] = {
    {440, 200}, {415, 200}, {392, 200}, {370, 200},
    {0, 100}, {330, 400},
};
static const Note PROGMEM mel_evolution[] = {
    {262, 80}, {330, 80}, {392, 80}, {523, 80}, {659, 80},
    {784, 80}, {1047, 160}, {0, 80},
    {1047, 80}, {1318, 80}, {1568, 200},
};
static const Note PROGMEM mel_tick[] = {
    {880, 30},
};

struct MelodyInfo {
    const Note* notes;
    uint8_t     len;
};

static const MelodyInfo MELODIES[] = {
    { nullptr,        0 },  // NONE
    { mel_feed,       5 },
    { mel_play,       6 },
    { mel_sleep,      4 },
    { mel_sad,        3 },
    { mel_dead,       6 },
    { mel_levelup,    8 },
    { mel_gameover,   6 },
    { mel_evolution, 11 },
    { mel_tick,       1 },
};

// ─── Implementation ───────────────────────────────────────────────────────────

void AudioManager::init() {
    queue = xQueueCreate(4, sizeof(MelodyID));
    configASSERT(queue);
    _stopRequested = false;

    ledcSetup(BUZZER_CHANNEL, 440, BUZZER_RESOLUTION);
    ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0);  // silent
}

void AudioManager::play(MelodyID id) {
    _stopRequested = true;          // stop current melody
    xQueueSend(queue, &id, 0);      // enqueue new one
}

void AudioManager::stop() {
    _stopRequested = true;
    ledcWrite(BUZZER_CHANNEL, 0);
}

void AudioManager::runTask() {
    MelodyID id;
    while (true) {
        if (xQueueReceive(queue, &id, portMAX_DELAY) == pdTRUE) {
            uint8_t idx = (uint8_t)id;
            if (idx == 0 || idx >= sizeof(MELODIES) / sizeof(MELODIES[0])) continue;
            const MelodyInfo& m = MELODIES[idx];
            if (m.notes == nullptr || m.len == 0) continue;
            _stopRequested = false;
            playMelody(m.notes, m.len);
        }
    }
}

void AudioManager::playMelody(const Note* notes, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (_stopRequested) break;
        Note n;
        memcpy_P(&n, &notes[i], sizeof(Note));
        playNote(n.freq, n.durMs);
    }
    ledcWrite(BUZZER_CHANNEL, 0);
}

void AudioManager::playNote(uint16_t freq, uint16_t durMs) {
    if (freq == 0) {
        ledcWrite(BUZZER_CHANNEL, 0);
    } else {
        ledcWriteTone(BUZZER_CHANNEL, freq);
        ledcWrite(BUZZER_CHANNEL, BUZZER_DUTY);
    }

    uint32_t start = millis();
    while ((millis() - start) < durMs) {
        if (_stopRequested) return;
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

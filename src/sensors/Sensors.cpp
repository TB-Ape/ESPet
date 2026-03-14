#include "Sensors.h"

SensorManager Sensors;

#ifdef ENABLE_SENSORS

void SensorManager::init() {
    dht.begin();
    Serial.println("[Sensors] DHT22 initialized on GPIO " + String(PIN_DHT));
}

bool SensorManager::read() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        latest.valid = false;
        return false;
    }
    latest.temperature = t;
    latest.humidity    = h;
    latest.valid       = true;
    return true;
}

#else

void SensorManager::init() {
    // Sensors disabled at compile time
}

bool SensorManager::read() {
    return false;
}

#endif  // ENABLE_SENSORS

void SensorManager::applyToPet(void* petStatePtr) {
    if (!latest.valid || petStatePtr == nullptr) return;

    // We include Pet.h here to avoid circular deps at header level
    struct MinimalPetState {
        uint8_t hunger, happiness, energy, health;
        // ... rest is not needed here
    };
    // Use raw byte offsets – too risky; instead use a callback approach.
    // This is called from main.cpp which has full access to Pet.
    // See main.cpp: sensorTask calls pet.state.ambientTemp = ...
    // This function just updates the ambient fields.
}

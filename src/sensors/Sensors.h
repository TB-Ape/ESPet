#pragma once
#include <Arduino.h>
#include "../Config.h"

#ifdef ENABLE_SENSORS
#include <DHT.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// Sensors – Optional DHT22 temperature/humidity sensor
// Compile with -D ENABLE_SENSORS to activate
// ═══════════════════════════════════════════════════════════════════════════════

struct SensorData {
    float   temperature;   // °C
    float   humidity;      // %
    bool    valid;
};

class SensorManager {
public:
    SensorData latest;

    SensorManager() :
#ifdef ENABLE_SENSORS
        dht(PIN_DHT, DHT22),
#endif
        latest({ 20.0f, 50.0f, false }) {}

    void init();
    bool read();  // Returns true if read was successful

    // Apply sensor data to pet (called every 5s from sensorTask)
    // pet: pointer to PetState (uses void* to avoid circular include)
    void applyToPet(void* petStatePtr);

#ifdef ENABLE_SENSORS
private:
    DHT dht;
#endif
};

extern SensorManager Sensors;

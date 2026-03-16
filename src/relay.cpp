#include "relay.h"
#include "config.h"
#include <Arduino.h>

// Time-proportional window control
static unsigned long windowStartTime = 0;

void relayInit() {
    pinMode(SSR_PIN, OUTPUT);
    digitalWrite(SSR_PIN, LOW);
    
    windowStartTime = millis();
}

void updateRelay(MachineState &state) {
    double powerPercent = state.heater_power;
    // Clamp to valid range
    if (powerPercent < 0.0) powerPercent = 0.0;
    if (powerPercent > 100.0) powerPercent = 100.0;
    

    // Reset window if needed
    if (millis() - windowStartTime >= state.relay_window_size) {
        windowStartTime += state.relay_window_size;
    }
    
    unsigned long onTime = (unsigned long)((powerPercent / 100.0) * state.relay_window_size);
    unsigned long windowPosition = millis() - windowStartTime;
    
    if (onTime > windowPosition) {
        digitalWrite(SSR_PIN, HIGH);
    } else {
        digitalWrite(SSR_PIN, LOW);
    }
};
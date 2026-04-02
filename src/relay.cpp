#include "relay.h"
#include "config.h"
#include <Arduino.h>

// Time-proportional window control
static unsigned long windowStartTime = 0;

void relayInit() {
    pinMode(SSR_PIN, OUTPUT);
    digitalWrite(SSR_PIN, LOW);

    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, HIGH); // Assuming active LOW for pump
    pinMode(SOLENOID_PIN, OUTPUT);
    digitalWrite(SOLENOID_PIN, HIGH); // Assuming active LOW for solenoid
    
    windowStartTime = millis();
}

void updatePumpSolenoid(MachineState &state) {
     if (state.pump_active) {
        digitalWrite(PUMP_PIN, LOW); // Active LOW
    } else {
        digitalWrite(PUMP_PIN, HIGH);
    }

    if (state.solenoid_active) {
        digitalWrite(SOLENOID_PIN, LOW); // Active LOW
    } else {
        digitalWrite(SOLENOID_PIN, HIGH);
    }
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

    updatePumpSolenoid(state);
};
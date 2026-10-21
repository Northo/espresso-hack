#include <Arduino.h>
#include <Adafruit_MAX31855.h>
#include <esp_log.h>
#include "state.h"
#include "controller.h"
#include "cleaning.h"
#include "relay.h"
#include "thermocouple.h"
#include "config.h"
#if HAS_DISPLAY
#include "display.h"
#endif
#include "network/wifi.h"
#include "network/ota.h"
#if HAS_DISPLAY_LINK
#include "network/display_link.h"
#endif
#include "button_interface.h"

MachineState machine_state;

#if HAS_DISPLAY
constexpr int LCD_ON_BATTERY_PIN = 15;
#endif

void setup() {
#if HAS_DISPLAY
    // Turn on display when using external power.
    pinMode(LCD_ON_BATTERY_PIN, OUTPUT);
    digitalWrite(LCD_ON_BATTERY_PIN, HIGH);
#endif

    Serial.begin(115200);
    ESP_LOGI("Main", "Starting up...");
#if HAS_DISPLAY
    initDisplay();
#endif
    initThermocouple();
    relayInit();
    setupController(machine_state);
    
    setupWiFi();
    setupOTA(machine_state);
#if HAS_DISPLAY_LINK
    initDisplayLink(machine_state);
#endif
    setupButtons(machine_state);  // Must happen after controller setup
}

void loop() {
    static unsigned long last_log_time = 0;
    if (millis() - last_log_time >= 1000) {
        last_log_time = millis();
        ESP_LOGI("Main", "Current Temp: %.2f °C, Target Temp: %.2f °C, Heater Power: %.1f%%", machine_state.current_temperature, machine_state.target_temperature, machine_state.heater_power);
    }
    updateThermocoupleReading(machine_state);
    updateController(machine_state);
    updateCleaning(machine_state);
    updateRelay(machine_state);
    handleButtons(machine_state);

#if HAS_DISPLAY
    updateDisplay(machine_state);
#endif
#if HAS_DISPLAY_LINK
    handleDisplayLink(machine_state);
#endif
    handleOTA();
}

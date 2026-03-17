#include <Arduino.h>
#include <Adafruit_MAX31855.h>
#include <esp_log.h>
#include "state.h"
#include "controller.h"
#include "relay.h"
#include "thermocouple.h"
#include "display.h"

MachineState machine_state;

void setup() {
    Serial.begin(115200);
    ESP_LOGI("Main", "Starting up...");
    initDisplay();
    initThermocouple();
    relayInit();
    setupController(machine_state);
}

void loop() {
    static unsigned long last_log_time = 0;
    if (millis() - last_log_time >= 1000) {
        last_log_time = millis();
        ESP_LOGI("Main", "Current Temp: %.2f °C, Target Temp: %.2f °C, Heater Power: %.1f%%", machine_state.current_temperature, machine_state.target_temperature, machine_state.heater_power);
    }
    updateThermocoupleReading(machine_state);
    updateController(machine_state);
    updateRelay(machine_state);
    updateDisplay(machine_state);
}

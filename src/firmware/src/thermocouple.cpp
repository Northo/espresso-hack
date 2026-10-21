#include "thermocouple.h"
#include "config.h"

#include "protocol.h"

#include <Adafruit_MAX31855.h>
#include <esp_log.h>

Adafruit_MAX31855 thermocouple(THERMO_CLK_PIN, THERMO_CS_PIN, THERMO_SO_PIN);

static long last_read_time = 0;
static bool ema_initialized = false;

void initThermocouple() {
    thermocouple.begin();
}

void updateThermocoupleReading(MachineState &state) {
    if (millis() - last_read_time < THERMOCUPLE_READ_INTERVAL) {
        return; // Not time to read yet
    }

    last_read_time = millis();

    double tempC = thermocouple.readCelsius();
    uint8_t fault = 0;
    if (isnan(tempC)) {
        // The MAX31855 reports faults in the low 3 bits of the frame; readError() clocks out a fresh one.
        fault = thermocouple.readError() & 0x07;
        if (fault == 0) fault = proto::THERMO_FAULT_UNKNOWN;
    } else if (tempC == 0.0) {
        // An all-zero frame (SO stuck low, wrong pins, no power) has no fault bits set and decodes as exactly
        // 0.0 °C. A working chip never reports a 0.0 internal (cold-junction) temperature on top of that.
        double internal = thermocouple.readInternal();
        if (!isnan(internal) && internal == 0.0) fault = proto::THERMO_FAULT_UNKNOWN;
    }

    if (fault) {
        // Don't feed the bad sample to the controller: current_temperature keeps its last good value and the
        // controller holds its output (temperature_valid == false).
        state.temperature_valid = false;
        state.thermo_fault = fault;
        state.thermo_fault_latched |= fault;
        state.thermo_fault_count++;
        ESP_LOGW("Thermo", "bad sample, fault 0x%02x (count %lu)", fault, (unsigned long)state.thermo_fault_count);

        if (!state.safety_shutdown && millis() - state.last_valid_temperature_ms >= THERMO_FAULT_TIMEOUT_MS) {
            state.safety_shutdown = true;
            state.current_temperature = 0.0; // don't show a stale value
            ESP_LOGE("Thermo", "persistent sensor fault: SAFETY SHUTDOWN, heater off until reboot");
        }
        return;
    }

    state.thermo_fault = 0;
    state.temperature_valid = true;
    state.last_valid_temperature_ms = millis();
    if (state.safety_shutdown) return; // latched; readings recovering don't re-enable the heater

    if (!ema_initialized) {
        state.current_temperature = tempC;
        ema_initialized = true;
    } else {
        float alpha = state.temperature_ema_alpha;
        state.current_temperature = alpha * tempC + (1 - alpha) * state.current_temperature;
    }
}

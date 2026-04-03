#include "thermocouple.h"
#include "config.h"

#include <Adafruit_MAX31855.h>

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
    if (isnan(tempC)) {
        // TODO: handle error
        // Set a flag in state or something
        state.current_temperature = 0.0;
        return;
    }

    if (!ema_initialized) {
        state.current_temperature = tempC;
        ema_initialized = true;
    } else {
        float alpha = state.temperature_ema_alpha;
        state.current_temperature = alpha * tempC + (1 - alpha) * state.current_temperature;
    }
}
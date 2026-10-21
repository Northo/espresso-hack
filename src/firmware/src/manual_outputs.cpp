#include "manual_outputs.h"
#include "config.h"
#include <Arduino.h>
#include <esp_log.h>

static const char *TAG = "Manual";

static bool outputsOwnedByProgram(const MachineState &state) {
    return state.auto_brew_enabled || state.clean_state != CleanState::IDLE;
}

static void switchOutput(MachineState &state, bool &output, bool on) {
    if (on && !state.pump_active && !state.solenoid_active) state.manual_outputs_since = millis();
    output = on;
}

bool setManualPump(MachineState &state, bool on) {
    if (outputsOwnedByProgram(state)) return false;
    ESP_LOGI(TAG, "pump %s", on ? "on" : "off");
    switchOutput(state, state.pump_active, on);
    return true;
}

bool setManualValve(MachineState &state, bool on) {
    if (outputsOwnedByProgram(state)) return false;
    ESP_LOGI(TAG, "valve %s", on ? "open" : "closed");
    switchOutput(state, state.solenoid_active, on);
    return true;
}

void updateManualOutputs(MachineState &state) {
    if (outputsOwnedByProgram(state)) return;
    if (!state.pump_active && !state.solenoid_active) return;
    if (millis() - state.manual_outputs_since >= (unsigned long)MANUAL_OUTPUT_TIMEOUT_MS) {
        ESP_LOGW(TAG, "timeout, closing pump and valve");
        state.pump_active = false;
        state.solenoid_active = false;
    }
}

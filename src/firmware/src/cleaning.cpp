#include "cleaning.h"
#include "config.h"
#include "protocol.h"
#include <Arduino.h>
#include <esp_log.h>

static const char *TAG = "Cleaning";

static void pumpOff(MachineState &state) {
    state.pump_active = false;
    state.solenoid_active = false;
}

static void beginPumpPhase(MachineState &state) {
    state.clean_pump_phase = true;
    state.clean_phase_start = millis();
}

bool startCleaning(MachineState &state) {
    if (state.auto_brew_enabled || state.clean_state != CleanState::IDLE) return false;
    ESP_LOGI(TAG, "Starting: detergent cycle 1/%d", proto::CLEAN_CYCLES);
    state.clean_state = CleanState::DETERGENT;
    state.clean_cycle = 1;
    beginPumpPhase(state);
    return true;
}

void continueCleaning(MachineState &state) {
    if (state.clean_state != CleanState::WAIT_REMOVE) return;
    ESP_LOGI(TAG, "Continuing: water cycle 1/%d", proto::CLEAN_CYCLES);
    state.clean_state = CleanState::WATER;
    state.clean_cycle = 1;
    beginPumpPhase(state);
}

void cancelCleaning(MachineState &state) {
    if (state.clean_state != CleanState::IDLE) ESP_LOGI(TAG, "Cancelled/closed");
    state.clean_state = CleanState::IDLE;
    state.clean_cycle = 0;
    state.clean_pump_phase = false;
    pumpOff(state);
}

void updateCleaning(MachineState &state) {
    if (state.clean_state == CleanState::IDLE) return;

    // Anything else driving the pump (e.g. the physical brew button) wins: abort cleaning.
    if (state.auto_brew_enabled) {
        cancelCleaning(state);
        return;
    }

    if (state.clean_state != CleanState::DETERGENT && state.clean_state != CleanState::WATER) {
        pumpOff(state); // waiting / done
        return;
    }

    unsigned long elapsed = millis() - state.clean_phase_start;

    if (state.clean_pump_phase) {
        state.pump_active = true;
        state.solenoid_active = true;
        if (elapsed >= (unsigned long)CLEAN_PUMP_ON_MS) {
            state.clean_pump_phase = false;
            state.clean_phase_start = millis();
            pumpOff(state);
        }
        return;
    }

    pumpOff(state);
    if (elapsed < (unsigned long)CLEAN_PUMP_OFF_MS) return;

    if (state.clean_cycle < proto::CLEAN_CYCLES) {
        state.clean_cycle++;
        ESP_LOGI(TAG, "Cycle %d/%d", state.clean_cycle, proto::CLEAN_CYCLES);
        beginPumpPhase(state);
    } else if (state.clean_state == CleanState::DETERGENT) {
        ESP_LOGI(TAG, "Detergent cycles done, waiting for continue");
        state.clean_state = CleanState::WAIT_REMOVE;
    } else {
        ESP_LOGI(TAG, "Cleaning done");
        state.clean_state = CleanState::DONE;
    }
}

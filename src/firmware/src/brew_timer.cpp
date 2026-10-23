#include "brew_timer.h"
#include <Arduino.h>
#include <math.h>
#include <esp_log.h>

void startBrew(MachineState &state) {
    if (state.auto_brew_enabled || state.safety_shutdown || state.clean_state != CleanState::IDLE) return;
    const uint32_t now = millis();
    state.auto_brew_start_time = now;
    state.brew_record.start(now, state.journal_setup_revision,
        state.auto_brew_preinfusion_duration, state.auto_brew_bloom_duration,
        state.auto_brew_duration, (int16_t)lround(state.target_temperature * 10));
    state.auto_brew_enabled = true;
    state.brew_hold = false;
}

void stopBrew(MachineState &state) {
    if (state.auto_brew_enabled) {
        const uint32_t now = millis();
        state.brew_record.finish(now, state.temperature_valid
            ? (int16_t)lround(state.current_temperature * 10) : INT16_MIN);
        if (state.brew_record.phase == BrewRecord::FINISHED) {
            state.last_brew_ms = state.brew_record.duration_ms;
            state.last_brew_end_ms = now;
            ++state.brew_count;
            ESP_LOGI("Brew", "shot #%lu main brew: %.3f s", (unsigned long)state.brew_count,
                     state.last_brew_ms / 1000.0);
        }
    }
    state.auto_brew_enabled = false;
    state.brew_hold = false;
    state.pump_active = false;
    state.solenoid_active = false;
}

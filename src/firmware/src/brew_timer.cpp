#include "brew_timer.h"
#include <Arduino.h>
#include <esp_log.h>

void updateBrewTimer(MachineState &state) {
    static bool was_brewing = false;

    if (was_brewing && !state.auto_brew_enabled) {
        state.last_brew_end_ms = millis();
        state.last_brew_ms = state.last_brew_end_ms - state.auto_brew_start_time;
        state.brew_count++;
        // One line per shot, easy to grep from the serial log until shots are stored properly.
        ESP_LOGI("Brew", "shot #%lu finished: %.1f s (setpoint %.1f C, temp %.1f C)", (unsigned long)state.brew_count,
                 state.last_brew_ms / 1000.0, state.target_temperature, state.current_temperature);
    }
    was_brewing = state.auto_brew_enabled;
}

#include "button_interface.h"
#include "config.h"
#include <Button.h>

Button steamButton(STEAM_BUTTON_PIN);
Button feedForwardButton(FEED_FORWARD_BUTTON_PIN);
Button autoBrewButton(AUTO_BREW_BUTTON_PIN);

void setupButtons(MachineState& state) {
    steamButton.begin();
    if (steamButton.read() == Button::PRESSED) {
        state.mode = ControlMode::BANG_BANG;
        state.target_temperature = STEAM_TARGET_TEMPERATURE;
    }

    feedForwardButton.begin();
    autoBrewButton.begin();
};

void handleButtons(MachineState& state) {
    if (steamButton.pressed()) {
        state.mode = ControlMode::BANG_BANG;
        state.target_temperature = STEAM_TARGET_TEMPERATURE;
    } else if (steamButton.released()) {
        state.mode = ControlMode::PID;
        state.target_temperature = DEFAULT_TARGET_TEMPERATURE;
    }

    if (feedForwardButton.released()) {
        state.do_feed_forward = true;
        state.feed_forward_start_time = millis();
    }

    if (autoBrewButton.released()) {
        if (!state.auto_brew_enabled) {
            state.auto_brew_enabled = true;
            state.auto_brew_start_time = millis();
        } else {
            state.auto_brew_enabled = false;
            // TODO: controller should handle setting pump and solenoid when enabled goes to false.
            state.pump_active = false;
            state.solenoid_active = false;
        }
    }
}


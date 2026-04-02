#include "button_interface.h"
#include "config.h"
#include <Button.h>

Button steamButton(STEAM_BUTTON_PIN);
Button feedForwardButton(FEED_FORWARD_BUTTON_PIN);

void setupButtons(MachineState& state) {
    steamButton.begin();
    if (steamButton.read() == Button::PRESSED) {
        state.mode = ControlMode::BANG_BANG;
        state.target_temperature = STEAM_TARGET_TEMPERATURE;
    }

    feedForwardButton.begin();
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
}


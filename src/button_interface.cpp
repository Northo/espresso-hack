#include "button_interface.h"
#include "config.h"
#include <Button.h>

Button steamButton(STEAM_BUTTON_PIN);

void setupButtons(MachineState& state) {
    steamButton.begin();
    if (steamButton.read() == Button::PRESSED) {
        state.mode = ControlMode::BANG_BANG;
        state.target_temperature = STEAM_TARGET_TEMPERATURE;
    }
};

void handleButtons(MachineState& state) {
    if (steamButton.pressed()) {
        state.mode = ControlMode::BANG_BANG;
        state.target_temperature = STEAM_TARGET_TEMPERATURE;
    } else if (steamButton.released()) {
        state.mode = ControlMode::PID;
        state.target_temperature = DEFAULT_TARGET_TEMPERATURE;
    }
}


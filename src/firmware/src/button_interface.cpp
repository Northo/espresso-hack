#include "button_interface.h"
#include "config.h"
#include <Button.h>

Button steamButton(STEAM_BUTTON_PIN);

void setupButtons(MachineState& state) {
    steamButton.begin();
    if (STEAM_BUTTON_ENABLED && steamButton.read() == Button::PRESSED) {
        Serial.println("Starting in steam mode");
        enterSteamMode(state);
    }
};

void handleButtons(MachineState& state) {
    if (STEAM_BUTTON_ENABLED) {
        if (steamButton.pressed()) {
            enterSteamMode(state);
        } else if (steamButton.released()) {
            exitSteamMode(state);
        }
    }
}


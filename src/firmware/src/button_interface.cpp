#include "button_interface.h"
#include "config.h"

#if STEAM_BUTTON_ENABLED
#include <Button.h>

Button steamButton(STEAM_BUTTON_PIN);
#endif

void setupButtons(MachineState& state) {
#if STEAM_BUTTON_ENABLED
    steamButton.begin();
    if (steamButton.read() == Button::PRESSED) {
        Serial.println("Starting in steam mode");
        enterSteamMode(state);
    }
#endif
}

void handleButtons(MachineState& state) {
#if STEAM_BUTTON_ENABLED
    if (steamButton.pressed()) {
        enterSteamMode(state);
    } else if (steamButton.released()) {
        exitSteamMode(state);
    }
#endif
}

#pragma once

#include "state.h"

// ESP-NOW link to the T-RGB display (protocol: src/shared/protocol.h).
// Call initDisplayLink() after WiFi is up: ESP-NOW runs on the WiFi channel,
// so the display must be on the same channel as your router.
void initDisplayLink(MachineState &machine_state);

// Applies received commands and periodically broadcasts machine state.
void handleDisplayLink(MachineState &machine_state);

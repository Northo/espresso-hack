#pragma once

#include "state.h"

// Records the duration of each finished brew into state (last_brew_ms, last_brew_end_ms, brew_count).
// Detects the end by watching auto_brew_enabled, so it covers every way a brew can finish: running out
// its timeline, STOP from the display, or a failsafe.
void updateBrewTimer(MachineState &state);

#pragma once

#include "state.h"

// Manual control of the pump and solenoid valve, independent of each other.
// Any input source (display link, later physical buttons) should go through these.
// Both return false (and do nothing) while a brew or cleaning sequence owns the outputs.
bool setManualPump(MachineState &state, bool on);
bool setManualValve(MachineState &state, bool on);

// Failsafe: closes manually opened outputs after MANUAL_OUTPUT_TIMEOUT_MS.
// Call every loop iteration.
void updateManualOutputs(MachineState &state);

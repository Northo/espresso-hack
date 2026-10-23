#pragma once
#include "state.h"

// All brew entry/exit paths must use these (including future physical buttons).
// No networking, allocation, or blocking work in the machine control loop.
void startBrew(MachineState &state);
void stopBrew(MachineState &state);

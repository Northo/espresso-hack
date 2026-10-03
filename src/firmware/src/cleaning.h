#pragma once

#include "state.h"

// Backflush-style cleaning: CLEAN_CYCLES cycles with detergent, a pause to remove
// the detergent, then CLEAN_CYCLES cycles with water. One cycle is pump + solenoid on
// (pressurise against the blind basket), then both off (the 3-way valve vents back to the drain).

bool startCleaning(MachineState &state);    // false if brewing or already cleaning
void continueCleaning(MachineState &state); // from WAIT_REMOVE to the water cycles
void cancelCleaning(MachineState &state);   // abort, or acknowledge DONE

// Drives pump/solenoid while cleaning. Call every loop iteration before updateRelay().
void updateCleaning(MachineState &state);

#pragma once
#include <stdint.h>
#include "config.h"

enum class ControlMode {
    PID,
    MANUAL_MODE,
    BANG_BANG,
};

inline const char* controlModeToString(ControlMode mode) {
    switch (mode) {
        case ControlMode::PID:         return "PID";
        case ControlMode::MANUAL_MODE: return "MANUAL";
        case ControlMode::BANG_BANG:   return "BANG_BANG";
        default:                       return "UNKNOWN";
    }
}

// Values match proto::CleanState (src/shared/protocol.h).
enum class CleanState : uint8_t { IDLE = 0, DETERGENT = 1, WAIT_REMOVE = 2, WATER = 3, DONE = 4 };

struct MachineState {
    ControlMode mode = ControlMode::PID;

    double target_temperature = DEFAULT_TARGET_TEMPERATURE;
    double normal_setpoint = DEFAULT_TARGET_TEMPERATURE; // brew setpoint, restored when leaving steam
    double current_temperature = 0.0;
    double kp = DEFAULT_KP;
    double ki = DEFAULT_KI;
    double kd = DEFAULT_KD;
    int pid_sample_time = 250; // ms
    double pid_power = 0.0;

    // Feed forward
    bool do_feed_forward = false;
    long feed_forward_start_time = 0;
    long feed_forward_duration = DEFAULT_FEED_FORWARD_DURATION;
    double feed_forward_power = DEFAULT_FEED_FORWARD_POWER;
    double feed_forward_factor = DEFAULT_FEED_FORWARD_FACTOR;

    // Auto brew mode
    bool auto_brew_enabled = false;
    bool brew_hold = false; // keep brewing past auto_brew_duration (until stopped or hold released)
    long auto_brew_start_time = 0;
    bool pump_active = false;
    bool solenoid_active = false;
    long auto_brew_preinfusion_duration = DEFAULT_AUTO_BREW_PREINFUSION_DURATION;
    long auto_brew_duration = DEFAULT_AUTO_BREW_DURATION;
    long auto_brew_bloom_duration = DEFAULT_AUTO_BREW_BLOOM_DURATION;

    // Manual pump/valve control (see manual_outputs.cpp)
    unsigned long manual_outputs_since = 0; // millis() when an output was last manually switched on

    // Last finished brew (see brew_timer.cpp)
    unsigned long last_brew_ms = 0;     // total duration, preinfusion included; 0 = none since boot
    unsigned long last_brew_end_ms = 0; // millis() when it ended
    uint32_t brew_count = 0;            // brews finished since boot

    // Cleaning sequence (see cleaning.cpp)
    CleanState clean_state = CleanState::IDLE;
    uint8_t clean_cycle = 0;          // 1-based
    bool clean_pump_phase = false;    // true: pump on, false: off/venting
    unsigned long clean_phase_start = 0;

    // Relay parameters
    int relay_window_size = 1000; // ms for time-proportional control

    // Temperature input
    uint8_t thermo_fault = 0;         // current MAX31855 fault bits (proto::ThermoFault)
    uint8_t thermo_fault_latched = 0; // OR of all faults since cleared
    uint32_t thermo_fault_count = 0;  // failed reads since boot
    bool temperature_valid = false;   // the latest sample was good; false -> control loop must not use current_temperature
    unsigned long last_valid_temperature_ms = 0; // millis() of the last good sample (boot time until the first one)
    bool safety_shutdown = false;     // latched: persistent sensor fault, heater forced off until reboot
    float temperature_ema_alpha = DEFAULT_TEMPERATURE_EMA_ALPHA;

    // Manual mode
    double manual_power = 0.0;

    // Bang Bang mode
    double bang_bang_power = 100.0;

    // Actual output to SSR, updated by controller
    double heater_power = 0.0;
};

// Steam mode switching, shared by the physical steam switch and the display's steam toggle.
// The brew setpoint is remembered on entry and restored on exit.
inline void enterSteamMode(MachineState &state) {
    if (state.mode == ControlMode::BANG_BANG) return; // already in steam: don't overwrite the saved setpoint
    state.normal_setpoint = state.target_temperature;
    state.mode = ControlMode::BANG_BANG;
    state.target_temperature = STEAM_TARGET_TEMPERATURE;
}

inline void exitSteamMode(MachineState &state) {
    if (state.mode != ControlMode::BANG_BANG) return;
    state.mode = ControlMode::PID;
    state.target_temperature = state.normal_setpoint;
}

#pragma once
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

struct MachineState {
    ControlMode mode = ControlMode::PID;

    double target_temperature = DEFAULT_TARGET_TEMPERATURE;
    double current_temperature = 0.0;
    double kp = DEFAULT_KP;
    double ki = DEFAULT_KI;
    double kd = DEFAULT_KD;
    int pid_sample_time = 250; // ms

    // Relay parameters
    int relay_window_size = 1000; // ms for time-proportional control

    // Temperature input
    float temperature_ema_alpha = 0.2;

    // Manual mode
    double manual_power = 0.0;

    // Bang Bang mode
    double bang_bang_power = 100.0;

    // Actual output to SSR, updated by controller
    double heater_power = 0.0;
};
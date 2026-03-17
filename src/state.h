#pragma once
#include "config.h"

enum class ControlMode {
    PID,
    MANUAL_MODE,
};

inline const char* controlModeToString(ControlMode mode) {
    switch (mode) {
        case ControlMode::PID:         return "PID";
        case ControlMode::MANUAL_MODE: return "MANUAL";
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

    int relay_window_size = 1000; // ms for time-proportional control

    float temperature_ema_alpha = 0.2;

    double manual_power = 0.0;

    double heater_power = 0.0;
};
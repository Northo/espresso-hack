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
    double pid_power = 0.0;

    // Feed forward
    bool do_feed_forward = false;
    long feed_forward_start_time = 0;
    long feed_forward_duration = DEFAULT_FEED_FORWARD_DURATION;
    double feed_forward_power = DEFAULT_FEED_FORWARD_POWER;

    // Auto brew mode
    bool auto_brew_enabled = false;
    long auto_brew_start_time = 0;
    bool pump_active = false;
    bool solenoid_active = false;
    long auto_brew_preinfusion_duration = DEFAULT_AUTO_BREW_PREINFUSION_DURATION;
    long auto_brew_duration = DEFAULT_AUTO_BREW_DURATION;
    long auto_brew_bloom_duration = DEFAULT_AUTO_BREW_BLOOM_DURATION;

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
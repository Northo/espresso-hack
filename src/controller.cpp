#include "controller.h"
#include <PID_v1.h>

extern MachineState machine_state;

// At this point, state may not be initialized, so scary to take the tuning parameters. However, we expect them to be set explicitly later.
static PID pid(&machine_state.current_temperature, &machine_state.heater_power, &machine_state.target_temperature, machine_state.kp, machine_state.ki, machine_state.kd, DIRECT, P_ON_M);

void setupController(MachineState &state) {
    pid.SetTunings(state.kp, state.ki, state.kd);
    pid.SetMode(state.mode == ControlMode::PID ? AUTOMATIC : MANUAL);
    pid.SetOutputLimits(0, 100);
    pid.SetSampleTime(state.pid_sample_time);
}

void updateController(MachineState &state) {
    if (state.mode == ControlMode::PID) {
        pid.SetMode(AUTOMATIC); // triggers bumpless transfer
        pid.Compute();
    } else if (state.mode == ControlMode::MANUAL_MODE) {
        pid.SetMode(MANUAL);
        state.heater_power = state.manual_power;
    } else if (state.mode == ControlMode::BANG_BANG) {
        pid.SetMode(MANUAL);
        state.heater_power = (state.current_temperature < state.target_temperature) ? state.bang_bang_power : 0.0;
    } else {
        // Panic! Nothing else implemented
        pid.SetMode(MANUAL);
        state.heater_power = 0;
    }
}
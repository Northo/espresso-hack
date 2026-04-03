#include "controller.h"
#include <PID_v1.h>
#include <Arduino.h>

extern MachineState machine_state;

// At this point, state may not be initialized, so scary to take the tuning parameters. However, we expect them to be set explicitly later.
static PID pid(&machine_state.current_temperature, &machine_state.pid_power, &machine_state.target_temperature, machine_state.kp, machine_state.ki, machine_state.kd, DIRECT, P_ON_M);

void setupController(MachineState &state)
{
    pid.SetTunings(state.kp, state.ki, state.kd);
    pid.SetMode(state.mode == ControlMode::PID ? AUTOMATIC : MANUAL);
    pid.SetOutputLimits(0, 100);
    pid.SetSampleTime(state.pid_sample_time);
}

enum class BrewStage {
    NONE,
    PREINFUSION,
    BLOOM,
    BREWING,
    DONE
};

static BrewStage brew_stage = BrewStage::NONE;

void updateController(MachineState &state)
{
    if (state.mode == ControlMode::PID)
    {
        pid.SetMode(AUTOMATIC); // triggers bumpless transfer
        pid.Compute();
        state.heater_power = state.pid_power;
    }
    else if (state.mode == ControlMode::MANUAL_MODE)
    {
        pid.SetMode(MANUAL);
        state.heater_power = state.manual_power;
    }
    else if (state.mode == ControlMode::BANG_BANG)
    {
        pid.SetMode(MANUAL);
        state.heater_power = (state.current_temperature < state.target_temperature) ? state.bang_bang_power : 0.0;
    }
    else
    {
        // Panic! Nothing else implemented
        pid.SetMode(MANUAL);
        state.heater_power = 0;
        return;
    }

    if (state.do_feed_forward)
    {
        if (millis() - state.feed_forward_start_time < state.feed_forward_duration)
        {
            state.heater_power = state.heater_power + state.feed_forward_power;
            if (state.heater_power > 100)
                state.heater_power = 100;
        }
        else
        {
            state.do_feed_forward = false;
        }
    }

    if (state.auto_brew_enabled)
    {
        auto elapsed = millis() - state.auto_brew_start_time;

        if (elapsed < state.auto_brew_preinfusion_duration)
        {
            brew_stage = BrewStage::PREINFUSION;
            state.pump_active = true;
            state.solenoid_active = true;
        }
        else if (elapsed < state.auto_brew_preinfusion_duration + state.auto_brew_bloom_duration)
        {
            brew_stage = BrewStage::BLOOM;
            state.pump_active = false;
            state.solenoid_active = true;
        }
        else if (elapsed < state.auto_brew_preinfusion_duration + state.auto_brew_bloom_duration + state.auto_brew_duration)
        {
            state.pump_active = true;
            state.solenoid_active = true;
            // Activate feed forward when brew starts
            // TODO: give more control over power and duration
            if (brew_stage != BrewStage::BREWING) {
                // Transitioning into brew stage, start feed forward if not already active
                if (!state.do_feed_forward) {
                    state.do_feed_forward = true;
                    state.feed_forward_start_time = millis();
                }
            }
            brew_stage = BrewStage::BREWING;

        }
        else
        {
            brew_stage = BrewStage::DONE;
            state.auto_brew_enabled = false;
            state.pump_active = false;
            state.solenoid_active = false;
        }
    }
}
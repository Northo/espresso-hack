#include "controller.h"
#include "brew_timer.h"
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

enum class BrewStage
{
    NONE,
    PREINFUSION,
    BLOOM,
    BREWING,
    DONE
};

// Computes state.heater_power from the current mode. Only called with a valid temperature sample.
static void computeHeaterPower(MachineState &state)
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
            state.heater_power = state.heater_power + state.feed_forward_power * state.feed_forward_factor;
            if (state.heater_power > 100)
                state.heater_power = 100;
        }
        else
        {
            state.do_feed_forward = false;
        }
    }
}

void updateController(MachineState &state)
{
    if (state.safety_shutdown)
    {
        // Persistent sensor fault: heater off, whatever the mode. Latched until reboot.
        pid.SetMode(MANUAL);
        state.pid_power = 0;
        state.heater_power = 0;
        state.do_feed_forward = false;
        state.manual_power = 0;
        if (state.auto_brew_enabled) stopBrew(state);
        return;
    }

    // A bad/missing sample (e.g. SPI jitter) must not reach the controller: a 0 would read as "very cold" and
    // crank up the power. Hold the current output instead; the safety timeout in thermocouple.cpp takes over if
    // the fault persists. Before the first good sample the output stays at its initial 0.
    if (state.temperature_valid)
        computeHeaterPower(state);

    if (!state.auto_brew_enabled) {
        state.brew_hold = false;
    }

    if (state.auto_brew_enabled)
    {
        const uint32_t now = millis();
        const uint32_t elapsed = now - state.brew_record.started_ms;
        const auto &record = state.brew_record;
        BrewStage brew_stage;

        if (elapsed < record.preinfusion_ms)
            brew_stage = BrewStage::PREINFUSION;
        else if (elapsed < record.preinfusion_ms + record.bloom_ms)
            brew_stage = BrewStage::BLOOM;
        else if (elapsed < record.preinfusion_ms + record.bloom_ms + record.configured_brew_ms)
            brew_stage = BrewStage::BREWING;
        else if (state.brew_hold && elapsed < record.preinfusion_ms + record.bloom_ms + record.configured_brew_ms + MAX_BREW_HOLD_EXTRA)
            brew_stage = BrewStage::BREWING; // hold: keep going until stopped (or the failsafe limit)
        else
            brew_stage = BrewStage::DONE;

        switch (brew_stage)
        {
        case BrewStage::PREINFUSION:
            state.pump_active = true;
            state.solenoid_active = true;
            break;
        case BrewStage::BLOOM:
            state.pump_active = false;
            state.solenoid_active = true;
            break;
        case BrewStage::BREWING:
            state.pump_active = true;
            state.solenoid_active = true;

            // If we transitioned into the brewing stage and feed forward isn't already active, start it
            if (!state.brew_record.main_started && !state.do_feed_forward)
            {
                state.do_feed_forward = true;
                state.feed_forward_start_time = millis();
            }
            state.brew_record.enterMain(now);

            break;
        case BrewStage::DONE:
        default:  // Something went wrong, fail safe by turning everything off
            stopBrew(state);
            break;
        }
    }
}
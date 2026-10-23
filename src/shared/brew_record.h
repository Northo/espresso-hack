#pragma once
#include <stdint.h>

// Controller-owned snapshot. No Arduino/network dependencies, so timing can be
// tested on the host. All arithmetic is unsigned and survives millis() wrap.
struct BrewRecord {
    uint32_t sequence = 0;
    uint32_t setup_revision = 0;
    uint32_t started_ms = 0;
    uint32_t main_started_ms = 0;
    uint32_t duration_ms = 0;
    uint32_t preinfusion_ms = 0;
    uint32_t bloom_ms = 0;
    uint32_t configured_brew_ms = 0;
    int16_t setpoint_x10 = 0;
    int16_t end_temperature_x10 = INT16_MIN; // unknown, not a fabricated zero
    bool main_started = false;
    enum Phase : uint8_t { NONE, RUNNING, FINISHED, ABORTED };
    Phase phase = NONE;

    void start(uint32_t now, uint32_t revision, uint32_t pre, uint32_t bloom,
               uint32_t brew, int16_t setpoint) {
        ++sequence;
        setup_revision = revision;
        started_ms = now;
        preinfusion_ms = pre;
        bloom_ms = bloom;
        configured_brew_ms = brew;
        setpoint_x10 = setpoint;
        main_started = false;
        duration_ms = 0;
        end_temperature_x10 = INT16_MIN;
        phase = RUNNING;
    }
    void enterMain(uint32_t now) {
        if (!main_started) {
            main_started = true;
            main_started_ms = now;
        }
    }
    void finish(uint32_t now, int16_t temperature) {
        if (phase != RUNNING) return;
        duration_ms = main_started ? uint32_t(now - main_started_ms) : 0;
        end_temperature_x10 = temperature;
        phase = main_started ? FINISHED : ABORTED;
    }
};

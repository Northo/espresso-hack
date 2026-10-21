#pragma once
// Wire protocol between the controller and this display.
// Identical payloads are used over ESP-NOW and serial (serial adds framing, see transport_serial.cpp).
// Little-endian, packed. Shared by src/ui (display) and src/firmware (controller) via -I../shared.
#include <stddef.h>
#include <stdint.h>

namespace proto {

enum MsgType : uint8_t {
    MSG_STATE = 1,   // controller -> display
    MSG_COMMAND = 2, // display -> controller
};

enum StateFlags : uint8_t {
    FLAG_BREWING = 1 << 0,
    FLAG_STEAM = 1 << 1,
    FLAG_HOLD = 1 << 2, // brew hold: keep brewing past the brew duration
    FLAG_CLEAN_PUMP = 1 << 3, // cleaning: pump currently on
    FLAG_PUMP = 1 << 4,       // pump output is on (any source)
    FLAG_VALVE = 1 << 5,      // solenoid valve is open (any source)
};

enum BrewStage : uint8_t {
    STAGE_NONE = 0, // not brewing
    STAGE_PREINFUSION = 1,
    STAGE_BLOOM = 2,
    STAGE_BREWING = 3,
};

// Cleaning sequence: CLEAN_CYCLES cycles with detergent, a pause to remove the
// detergent, then CLEAN_CYCLES cycles with water.
constexpr uint8_t CLEAN_CYCLES = 5;

enum CleanState : uint8_t {
    CLEAN_IDLE = 0,
    CLEAN_DETERGENT = 1,   // running detergent cycles
    CLEAN_WAIT_REMOVE = 2, // waiting for the user to remove detergent and press continue
    CLEAN_WATER = 3,       // running water cycles
    CLEAN_DONE = 4,        // finished, waiting for acknowledgement (CMD_CLEAN_CANCEL)
};

struct __attribute__((packed)) StateMsg {
    uint8_t type;             // MSG_STATE
    int16_t temperature_x10;  // current temperature, 0.1 °C
    int16_t setpoint_x10;     // target temperature, 0.1 °C
    uint16_t brew_duration_s; // configured duration of the main brewing stage
    uint8_t flags;            // StateFlags
    uint16_t brew_elapsed_s;  // seconds since brew start (0 when not brewing)
    uint8_t brew_stage;       // BrewStage
    uint8_t clean_state;      // CleanState
    uint8_t clean_cycle;      // current cycle, 1..CLEAN_CYCLES (0 when idle)
    uint8_t preinfusion_s;    // configured duration of the preinfusion stage (brew_elapsed_s counts from its start)
    uint8_t bloom_s;          // configured duration of the bloom stage
};

enum CommandId : uint8_t {
    CMD_BREW_START = 1,        // starts a brew; ignored if one is running
    CMD_STEAM_TOGGLE = 2,
    CMD_SET_SETPOINT = 3,      // value: setpoint in 0.1 °C
    CMD_SET_BREW_DURATION = 4, // value: seconds
    CMD_BREW_STOP = 5,         // stops a running brew
    CMD_BREW_HOLD_TOGGLE = 6,  // toggles hold while brewing
    CMD_CLEAN_START = 7,       // start the cleaning sequence (ignored if brewing/cleaning)
    CMD_CLEAN_CANCEL = 8,      // abort cleaning, or acknowledge CLEAN_DONE
    CMD_CLEAN_CONTINUE = 9,    // proceed from CLEAN_WAIT_REMOVE to the water cycles
    CMD_SET_PREINFUSION_DURATION = 10, // value: seconds (0 = skip the stage)
    CMD_SET_BLOOM_DURATION = 11,       // value: seconds (0 = skip the stage)
    CMD_PUMP_SET = 12,  // value: 1 = on, 0 = off. Manual; ignored while brewing/cleaning, auto-off after a timeout
    CMD_VALVE_SET = 13, // value: 1 = open, 0 = closed. Same rules as CMD_PUMP_SET
};

struct __attribute__((packed)) CommandMsg {
    uint8_t type; // MSG_COMMAND
    uint8_t cmd;  // CommandId
    int16_t value;
};

constexpr size_t MAX_PAYLOAD = 16;

} // namespace proto

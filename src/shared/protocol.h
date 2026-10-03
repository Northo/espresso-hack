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
};

enum BrewStage : uint8_t {
    STAGE_NONE = 0, // not brewing
    STAGE_PREINFUSION = 1,
    STAGE_BLOOM = 2,
    STAGE_BREWING = 3,
};

struct __attribute__((packed)) StateMsg {
    uint8_t type;             // MSG_STATE
    int16_t temperature_x10;  // current temperature, 0.1 °C
    int16_t setpoint_x10;     // target temperature, 0.1 °C
    uint16_t brew_duration_s; // configured duration of the main brewing stage
    uint8_t flags;            // StateFlags
    uint16_t brew_elapsed_s;  // seconds since brew start (0 when not brewing)
    uint8_t brew_stage;       // BrewStage
};

enum CommandId : uint8_t {
    CMD_BREW_START = 1,        // starts a brew; ignored if one is running
    CMD_STEAM_TOGGLE = 2,
    CMD_SET_SETPOINT = 3,      // value: setpoint in 0.1 °C
    CMD_SET_BREW_DURATION = 4, // value: seconds
    CMD_BREW_STOP = 5,         // stops a running brew
    CMD_BREW_HOLD_TOGGLE = 6,  // toggles hold while brewing
};

struct __attribute__((packed)) CommandMsg {
    uint8_t type; // MSG_COMMAND
    uint8_t cmd;  // CommandId
    int16_t value;
};

constexpr size_t MAX_PAYLOAD = 16;

} // namespace proto

#pragma once
// Wire protocol between the controller and this display.
// Identical payloads are used over ESP-NOW and serial (serial adds framing, see transport_serial.cpp).
// Little-endian, packed. Keep in sync with the controller firmware.
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
};

struct __attribute__((packed)) StateMsg {
    uint8_t type;             // MSG_STATE
    int16_t temperature_x10;  // current temperature, 0.1 °C
    int16_t setpoint_x10;     // target temperature, 0.1 °C
    uint16_t brew_duration_s; // configured brew duration
    uint8_t flags;            // StateFlags
};

enum CommandId : uint8_t {
    CMD_BREW_START = 1,
    CMD_STEAM_TOGGLE = 2,
    CMD_SET_SETPOINT = 3,      // value: setpoint in 0.1 °C
    CMD_SET_BREW_DURATION = 4, // value: seconds
};

struct __attribute__((packed)) CommandMsg {
    uint8_t type; // MSG_COMMAND
    uint8_t cmd;  // CommandId
    int16_t value;
};

constexpr size_t MAX_PAYLOAD = 16;

} // namespace proto

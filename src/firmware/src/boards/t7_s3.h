#pragma once

// Pinout: LilyGO T7-S3 (ESP32-S3, no screen)
// TODO: fill in pins
constexpr int SSR_PIN = 21;
constexpr int THERMO_CLK_PIN = 46;
constexpr int THERMO_CS_PIN = 47;
constexpr int THERMO_SO_PIN = 48;
// The physical steam switch is disabled for now (pump/solenoid noise triggers it falsely);
// steam mode is toggled from the display instead. Set to 1 to enable (and define the pin).
#define STEAM_BUTTON_ENABLED 0
#if STEAM_BUTTON_ENABLED
constexpr int STEAM_BUTTON_PIN = -1;
#endif
constexpr int PUMP_PIN = 14;
constexpr int SOLENOID_PIN = 13;

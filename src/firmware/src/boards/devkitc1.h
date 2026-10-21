#pragma once

// Pinout: Espressif ESP32-S3-DevKitC-1 v1.1
constexpr int SSR_PIN = 19;
constexpr int THERMO_CLK_PIN = 11;
constexpr int THERMO_CS_PIN = 10;
constexpr int THERMO_SO_PIN = 12;
// The physical steam switch is disabled for now (pump/solenoid noise triggers it falsely);
// steam mode is toggled from the display instead. Set to 1 to enable (and define the pin).
#define STEAM_BUTTON_ENABLED 0
#if STEAM_BUTTON_ENABLED
constexpr int STEAM_BUTTON_PIN = 18;
#endif
constexpr int PUMP_PIN = 14;
constexpr int SOLENOID_PIN = 13;

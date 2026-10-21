#pragma once

// Pin assignments live in boards/<board>.h, selected by BOARD_* build flag in platformio.ini.
#include "boards/pins.h"

constexpr double DEFAULT_TARGET_TEMPERATURE = 93.0; // °C
constexpr double DEFAULT_KP = 1.7294;
constexpr double DEFAULT_KI = 0.014412;
constexpr double DEFAULT_KD = 0.0;



constexpr int THERMOCUPLE_READ_INTERVAL = 100; // ms
constexpr float DEFAULT_TEMPERATURE_EMA_ALPHA = 1.0;
// Bad samples are ignored (the controller holds its last output) for this long. If no valid reading arrives
// within this time, the safety shutdown trips: the heater is forced off until the controller is rebooted.
constexpr unsigned long THERMO_FAULT_TIMEOUT_MS = 1500;

constexpr const char *WIFI_HOSTNAME = "espresso-controller";
constexpr int OTA_PORT = 3232;

// STEAM_BUTTON_ENABLED and STEAM_BUTTON_PIN are set per board in boards/*.h.
constexpr double STEAM_TARGET_TEMPERATURE = 130.0; // °C

constexpr long DEFAULT_FEED_FORWARD_DURATION = 20000; // ms
constexpr double DEFAULT_FEED_FORWARD_POWER = 70.0;
constexpr double DEFAULT_FEED_FORWARD_FACTOR = 1.0;


constexpr long  DEFAULT_AUTO_BREW_DURATION = 30000; // ms
constexpr long DEFAULT_AUTO_BREW_PREINFUSION_DURATION = 2000; // ms
constexpr long DEFAULT_AUTO_BREW_BLOOM_DURATION = 5000; // ms

constexpr long MANUAL_OUTPUT_TIMEOUT_MS = 30000; // failsafe: manually opened pump/valve close again after this long

constexpr long MAX_BREW_HOLD_EXTRA = 60000; // ms, failsafe: hold can extend a brew by at most this much

constexpr long CLEAN_PUMP_ON_MS = 10000;  // cleaning cycle: pump + solenoid on (pressurise)
constexpr long CLEAN_PUMP_OFF_MS = 10000; // cleaning cycle: both off (group vents to drain)

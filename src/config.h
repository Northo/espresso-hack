#pragma once

constexpr double DEFAULT_TARGET_TEMPERATURE = 93.0; // °C
constexpr double DEFAULT_KP = 1.7294;
constexpr double DEFAULT_KI = 0.014412;
constexpr double DEFAULT_KD = 0.0;

constexpr int SSR_PIN = 43;

constexpr int THERMO_CLK_PIN = 12;
constexpr int THERMO_CS_PIN = 10;
constexpr int THERMO_SO_PIN = 13;

constexpr int THERMOCUPLE_READ_INTERVAL = 100; // ms

constexpr const char *WIFI_HOSTNAME = "espresso-controller";
constexpr int OTA_PORT = 3232;

constexpr int STEAM_BUTTON_PIN = 44;
constexpr double STEAM_TARGET_TEMPERATURE = 130.0; // °C

constexpr int FEED_FORWARD_BUTTON_PIN = 14;
constexpr long DEFAULT_FEED_FORWARD_DURATION = 20000; // ms
constexpr double DEFAULT_FEED_FORWARD_POWER = 50.0;

constexpr int PUMP_PIN = 21;
constexpr int SOLENOID_PIN = 16;

constexpr int AUTO_BREW_BUTTON_PIN = 0;
constexpr long  DEFAULT_AUTO_BREW_DURATION = 30000; // ms
constexpr long DEFAULT_AUTO_BREW_PREINFUSION_DURATION = 2000; // ms
constexpr long DEFAULT_AUTO_BREW_BLOOM_DURATION = 5000; // ms

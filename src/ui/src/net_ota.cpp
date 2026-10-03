#include "net_ota.h"
#include "secrets.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

static constexpr const char *HOSTNAME = "espresso-display";
static bool g_ota_started = false;

void ota_begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false); // modem sleep makes ESP-NOW reception unreliable
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    ArduinoOTA.setHostname(HOSTNAME);
}

void ota_poll()
{
    if (!g_ota_started && WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.begin();
        g_ota_started = true;
    }
    if (g_ota_started) ArduinoOTA.handle();
}

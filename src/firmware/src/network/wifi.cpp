#include "wifi.h"
#include "config.h"
#include "secrets.h"
#include <WiFi.h>
#include <esp_log.h>

void setupWiFi() {
    // Joining/rejoining the AP must not stop the machine's control loop.
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    ESP_LOGI("WIFI", "Connecting in background");
}

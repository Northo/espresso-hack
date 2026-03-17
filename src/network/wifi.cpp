#include "wifi.h"
#include "config.h"
#include "secrets.h"
#include <WiFi.h>
#include <esp_log.h>


void setupWiFi() {
    ESP_LOGI("WIFI", "Starting WiFi connection to SSID: %s", WIFI_SSID);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        ESP_LOGI("WIFI", "Waiting for WiFi connection...");
        delay(500);
    }
    WiFi.setSleep(false);
    ESP_LOGI("WIFI", "Connected to WiFi! IP address: %s", WiFi.localIP().toString().c_str());
}
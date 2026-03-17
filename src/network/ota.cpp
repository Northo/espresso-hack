#include "ota.h"
#include "config.h"
#include <ArduinoOTA.h>
#include <esp_log.h>

static unsigned long last_ota_time = 0;

void setupOTA() {
    ArduinoOTA.setHostname(WIFI_HOSTNAME);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
    });

    ArduinoOTA
    .onStart([]() {
      ESP_LOGI("OTA", "Start OTA update");
    })
    .onEnd([]() {
      ESP_LOGI("OTA", "End OTA update");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        ESP_LOGI("OTA", "Progress: %u%%", (progress / (total / 100)));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();
};

void handleOTA() {
    ArduinoOTA.handle();
}
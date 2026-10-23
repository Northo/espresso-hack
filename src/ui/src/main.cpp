#include <Arduino.h>
#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include "link/controller_link.h"
#include "journal_ui.h"
#ifndef LINK_NO_OTA
#include "net_ota.h"
#endif

// EEZ Studio generated UI lives in src/ui/ (EEZ build output, do not edit).
#if __has_include("ui/ui.h")
#include "ui/ui.h"
#define HAS_EEZ_UI 1
extern "C" void app_chart_tick(); // ui_bindings.cpp: temperature history chart sampling
#endif

LilyGo_RGBPanel panel;

void setup()
{
#ifndef LINK_USE_SERIAL // serial carries the link protocol in that mode; no debug text
    Serial.begin(115200);
#endif

    if (!panel.begin()) {
        while (1) {
#ifndef LINK_USE_SERIAL
            Serial.println("Error, failed to initialize T-RGB");
#endif
            delay(1000);
        }
    }

    controller_link_begin();  // ESP-NOW (or serial with -DLINK_USE_SERIAL)

#ifndef LINK_NO_OTA
    ota_begin(); // after ESP-NOW init; joins WiFi in the background
#endif

    beginLvglHelper(panel);   // LVGL display + touch drivers

#ifdef HAS_EEZ_UI
    ui_init();
    journal_ui_begin();
#endif

    panel.setBrightness(16);  // 0..16
}

void loop()
{
    controller_link_poll();
#ifndef LINK_NO_OTA
    ota_poll();
#endif
    lv_timer_handler();
#ifdef HAS_EEZ_UI
    ui_tick();
    app_chart_tick();
    journal_ui_tick();
#endif
    delay(2);
}

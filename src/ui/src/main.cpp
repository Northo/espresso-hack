#include <Arduino.h>
#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include "link/controller_link.h"

// EEZ Studio generated UI lives in src/ui/ (EEZ build output, do not edit).
#if __has_include("ui/ui.h")
#include "ui/ui.h"
#define HAS_EEZ_UI 1
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

    beginLvglHelper(panel);   // LVGL display + touch drivers

#ifdef HAS_EEZ_UI
    ui_init();
#endif

    panel.setBrightness(16);  // 0..16
}

void loop()
{
    controller_link_poll();
    lv_timer_handler();
#ifdef HAS_EEZ_UI
    ui_tick();
#endif
    delay(2);
}

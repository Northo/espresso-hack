#pragma once

// Joins the WiFi network (non-blocking) and serves ArduinoOTA once connected.
// Joining the router also puts ESP-NOW on the router's channel, the same one the controller is on.
void ota_begin();
void ota_poll(); // call from loop()

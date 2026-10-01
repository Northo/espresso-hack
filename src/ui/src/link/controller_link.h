#pragma once
#include "protocol.h"

// Link to the espresso controller (ESP-NOW by default, serial with -DLINK_USE_SERIAL).
bool controller_link_begin();
void controller_link_poll(); // call from loop()

void controller_link_send_command(proto::CommandId cmd, int16_t value = 0);

// Implemented by the app: called from controller_link_poll() when a state message arrives.
void app_on_controller_state(const proto::StateMsg &state);

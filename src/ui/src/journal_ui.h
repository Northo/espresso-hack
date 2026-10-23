#pragma once
#include "protocol.h"

void journal_ui_begin();
void journal_ui_tick();
void journal_on_state(const proto::StateMsg &state);
void journal_on_status(const proto::JournalStatusMsg &status);
void journal_on_shot(const proto::ShotMsg &shot);

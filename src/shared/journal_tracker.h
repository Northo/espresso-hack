#pragma once
#include "protocol.h"

// Ignore repeated/reordered ESP-NOW results, including delayed RUNNING packets
// after FINISHED. A new boot permits sequence numbers to start over.
struct JournalTracker {
    uint64_t boot_id = 0;
    uint32_t sequence = 0;
    uint8_t phase = 0;
    void status(const proto::JournalStatusMsg &s) {
        if (boot_id != s.boot_id) { boot_id = s.boot_id; sequence = s.sequence; phase = 0; }
        else if (s.sequence > sequence) { sequence = s.sequence; phase = 0; }
    }
    bool accept(const proto::ShotMsg &s) {
        if (s.boot_id != boot_id || s.sequence < sequence || s.phase < 1 || s.phase > 3) return false;
        if (s.sequence > sequence) { sequence = s.sequence; phase = 0; }
        if (phase >= 2 || s.phase <= phase) return false;
        phase = s.phase;
        return true;
    }
};

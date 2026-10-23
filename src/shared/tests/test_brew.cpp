#include "state.h"
#include "controller.h"
#include "brew_timer.h"
#include "journal_tracker.h"
#include <cassert>
#include <iostream>

MachineState machine_state;
static uint32_t clock_ms;
unsigned long millis() { return clock_ms; }
static void at(MachineState &s, uint32_t time) { clock_ms = time; updateController(s); }
static MachineState fresh(uint32_t now = 100) {
    clock_ms = now;
    MachineState s;
    s.journal_setup_revision = 12;
    s.temperature_valid = true;
    s.current_temperature = 92.3;
    startBrew(s);
    updateController(s);
    return s;
}

int main() {
    // Normal sequence: the reported 30 s is main-only, not 37 s.
    auto s = fresh();
    assert(s.pump_active && s.solenoid_active);
    at(s, 2100); assert(!s.pump_active && s.solenoid_active);
    at(s, 7100); assert(s.brew_record.main_started && s.pump_active);
    s.journal_setup_revision = 99;
    s.auto_brew_preinfusion_duration = 20000; // applies only to the next shot
    at(s, 37100);
    assert(!s.auto_brew_enabled && !s.pump_active && !s.solenoid_active);
    assert(s.last_brew_ms == 30000 && s.brew_count == 1);
    assert(s.brew_record.setup_revision == 12 && s.brew_record.preinfusion_ms == 2000);
    assert(s.brew_record.setpoint_x10 == 930 && s.brew_record.end_temperature_x10 == 923);
    stopBrew(s); assert(s.brew_count == 1); // repeated STOP is harmless

    // Abort before main: no zero-duration phantom shot, even during bloom.
    s = fresh(); at(s, 1100); stopBrew(s);
    assert(s.brew_record.phase == BrewRecord::ABORTED && s.brew_count == 0);
    s = fresh(); at(s, 4000); stopBrew(s);
    assert(s.brew_record.phase == BrewRecord::ABORTED && s.last_brew_ms == 0);

    // Manual STOP counts the actual main stage, not its configured target.
    s = fresh(); at(s, 7100); clock_ms = 19345; stopBrew(s);
    assert(s.last_brew_ms == 12245 && s.brew_count == 1);

    // HOLD extension and failsafe are both part of the actual main time.
    s = fresh(); at(s, 7100); s.brew_hold = true; at(s, 42100);
    assert(s.auto_brew_enabled); s.brew_hold = false; at(s, 42101);
    assert(s.last_brew_ms == 35001);
    s = fresh(); at(s, 7100); s.brew_hold = true; at(s, 97100);
    assert(!s.auto_brew_enabled && s.last_brew_ms == 90000);

    // Actual entry, not subtraction of planned pre/bloom durations.
    s = fresh(); at(s, 7150); clock_ms = 12000; stopBrew(s);
    assert(s.last_brew_ms == 4850);

    // No preinfusion/bloom, and unsigned millis rollover.
    s = MachineState(); s.auto_brew_preinfusion_duration = s.auto_brew_bloom_duration = 0;
    clock_ms = UINT32_MAX - 1000; startBrew(s); updateController(s);
    clock_ms = 999; stopBrew(s); assert(s.last_brew_ms == 2000);

    s = MachineState(); s.auto_brew_preinfusion_duration = s.auto_brew_bloom_duration = 0;
    clock_ms = UINT32_MAX - 999; startBrew(s); updateController(s);
    clock_ms = 0; stopBrew(s); assert(s.last_brew_ms == 1000 && s.brew_count == 1);

    // Safety still wins, and invalid temperatures are not uploaded as zero.
    s = fresh(); at(s, 7100); s.safety_shutdown = true; s.temperature_valid = false;
    at(s, 8100); assert(!s.auto_brew_enabled && !s.pump_active && s.heater_power == 0);
    assert(s.last_brew_ms == 1000 && s.brew_record.end_temperature_x10 == INT16_MIN);
    auto seq = s.brew_record.sequence; startBrew(s); assert(s.brew_record.sequence == seq);
    s = MachineState(); s.clean_state = CleanState::DETERGENT; startBrew(s); assert(!s.auto_brew_enabled);

    // Repeated and reordered result packets cannot duplicate a shot or rewind UI.
    JournalTracker tracker;
    proto::JournalStatusMsg status = {proto::MSG_JOURNAL_STATUS, 123, 1, 12};
    tracker.status(status);
    proto::ShotMsg shot = {}; shot.boot_id = 123; shot.sequence = 1; shot.phase = 1;
    assert(tracker.accept(shot)); assert(!tracker.accept(shot));
    shot.phase = 2; assert(tracker.accept(shot)); assert(!tracker.accept(shot));
    shot.phase = 1; assert(!tracker.accept(shot));
    shot.sequence = 2; assert(tracker.accept(shot)); // missing status packet
    shot.sequence = 1; shot.phase = 2; assert(!tracker.accept(shot));
    status.sequence = 3; tracker.status(status);
    shot.sequence = 2; assert(!tracker.accept(shot));
    status.boot_id = 456; status.sequence = 0; tracker.status(status);
    assert(!tracker.accept(shot));
    shot.boot_id = 456; shot.sequence = 1; assert(tracker.accept(shot));
    assert(sizeof(proto::ShotMsg) == 30 && sizeof(proto::JournalStatusMsg) == 17);
    std::cout << "Controller timing, safety, snapshots and journal packet tests passed.\n";
}

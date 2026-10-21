// Implementations of the native variables (vars.h) and actions (actions.h)
// declared by the EEZ project, plus the model updated from the controller link.
#include "ui/vars.h"
#include "ui/actions.h"
#include "ui/ui.h"
#include "link/controller_link.h"
#include "ui/screens.h"
#include <Arduino.h>
#include <stdio.h>

static int32_t s_temperature, s_setpoint, s_brew_seconds, s_preinfusion_seconds, s_bloom_seconds;
// Latest raw controller values (0.1 °C) feeding the history chart.
static bool s_have_state, s_chart_ready;
static int16_t s_temperature_x10, s_setpoint_x10;
static uint32_t s_chart_last_ms;
static uint32_t s_setpoint_hold_until; // ignore controller setpoints until then (while dragging the knob)

static char s_temperature_buf[16] = "--", s_setpoint_buf[16] = "--", s_brew_seconds_buf[16] = "--",
                  s_preinfusion_seconds_buf[16] = "--", s_bloom_seconds_buf[16] = "--";
static const char *s_temperature_text = s_temperature_buf, *s_setpoint_text = s_setpoint_buf,
                  *s_brew_seconds_text = s_brew_seconds_buf,
                  *s_preinfusion_seconds_text = s_preinfusion_seconds_buf, *s_bloom_seconds_text = s_bloom_seconds_buf;

static bool s_steam_active, s_brewing, s_brew_hold, s_brew_idle = true;
static char s_brew_button_buf[8] = "BREW", s_brew_status_buf[32] = "";
static const char *s_brew_button_text = s_brew_button_buf, *s_brew_status_text = s_brew_status_buf;

// Manual pump/valve state, as reported by the controller (any source).
static bool s_pump_on, s_valve_on;
static char s_pump_button_buf[16] = "PUMP OFF", s_valve_button_buf[16] = "VALVE CLOSED";
static const char *s_pump_button_text = s_pump_button_buf, *s_valve_button_text = s_valve_button_buf;

// Cleaning screen state, driven by the controller.
static uint8_t s_clean_state = proto::CLEAN_IDLE;
static bool s_clean_running, s_clean_done;
static char s_clean_title_buf[32] = "", s_clean_detail_buf[96] = "", s_clean_primary_buf[16] = "START",
            s_clean_secondary_buf[16] = "BACK";
static const char *s_clean_title_text = s_clean_title_buf, *s_clean_detail_text = s_clean_detail_buf,
                  *s_clean_primary_text = s_clean_primary_buf, *s_clean_secondary_text = s_clean_secondary_buf;

// Thermocouple (MAX31855) fault readout, see proto::ThermoFault.
static char s_thermo_status_buf[160] = "No data";
static const char *s_thermo_status_text = s_thermo_status_buf;

static constexpr int32_t SETPOINT_MIN = 20, SETPOINT_MAX = 160; // matches the arc range
static constexpr int32_t BREW_MIN = 1, BREW_MAX = 120;
static constexpr int32_t STAGE_MIN = 0, STAGE_MAX = 30; // preinfusion / bloom, seconds (0 skips the stage)

static void refresh_texts()
{
    snprintf(s_temperature_buf, sizeof(s_temperature_buf), "%ld\xC2\xB0" "C", (long)s_temperature);
    snprintf(s_setpoint_buf, sizeof(s_setpoint_buf), "%ld\xC2\xB0" "C", (long)s_setpoint);
    snprintf(s_brew_seconds_buf, sizeof(s_brew_seconds_buf), "%lds", (long)s_brew_seconds);
    snprintf(s_preinfusion_seconds_buf, sizeof(s_preinfusion_seconds_buf), "%lds", (long)s_preinfusion_seconds);
    snprintf(s_bloom_seconds_buf, sizeof(s_bloom_seconds_buf), "%lds", (long)s_bloom_seconds);
}

static void update_clean_texts(const proto::StateMsg &st)
{
    s_clean_state = st.clean_state;
    s_clean_running = st.clean_state == proto::CLEAN_DETERGENT || st.clean_state == proto::CLEAN_WATER;
    s_clean_done = st.clean_state == proto::CLEAN_DONE;

    const char *title = "Clean machine", *detail = "", *primary = "START", *secondary = "BACK";
    char title_buf[32];
    switch (st.clean_state) {
    case proto::CLEAN_IDLE:
        detail = "Insert the blind basket with detergent, then press START.";
        break;
    case proto::CLEAN_DETERGENT:
    case proto::CLEAN_WATER:
        snprintf(title_buf, sizeof(title_buf), "%s %u/%u",
                 st.clean_state == proto::CLEAN_DETERGENT ? "Detergent" : "Water", (unsigned)st.clean_cycle,
                 (unsigned)proto::CLEAN_CYCLES);
        title = title_buf;
        detail = (st.flags & proto::FLAG_CLEAN_PUMP) ? "Pump on" : "Releasing pressure";
        secondary = "CANCEL";
        break;
    case proto::CLEAN_WAIT_REMOVE:
        title = "Detergent done";
        detail = "Remove the detergent and rinse the basket, then press CONTINUE.";
        primary = "CONTINUE";
        secondary = "CANCEL";
        break;
    case proto::CLEAN_DONE:
        title = "Cleaning done";
        detail = "Remove the blind basket.";
        primary = "DONE";
        break;
    }
    snprintf(s_clean_title_buf, sizeof(s_clean_title_buf), "%s", title);
    snprintf(s_clean_detail_buf, sizeof(s_clean_detail_buf), "%s", detail);
    snprintf(s_clean_primary_buf, sizeof(s_clean_primary_buf), "%s", primary);
    snprintf(s_clean_secondary_buf, sizeof(s_clean_secondary_buf), "%s", secondary);
}

// "OK" or a comma separated list of the fault names in `bits`.
static void format_thermo_faults(char *out, size_t n, uint8_t bits)
{
    if (!(bits & 0x0F)) { snprintf(out, n, "OK"); return; }
    out[0] = '\0';
    size_t len = 0;
    auto add = [&](uint8_t bit, const char *name) {
        if (bits & bit) len += snprintf(out + len, n - len, "%s%s", len ? ", " : "", name);
    };
    add(proto::THERMO_FAULT_OPEN, "open circuit");
    add(proto::THERMO_FAULT_GND, "short to GND");
    add(proto::THERMO_FAULT_VCC, "short to VCC");
    add(proto::THERMO_FAULT_UNKNOWN, "no data");
}

static void update_thermo_text(uint8_t fault, bool safety)
{
    const uint8_t now = fault & 0x0F, latched = fault >> 4;
    char now_s[64], latched_s[64];
    format_thermo_faults(now_s, sizeof(now_s), now);
    format_thermo_faults(latched_s, sizeof(latched_s), latched);
    snprintf(s_thermo_status_buf, sizeof(s_thermo_status_buf), "Now: 0x%02X %s\nSince clear: 0x%02X %s%s", now, now_s,
             latched, latched_s, safety ? "\nSAFETY SHUTDOWN: heater off, reboot to reset" : "");
}

static int32_t round_x10(int16_t x10) { return (x10 + (x10 >= 0 ? 5 : -5)) / 10; }

// State update from the controller (called from loop context via controller_link_poll()).
void app_on_controller_state(const proto::StateMsg &st)
{
    s_temperature_x10 = st.temperature_x10;
    s_setpoint_x10 = st.setpoint_x10;
    s_have_state = true;
    s_temperature = round_x10(st.temperature_x10);
    if ((int32_t)(millis() - s_setpoint_hold_until) >= 0) s_setpoint = round_x10(st.setpoint_x10);
    s_brew_seconds = st.brew_duration_s;
    s_preinfusion_seconds = st.preinfusion_s;
    s_bloom_seconds = st.bloom_s;
    refresh_texts();

    s_steam_active = st.flags & proto::FLAG_STEAM;
    s_brewing = st.flags & proto::FLAG_BREWING;
    s_brew_hold = st.flags & proto::FLAG_HOLD;
    s_brew_idle = !s_brewing;
    // The BREW button doubles as a cancel button while brewing.
    snprintf(s_brew_button_buf, sizeof(s_brew_button_buf), "%s", s_brewing ? "STOP" : "BREW");

    update_clean_texts(st);
    update_thermo_text(st.thermo_fault, st.flags & proto::FLAG_SAFETY);

    s_pump_on = st.flags & proto::FLAG_PUMP;
    s_valve_on = st.flags & proto::FLAG_VALVE;
    snprintf(s_pump_button_buf, sizeof(s_pump_button_buf), "%s", s_pump_on ? "PUMP ON" : "PUMP OFF");
    snprintf(s_valve_button_buf, sizeof(s_valve_button_buf), "%s", s_valve_on ? "VALVE OPEN" : "VALVE CLOSED");

    // Brew status: while brewing, "STAGE <seconds left in this stage>" (or "BREW +Ns HOLD" once a hold runs
    // past the main stage); when idle, the plan, e.g. "2 + 5 + 30 s".
    const unsigned pre = st.preinfusion_s, bloom = st.bloom_s, brew = st.brew_duration_s;
    const unsigned el = st.brew_elapsed_s;
    if (s_brewing && st.brew_stage != proto::STAGE_NONE) {
        const char *stage = "BREW";
        unsigned end = pre + bloom + brew;
        if (st.brew_stage == proto::STAGE_PREINFUSION) { stage = "PREINFUSE"; end = pre; }
        else if (st.brew_stage == proto::STAGE_BLOOM) { stage = "BLOOM"; end = pre + bloom; }
        if (el < end) {
            snprintf(s_brew_status_buf, sizeof(s_brew_status_buf), "%s %us", stage, end - el);
        } else {
            snprintf(s_brew_status_buf, sizeof(s_brew_status_buf), "%s +%us HOLD", stage, el - end);
        }
    } else if (!s_brewing) {
        char *p = s_brew_status_buf;
        char *const e = s_brew_status_buf + sizeof(s_brew_status_buf);
        const unsigned parts[3] = {pre, bloom, brew};
        bool first = true;
        for (unsigned v : parts) {
            if (v == 0) continue;
            p += snprintf(p, e - p, first ? "%u" : " + %u", v);
            first = false;
        }
        if (first) s_brew_status_buf[0] = '\0';
        else snprintf(p, e - p, " s");
    } else {
        s_brew_status_buf[0] = '\0';
    }
}

static void set_setpoint(int32_t v)
{
    if (v < SETPOINT_MIN || v > SETPOINT_MAX || v == s_setpoint) return;
    s_setpoint = v; // optimistic; the controller's next state message confirms it
    refresh_texts();
    controller_link_send_command(proto::CMD_SET_SETPOINT, v * 10);
}

static void change_setpoint(int32_t delta) { set_setpoint(s_setpoint + delta); }

// Dragging the (invisible) setpoint arc on Main ends up in set_var_setpoint(), see below.

static void change_brew_time(int32_t delta)
{
    int32_t v = s_brew_seconds + delta;
    if (v < BREW_MIN || v > BREW_MAX) return;
    s_brew_seconds = v;
    refresh_texts();
    controller_link_send_command(proto::CMD_SET_BREW_DURATION, v);
}

static void change_stage_time(int32_t &seconds, int32_t delta, proto::CommandId cmd)
{
    int32_t v = seconds + delta;
    if (v < STAGE_MIN || v > STAGE_MAX) return;
    seconds = v;
    refresh_texts();
    controller_link_send_command(cmd, v);
}

extern "C" {

int32_t get_var_temperature() { return s_temperature; }
void set_var_temperature(int32_t v) { s_temperature = v; }
int32_t get_var_setpoint() { return s_setpoint; }
// Written by the Main screen's setpoint arc when the user drags its knob. Sends the new setpoint to the controller.
void set_var_setpoint(int32_t v)
{
    if (s_steam_active) return; // the steam setpoint is fixed; the arc snaps back on the next tick
    s_setpoint_hold_until = millis() + 1500; // don't let in-flight controller states undo the drag
    set_setpoint(v);
}
int32_t get_var_preinfusion_seconds() { return s_preinfusion_seconds; }
void set_var_preinfusion_seconds(int32_t v) { s_preinfusion_seconds = v; }
int32_t get_var_bloom_seconds() { return s_bloom_seconds; }
void set_var_bloom_seconds(int32_t v) { s_bloom_seconds = v; }
const char *get_var_preinfusion_seconds_text() { return s_preinfusion_seconds_text; }
void set_var_preinfusion_seconds_text(const char *v) { s_preinfusion_seconds_text = v; }
const char *get_var_bloom_seconds_text() { return s_bloom_seconds_text; }
void set_var_bloom_seconds_text(const char *v) { s_bloom_seconds_text = v; }
int32_t get_var_brew_seconds() { return s_brew_seconds; }
void set_var_brew_seconds(int32_t v) { s_brew_seconds = v; }

bool get_var_steam_active() { return s_steam_active; }
void set_var_steam_active(bool v) { s_steam_active = v; }
bool get_var_clean_running() { return s_clean_running; }
void set_var_clean_running(bool v) { s_clean_running = v; }
bool get_var_clean_done() { return s_clean_done; }
void set_var_clean_done(bool v) { s_clean_done = v; }
const char *get_var_clean_title_text() { return s_clean_title_text; }
void set_var_clean_title_text(const char *v) { s_clean_title_text = v; }
const char *get_var_clean_detail_text() { return s_clean_detail_text; }
void set_var_clean_detail_text(const char *v) { s_clean_detail_text = v; }
const char *get_var_clean_primary_text() { return s_clean_primary_text; }
void set_var_clean_primary_text(const char *v) { s_clean_primary_text = v; }
const char *get_var_clean_secondary_text() { return s_clean_secondary_text; }
void set_var_clean_secondary_text(const char *v) { s_clean_secondary_text = v; }
const char *get_var_pump_button_text() { return s_pump_button_text; }
void set_var_pump_button_text(const char *v) { s_pump_button_text = v; }
const char *get_var_valve_button_text() { return s_valve_button_text; }
void set_var_valve_button_text(const char *v) { s_valve_button_text = v; }
const char *get_var_thermo_status_text() { return s_thermo_status_text; }
void set_var_thermo_status_text(const char *v) { s_thermo_status_text = v; }
bool get_var_brew_hold() { return s_brew_hold; }
void set_var_brew_hold(bool v) { s_brew_hold = v; }
// True when the temperature is below the arc range (85 °C): hides the arc's start-cap dot.
bool get_var_temp_below_range() { return s_temperature_x10 < 850; }
void set_var_temp_below_range(bool v) {}
bool get_var_brew_idle() { return s_brew_idle; }
void set_var_brew_idle(bool v) { s_brew_idle = v; }
bool get_var_brewing() { return s_brewing; }
void set_var_brewing(bool v) { s_brewing = v; }
const char *get_var_brew_button_text() { return s_brew_button_text; }
void set_var_brew_button_text(const char *v) { s_brew_button_text = v; }
const char *get_var_brew_status_text() { return s_brew_status_text; }
void set_var_brew_status_text(const char *v) { s_brew_status_text = v; }

int32_t get_var_temperature_x10() { return s_temperature_x10; }
void set_var_temperature_x10(int32_t v) { s_temperature_x10 = (int16_t)v; }

const char *get_var_temperature_text() { return s_temperature_text; }
void set_var_temperature_text(const char *v) { s_temperature_text = v; }
const char *get_var_setpoint_text() { return s_setpoint_text; }
void set_var_setpoint_text(const char *v) { s_setpoint_text = v; }
const char *get_var_brew_seconds_text() { return s_brew_seconds_text; }
void set_var_brew_seconds_text(const char *v) { s_brew_seconds_text = v; }

// The BREW button starts a brew, and becomes STOP while one is running.
void action_brew_start(lv_event_t *e)
{
    controller_link_send_command(s_brewing ? proto::CMD_BREW_STOP : proto::CMD_BREW_START);
}
void action_brew_hold_toggle(lv_event_t *e) { controller_link_send_command(proto::CMD_BREW_HOLD_TOGGLE); }
void action_steam_toggle(lv_event_t *e) { controller_link_send_command(proto::CMD_STEAM_TOGGLE); }
void action_setpoint_inc(lv_event_t *e) { change_setpoint(+1); }
void action_setpoint_dec(lv_event_t *e) { change_setpoint(-1); }
void action_preinfusion_inc(lv_event_t *e) { change_stage_time(s_preinfusion_seconds, +1, proto::CMD_SET_PREINFUSION_DURATION); }
void action_preinfusion_dec(lv_event_t *e) { change_stage_time(s_preinfusion_seconds, -1, proto::CMD_SET_PREINFUSION_DURATION); }
void action_bloom_inc(lv_event_t *e) { change_stage_time(s_bloom_seconds, +1, proto::CMD_SET_BLOOM_DURATION); }
void action_bloom_dec(lv_event_t *e) { change_stage_time(s_bloom_seconds, -1, proto::CMD_SET_BLOOM_DURATION); }
void action_brew_time_inc(lv_event_t *e) { change_brew_time(+1); }
void action_brew_time_dec(lv_event_t *e) { change_brew_time(-1); }

void action_open_manual(lv_event_t *e) { loadScreen(SCREEN_ID_MANUAL); }
void action_manual_back(lv_event_t *e) { loadScreen(SCREEN_ID_SETTINGS); }
// Absolute set commands (not toggles) so a lost packet or a second input source can't invert the meaning.
// The button text follows the controller's state, so it only changes once the controller confirms.
void action_pump_toggle(lv_event_t *e) { controller_link_send_command(proto::CMD_PUMP_SET, !s_pump_on); }
void action_valve_toggle(lv_event_t *e) { controller_link_send_command(proto::CMD_VALVE_SET, !s_valve_on); }
void action_manual_gesture(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) loadScreen(SCREEN_ID_SETTINGS);
}

void action_open_sensor(lv_event_t *e) { loadScreen(SCREEN_ID_SENSOR); }
void action_sensor_back(lv_event_t *e) { loadScreen(SCREEN_ID_SETTINGS); }
void action_sensor_clear(lv_event_t *e) { controller_link_send_command(proto::CMD_THERMO_FAULT_CLEAR); }
void action_sensor_gesture(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) loadScreen(SCREEN_ID_SETTINGS);
}

void action_open_clean(lv_event_t *e) { loadScreen(SCREEN_ID_CLEAN); }

// START when idle, CONTINUE after the detergent cycles, DONE (acknowledge and leave) when finished.
void action_clean_primary(lv_event_t *e)
{
    switch (s_clean_state) {
    case proto::CLEAN_IDLE: controller_link_send_command(proto::CMD_CLEAN_START); break;
    case proto::CLEAN_WAIT_REMOVE: controller_link_send_command(proto::CMD_CLEAN_CONTINUE); break;
    case proto::CLEAN_DONE:
        controller_link_send_command(proto::CMD_CLEAN_CANCEL);
        loadScreen(SCREEN_ID_SETTINGS);
        break;
    }
}

// BACK when idle, CANCEL while a sequence is in progress.
void action_clean_secondary(lv_event_t *e)
{
    if (s_clean_state == proto::CLEAN_IDLE) {
        loadScreen(SCREEN_ID_SETTINGS);
    } else {
        controller_link_send_command(proto::CMD_CLEAN_CANCEL);
    }
}

void action_clean_gesture(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (s_clean_state == proto::CLEAN_IDLE && (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM)) {
        loadScreen(SCREEN_ID_SETTINGS);
    }
}

void action_main_gesture(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT || dir == LV_DIR_TOP) {
        loadScreen(SCREEN_ID_SETTINGS);
    }
}

void action_settings_gesture(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) {
        loadScreen(SCREEN_ID_MAIN);
    } else if (dir == LV_DIR_LEFT || dir == LV_DIR_TOP) {
        loadScreen(SCREEN_ID_CHART);
    }
}

void action_chart_gesture(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) {
        loadScreen(SCREEN_ID_SETTINGS);
    }
}

// Temperature history chart: one sample per second, CHART_POINTS wide (see the Chart screen).
// The chart range is 85-100 °C; values are stored in 0.1 °C so the line has sub-degree resolution.

void app_chart_tick()
{
    if (!s_have_state) return;
    uint32_t now = millis();
    if (s_chart_ready && now - s_chart_last_ms < 1000) return;

    lv_obj_t *chart = objects.temp_chart;
    if (!chart) return;
    if (!s_chart_ready) {
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 850, 1000);
        lv_chart_set_all_value(chart, screen_chart_state.temperature, LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, screen_chart_state.setpoint, LV_CHART_POINT_NONE);
        s_chart_ready = true;
        s_chart_last_ms = now;
    } else {
        s_chart_last_ms += 1000; // keep a steady 1 s cadence
    }
    lv_chart_set_next_value(chart, screen_chart_state.temperature, s_temperature_x10);
    lv_chart_set_next_value(chart, screen_chart_state.setpoint, s_setpoint_x10);
}

}

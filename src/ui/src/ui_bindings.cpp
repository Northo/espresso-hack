// Implementations of the native variables (vars.h) and actions (actions.h)
// declared by the EEZ project, plus the model updated from the controller link.
#include "ui/vars.h"
#include "ui/actions.h"
#include "ui/ui.h"
#include "link/controller_link.h"
#include "ui/screens.h"
#include <Arduino.h>
#include <stdio.h>

static int32_t s_temperature, s_setpoint, s_brew_seconds;
// Setpoint marker on the Main arc: a thin wedge (angles in degrees) drawn around the setpoint position.
// Matches the arc geometry in the EEZ project: 85-100 °C spans 200..340 degrees.
static int32_t s_setpoint_angle_start = 200, s_setpoint_angle_end = 202;
// Latest raw controller values (0.1 °C) feeding the history chart.
static bool s_have_state, s_chart_ready;
static int16_t s_temperature_x10, s_setpoint_x10;
static uint32_t s_chart_last_ms;

static char s_temperature_buf[16] = "--", s_setpoint_buf[16] = "--", s_brew_seconds_buf[16] = "--";
static const char *s_temperature_text = s_temperature_buf, *s_setpoint_text = s_setpoint_buf,
                  *s_brew_seconds_text = s_brew_seconds_buf;

static bool s_steam_active, s_brewing, s_brew_hold, s_brew_idle = true;
static char s_brew_button_buf[8] = "BREW", s_brew_status_buf[32] = "";
static const char *s_brew_button_text = s_brew_button_buf, *s_brew_status_text = s_brew_status_buf;

// Cleaning screen state, driven by the controller.
static uint8_t s_clean_state = proto::CLEAN_IDLE;
static bool s_clean_running, s_clean_done;
static char s_clean_title_buf[32] = "", s_clean_detail_buf[96] = "", s_clean_primary_buf[16] = "START",
            s_clean_secondary_buf[16] = "BACK";
static const char *s_clean_title_text = s_clean_title_buf, *s_clean_detail_text = s_clean_detail_buf,
                  *s_clean_primary_text = s_clean_primary_buf, *s_clean_secondary_text = s_clean_secondary_buf;

static constexpr int32_t SETPOINT_MIN = 20, SETPOINT_MAX = 160; // matches the arc range
static constexpr int32_t BREW_MIN = 1, BREW_MAX = 120;

static void refresh_setpoint_marker()
{
    constexpr int32_t TEMP_MIN = 85, TEMP_MAX = 100, ANGLE_MIN = 200, ANGLE_MAX = 340, HALF_WIDTH = 1;
    int32_t sp = s_setpoint < TEMP_MIN ? TEMP_MIN : (s_setpoint > TEMP_MAX ? TEMP_MAX : s_setpoint);
    int32_t a = ANGLE_MIN + (sp - TEMP_MIN) * (ANGLE_MAX - ANGLE_MIN) / (TEMP_MAX - TEMP_MIN);
    s_setpoint_angle_start = a - HALF_WIDTH;
    s_setpoint_angle_end = a + HALF_WIDTH;
}

static void refresh_texts()
{
    refresh_setpoint_marker();
    snprintf(s_temperature_buf, sizeof(s_temperature_buf), "%ld\xC2\xB0" "C", (long)s_temperature);
    snprintf(s_setpoint_buf, sizeof(s_setpoint_buf), "%ld\xC2\xB0" "C", (long)s_setpoint);
    snprintf(s_brew_seconds_buf, sizeof(s_brew_seconds_buf), "%lds", (long)s_brew_seconds);
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

static int32_t round_x10(int16_t x10) { return (x10 + (x10 >= 0 ? 5 : -5)) / 10; }

// State update from the controller (called from loop context via controller_link_poll()).
void app_on_controller_state(const proto::StateMsg &st)
{
    s_temperature_x10 = st.temperature_x10;
    s_setpoint_x10 = st.setpoint_x10;
    s_have_state = true;
    s_temperature = round_x10(st.temperature_x10);
    s_setpoint = round_x10(st.setpoint_x10);
    s_brew_seconds = st.brew_duration_s;
    refresh_texts();

    s_steam_active = st.flags & proto::FLAG_STEAM;
    s_brewing = st.flags & proto::FLAG_BREWING;
    s_brew_hold = st.flags & proto::FLAG_HOLD;
    s_brew_idle = !s_brewing;
    // The BREW button doubles as a cancel button while brewing.
    snprintf(s_brew_button_buf, sizeof(s_brew_button_buf), "%s", s_brewing ? "STOP" : "BREW");

    update_clean_texts(st);

    const char *stage = nullptr;
    switch (st.brew_stage) {
    case proto::STAGE_PREINFUSION: stage = "PREINFUSE"; break;
    case proto::STAGE_BLOOM: stage = "BLOOM"; break;
    case proto::STAGE_BREWING: stage = "BREW"; break;
    }
    if (s_brewing && stage) {
        snprintf(s_brew_status_buf, sizeof(s_brew_status_buf), "%s %us", stage, (unsigned)st.brew_elapsed_s);
    } else {
        s_brew_status_buf[0] = '\0';
    }
}

static void change_setpoint(int32_t delta)
{
    int32_t v = s_setpoint + delta;
    if (v < SETPOINT_MIN || v > SETPOINT_MAX) return;
    s_setpoint = v; // optimistic; the controller's next state message confirms it
    refresh_texts();
    controller_link_send_command(proto::CMD_SET_SETPOINT, v * 10);
}

static void change_brew_time(int32_t delta)
{
    int32_t v = s_brew_seconds + delta;
    if (v < BREW_MIN || v > BREW_MAX) return;
    s_brew_seconds = v;
    refresh_texts();
    controller_link_send_command(proto::CMD_SET_BREW_DURATION, v);
}

extern "C" {

int32_t get_var_temperature() { return s_temperature; }
void set_var_temperature(int32_t v) { s_temperature = v; }
int32_t get_var_setpoint() { return s_setpoint; }
void set_var_setpoint(int32_t v) { s_setpoint = v; }
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
bool get_var_brew_hold() { return s_brew_hold; }
void set_var_brew_hold(bool v) { s_brew_hold = v; }
bool get_var_brew_idle() { return s_brew_idle; }
void set_var_brew_idle(bool v) { s_brew_idle = v; }
bool get_var_brewing() { return s_brewing; }
void set_var_brewing(bool v) { s_brewing = v; }
const char *get_var_brew_button_text() { return s_brew_button_text; }
void set_var_brew_button_text(const char *v) { s_brew_button_text = v; }
const char *get_var_brew_status_text() { return s_brew_status_text; }
void set_var_brew_status_text(const char *v) { s_brew_status_text = v; }

int32_t get_var_setpoint_angle_start() { return s_setpoint_angle_start; }
void set_var_setpoint_angle_start(int32_t v) { s_setpoint_angle_start = v; }
int32_t get_var_setpoint_angle_end() { return s_setpoint_angle_end; }
void set_var_setpoint_angle_end(int32_t v) { s_setpoint_angle_end = v; }

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
void action_brew_time_inc(lv_event_t *e) { change_brew_time(+1); }
void action_brew_time_dec(lv_event_t *e) { change_brew_time(-1); }

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

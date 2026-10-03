// Implementations of the native variables (vars.h) and actions (actions.h)
// declared by the EEZ project, plus the model updated from the controller link.
#include "ui/vars.h"
#include "ui/actions.h"
#include "ui/ui.h"
#include "link/controller_link.h"
#include <stdio.h>

static int32_t s_temperature, s_setpoint, s_brew_seconds;
static char s_temperature_buf[16] = "--", s_setpoint_buf[16] = "--", s_brew_seconds_buf[16] = "--";
static const char *s_temperature_text = s_temperature_buf, *s_setpoint_text = s_setpoint_buf,
                  *s_brew_seconds_text = s_brew_seconds_buf;

static bool s_steam_active, s_brewing, s_brew_hold, s_brew_idle = true;
static char s_brew_button_buf[8] = "BREW", s_brew_status_buf[32] = "";
static const char *s_brew_button_text = s_brew_button_buf, *s_brew_status_text = s_brew_status_buf;

static constexpr int32_t SETPOINT_MIN = 20, SETPOINT_MAX = 160; // matches the arc range
static constexpr int32_t BREW_MIN = 1, BREW_MAX = 120;

static void refresh_texts()
{
    snprintf(s_temperature_buf, sizeof(s_temperature_buf), "%ld\xC2\xB0" "C", (long)s_temperature);
    snprintf(s_setpoint_buf, sizeof(s_setpoint_buf), "%ld\xC2\xB0" "C", (long)s_setpoint);
    snprintf(s_brew_seconds_buf, sizeof(s_brew_seconds_buf), "%lds", (long)s_brew_seconds);
}

static int32_t round_x10(int16_t x10) { return (x10 + (x10 >= 0 ? 5 : -5)) / 10; }

// State update from the controller (called from loop context via controller_link_poll()).
void app_on_controller_state(const proto::StateMsg &st)
{
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
    }
}

}

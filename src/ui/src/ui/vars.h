#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables

extern int32_t get_var_temperature();
extern void set_var_temperature(int32_t value);
extern int32_t get_var_setpoint();
extern void set_var_setpoint(int32_t value);
extern int32_t get_var_brew_seconds();
extern void set_var_brew_seconds(int32_t value);
extern const char *get_var_temperature_text();
extern void set_var_temperature_text(const char *value);
extern const char *get_var_setpoint_text();
extern void set_var_setpoint_text(const char *value);
extern const char *get_var_brew_seconds_text();
extern void set_var_brew_seconds_text(const char *value);
extern bool get_var_steam_active();
extern void set_var_steam_active(bool value);
extern bool get_var_brewing();
extern void set_var_brewing(bool value);
extern const char *get_var_brew_button_text();
extern void set_var_brew_button_text(const char *value);
extern const char *get_var_brew_status_text();
extern void set_var_brew_status_text(const char *value);
extern bool get_var_brew_hold();
extern void set_var_brew_hold(bool value);
extern bool get_var_brew_idle();
extern void set_var_brew_idle(bool value);
extern const char *get_var_clean_title_text();
extern void set_var_clean_title_text(const char *value);
extern const char *get_var_clean_detail_text();
extern void set_var_clean_detail_text(const char *value);
extern const char *get_var_clean_primary_text();
extern void set_var_clean_primary_text(const char *value);
extern const char *get_var_clean_secondary_text();
extern void set_var_clean_secondary_text(const char *value);
extern bool get_var_clean_running();
extern void set_var_clean_running(bool value);
extern bool get_var_clean_done();
extern void set_var_clean_done(bool value);
extern int32_t get_var_setpoint_angle_start();
extern void set_var_setpoint_angle_start(int32_t value);
extern int32_t get_var_setpoint_angle_end();
extern void set_var_setpoint_angle_end(int32_t value);
extern int32_t get_var_temperature_x10();
extern void set_var_temperature_x10(int32_t value);
extern bool get_var_temp_below_range();
extern void set_var_temp_below_range(bool value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/
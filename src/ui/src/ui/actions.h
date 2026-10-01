#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_brew_start(lv_event_t * e);
extern void action_steam_toggle(lv_event_t * e);
extern void action_setpoint_inc(lv_event_t * e);
extern void action_setpoint_dec(lv_event_t * e);
extern void action_brew_time_inc(lv_event_t * e);
extern void action_brew_time_dec(lv_event_t * e);
extern void action_main_gesture(lv_event_t * e);
extern void action_settings_gesture(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
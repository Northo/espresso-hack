#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: screen_bg
lv_style_t *get_style_screen_bg_MAIN_DEFAULT();
void add_style_screen_bg(lv_obj_t *obj);
void remove_style_screen_bg(lv_obj_t *obj);

// Style: arc_temp
lv_style_t *get_style_arc_temp_MAIN_DEFAULT();
lv_style_t *get_style_arc_temp_INDICATOR_DEFAULT();
lv_style_t *get_style_arc_temp_KNOB_DEFAULT();
void add_style_arc_temp(lv_obj_t *obj);
void remove_style_arc_temp(lv_obj_t *obj);

// Style: arc_setpoint
lv_style_t *get_style_arc_setpoint_MAIN_DEFAULT();
lv_style_t *get_style_arc_setpoint_INDICATOR_DEFAULT();
lv_style_t *get_style_arc_setpoint_KNOB_DEFAULT();
void add_style_arc_setpoint(lv_obj_t *obj);
void remove_style_arc_setpoint(lv_obj_t *obj);

// Style: lbl_temp
lv_style_t *get_style_lbl_temp_MAIN_DEFAULT();
void add_style_lbl_temp(lv_obj_t *obj);
void remove_style_lbl_temp(lv_obj_t *obj);

// Style: btn_brew
lv_style_t *get_style_btn_brew_MAIN_DEFAULT();
lv_style_t *get_style_btn_brew_MAIN_CHECKED();
void add_style_btn_brew(lv_obj_t *obj);
void remove_style_btn_brew(lv_obj_t *obj);

// Style: btn_steam
lv_style_t *get_style_btn_steam_MAIN_DEFAULT();
lv_style_t *get_style_btn_steam_MAIN_CHECKED();
void add_style_btn_steam(lv_obj_t *obj);
void remove_style_btn_steam(lv_obj_t *obj);

// Style: lbl_small
lv_style_t *get_style_lbl_small_MAIN_DEFAULT();
void add_style_lbl_small(lv_obj_t *obj);
void remove_style_lbl_small(lv_obj_t *obj);

// Style: btn_round
lv_style_t *get_style_btn_round_MAIN_DEFAULT();
void add_style_btn_round(lv_obj_t *obj);
void remove_style_btn_round(lv_obj_t *obj);

// Style: lbl_value
lv_style_t *get_style_lbl_value_MAIN_DEFAULT();
void add_style_lbl_value(lv_obj_t *obj);
void remove_style_lbl_value(lv_obj_t *obj);

// Style: sheet_hint
lv_style_t *get_style_sheet_hint_MAIN_DEFAULT();
void add_style_sheet_hint(lv_obj_t *obj);
void remove_style_sheet_hint(lv_obj_t *obj);

// Style: sheet_grab
lv_style_t *get_style_sheet_grab_MAIN_DEFAULT();
void add_style_sheet_grab(lv_obj_t *obj);
void remove_style_sheet_grab(lv_obj_t *obj);

// Style: btn_hold
lv_style_t *get_style_btn_hold_MAIN_DEFAULT();
lv_style_t *get_style_btn_hold_MAIN_CHECKED();
void add_style_btn_hold(lv_obj_t *obj);
void remove_style_btn_hold(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/
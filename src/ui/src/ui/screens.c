#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

screen_main_1_state_t screen_main_1_state;
screen_chart_state_t screen_chart_state;

// Global state variables

lv_style_t Setpoint_temp_main_style;
static bool Setpoint_temp_main_style_initialized;
lv_style_t Setpoint_temp_items_style;
static bool Setpoint_temp_items_style_initialized;
lv_style_t Setpoint_temp_indicator_style;
static bool Setpoint_temp_indicator_style_initialized;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_temperature(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_temperature_x10(value);
        }
    }
}

static void event_handler_cb_main_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_brewing(value);
        }
    }
}

static void event_handler_cb_main_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_steam_active(value);
        }
    }
}

static void event_handler_cb_main_obj2(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_brew_hold(value);
        }
    }
}

static void event_handler_cb_main_1_obj14(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_brewing(value);
        }
    }
}

static void event_handler_cb_main_1_obj15(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_steam_active(value);
        }
    }
}

static void event_handler_cb_main_1_obj16(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_brew_hold(value);
        }
    }
}

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 480);
    lv_obj_add_event_cb(obj, action_main_gesture, LV_EVENT_GESTURE, (void *)0);
    add_style_screen_bg(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            // temperature
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.temperature = obj;
            lv_obj_set_pos(obj, 20, 20);
            lv_obj_set_size(obj, 440, 440);
            lv_arc_set_range(obj, 850, 1000);
            lv_arc_set_bg_start_angle(obj, 200);
            lv_arc_set_bg_end_angle(obj, 340);
            lv_obj_add_event_cb(obj, event_handler_cb_main_temperature, LV_EVENT_ALL, 0);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            add_style_arc_temp(obj);
        }
        {
            // setpoint_indicator
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.setpoint_indicator = obj;
            lv_obj_set_pos(obj, 14, 14);
            lv_obj_set_size(obj, 452, 452);
            lv_arc_set_bg_start_angle(obj, 200);
            lv_arc_set_bg_end_angle(obj, 340);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            add_style_arc_setpoint(obj);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 140, 110);
            lv_obj_set_size(obj, 200, 60);
            add_style_lbl_temp(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 120, 230);
            lv_obj_set_size(obj, 240, 30);
            add_style_lbl_small(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 80, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_brew_start, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj0, LV_EVENT_ALL, 0);
            add_style_btn_brew(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj5 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 260, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_steam_toggle, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj1, LV_EVENT_ALL, 0);
            add_style_btn_steam(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "STEAM");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 260, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_brew_hold_toggle, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj2, LV_EVENT_ALL, 0);
            add_style_btn_hold(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "HOLD");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 110, 436);
            lv_obj_set_size(obj, 260, 44);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            add_style_sheet_hint(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 94, 8);
                    lv_obj_set_size(obj, 40, 5);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_sheet_grab(obj);
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    {
        int32_t new_val = get_var_temperature_x10();
        int32_t cur_val = lv_arc_get_value(objects.temperature);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.temperature;
            lv_arc_set_value(objects.temperature, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_setpoint_angle_start();
        int32_t cur_val = lv_arc_get_angle_start(objects.setpoint_indicator);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.setpoint_indicator;
            lv_arc_set_start_angle(objects.setpoint_indicator, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_setpoint_angle_end();
        int32_t cur_val = lv_arc_get_angle_end(objects.setpoint_indicator);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.setpoint_indicator;
            lv_arc_set_end_angle(objects.setpoint_indicator, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_temperature_text();
        const char *cur_val = lv_label_get_text(objects.obj3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj3;
            lv_label_set_text(objects.obj3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_brew_status_text();
        const char *cur_val = lv_label_get_text(objects.obj4);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj4;
            lv_label_set_text(objects.obj4, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brewing();
        bool cur_val = lv_obj_has_state(objects.obj0, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj0;
            if (new_val) {
                lv_obj_add_state(objects.obj0, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj0, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_brew_button_text();
        const char *cur_val = lv_label_get_text(objects.obj5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj5;
            lv_label_set_text(objects.obj5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brewing();
        bool cur_val = lv_obj_has_flag(objects.obj1, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj1;
            if (new_val) {
                lv_obj_add_flag(objects.obj1, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj1, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_steam_active();
        bool cur_val = lv_obj_has_state(objects.obj1, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj1;
            if (new_val) {
                lv_obj_add_state(objects.obj1, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj1, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brew_idle();
        bool cur_val = lv_obj_has_flag(objects.obj2, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj2;
            if (new_val) {
                lv_obj_add_flag(objects.obj2, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj2, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brew_hold();
        bool cur_val = lv_obj_has_state(objects.obj2, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj2;
            if (new_val) {
                lv_obj_add_state(objects.obj2, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj2, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 480);
    lv_obj_add_event_cb(obj, action_settings_gesture, LV_EVENT_GESTURE, (void *)0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SNAPPABLE|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    add_style_screen_bg(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 140, 40);
            lv_obj_set_size(obj, 200, 30);
            add_style_lbl_small(obj);
            lv_label_set_text_static(obj, "Settings");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 100, 100);
            lv_obj_set_size(obj, 280, 30);
            add_style_lbl_small(obj);
            lv_label_set_text_static(obj, "Temperature setpoint");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 180, 152);
            lv_obj_set_size(obj, 120, 40);
            add_style_lbl_value(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 100, 225);
            lv_obj_set_size(obj, 280, 30);
            add_style_lbl_small(obj);
            lv_label_set_text_static(obj, "Brew duration");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 180, 277);
            lv_obj_set_size(obj, 120, 40);
            add_style_lbl_value(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 100, 144);
            lv_obj_set_size(obj, 64, 56);
            lv_obj_add_event_cb(obj, action_setpoint_dec, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_round(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "-");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 316, 144);
            lv_obj_set_size(obj, 64, 56);
            lv_obj_add_event_cb(obj, action_setpoint_inc, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_round(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "+");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 100, 269);
            lv_obj_set_size(obj, 64, 56);
            lv_obj_add_event_cb(obj, action_brew_time_dec, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_round(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "-");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 316, 269);
            lv_obj_set_size(obj, 64, 56);
            lv_obj_add_event_cb(obj, action_brew_time_inc, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_round(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "+");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 170, 350);
            lv_obj_set_size(obj, 140, 56);
            lv_obj_add_event_cb(obj, action_open_clean, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_hold(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "CLEAN");
                }
            }
        }
    }
    
    tick_screen_settings();
}

void tick_screen_settings() {
    {
        const char *new_val = get_var_setpoint_text();
        const char *cur_val = lv_label_get_text(objects.obj6);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj6;
            lv_label_set_text(objects.obj6, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_brew_seconds_text();
        const char *cur_val = lv_label_get_text(objects.obj7);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj7;
            lv_label_set_text(objects.obj7, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_clean() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.clean = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 480);
    lv_obj_add_event_cb(obj, action_clean_gesture, LV_EVENT_GESTURE, (void *)0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SNAPPABLE|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    add_style_screen_bg(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 90, 70);
            lv_obj_set_size(obj, 300, 40);
            add_style_lbl_value(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 90, 130);
            lv_obj_set_size(obj, 300, 150);
            add_style_lbl_small(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, 80, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_clean_primary, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_brew(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj11 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 260, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_clean_secondary, LV_EVENT_CLICKED, (void *)0);
            add_style_btn_hold(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj13 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
    
    tick_screen_clean();
}

void tick_screen_clean() {
    {
        const char *new_val = get_var_clean_title_text();
        const char *cur_val = lv_label_get_text(objects.obj8);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj8;
            lv_label_set_text(objects.obj8, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_clean_detail_text();
        const char *cur_val = lv_label_get_text(objects.obj9);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj9;
            lv_label_set_text(objects.obj9, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_clean_running();
        bool cur_val = lv_obj_has_flag(objects.obj10, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj10;
            if (new_val) {
                lv_obj_add_flag(objects.obj10, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj10, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_clean_primary_text();
        const char *cur_val = lv_label_get_text(objects.obj11);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj11;
            lv_label_set_text(objects.obj11, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_clean_done();
        bool cur_val = lv_obj_has_flag(objects.obj12, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj12;
            if (new_val) {
                lv_obj_add_flag(objects.obj12, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj12, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_clean_secondary_text();
        const char *cur_val = lv_label_get_text(objects.obj13);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj13;
            lv_label_set_text(objects.obj13, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_main_1() {
    screen_main_1_state_t *state = &screen_main_1_state;
    (void)state;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_1 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 480);
    lv_obj_add_event_cb(obj, action_main_gesture, LV_EVENT_GESTURE, (void *)0);
    add_style_screen_bg(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            // Setpoint
            lv_obj_t *obj = lv_scale_create(parent_obj);
            objects.setpoint = obj;
            lv_obj_set_pos(obj, 20, 20);
            lv_obj_set_size(obj, 440, 440);
            lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
            lv_scale_set_range(obj, 20, 160);
            lv_scale_set_angle_range(obj, 140);
            lv_scale_set_rotation(obj, 200);
            lv_scale_set_total_tick_count(obj, 31);
            lv_scale_set_major_tick_every(obj, 2);
            lv_scale_set_label_show(obj, false);
            {
                state->temp = lv_scale_add_section(obj);
                lv_scale_section_set_range(state->temp, 0, 78);
                {
                    if (!Setpoint_temp_main_style_initialized) {
                        lv_style_init(&Setpoint_temp_main_style);
                        Setpoint_temp_main_style_initialized = true;
                        lv_style_set_arc_width(&Setpoint_temp_main_style, 0);
                        lv_style_set_arc_color(&Setpoint_temp_main_style, lv_color_hex(0xffffff));
                    }
                    lv_scale_section_set_style(state->temp, LV_PART_MAIN, &Setpoint_temp_main_style);
                }
                {
                    if (!Setpoint_temp_items_style_initialized) {
                        lv_style_init(&Setpoint_temp_items_style);
                        Setpoint_temp_items_style_initialized = true;
                        lv_style_set_line_width(&Setpoint_temp_items_style, 10);
                        lv_style_set_line_color(&Setpoint_temp_items_style, lv_color_hex(0xcc0000));
                    }
                    lv_scale_section_set_style(state->temp, LV_PART_ITEMS, &Setpoint_temp_items_style);
                }
                {
                    if (!Setpoint_temp_indicator_style_initialized) {
                        lv_style_init(&Setpoint_temp_indicator_style);
                        Setpoint_temp_indicator_style_initialized = true;
                        lv_style_set_line_width(&Setpoint_temp_indicator_style, 10);
                        lv_style_set_line_color(&Setpoint_temp_indicator_style, lv_color_hex(0xcc0000));
                        lv_style_set_text_color(&Setpoint_temp_indicator_style, lv_color_hex(0xfe6a6a));
                    }
                    lv_scale_section_set_style(state->temp, LV_PART_INDICATOR, &Setpoint_temp_indicator_style);
                }
            }
            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x383838), LV_PART_MAIN);
            lv_obj_set_style_length(obj, 15, LV_PART_ITEMS);
            lv_obj_set_style_line_width(obj, 10, LV_PART_ITEMS);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x303030), LV_PART_ITEMS);
            lv_obj_set_style_length(obj, 15, LV_PART_INDICATOR);
            lv_obj_set_style_line_width(obj, 10, LV_PART_INDICATOR);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x303030), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcfcf), LV_PART_INDICATOR);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj17 = obj;
            lv_obj_set_pos(obj, 140, 110);
            lv_obj_set_size(obj, 200, 60);
            add_style_lbl_temp(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj18 = obj;
            lv_obj_set_pos(obj, 120, 230);
            lv_obj_set_size(obj, 240, 30);
            add_style_lbl_small(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj14 = obj;
            lv_obj_set_pos(obj, 80, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_brew_start, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_1_obj14, LV_EVENT_ALL, 0);
            add_style_btn_brew(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj19 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj15 = obj;
            lv_obj_set_pos(obj, 260, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_steam_toggle, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_1_obj15, LV_EVENT_ALL, 0);
            add_style_btn_steam(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "STEAM");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj16 = obj;
            lv_obj_set_pos(obj, 260, 320);
            lv_obj_set_size(obj, 140, 64);
            lv_obj_add_event_cb(obj, action_brew_hold_toggle, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_1_obj16, LV_EVENT_ALL, 0);
            add_style_btn_hold(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "HOLD");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 110, 436);
            lv_obj_set_size(obj, 260, 44);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            add_style_sheet_hint(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 94, 8);
                    lv_obj_set_size(obj, 40, 5);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_sheet_grab(obj);
                }
            }
        }
    }
    
    tick_screen_main_1();
}

void tick_screen_main_1() {
    screen_main_1_state_t *state = &screen_main_1_state;
    (void)state;
    {
        const char *new_val = get_var_temperature_text();
        const char *cur_val = lv_label_get_text(objects.obj17);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj17;
            lv_label_set_text(objects.obj17, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_brew_status_text();
        const char *cur_val = lv_label_get_text(objects.obj18);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj18;
            lv_label_set_text(objects.obj18, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brewing();
        bool cur_val = lv_obj_has_state(objects.obj14, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj14;
            if (new_val) {
                lv_obj_add_state(objects.obj14, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj14, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_brew_button_text();
        const char *cur_val = lv_label_get_text(objects.obj19);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj19;
            lv_label_set_text(objects.obj19, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brewing();
        bool cur_val = lv_obj_has_flag(objects.obj15, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj15;
            if (new_val) {
                lv_obj_add_flag(objects.obj15, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj15, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_steam_active();
        bool cur_val = lv_obj_has_state(objects.obj15, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj15;
            if (new_val) {
                lv_obj_add_state(objects.obj15, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj15, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brew_idle();
        bool cur_val = lv_obj_has_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj16;
            if (new_val) {
                lv_obj_add_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_brew_hold();
        bool cur_val = lv_obj_has_state(objects.obj16, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj16;
            if (new_val) {
                lv_obj_add_state(objects.obj16, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(objects.obj16, LV_STATE_CHECKED);
            }
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_chart() {
    screen_chart_state_t *state = &screen_chart_state;
    (void)state;
    lv_obj_t *obj = lv_obj_create(0);
    objects.chart = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 480);
    lv_obj_add_event_cb(obj, action_chart_gesture, LV_EVENT_GESTURE, (void *)0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SNAPPABLE|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    add_style_screen_bg(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 140, 30);
            lv_obj_set_size(obj, 200, 30);
            add_style_lbl_small(obj);
            lv_label_set_text_static(obj, "Temperature");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj20 = obj;
            lv_obj_set_pos(obj, 180, 55);
            lv_obj_set_size(obj, 120, 40);
            add_style_lbl_value(obj);
            lv_label_set_text(obj, "");
        }
        {
            // temp_chart
            lv_obj_t *obj = lv_chart_create(parent_obj);
            objects.temp_chart = obj;
            lv_obj_set_pos(obj, 66, 105);
            lv_obj_set_size(obj, 394, 340);
            lv_chart_set_point_count(obj, 300);
            lv_chart_set_div_line_count(obj, 0, 0);
            lv_chart_series_t *ser0 = lv_chart_add_series(obj, lv_color_hex(0xff7043), LV_CHART_AXIS_PRIMARY_Y);
            state->temperature = ser0;
            lv_chart_set_range(obj, LV_CHART_AXIS_PRIMARY_Y, 85, 100);
            lv_chart_series_t *ser1 = lv_chart_add_series(obj, lv_color_hex(0x9e9e9e), LV_CHART_AXIS_PRIMARY_Y);
            state->setpoint = ser1;
            lv_chart_set_range(obj, LV_CHART_AXIS_PRIMARY_Y, 85, 100);
            add_style_chart_flat(obj);
        }
        {
            // temp_scale
            lv_obj_t *obj = lv_scale_create(parent_obj);
            objects.temp_scale = obj;
            lv_obj_set_pos(obj, 34, 105);
            lv_obj_set_size(obj, 50, 340);
            lv_scale_set_mode(obj, LV_SCALE_MODE_VERTICAL_LEFT);
            lv_scale_set_range(obj, 85, 100);
            lv_scale_set_angle_range(obj, 270);
            lv_scale_set_rotation(obj, 135);
            lv_scale_set_total_tick_count(obj, 16);
            lv_scale_set_major_tick_every(obj, 5);
            lv_scale_set_label_show(obj, true);
            lv_obj_set_style_line_opa(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_length(obj, 4, LV_PART_ITEMS);
            lv_obj_set_style_line_width(obj, 1, LV_PART_ITEMS);
            lv_obj_set_style_line_color(obj, lv_color_hex(theme_colors[active_theme_index][3]), LV_PART_ITEMS);
            lv_obj_set_style_line_opa(obj, 60, LV_PART_ITEMS);
            lv_obj_set_style_length(obj, 8, LV_PART_INDICATOR);
            lv_obj_set_style_line_width(obj, 1, LV_PART_INDICATOR);
            lv_obj_set_style_line_color(obj, lv_color_hex(theme_colors[active_theme_index][3]), LV_PART_INDICATOR);
            lv_obj_set_style_line_opa(obj, 120, LV_PART_INDICATOR);
            lv_obj_set_style_text_color(obj, lv_color_hex(theme_colors[active_theme_index][3]), LV_PART_INDICATOR);
            lv_obj_set_style_text_opa(obj, 160, LV_PART_INDICATOR);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_INDICATOR);
        }
    }
    
    tick_screen_chart();
}

void tick_screen_chart() {
    screen_chart_state_t *state = &screen_chart_state;
    (void)state;
    {
        const char *new_val = get_var_temperature_text();
        const char *cur_val = lv_label_get_text(objects.obj20);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj20;
            lv_label_set_text(objects.obj20, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_settings,
    tick_screen_clean,
    tick_screen_main_1,
    tick_screen_chart,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 5) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;
void change_color_theme(uint32_t theme_index) {
    active_theme_index = theme_index;
    
    {
        screen_chart_state_t *state = &screen_chart_state;
        (void)state;
        lv_obj_set_style_line_color(objects.temp_scale, lv_color_hex(theme_colors[theme_index][3]), LV_PART_ITEMS);
        lv_obj_set_style_line_color(objects.temp_scale, lv_color_hex(theme_colors[theme_index][3]), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(objects.temp_scale, lv_color_hex(theme_colors[theme_index][3]), LV_PART_INDICATOR);
    }
    lv_style_set_bg_color(get_style_screen_bg_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][0]));
    lv_style_set_arc_color(get_style_arc_temp_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][1]));
    lv_style_set_arc_color(get_style_arc_temp_INDICATOR_DEFAULT(), lv_color_hex(theme_colors[theme_index][2]));
    lv_style_set_arc_color(get_style_arc_setpoint_INDICATOR_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_arc_setpoint_KNOB_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_text_color(get_style_lbl_temp_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_btn_brew_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][2]));
    lv_style_set_text_color(get_style_btn_brew_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][0]));
    lv_style_set_border_color(get_style_btn_brew_MAIN_CHECKED(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_btn_steam_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][5]));
    lv_style_set_text_color(get_style_btn_steam_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][0]));
    lv_style_set_border_color(get_style_btn_steam_MAIN_CHECKED(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_text_color(get_style_lbl_small_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_btn_round_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][4]));
    lv_style_set_text_color(get_style_btn_round_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_text_color(get_style_lbl_value_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][2]));
    lv_style_set_bg_color(get_style_sheet_hint_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][1]));
    lv_style_set_bg_color(get_style_sheet_grab_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_btn_hold_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][1]));
    lv_style_set_text_color(get_style_btn_hold_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][3]));
    lv_style_set_bg_color(get_style_btn_hold_MAIN_CHECKED(), lv_color_hex(theme_colors[theme_index][2]));
    lv_style_set_text_color(get_style_btn_hold_MAIN_CHECKED(), lv_color_hex(theme_colors[theme_index][0]));
    lv_style_set_line_color(get_style_chart_flat_MAIN_DEFAULT(), lv_color_hex(theme_colors[theme_index][1]));
    lv_obj_invalidate(objects.main);
    lv_obj_invalidate(objects.settings);
    lv_obj_invalidate(objects.clean);
    lv_obj_invalidate(objects.main_1);
    lv_obj_invalidate(objects.chart);
}
uint32_t theme_colors[1][6] = {
    { 0xff101114, 0xff2a2d33, 0xffff8a1f, 0xfff2f2f2, 0xff1e2126, 0xff3aa0ff },
};

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_settings();
    create_screen_clean();
    create_screen_main_1();
    create_screen_chart();
}
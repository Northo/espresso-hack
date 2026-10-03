#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: screen_bg
//

void init_style_screen_bg_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][0]));
    lv_style_set_bg_opa(style, 255);
};

lv_style_t *get_style_screen_bg_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_screen_bg_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_screen_bg(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_screen_bg_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_screen_bg(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_screen_bg_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: arc_temp
//

void init_style_arc_temp_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_arc_color(style, lv_color_hex(theme_colors[active_theme_index][1]));
    lv_style_set_arc_width(style, 16);
    lv_style_set_arc_rounded(style, true);
};

lv_style_t *get_style_arc_temp_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_temp_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_arc_temp_INDICATOR_DEFAULT(lv_style_t *style) {
    lv_style_set_arc_color(style, lv_color_hex(theme_colors[active_theme_index][2]));
    lv_style_set_arc_width(style, 16);
    lv_style_set_arc_rounded(style, true);
};

lv_style_t *get_style_arc_temp_INDICATOR_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_temp_INDICATOR_DEFAULT(style);
    }
    return style;
};

void init_style_arc_temp_KNOB_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_opa(style, 0);
};

lv_style_t *get_style_arc_temp_KNOB_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_temp_KNOB_DEFAULT(style);
    }
    return style;
};

void add_style_arc_temp(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_arc_temp_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_arc_temp_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_arc_temp_KNOB_DEFAULT(), LV_PART_KNOB | LV_STATE_DEFAULT);
};

void remove_style_arc_temp(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_arc_temp_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_arc_temp_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_arc_temp_KNOB_DEFAULT(), LV_PART_KNOB | LV_STATE_DEFAULT);
};

//
// Style: arc_setpoint
//

void init_style_arc_setpoint_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_arc_opa(style, 0);
};

lv_style_t *get_style_arc_setpoint_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_setpoint_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_arc_setpoint_INDICATOR_DEFAULT(lv_style_t *style) {
    lv_style_set_arc_opa(style, 0);
};

lv_style_t *get_style_arc_setpoint_INDICATOR_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_setpoint_INDICATOR_DEFAULT(style);
    }
    return style;
};

void init_style_arc_setpoint_KNOB_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 255);
};

lv_style_t *get_style_arc_setpoint_KNOB_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_arc_setpoint_KNOB_DEFAULT(style);
    }
    return style;
};

void add_style_arc_setpoint(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_arc_setpoint_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_arc_setpoint_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_arc_setpoint_KNOB_DEFAULT(), LV_PART_KNOB | LV_STATE_DEFAULT);
};

void remove_style_arc_setpoint(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_arc_setpoint_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_arc_setpoint_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_arc_setpoint_KNOB_DEFAULT(), LV_PART_KNOB | LV_STATE_DEFAULT);
};

//
// Style: lbl_temp
//

void init_style_lbl_temp_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
    lv_style_set_text_font(style, &lv_font_montserrat_48);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
};

lv_style_t *get_style_lbl_temp_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_lbl_temp_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_lbl_temp(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_lbl_temp_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_lbl_temp(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_lbl_temp_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: btn_brew
//

void init_style_btn_brew_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][2]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 32);
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][0]));
};

lv_style_t *get_style_btn_brew_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_brew_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_btn_brew_MAIN_CHECKED(lv_style_t *style) {
    lv_style_set_border_width(style, 4);
    lv_style_set_border_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
};

lv_style_t *get_style_btn_brew_MAIN_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_brew_MAIN_CHECKED(style);
    }
    return style;
};

void add_style_btn_brew(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_btn_brew_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_btn_brew_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

void remove_style_btn_brew(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_btn_brew_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_btn_brew_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

//
// Style: btn_steam
//

void init_style_btn_steam_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][5]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 32);
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][0]));
};

lv_style_t *get_style_btn_steam_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_steam_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_btn_steam_MAIN_CHECKED(lv_style_t *style) {
    lv_style_set_border_width(style, 4);
    lv_style_set_border_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
};

lv_style_t *get_style_btn_steam_MAIN_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_steam_MAIN_CHECKED(style);
    }
    return style;
};

void add_style_btn_steam(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_btn_steam_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_btn_steam_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

void remove_style_btn_steam(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_btn_steam_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_btn_steam_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

//
// Style: lbl_small
//

void init_style_lbl_small_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
    lv_style_set_text_font(style, &lv_font_montserrat_20);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
};

lv_style_t *get_style_lbl_small_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_lbl_small_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_lbl_small(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_lbl_small_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_lbl_small(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_lbl_small_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: btn_round
//

void init_style_btn_round_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][4]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 28);
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
};

lv_style_t *get_style_btn_round_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_round_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_btn_round(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_btn_round_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_btn_round(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_btn_round_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: lbl_value
//

void init_style_lbl_value_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][2]));
    lv_style_set_text_font(style, &lv_font_montserrat_32);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
};

lv_style_t *get_style_lbl_value_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_lbl_value_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_lbl_value(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_lbl_value_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_lbl_value(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_lbl_value_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: sheet_hint
//

void init_style_sheet_hint_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][1]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 30);
    lv_style_set_border_width(style, 0);
    lv_style_set_shadow_width(style, 24);
    lv_style_set_shadow_color(style, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(style, 200);
};

lv_style_t *get_style_sheet_hint_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_sheet_hint_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_sheet_hint(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_sheet_hint_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_sheet_hint(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_sheet_hint_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: sheet_grab
//

void init_style_sheet_grab_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
    lv_style_set_bg_opa(style, 110);
    lv_style_set_radius(style, 3);
    lv_style_set_border_width(style, 0);
};

lv_style_t *get_style_sheet_grab_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_sheet_grab_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_sheet_grab(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_sheet_grab_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_sheet_grab(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_sheet_grab_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: btn_hold
//

void init_style_btn_hold_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][1]));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_radius(style, 32);
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][3]));
};

lv_style_t *get_style_btn_hold_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_hold_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_btn_hold_MAIN_CHECKED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][2]));
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][0]));
};

lv_style_t *get_style_btn_hold_MAIN_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_btn_hold_MAIN_CHECKED(style);
    }
    return style;
};

void add_style_btn_hold(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_btn_hold_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_btn_hold_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

void remove_style_btn_hold(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_btn_hold_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_btn_hold_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_screen_bg,
        add_style_arc_temp,
        add_style_arc_setpoint,
        add_style_lbl_temp,
        add_style_btn_brew,
        add_style_btn_steam,
        add_style_lbl_small,
        add_style_btn_round,
        add_style_lbl_value,
        add_style_sheet_hint,
        add_style_sheet_grab,
        add_style_btn_hold,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_screen_bg,
        remove_style_arc_temp,
        remove_style_arc_setpoint,
        remove_style_lbl_temp,
        remove_style_btn_brew,
        remove_style_btn_steam,
        remove_style_lbl_small,
        remove_style_btn_round,
        remove_style_lbl_value,
        remove_style_sheet_hint,
        remove_style_sheet_grab,
        remove_style_btn_hold,
    };
    remove_style_funcs[styleIndex](obj);
}
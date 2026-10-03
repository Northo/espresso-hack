#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_SETTINGS = 2,
    SCREEN_ID_CLEAN = 3,
    SCREEN_ID_MAIN_1 = 4,
    SCREEN_ID_CHART = 5,
    _SCREEN_ID_LAST = 5
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *settings;
    lv_obj_t *clean;
    lv_obj_t *main_1;
    lv_obj_t *chart;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *setpoint;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *temp_chart;
    lv_obj_t *temp_scale;
    lv_obj_t *obj22;
} objects_t;

extern objects_t objects;

typedef struct {
    lv_scale_section_t *temp;
} screen_main_1_state_t;

typedef struct {
    lv_chart_series_t *temperature;
    lv_chart_series_t *setpoint;
} screen_chart_state_t;

extern screen_main_1_state_t screen_main_1_state;
extern screen_chart_state_t screen_chart_state;

void create_screen_main();
void tick_screen_main();

void create_screen_settings();
void tick_screen_settings();

void create_screen_clean();
void tick_screen_clean();

void create_screen_main_1();
void tick_screen_main_1();

void create_screen_chart();
void tick_screen_chart();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

// Color themes

enum Themes {
    THEME_ID_DEFAULT,
};
enum Colors {
    COLOR_ID_BG,
    COLOR_ID_TRACK,
    COLOR_ID_ACCENT,
    COLOR_ID_TEXT,
    COLOR_ID_BTN,
    COLOR_ID_STEAM,
};
void change_color_theme(uint32_t themeIndex);
extern uint32_t theme_colors[1][6];
extern uint32_t active_theme_index;

// Global state variables

extern lv_style_t Setpoint_temp_main_style;
extern lv_style_t Setpoint_temp_items_style;
extern lv_style_t Setpoint_temp_indicator_style;

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
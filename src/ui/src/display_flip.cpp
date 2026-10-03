#include "display_flip.h"
#include <Arduino.h>
#include <lvgl.h>

// Replaces the flush and touch callbacks installed by beginLvglHelper() with versions
// that mirror both axes.

static void flip_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
    auto *plane = (LilyGo_Display *)lv_display_get_user_data(disp);
    uint16_t *px = (uint16_t *)color_p;

    // Reversing the pixel order of the area is a 180 degree rotation of that area.
    size_t n = (size_t)lv_area_get_width(area) * lv_area_get_height(area);
    for (size_t i = 0; i < n / 2; i++) {
        uint16_t t = px[i];
        px[i] = px[n - 1 - i];
        px[n - 1 - i] = t;
    }

    // ...and the area itself moves to the opposite corner.
    uint16_t w = plane->width(), h = plane->height();
    plane->pushColors(w - 1 - area->x2, h - 1 - area->y2, w - area->x1, h - area->y1, px);
    lv_display_flush_ready(disp);
}

static void flip_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto *plane = (LilyGo_Display *)lv_indev_get_user_data(indev);
    int16_t x, y;
    if (plane->getPoint(&x, &y, 1)) {
        data->point.x = plane->width() - 1 - x;
        data->point.y = plane->height() - 1 - y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void display_flip_begin(LilyGo_Display &)
{
    lv_display_set_flush_cb(lv_display_get_default(), flip_flush);
    lv_indev_set_read_cb(lv_indev_get_next(NULL), flip_touch_read);
    lv_obj_invalidate(lv_screen_active());
}

#include "display.h"
#include <TFT_eSPI.h>
#include <Arduino.h>

static TFT_eSPI tft;
static TFT_eSprite spr(&tft);

// Palette
static const uint16_t CLR_BG = TFT_BLACK;
static const uint16_t CLR_VALUE = TFT_WHITE;
static const uint16_t CLR_LABEL = 0x7BEF;   // mid-grey
static const uint16_t CLR_DIVIDER = 0x39E7; // dark grey
static const uint16_t CLR_TEMP = 0xFD20;    // orange  (hot)
static const uint16_t CLR_PID = 0x07E0;     // green
static const uint16_t CLR_MANUAL = 0xFFE0;  // yellow

// Layout constants (portrait 170x320)
static const int W = TFT_WIDTH;  // 170
static const int H = TFT_HEIGHT; // 320
static const int HEADER_H = 32;
static const int SECTION_H = (H - HEADER_H) / 3; // 96 px per section

// Draws one value section (label, large numeric value, unit suffix).
// section: 0 = temperature, 1 = setpoint, 2 = power
static void drawSection(int section, const char *label,
                        const char *value, const char *unit,
                        uint16_t value_color)
{
    const int y = HEADER_H + section * SECTION_H;

    spr.drawFastHLine(0, y, W, CLR_DIVIDER);

    // Label row (Font2 = 16 px tall)
    spr.setTextFont(2);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(CLR_LABEL, CLR_BG);
    spr.drawString(label, 8, y + 8);

    // Value row: Font6 (48 px tall digits), anchored to bottom of section
    const int baseline = y + SECTION_H - 14;
    spr.setTextFont(6);
    spr.setTextDatum(BL_DATUM);
    spr.setTextColor(value_color, CLR_BG);
    spr.drawString(value, 8, baseline);

    // Unit suffix: Font4 (26 px tall), right of value, same baseline
    const int val_w = spr.textWidth(value); // still Font6 here
    spr.setTextFont(4);
    spr.setTextDatum(BL_DATUM);
    spr.setTextColor(CLR_LABEL, CLR_BG);
    spr.drawString(unit, 8 + val_w + 4, baseline);
}

void initDisplay()
{
    tft.init();
    tft.setRotation(0);

    // Enable backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    tft.fillScreen(CLR_BG);

    // Full-screen sprite for flicker-free updates (170*320*2 = ~107 KB)
    spr.createSprite(W, H);
    spr.setTextWrap(false);
}

void updateDisplay(const MachineState &state)
{
    // Limit refresh rate to 10 Hz
    static unsigned long last_update = 0;
    if (millis() - last_update < 100)
        return;
    last_update = millis();

    spr.fillSprite(CLR_BG);

    // ── Header ────────────────────────────────────────────────────────────
    spr.setTextFont(4);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(CLR_VALUE, CLR_BG);
    spr.drawString("espresso", 8, HEADER_H / 2);

    const bool is_pid = (state.mode == ControlMode::PID);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(is_pid ? CLR_PID : CLR_MANUAL, CLR_BG);
    spr.drawString(controlModeToString(state.mode), W - 8, HEADER_H / 2);

    // ── Sections ──────────────────────────────────────────────────────────
    char buf[16];

    snprintf(buf, sizeof(buf), "%.1f", state.current_temperature);
    drawSection(0, "TEMPERATURE", buf, "C", CLR_TEMP);

    if (!state.auto_brew_enabled)
    {
        snprintf(buf, sizeof(buf), "%.1f", state.target_temperature);
        drawSection(1, "SETPOINT", buf, "C", CLR_VALUE);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.1f", (millis() - state.auto_brew_start_time) / 1000.0);
        drawSection(1, "AUTO BREW", buf, "S", CLR_VALUE);
    }

    snprintf(buf, sizeof(buf), "%.1f", state.heater_power);
    drawSection(2, "OUTPUT POWER", buf, "%", CLR_VALUE);

    spr.pushSprite(0, 0);
}

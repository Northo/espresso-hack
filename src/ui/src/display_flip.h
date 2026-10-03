#pragma once
#include <LilyGo_Display.h>

// Rotates the whole UI (drawing and touch) by 180 degrees, for an upside-down mounted display.
// Call right after beginLvglHelper(). Enabled with -DDISPLAY_FLIP=1.
void display_flip_begin(LilyGo_Display &panel);

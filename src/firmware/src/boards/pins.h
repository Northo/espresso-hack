#pragma once

#if defined(BOARD_TDISPLAY_S3)
#include "tdisplay_s3.h"
#elif defined(BOARD_DEVKITC1)
#include "devkitc1.h"
#elif defined(BOARD_T7_S3)
#include "t7_s3.h"
#else
#error "Define a board: -DBOARD_TDISPLAY_S3, -DBOARD_DEVKITC1 or -DBOARD_T7_S3 (see platformio.ini)"
#endif

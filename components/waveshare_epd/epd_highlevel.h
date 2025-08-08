#ifndef EPD_HIGHLEVEL_H
#define EPD_HIGHLEVEL_H

#include "epd_driver.h"

// Color definitions
#define EPD_COLOR_BLACK 0x00
#define EPD_COLOR_WHITE 0xFF
#define EPD_COLOR_RED   0x01 // Or whatever the driver uses

void epd_draw_filled_rect(int x, int y, int w, int h, int color, unsigned char* buffer);


#endif // EPD_HIGHLEVEL_H

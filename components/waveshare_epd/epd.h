#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simple drawing API compatible tokens used by display_manager.cpp

#define EPD_BLACK 0xFF
#define EPD_WHITE 0x00
#define EPD_RED   0x00 // Monochrome panel; treat red as white for now

void epd_begin(void);
void epd_clear(void);
void epd_update(void);

// Minimal GFX-like drawing functions on software framebuffer
void epd_fill_screen(uint8_t color);
void epd_fill_rect(int x, int y, int w, int h, uint8_t color);
void epd_draw_rect(int x, int y, int w, int h, uint8_t color);
void epd_set_cursor(int x, int y);
void epd_set_text_color(uint8_t color);
void epd_set_text_size(int size);
void epd_print(const char *s);

#ifdef __cplusplus
}
#endif




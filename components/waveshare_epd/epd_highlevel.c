#include "epd_highlevel.h"

// Note: This is a simplified implementation. A real driver would need to handle
// different color modes and the specifics of the E-Ink display memory layout.
// This implementation assumes a simple linear buffer of 1 bit per pixel.

static void epd_draw_pixel(int x, int y, int color, unsigned char* buffer)
{
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) {
        return;
    }
    // This is for a 1-bit monochrome buffer. BWR displays are more complex.
    if (color == EPD_COLOR_BLACK) {
        buffer[(x + y * EPD_WIDTH) / 8] &= ~(0x80 >> (x % 8));
    } else {
        buffer[(x + y * EPD_WIDTH) / 8] |= (0x80 >> (x % 8));
    }
}

void epd_draw_filled_rect(int x, int y, int w, int h, int color, unsigned char* buffer)
{
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            epd_draw_pixel(i, j, color, buffer);
        }
    }
}

void epd_draw_char(int x, int y, char ascii_char, sFONT *font, int color, unsigned char* buffer)
{
    const uint8_t *font_char = &font->table[(ascii_char - ' ') * font->height];

    for (int i = 0; i < font->width; i++) {
        for (int j = 0; j < font->height; j++) {
            if ((font_char[j] << i) & 0x80) {
                epd_draw_pixel(x + i, y + j, color, buffer);
            }
        }
    }
}

void epd_draw_string(int x, int y, const char *text, sFONT *font, int color, unsigned char* buffer)
{
    const char *p_text = text;
    int ref_x = x;
    int ref_y = y;

    while (*p_text != '\0') {
        epd_draw_char(ref_x, ref_y, *p_text, font, color, buffer);
        ref_x += font->width;
        p_text++;
    }
}

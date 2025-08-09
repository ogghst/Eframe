#include "epd.h"
#include "epd_driver.h"
#include "epd_gui.h"
#include <string.h>

// Simple software framebuffer and text cursor state
static uint8_t framebuffer[EPD_ARRAY];
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t text_color = EPD_BLACK;
static int text_size = 1;

void epd_begin(void)
{
    ws_epd_bus_init();
    ws_epd_init_full();
    memset(framebuffer, 0x00, sizeof(framebuffer));
    Image_Init(framebuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
}

void epd_clear(void)
{
    memset(framebuffer, 0x00, sizeof(framebuffer));
}

void epd_update(void)
{
    ws_epd_write_full(framebuffer);
}

void epd_fill_screen(uint8_t color)
{
    Gui_SelectImage(framebuffer);
    Gui_Clear(color);
}

void epd_fill_rect(int x, int y, int w, int h, uint8_t color)
{
    Gui_SelectImage(framebuffer);
    Gui_Draw_Rectangle(x, y, x + w, y + h, color, FULL, PIXEL_1X1);
}

void epd_draw_rect(int x, int y, int w, int h, uint8_t color)
{
    Gui_SelectImage(framebuffer);
    Gui_Draw_Rectangle(x, y, x + w, y + h, color, EMPTY, PIXEL_1X1);
}

void epd_set_cursor(int x, int y)
{
    cursor_x = x;
    cursor_y = y;
}

void epd_set_text_color(uint8_t color)
{
    text_color = color;
}

void epd_set_text_size(int size)
{
    text_size = size;
}

// Use built-in font sizes mapping
static FONT* font_for_size(int size)
{
    switch (size) {
        case 1: return &Font8;
        case 2: return &Font12;
        case 3: return &Font16;
        case 4: return &Font20;
        case 5: return &Font24;
        default: return &Font12;
    }
}

void epd_print(const char *s)
{
    Gui_SelectImage(framebuffer);
    FONT *font = font_for_size(text_size);
    Gui_Draw_Str((uint16_t)cursor_x, (uint16_t)cursor_y, s, font, FONT_BACKGROUND, text_color);
}




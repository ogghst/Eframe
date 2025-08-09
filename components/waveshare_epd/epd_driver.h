#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display geometry
#define EPD_WIDTH   800
#define EPD_HEIGHT  480
#define EPD_ARRAY   (EPD_WIDTH * EPD_HEIGHT / 8)

// Mono colors
#define EPD_WHITE 0x00
#define EPD_BLACK 0xFF

// Default pins (match Waveshare ESP32 driver board example)
#ifndef EPD_PIN_CS
#define EPD_PIN_CS    27
#endif
#ifndef EPD_PIN_DC
#define EPD_PIN_DC    14
#endif
#ifndef EPD_PIN_RST
#define EPD_PIN_RST   33
#endif
#ifndef EPD_PIN_BUSY
#define EPD_PIN_BUSY  13
#endif
#ifndef EPD_PIN_MOSI
#define EPD_PIN_MOSI  23
#endif
#ifndef EPD_PIN_SCK
#define EPD_PIN_SCK   18
#endif

// Public API
esp_err_t ws_epd_bus_init(void);
void ws_epd_reset(void);

// Initialization modes
void ws_epd_init_full(void);
void ws_epd_init_fast(void);
void ws_epd_init_partial(void);

// Frame operations
void ws_epd_update(void);
void ws_epd_write_full(const uint8_t *framebuffer);   // write and refresh
void ws_epd_clear_white(void);
void ws_epd_clear_black(void);

// Power
void ws_epd_sleep(void);

#ifdef __cplusplus
}
#endif




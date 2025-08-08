#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Font8
// Monospaced 8x8 font
typedef struct {
    const uint8_t *table;
    uint16_t width;
    uint16_t height;
} sFONT;

extern const uint8_t font8_table[];
extern sFONT Font8;

#endif // FONT_H

#ifndef WIDGET_DATA_H
#define WIDGET_DATA_H

#include <stdint.h>

typedef struct {
    char value[64];
    char unit[16];
    uint32_t timestamp;
} info_card_data_t;

typedef struct {
    char value[64];
    char unit[16];
    char icon[32];
    uint32_t timestamp;
} weather_card_data_t;

typedef struct {
    char label[32];
    char value[64];
} list_item_t;

typedef struct {
    list_item_t items[10];
    int num_items;
    uint32_t timestamp;
} list_widget_data_t;

#endif // WIDGET_DATA_H

#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <stdint.h>

// MQTT Configuration
typedef struct {
    char server[128];
    int port;
    char username[64];
    char password[64];
    char client_id[64];
} mqtt_config_t;

// Widget Position and Size
typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    int width;
    int height;
} size_t;

// Widget Configuration
typedef struct {
    char name[64];
    char type[32];
    position_t position;
    size_t size;
    char topic[128];
} widget_config_t;

// Button Action
typedef struct {
    char type[32];
    char topic[128];
    char payload[128];
} button_action_t;

// Button Configuration
typedef struct {
    int gpio;
    button_action_t action;
} button_config_t;

// Main Configuration Struct
typedef struct {
    mqtt_config_t mqtt;
    widget_config_t widgets[10]; // Max 10 widgets
    int num_widgets;
    button_config_t buttons[4]; // Max 4 buttons
    int num_buttons;
} app_config_t;

#endif // CONFIG_TYPES_H

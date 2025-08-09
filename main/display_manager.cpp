#include "display_manager.hpp"
#include "esp_log.h"
#include <string>
#include <vector>

// CalEPD driver
#include <epd.h>
#include <epdspi.h>
#include <goodisplay/gdey075T7.h>

// Adafruit GFX
#include <Adafruit_GFX.h>

// Config and data
#include "config_parser.h"
#include "widget_data.h"

static const char *TAG = "DISPLAY";

// EPD driver objects
EpdSpi io;
Gdey075T7 display(io);

// Widget data store
static widget_data_t widget_data_store[10]; // Max 10 widgets

extern "C" void display_init(void)
{
    ESP_LOGI(TAG, "Initializing display");
    display.init(false); // false for production
}

static void display_render_info_card(const widget_config_t *widget, const info_card_data_t *data)
{
    ESP_LOGI(TAG, "Rendering info card: %s, value: %s %s", widget->name, data->value, data->unit);

    int x, y, w, h;
    //display_get_grid_rect(widget->position.x, widget->position.y, widget->size.width, widget->size.height, &x, &y, &w, &h);
    // For now, let's just use hardcoded values
    x = 10; y = 10; w = 100; h = 50;


    display.fillRect(x, y, w, h, EPD_WHITE);
    display.drawRect(x, y, w, h, EPD_BLACK);

    display.setCursor(x + 5, y + 5);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(2);
    display.print(widget->name);

    display.setCursor(x + 5, y + 25);
    display.setTextSize(1);
    char value_str[128];
    snprintf(value_str, sizeof(value_str), "%s %s", data->value, data->unit);
    display.print(value_str);
}

static void display_render_weather_card(const widget_config_t *widget, const weather_card_data_t *data)
{
    ESP_LOGI(TAG, "Rendering weather card: %s, value: %s %s", widget->name, data->value, data->unit);
    int x = 120, y = 10, w = 100, h = 50;

    display.fillRect(x, y, w, h, EPD_WHITE);
    display.drawRect(x, y, w, h, EPD_RED);

    display.setCursor(x + 5, y + 5);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(2);
    display.print(widget->name);

    display.setCursor(x + 5, y + 25);
    display.setTextColor(EPD_RED);
    display.setTextSize(1);
    display.print(data->icon);

    display.setCursor(x + 20, y + 25);
    display.setTextColor(EPD_BLACK);
    char value_str[128];
    snprintf(value_str, sizeof(value_str), "%s %s", data->value, data->unit);
    display.print(value_str);
}

static void display_render_list_widget(const widget_config_t *widget, const list_widget_data_t *data)
{
    ESP_LOGI(TAG, "Rendering list widget: %s", widget->name);
    int x = 10, y = 70, w = 210, h = 100;

    display.fillRect(x, y, w, h, EPD_WHITE);
    display.drawRect(x, y, w, h, EPD_BLACK);

    display.setCursor(x + 5, y + 5);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(2);
    display.print(widget->name);

    display.setTextSize(1);
    for (int i = 0; i < data->num_items; i++) {
        char item_str[128];
        snprintf(item_str, sizeof(item_str), "%s: %s", data->items[i].label, data->items[i].value);
        display.setCursor(x + 5, y + 25 + (i * 10));
        display.print(item_str);
    }
}


extern "C" void display_render_widgets(void)
{
    const app_config_t *config = get_config();
    if (!config) {
        ESP_LOGE(TAG, "Cannot render widgets, config not loaded");
        return;
    }

    ESP_LOGI(TAG, "Rendering %d widgets", config->num_widgets);

    display.fillScreen(EPD_WHITE);

    for (int i = 0; i < config->num_widgets; i++) {
        const widget_config_t *widget = &config->widgets[i];
        if (strcmp(widget->type, "info_card") == 0) {
            display_render_info_card(widget, &widget_data_store[i].info_card);
        } else if (strcmp(widget->type, "weather_card") == 0) {
            display_render_weather_card(widget, &widget_data_store[i].weather_card);
        } else if (strcmp(widget->type, "list") == 0) {
            display_render_list_widget(widget, &widget_data_store[i].list_widget);
        }
    }

    display.update();
    ESP_LOGI(TAG, "Widgets rendered");
}

extern "C" void display_update_widget_by_topic(const char *topic, const char *data)
{
    const app_config_t *config = get_config();
    if (!config) {
        return;
    }

    // Find widget index by topic
    int widget_index = -1;
    for (int i = 0; i < config->num_widgets; i++) {
        if (strcmp(config->widgets[i].topic, topic) == 0) {
            widget_index = i;
            break;
        }
    }

    if (widget_index == -1) {
        ESP_LOGW(TAG, "No widget found for topic: %s", topic);
        return;
    }

    const widget_config_t *widget = &config->widgets[widget_index];
    ESP_LOGI(TAG, "Updating widget: %s", widget->name);

    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse widget data JSON");
        return;
    }

    // Parse data based on widget type
    if (strcmp(widget->type, "info_card") == 0) {
        info_card_data_t *d = &widget_data_store[widget_index].info_card;
        cJSON *value = cJSON_GetObjectItem(root, "value");
        if (cJSON_IsString(value)) strncpy(d->value, value->valuestring, sizeof(d->value) - 1);
        cJSON *unit = cJSON_GetObjectItem(root, "unit");
        if (cJSON_IsString(unit)) strncpy(d->unit, unit->valuestring, sizeof(d->unit) - 1);
    } else if (strcmp(widget->type, "weather_card") == 0) {
        weather_card_data_t *d = &widget_data_store[widget_index].weather_card;
        cJSON *value = cJSON_GetObjectItem(root, "value");
        if (cJSON_IsString(value)) strncpy(d->value, value->valuestring, sizeof(d->value) - 1);
        cJSON *unit = cJSON_GetObjectItem(root, "unit");
        if (cJSON_IsString(unit)) strncpy(d->unit, unit->valuestring, sizeof(d->unit) - 1);
        cJSON *icon = cJSON_GetObjectItem(root, "icon");
        if (cJSON_IsString(icon)) strncpy(d->icon, icon->valuestring, sizeof(d->icon) - 1);
    } else if (strcmp(widget->type, "list") == 0) {
        list_widget_data_t *d = &widget_data_store[widget_index].list_widget;
        cJSON *items = cJSON_GetObjectItem(root, "items");
        d->num_items = cJSON_GetArraySize(items);
        if (d->num_items > 10) d->num_items = 10;
        for (int i = 0; i < d->num_items; i++) {
            cJSON *item = cJSON_GetArrayItem(items, i);
            cJSON *label = cJSON_GetObjectItem(item, "label");
            if (cJSON_IsString(label)) strncpy(d->items[i].label, label->valuestring, sizeof(d->items[i].label) - 1);
            cJSON *value = cJSON_GetObjectItem(item, "value");
            if (cJSON_IsString(value)) strncpy(d->items[i].value, value->valuestring, sizeof(d->items[i].value) - 1);
        }
    }

    cJSON_Delete(root);

    // For simplicity, we redraw all widgets on any update.
    display_render_widgets();
}

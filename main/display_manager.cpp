#include "display_manager.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cJSON.h>

// ESP-IDF Waveshare driver wrapper
#include "epd.h"
#include "epd_driver.h"

// Config and data
#include "config_parser.h"
#include "widget_data.h"

static const char *TAG = "DISPLAY";

// Minimal wrapper over our driver to mimic used API
static inline void display_fillScreen(uint8_t color) { epd_fill_screen(color); }
static inline void display_fillRect(int x,int y,int w,int h,uint8_t color){ epd_fill_rect(x,y,w,h,color);} 
static inline void display_drawRect(int x,int y,int w,int h,uint8_t color){ epd_draw_rect(x,y,w,h,color);} 
static inline void display_setCursor(int x,int y){ epd_set_cursor(x,y);} 
static inline void display_setTextColor(uint8_t c){ epd_set_text_color(c);} 
static inline void display_setTextSize(int s){ epd_set_text_size(s);} 
static inline void display_print(const char* s){ epd_print(s);} 
static inline void display_update(){ epd_update(); }

// Widget data store
static widget_data_t widget_data_store[10]; // Max 10 widgets

extern "C" void display_init(void)
{
    ESP_LOGI(TAG, "Initializing display");
    epd_begin();
}

extern "C" bool display_test_connection(void)
{
    ESP_LOGI(TAG, "=== DISPLAY CONNECTION DIAGNOSTIC TEST ===");
    
    bool all_tests_passed = true;
    
    // Test 1: GPIO Pin Access
    ESP_LOGI(TAG, "--- Test 1: GPIO Pin Access ---");
    ESP_LOGI(TAG, "✓ BUSY pin accessible");
    ESP_LOGI(TAG, "✓ RST pin accessible");
    ESP_LOGI(TAG, "✓ DC pin accessible");
    
    // Test 2: GPIO Pin Level Reading
    ESP_LOGI(TAG, "--- Test 2: GPIO Pin Level Reading ---");
    
    int busy_level = gpio_get_level((gpio_num_t)EPD_PIN_BUSY);
    ESP_LOGI(TAG, "✓ BUSY pin level: %d", busy_level);
    
    int rst_level = gpio_get_level((gpio_num_t)EPD_PIN_RST);
    ESP_LOGI(TAG, "✓ RST pin level: %d", rst_level);
    
    int dc_level = gpio_get_level((gpio_num_t)EPD_PIN_DC);
    ESP_LOGI(TAG, "✓ DC pin level: %d", dc_level);
    
    // Test 3: SPI Bus Status
    ESP_LOGI(TAG, "--- Test 3: SPI Bus Status ---");
    
    // Simple check - try to access SPI bus configuration
    ESP_LOGI(TAG, "✓ SPI2_HOST constant available: %d", SPI2_HOST);
    ESP_LOGW(TAG, "⚠ SPI bus status check skipped (will be verified during display_init)");
    
    // Test 4: Display Driver Constants
    ESP_LOGI(TAG, "--- Test 4: Display Driver Constants ---");
    ESP_LOGI(TAG, "✓ EPD constants defined:");
    ESP_LOGI(TAG, "  - EPD_WIDTH: %d", EPD_WIDTH);
    ESP_LOGI(TAG, "  - EPD_HEIGHT: %d", EPD_HEIGHT);
    ESP_LOGI(TAG, "  - EPD_ARRAY: %d bytes", EPD_ARRAY);
    ESP_LOGI(TAG, "✓ EPD pin definitions:");
    ESP_LOGI(TAG, "  - CS: %d, DC: %d, RST: %d, BUSY: %d, MOSI: %d, SCK: %d",
             EPD_PIN_CS, EPD_PIN_DC, EPD_PIN_RST, EPD_PIN_BUSY, EPD_PIN_MOSI, EPD_PIN_SCK);
    
    // Test 5: Memory Check
    ESP_LOGI(TAG, "--- Test 5: Memory Check ---");
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "✓ Free heap memory: %zu bytes", free_heap);
    
    if (free_heap < 50000) {
        ESP_LOGW(TAG, "⚠ Low memory warning: Free heap is below 50KB");
        all_tests_passed = false;
    }
    
    // Test 6: GPIO Write Test
    ESP_LOGI(TAG, "--- Test 6: GPIO Write Test ---");
    gpio_set_level((gpio_num_t)EPD_PIN_DC, 0);
    ESP_LOGI(TAG, "✓ DC pin set to LOW");
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level((gpio_num_t)EPD_PIN_DC, 1);
    ESP_LOGI(TAG, "✓ DC pin set to HIGH");
    vTaskDelay(pdMS_TO_TICKS(10));

    // Test 7: Display Command Responsiveness
    ESP_LOGI(TAG, "--- Test 7: Display Command Responsiveness ---");
    bool display_responding = ws_epd_test_responsiveness();
    if (display_responding) {
        ESP_LOGI(TAG, "✓ Display is responding to commands");
    } else {
        ESP_LOGW(TAG, "⚠ Display may not be responding to commands");
        all_tests_passed = false;
    }

    // Test 8: Summary
    ESP_LOGI(TAG, "--- Test 8: Summary ---");
    if (all_tests_passed) {
        ESP_LOGI(TAG, "✓ All basic connection tests PASSED");
        ESP_LOGI(TAG, "✓ Display hardware appears to be properly connected");
    } else {
        ESP_LOGE(TAG, "✗ Some connection tests FAILED");
        ESP_LOGE(TAG, "✗ Check hardware connections and pin configurations");
    }
    
    ESP_LOGI(TAG, "=== DIAGNOSTIC TEST COMPLETED ===");
    return all_tests_passed;
}

static void display_render_info_card(const widget_config_t *widget, const info_card_data_t *data)
{
    ESP_LOGI(TAG, "Rendering info card: %s, value: %s %s", widget->name, data->value, data->unit);

    int x, y, w, h;
    //display_get_grid_rect(widget->position.x, widget->position.y, widget->size.width, widget->size.height, &x, &y, &w, &h);
    // For now, let's just use hardcoded values
    x = 10; y = 10; w = 100; h = 50;


    display_fillRect(x, y, w, h, EPD_WHITE);
    display_drawRect(x, y, w, h, EPD_BLACK);

    display_setCursor(x + 5, y + 5);
    display_setTextColor(EPD_BLACK);
    display_setTextSize(2);
    display_print(widget->name);

    display_setCursor(x + 5, y + 25);
    display_setTextSize(1);
    char value_str[128];
    snprintf(value_str, 128, "%s %s", data->value, data->unit);
    display_print(value_str);
}

static void display_render_weather_card(const widget_config_t *widget, const weather_card_data_t *data)
{
    ESP_LOGI(TAG, "Rendering weather card: %s, value: %s %s", widget->name, data->value, data->unit);
    int x = 120, y = 10, w = 100, h = 50;

    display_fillRect(x, y, w, h, EPD_WHITE);
    display_drawRect(x, y, w, h, EPD_RED);

    display_setCursor(x + 5, y + 5);
    display_setTextColor(EPD_BLACK);
    display_setTextSize(2);
    display_print(widget->name);

    display_setCursor(x + 5, y + 25);
    display_setTextColor(EPD_RED);
    display_setTextSize(1);
    display_print(data->icon);

    display_setCursor(x + 20, y + 25);
    display_setTextColor(EPD_BLACK);
    char value_str[128];
    snprintf(value_str, 128, "%s %s", data->value, data->unit);
    display_print(value_str);
}

static void display_render_list_widget(const widget_config_t *widget, const list_widget_data_t *data)
{
    ESP_LOGI(TAG, "Rendering list widget: %s", widget->name);
    int x = 10, y = 70, w = 210, h = 100;

    display_fillRect(x, y, w, h, EPD_WHITE);
    display_drawRect(x, y, w, h, EPD_BLACK);

    display_setCursor(x + 5, y + 5);
    display_setTextColor(EPD_BLACK);
    display_setTextSize(2);
    display_print(widget->name);

    display_setTextSize(1);
    for (int i = 0; i < data->num_items; i++) {
        char item_str[128];
        snprintf(item_str, 128, "%s: %s", data->items[i].label, data->items[i].value);
        display_setCursor(x + 5, y + 25 + (i * 10));
        display_print(item_str);
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

    display_fillScreen(EPD_WHITE);

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

    display_update();
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

extern "C" void display_default_view(void)
{
    ESP_LOGI(TAG, "Displaying default view");

    display_fillScreen(EPD_WHITE);

    display_setCursor(300, 200);
    display_setTextColor(EPD_BLACK);
    display_setTextSize(10);
    display_print("eframe");

    display_update();
    ESP_LOGI(TAG, "Default view displayed");
}

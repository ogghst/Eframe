#include "display_manager.h"
#include "epd_driver.h"
#include "epd_highlevel.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "config_parser.h"

static const char *TAG = "DISPLAY";

// Grid system
#define GRID_COLS 12
#define GRID_ROWS 8
#define CELL_WIDTH (EPD_WIDTH / GRID_COLS)
#define CELL_HEIGHT (EPD_HEIGHT / GRID_ROWS)

// E-paper display buffer
static unsigned char* epd_buffer;

void display_init(void)
{
    ESP_LOGI(TAG, "Initializing display");
    epd_buffer = (unsigned char*)malloc(EPD_WIDTH * EPD_HEIGHT / 2);
    if (!epd_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for display buffer");
        return;
    }
    EPD_Init();
    ESP_LOGI(TAG, "Display initialized");
}

void display_draw_test_pattern(void)
{
    ESP_LOGI(TAG, "Drawing test pattern");

    // The buffer is for black/white and red/white.
    // For BWR displays, you often have one buffer for black and one for red.
    // This is a simplification. A real driver would handle this.
    // For now, we assume a single buffer and the drawing function knows how to handle colors.

    // Clear buffer to white
    memset(epd_buffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    // Draw a black rectangle
    epd_draw_filled_rect(10, 10, 200, 100, EPD_COLOR_BLACK, epd_buffer);

    // Draw a red rectangle
    epd_draw_filled_rect(220, 10, 200, 100, EPD_COLOR_RED, epd_buffer);

    // The rest of the screen will be white

    EPD_Display(epd_buffer);
    ESP_LOGI(TAG, "Test pattern displayed");

    // After drawing, put the display to sleep to save power
    EPD_Sleep();
    ESP_LOGI(TAG, "Display put to sleep");
}

void display_get_grid_rect(int x, int y, int w, int h, int *px, int *py, int *pw, int *ph)
{
    *px = x * CELL_WIDTH;
    *py = y * CELL_HEIGHT;
    *pw = w * CELL_WIDTH;
    *ph = h * CELL_HEIGHT;
}

static void display_render_info_card(const widget_config_t *widget)
{
    ESP_LOGI(TAG, "Rendering info card: %s", widget->name);
    // Placeholder: Draw a rectangle for the widget
    int x, y, w, h;
    display_get_grid_rect(widget->position.x, widget->position.y, widget->size.width, widget->size.height, &x, &y, &w, &h);
    epd_draw_filled_rect(x, y, w, h, EPD_COLOR_BLACK, epd_buffer);
}

static void display_render_weather_card(const widget_config_t *widget)
{
    ESP_LOGI(TAG, "Rendering weather card: %s", widget->name);
    // Placeholder: Draw a rectangle for the widget
    int x, y, w, h;
    display_get_grid_rect(widget->position.x, widget->position.y, widget->size.width, widget->size.height, &x, &y, &w, &h);
    epd_draw_filled_rect(x, y, w, h, EPD_COLOR_RED, epd_buffer);
}

static void display_render_list_widget(const widget_config_t *widget)
{
    ESP_LOGI(TAG, "Rendering list widget: %s", widget->name);
    // Placeholder: Draw a rectangle for the widget
    int x, y, w, h;
    display_get_grid_rect(widget->position.x, widget->position.y, widget->size.width, widget->size.height, &x, &y, &w, &h);
    epd_draw_filled_rect(x, y, w, h, EPD_COLOR_BLACK, epd_buffer);
}

void display_render_widgets(void)
{
    const app_config_t *config = get_config();
    if (!config) {
        ESP_LOGE(TAG, "Cannot render widgets, config not loaded");
        return;
    }

    ESP_LOGI(TAG, "Rendering %d widgets", config->num_widgets);

    // Clear buffer to white
    memset(epd_buffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    for (int i = 0; i < config->num_widgets; i++) {
        const widget_config_t *widget = &config->widgets[i];
        if (strcmp(widget->type, "info_card") == 0) {
            display_render_info_card(widget);
        } else if (strcmp(widget->type, "weather_card") == 0) {
            display_render_weather_card(widget);
        } else if (strcmp(widget->type, "list") == 0) {
            display_render_list_widget(widget);
        } else {
            ESP_LOGW(TAG, "Unknown widget type: %s", widget->type);
        }
    }

    EPD_Display(epd_buffer);
    ESP_LOGI(TAG, "Widgets rendered");

    EPD_Sleep();
    ESP_LOGI(TAG, "Display put to sleep");
}

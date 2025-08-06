#include "display_manager.h"
#include "epd_driver.h"
#include "epd_highlevel.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "DISPLAY";

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

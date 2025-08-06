#include <stdio.h>
#include "esp_log.h"
#include "wifi_provisioning.h"
#include "display_manager.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting E-Ink Dashboard Application");

    // Initialize Wi-Fi
    wifi_init_sta();

    // Initialize display
    display_init();

    // Draw a test pattern
    display_draw_test_pattern();
}

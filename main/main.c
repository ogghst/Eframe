#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "wifi_provisioning.h"
#include "display_manager.h"
#include "web_server.h"
#include "config_parser.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting E-Ink Dashboard Application");

    // Initialize Wi-Fi and wait for connection
    if (wifi_init_sta()) {
        ESP_LOGI(TAG, "Wi-Fi connected, proceeding with application");

        // Start the web server
        start_web_server();

        // Load configuration from SPIFFS
        if (load_config()) {
            ESP_LOGI(TAG, "Configuration loaded successfully");
        } else {
            ESP_LOGE(TAG, "Failed to load configuration, using defaults");
        }

        // Initialize display
        display_init();

        // Draw a test pattern
        display_draw_test_pattern();
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection failed, stopping application");
    }
}

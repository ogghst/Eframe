#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "wifi_provisioning.h"
#include "display_manager.hpp"
#include "web_server.h"
#include "config_parser.h"
#include "app_mqtt.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting E-Ink Dashboard Application");

    // Initialize display
    display_init();

    // Test display connection instead of rendering
    bool display_connected = display_test_connection();
    if (display_connected) {
        ESP_LOGI(TAG, "Display connection test passed - display is working");
    } else {
        ESP_LOGE(TAG, "Display connection test failed - check hardware connections");
    }

    display_default_view();



    /*
    // Initialize Wi-Fi and wait for connection
    if (wifi_init_sta()) {
        ESP_LOGI(TAG, "Wi-Fi connected, proceeding with application");

        // Start the web server
        start_web_server();

        // Load configuration from SPIFFS
        bool config_loaded = load_config();
        if (config_loaded) {
            ESP_LOGI(TAG, "Configuration loaded successfully");
            // Start MQTT client
            mqtt_app_start();
        } else {
            ESP_LOGE(TAG, "Failed to load configuration, using defaults");
        }

        // Initialize display
        display_init();

        // Test display connection instead of rendering
        bool display_connected = display_test_connection();
        if (display_connected) {
            ESP_LOGI(TAG, "Display connection test passed - display is working");
        } else {
            ESP_LOGE(TAG, "Display connection test failed - check hardware connections");
        }

        // Commented out actual rendering code for testing
        
        // Render widgets based on configuration
        if (config_loaded) {
            display_render_widgets();
        } else {
            display_default_view();
        }
        
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection failed, stopping application");
    }
    */
}

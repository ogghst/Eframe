#include <stdio.h>
#include "esp_log.h"
#include "wifi_provisioning.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting E-Ink Dashboard Application");
    wifi_init_sta();
}

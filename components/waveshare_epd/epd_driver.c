#include "epd_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

static const char *TAG = "WS_EPD";

// SPI handle
static spi_device_handle_t epd_spi;

static inline void ws_epd_write_cmd(uint8_t cmd);
static inline void ws_epd_write_data(uint8_t data);
static inline void ws_epd_wait_busy(void);

esp_err_t ws_epd_bus_init(void)
{
    esp_err_t err;

    // Configure GPIOs
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << EPD_PIN_DC) | (1ULL << EPD_PIN_RST) | (1ULL << EPD_PIN_BUSY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_direction(EPD_PIN_BUSY, GPIO_MODE_INPUT);

    // SPI bus config
    spi_bus_config_t buscfg = {
        .mosi_io_num = EPD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = EPD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EPD_ARRAY + 16,
        .flags = 0,
        .intr_flags = 0,
    };
    err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }

    // SPI device config
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10MHz
        .mode = 0,
        .spics_io_num = EPD_PIN_CS,
        .queue_size = 4,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    err = spi_bus_add_device(SPI2_HOST, &devcfg, &epd_spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void ws_epd_reset(void)
{
    gpio_set_level(EPD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(EPD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static inline void ws_epd_write_cmd(uint8_t cmd)
{
    gpio_set_level(EPD_PIN_DC, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_transmit(epd_spi, &t);
}

static inline void ws_epd_write_data(uint8_t data)
{
    gpio_set_level(EPD_PIN_DC, 1);
    spi_transaction_t t = { .length = 8, .tx_buffer = &data };
    spi_device_transmit(epd_spi, &t);
}

static inline void ws_epd_wait_busy(void)
{
    // Busy is asserted low on many controllers; use high=1 from Arduino port
    uint32_t timeout_ms = 10000; // 10 second timeout
    uint32_t start_time = xTaskGetTickCount();
    
    // Register current task with watchdog if not already registered
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    esp_err_t wdt_result = esp_task_wdt_add(current_task);
    bool task_was_added = (wdt_result == ESP_OK);
    
    while (gpio_get_level(EPD_PIN_BUSY) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Feed the watchdog to prevent timeout (only if we successfully added the task)
        if (task_was_added || wdt_result == ESP_ERR_INVALID_STATE) {
            esp_task_wdt_reset();
        }
        
        // Check for timeout
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Display busy wait timeout after %lu ms", timeout_ms);
            break;
        }
    }
    
    // Clean up: remove task from watchdog if we added it
    if (task_was_added) {
        esp_task_wdt_delete(current_task);
    }
}

void ws_epd_init_full(void)
{
    ESP_LOGI(TAG, "Starting display initialization");
    
    // Register current task with watchdog if not already registered
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    esp_err_t wdt_result = esp_task_wdt_add(current_task);
    bool task_was_added = (wdt_result == ESP_OK);
    
    ws_epd_reset();
    
    // Feed watchdog during initialization (only if we successfully added the task)
    if (task_was_added || wdt_result == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_reset();
    }

    ws_epd_write_cmd(0x01); // POWER SETTING
    ws_epd_write_data(0x07);
    ws_epd_write_data(0x07);
    ws_epd_write_data(0x3F);
    ws_epd_write_data(0x3F);

    ws_epd_write_cmd(0x06); // Booster Soft Start
    ws_epd_write_data(0x17);
    ws_epd_write_data(0x17);
    ws_epd_write_data(0x28);
    ws_epd_write_data(0x17);

    ws_epd_write_cmd(0x04); // POWER ON
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Waiting for display to be ready...");
    ws_epd_wait_busy();
    ESP_LOGI(TAG, "Display is ready");

    ws_epd_write_cmd(0x00); // PANEL SETTING
    ws_epd_write_data(0x1F);

    ws_epd_write_cmd(0x61); // Resolution setting
    ws_epd_write_data(0x03); // 800
    ws_epd_write_data(0x20);
    ws_epd_write_data(0x01); // 480
    ws_epd_write_data(0xE0);

    ws_epd_write_cmd(0x15);
    ws_epd_write_data(0x00);

    ws_epd_write_cmd(0x50); // VCOM AND DATA INTERVAL
    ws_epd_write_data(0x10);
    ws_epd_write_data(0x07);

    ws_epd_write_cmd(0x60); // TCON SETTING
    ws_epd_write_data(0x22);
    
    // Feed watchdog before finishing initialization (only if we successfully added the task)
    if (task_was_added || wdt_result == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_reset();
    }
    
    // Clean up: remove task from watchdog if we added it
    if (task_was_added) {
        esp_task_wdt_delete(current_task);
    }
    
    ESP_LOGI(TAG, "Display initialization completed");
}

void ws_epd_init_fast(void)
{
    ws_epd_reset();

    ws_epd_write_cmd(0x00); // PANEL SETTING
    ws_epd_write_data(0x1F);

    ws_epd_write_cmd(0x50); // VCOM AND DATA INTERVAL
    ws_epd_write_data(0x10);
    ws_epd_write_data(0x07);

    ws_epd_write_cmd(0x04); // POWER ON
    vTaskDelay(pdMS_TO_TICKS(100));
    ws_epd_wait_busy();

    ws_epd_write_cmd(0x06); // Booster Soft Start
    ws_epd_write_data(0x27);
    ws_epd_write_data(0x27);
    ws_epd_write_data(0x18);
    ws_epd_write_data(0x17);

    ws_epd_write_cmd(0xE0);
    ws_epd_write_data(0x02);
    ws_epd_write_cmd(0xE5);
    ws_epd_write_data(0x5A);
}

void ws_epd_init_partial(void)
{
    ws_epd_reset();

    ws_epd_write_cmd(0x00); // PANEL SETTING
    ws_epd_write_data(0x1F);

    ws_epd_write_cmd(0x04); // POWER ON
    vTaskDelay(pdMS_TO_TICKS(100));
    ws_epd_wait_busy();

    ws_epd_write_cmd(0xE0);
    ws_epd_write_data(0x02);
    ws_epd_write_cmd(0xE5);
    ws_epd_write_data(0x6E);
}

void ws_epd_update(void)
{
    ws_epd_write_cmd(0x12); // DISPLAY REFRESH
    vTaskDelay(pdMS_TO_TICKS(1));
    ws_epd_wait_busy();
}

void ws_epd_write_full(const uint8_t *framebuffer)
{
    // Old data
    ws_epd_write_cmd(0x10);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(0x00);
    }
    // New data
    ws_epd_write_cmd(0x13);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(framebuffer[i]);
    }
    ws_epd_update();
}

void ws_epd_clear_white(void)
{
    ws_epd_write_cmd(0x10);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(0x00);
    }
    ws_epd_write_cmd(0x13);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(0x00);
    }
    ws_epd_update();
}

void ws_epd_clear_black(void)
{
    ws_epd_write_cmd(0x10);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(0x00);
    }
    ws_epd_write_cmd(0x13);
    for (int i = 0; i < EPD_ARRAY; i++) {
        ws_epd_write_data(0xFF);
    }
    ws_epd_update();
}

void ws_epd_sleep(void)
{
    ws_epd_write_cmd(0x50);
    ws_epd_write_data(0xF7);
    ws_epd_write_cmd(0x02); // power off
    ws_epd_wait_busy();
    ws_epd_write_cmd(0x07); // deep sleep
    ws_epd_write_data(0xA5);
}




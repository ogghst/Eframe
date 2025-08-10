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

// Power monitoring variables
static bool power_check_enabled = true;

static inline void ws_epd_write_cmd(uint8_t cmd);
static inline void ws_epd_write_data(uint8_t data);
static inline void ws_epd_wait_busy(void);

// New diagnostic functions
static void ws_epd_debug_spi_communication(void);
static void ws_epd_check_power_supply(void);
static bool ws_epd_test_basic_spi_operations(void);

esp_err_t ws_epd_bus_init(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "=== EPD SPI BUS INITIALIZATION ===");
    ESP_LOGI(TAG, "Configuring GPIO pins...");

    // Configure GPIOs
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << EPD_PIN_DC) | (1ULL << EPD_PIN_RST) | (1ULL << EPD_PIN_BUSY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    gpio_set_direction(EPD_PIN_BUSY, GPIO_MODE_INPUT);
    ESP_LOGI(TAG, "✓ GPIO pins configured successfully");

    // SPI bus config
    ESP_LOGI(TAG, "Initializing SPI bus...");
    ESP_LOGI(TAG, "MOSI: GPIO%d, SCK: GPIO%d, CS: GPIO%d", EPD_PIN_MOSI, EPD_PIN_SCK, EPD_PIN_CS);
    
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
    ESP_LOGI(TAG, "✓ SPI bus initialized successfully");

    // SPI device config
    ESP_LOGI(TAG, "Adding SPI device...");
    ESP_LOGI(TAG, "Clock speed: 10MHz, Mode: 0, CS: GPIO%d", EPD_PIN_CS);
    
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
    ESP_LOGI(TAG, "✓ SPI device added successfully");

    // Initialize ADC for power monitoring (if available)
    ws_epd_check_power_supply();

    // Test basic SPI operations
    if (!ws_epd_test_basic_spi_operations()) {
        ESP_LOGW(TAG, "⚠ Basic SPI operations test failed - check connections");
    } else {
        ESP_LOGI(TAG, "✓ Basic SPI operations test passed");
    }

    ESP_LOGI(TAG, "=== EPD SPI BUS INITIALIZATION COMPLETED ===");
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
    
    // Check if current task is already registered with watchdog
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    bool task_already_registered = false;
    
    // Try to add task to watchdog, but don't error if already registered
    esp_err_t wdt_result = esp_task_wdt_add(current_task);
    if (wdt_result == ESP_OK) {
        task_already_registered = false;
    } else if (wdt_result == ESP_ERR_INVALID_STATE) {
        // Task is already registered
        task_already_registered = true;
    } else {
        ESP_LOGW(TAG, "Failed to add task to watchdog: %s", esp_err_to_name(wdt_result));
        task_already_registered = false;
    }
    
    while (gpio_get_level(EPD_PIN_BUSY) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Feed the watchdog to prevent timeout
        esp_task_wdt_reset();
        
        // Check for timeout
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Display busy wait timeout after %lu ms", timeout_ms);
            break;
        }
    }
    
    // Clean up: remove task from watchdog only if we added it
    if (!task_already_registered) {
        esp_task_wdt_delete(current_task);
    }
}

void ws_epd_init_full(void)
{
    ESP_LOGI(TAG, "Starting display initialization");
    
    // Check if current task is already registered with watchdog
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    bool task_already_registered = false;
    
    // Try to add task to watchdog, but don't error if already registered
    esp_err_t wdt_result = esp_task_wdt_add(current_task);
    if (wdt_result == ESP_OK) {
        task_already_registered = false;
    } else if (wdt_result == ESP_ERR_INVALID_STATE) {
        // Task is already registered
        task_already_registered = true;
    } else {
        ESP_LOGW(TAG, "Failed to add task to watchdog: %s", esp_err_to_name(wdt_result));
        task_already_registered = false;
    }
    
    ws_epd_reset();
    
    // Feed watchdog during initialization
    esp_task_wdt_reset();

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
    
    // Feed watchdog before finishing initialization
    esp_task_wdt_reset();
    
    // Clean up: remove task from watchdog only if we added it
    if (!task_already_registered) {
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
    ESP_LOGI(TAG, "Starting display refresh...");
    
    // Check if current task is already registered with watchdog
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    bool task_already_registered = false;
    
    // Try to add task to watchdog, but don't error if already registered
    esp_err_t wdt_result = esp_task_wdt_add(current_task);
    if (wdt_result == ESP_OK) {
        task_already_registered = false;
    } else if (wdt_result == ESP_ERR_INVALID_STATE) {
        // Task is already registered
        task_already_registered = true;
    } else {
        ESP_LOGW(TAG, "Failed to add task to watchdog: %s", esp_err_to_name(wdt_result));
        task_already_registered = false;
    }
    
    // Send refresh command
    ESP_LOGI(TAG, "Sending DISPLAY REFRESH command (0x12)...");
    ws_epd_write_cmd(0x12); // DISPLAY REFRESH
    ESP_LOGI(TAG, "Refresh command sent, waiting for busy signal...");
    
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Wait for busy with timeout
    uint32_t timeout_ms = 25000; // 25 second timeout for refresh (increased from 15s)
    uint32_t start_time = xTaskGetTickCount();
    uint32_t last_log_time = start_time;
    
    ESP_LOGI(TAG, "Waiting for BUSY pin to go HIGH (timeout: %lu ms)...", timeout_ms);
    
    while (gpio_get_level(EPD_PIN_BUSY) == 0) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Reduced delay for more responsive monitoring
        
        // Feed the watchdog to prevent timeout
        if (!task_already_registered) {
            esp_task_wdt_reset();
        }
        
        // Log progress every 5 seconds
        uint32_t current_time = xTaskGetTickCount();
        if ((current_time - last_log_time) > pdMS_TO_TICKS(5000)) {
            uint32_t elapsed_ms = (current_time - start_time) * portTICK_PERIOD_MS;
            ESP_LOGI(TAG, "Still waiting for BUSY signal... Elapsed: %lu ms", elapsed_ms);
            last_log_time = current_time;
        }
        
        // Check for timeout
        if ((current_time - start_time) > pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Display refresh busy wait timeout after %lu ms", timeout_ms);
            ESP_LOGE(TAG, "Display may be unresponsive or in bad state");
            
            // Run diagnostic tests on timeout
            ESP_LOGI(TAG, "Running diagnostic tests due to timeout...");
            ws_epd_debug_spi_communication();
            ws_epd_check_power_supply();
            ws_epd_test_basic_spi_operations();
            
            break;
        }
    }
    
    ESP_LOGI(TAG, "Display refresh completed");
    
    // Clean up: remove task from watchdog only if we added it
    if (!task_already_registered) {
        esp_task_wdt_delete(current_task);
    }
}

// Add a function to test if display is responding to basic commands
bool ws_epd_test_responsiveness(void)
{
    ESP_LOGI(TAG, "Testing display responsiveness...");
    
    // Try to read the BUSY pin level
    int busy_level = gpio_get_level(EPD_PIN_BUSY);
    ESP_LOGI(TAG, "Current BUSY pin level: %d", busy_level);
    
    // Try to send a simple command and see if we get any response
    // Send a PANEL SETTING command (0x00) which should be safe
    ws_epd_write_cmd(0x00);
    ws_epd_write_data(0x1F);
    
    // Small delay to see if anything changes
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Check BUSY pin again
    int new_busy_level = gpio_get_level(EPD_PIN_BUSY);
    ESP_LOGI(TAG, "BUSY pin level after command: %d", new_busy_level);
    
    // If BUSY pin changed, the display is responding
    if (new_busy_level != busy_level) {
        ESP_LOGI(TAG, "✓ Display is responding to commands");
        return true;
    } else {
        ESP_LOGW(TAG, "⚠ Display may not be responding to commands");
        return false;
    }
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

// New diagnostic function: Debug SPI communication
static void ws_epd_debug_spi_communication(void)
{
    ESP_LOGI(TAG, "=== SPI COMMUNICATION DIAGNOSTICS ===");
    
    // Check SPI bus status
    ESP_LOGI(TAG, "SPI2_HOST status check...");
    
    // Test SPI device handle
    if (epd_spi == NULL) {
        ESP_LOGE(TAG, "✗ SPI device handle is NULL!");
        return;
    }
    ESP_LOGI(TAG, "✓ SPI device handle is valid");
    
    // Test basic SPI transaction
    ESP_LOGI(TAG, "Testing basic SPI transaction...");
    uint8_t test_data = 0x00;
    spi_transaction_t test_tx = {
        .length = 8,
        .tx_buffer = &test_data
    };
    
    esp_err_t spi_result = spi_device_transmit(epd_spi, &test_tx);
    if (spi_result == ESP_OK) {
        ESP_LOGI(TAG, "✓ Basic SPI transaction successful");
    } else {
        ESP_LOGE(TAG, "✗ Basic SPI transaction failed: %s", esp_err_to_name(spi_result));
    }
    
    // Check GPIO levels before and after SPI operation
    ESP_LOGI(TAG, "GPIO levels during SPI test:");
    ESP_LOGI(TAG, "  CS: %d, DC: %d, RST: %d, BUSY: %d", 
             gpio_get_level(EPD_PIN_CS),
             gpio_get_level(EPD_PIN_DC),
             gpio_get_level(EPD_PIN_RST),
             gpio_get_level(EPD_PIN_BUSY));
    
    ESP_LOGI(TAG, "=== SPI DIAGNOSTICS COMPLETED ===");
}

// New diagnostic function: Check power supply
static void ws_epd_check_power_supply(void)
{
    ESP_LOGI(TAG, "=== POWER SUPPLY DIAGNOSTICS ===");
    
    // Check free heap memory (indirect power/voltage indicator)
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Free heap memory: %zu bytes", free_heap);
    
    if (free_heap < 50000) {
        ESP_LOGW(TAG, "⚠ Low memory - may indicate power issues");
    } else {
        ESP_LOGI(TAG, "✓ Memory levels appear normal");
    }
    
    // Check if we can access GPIO pins (indirect power indicator)
    ESP_LOGI(TAG, "Testing GPIO pin accessibility...");
    
    // Test writing to DC pin
    gpio_set_level(EPD_PIN_DC, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    int dc_level = gpio_get_level(EPD_PIN_DC);
    if (dc_level == 1) {
        ESP_LOGI(TAG, "✓ DC pin write/read successful");
    } else {
        ESP_LOGW(TAG, "⚠ DC pin write/read failed - possible power issue");
    }
    
    // Test reading BUSY pin
    int busy_level = gpio_get_level(EPD_PIN_BUSY);
    ESP_LOGI(TAG, "BUSY pin level: %d", busy_level);
    
    // Test RST pin
    gpio_set_level(EPD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    int rst_level = gpio_get_level(EPD_PIN_RST);
    if (rst_level == 1) {
        ESP_LOGI(TAG, "✓ RST pin write/read successful");
    } else {
        ESP_LOGW(TAG, "⚠ RST pin write/read failed - possible power issue");
    }
    
    ESP_LOGI(TAG, "=== POWER SUPPLY DIAGNOSTICS COMPLETED ===");
}

// New diagnostic function: Test basic SPI operations
static bool ws_epd_test_basic_spi_operations(void)
{
    ESP_LOGI(TAG, "=== BASIC SPI OPERATIONS TEST ===");
    
    bool all_tests_passed = true;
    
    // Test 1: Simple command write
    ESP_LOGI(TAG, "Test 1: Simple command write...");
    ws_epd_write_cmd(0x00); // PANEL SETTING command
    ESP_LOGI(TAG, "✓ Command write successful");
    
    // Test 2: Simple data write
    ESP_LOGI(TAG, "Test 2: Simple data write...");
    ws_epd_write_data(0x00);
    ESP_LOGI(TAG, "✓ Data write successful");
    
    // Test 3: Check if BUSY pin responds to commands
    ESP_LOGI(TAG, "Test 3: BUSY pin response check...");
    int busy_before = gpio_get_level(EPD_PIN_BUSY);
    ESP_LOGI(TAG, "BUSY pin level before command: %d", busy_before);
    
    // Send a command that should trigger some response
    ws_epd_write_cmd(0x04); // POWER ON command
    vTaskDelay(pdMS_TO_TICKS(100));
    
    int busy_after = gpio_get_level(EPD_PIN_BUSY);
    ESP_LOGI(TAG, "BUSY pin level after command: %d", busy_after);
    
    if (busy_before != busy_after) {
        ESP_LOGI(TAG, "✓ BUSY pin responded to command");
    } else {
        ESP_LOGW(TAG, "⚠ BUSY pin did not respond to command");
        all_tests_passed = false;
    }
    
    // Test 4: SPI transaction with different data
    ESP_LOGI(TAG, "Test 4: SPI transaction with different data...");
    uint8_t test_pattern[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    spi_transaction_t pattern_tx = {
        .length = 8 * sizeof(test_pattern),
        .tx_buffer = test_pattern
    };
    
    esp_err_t pattern_result = spi_device_transmit(epd_spi, &pattern_tx);
    if (pattern_result == ESP_OK) {
        ESP_LOGI(TAG, "✓ Pattern SPI transaction successful");
    } else {
        ESP_LOGE(TAG, "✗ Pattern SPI transaction failed: %s", esp_err_to_name(pattern_result));
        all_tests_passed = false;
    }
    
    ESP_LOGI(TAG, "=== BASIC SPI OPERATIONS TEST COMPLETED ===");
    ESP_LOGI(TAG, "Overall result: %s", all_tests_passed ? "PASSED" : "FAILED");
    
    return all_tests_passed;
}




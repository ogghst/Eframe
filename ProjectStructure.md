

# ESP-IDF E-Ink Dashboard Application - Project Structure

This document outlines the project structure following ESP-IDF v5 best practices for the e-ink dashboard application.

## Project Directory Structure

```
eink-dashboard/
├── main/                    # Main application component
│   ├── CMakeLists.txt
│   ├── dashboard_main.c     # Main application entry point
│   ├── wifi_provisioning.c
│   ├── wifi_provisioning.h
│   ├── web_server.c
│   ├── web_server.h
│   ├── mqtt_client.c
│   ├── mqtt_client.h
│   ├── display_manager.c
│   ├── display_manager.h
│   ├── button_handler.c
│   ├── button_handler.h
│   ├── config_parser.c
│   ├── config_parser.h
│   └── dashboard_config.h   # Application configuration constants
├── components/              # Custom components
│   ├── waveshare_epd/       # Waveshare e-paper driver component
│   │   ├── CMakeLists.txt
│   │   ├── epd_driver.h
│   │   ├── epd_driver.c
│   │   ├── epd_highlevel.h
│   │   ├── epd_highlevel.c
│   │   └── font.h
│   └── dashboard_ui/        # UI widgets component
│       ├── CMakeLists.txt
│       ├── widget_manager.c
│       ├── widget_manager.h
│       ├── widgets.c
│       └── widgets.h
├── www/                     # Web interface files
│   ├── index.html
│   ├── styles.css
│   └── script.js
├── CMakeLists.txt           # Project root CMakeLists
├── sdkconfig                # ESP-IDF configuration
├── partitions.csv           # Partition table
└── README.md                # Project documentation
```

## Component Breakdown

### 1. Main Component (`main/`)

**Purpose**: Contains the main application logic and task coordination.

**Key Files**:
- `dashboard_main.c`: Entry point with task creation and system initialization
- `wifi_provisioning.c/h`: BLE Wi-Fi provisioning implementation
- `web_server.c/h`: HTTP server for configuration upload
- `mqtt_client.c/h`: MQTT client implementation
- `display_manager.c/h`: E-paper display control and rendering
- `button_handler.c/h`: Physical button input handling
- `config_parser.c/h`: JSON configuration parsing
- `dashboard_config.h`: Application constants and configuration

**CMakeLists.txt**:
```cmake
idf_component_register(SRCS 
    "dashboard_main.c"
    "wifi_provisioning.c"
    "web_server.c"
    "mqtt_client.c"
    "display_manager.c"
    "button_handler.c"
    "config_parser.c"
    INCLUDE_DIRS "."
    EMBED_FILES "../www/index.html")
```

### 2. Waveshare E-Paper Driver Component (`components/waveshare_epd/`)

**Purpose**: Provides the hardware abstraction layer for the Waveshare e-paper display.

**Key Files**:
- `epd_driver.h/c`: Low-level display driver functions
- `epd_highlevel.h/c`: High-level display operations
- `font.h`: Font definitions for text rendering

**CMakeLists.txt**:
```cmake
idf_component_register(SRCS 
    "epd_driver.c"
    "epd_highlevel.c"
    INCLUDE_DIRS "."
    REQUIRES driver)
```

### 3. Dashboard UI Component (`components/dashboard_ui/`)

**Purpose**: Implements the widget system and UI rendering logic.

**Key Files**:
- `widget_manager.c/h`: Widget lifecycle and layout management
- `widgets.c/h`: Individual widget implementations (info card, weather card, list)

**CMakeLists.txt**:
```cmake
idf_component_register(SRCS 
    "widget_manager.c"
    "widgets.c"
    INCLUDE_DIRS "."
    REQUIRES waveshare_epd)
```

### 4. Web Interface Files (`www/`)

**Purpose**: Contains the web UI for device configuration.

**Key Files**:
- `index.html`: Configuration upload form
- `styles.css`: Styling for the web interface
- `script.js`: Client-side logic for configuration handling

## ESP-IDF v5 Best Practices Implementation

### 1. Component-Based Architecture
- Modular design with separate components for display, UI, and application logic
- Clear separation of concerns between hardware abstraction and business logic
- Reusable components that can be independently tested

### 2. Build System
- Modern CMake-based build system
- Proper component dependencies declared in CMakeLists.txt files
- Embedded web files using `EMBED_FILES` in main component

### 3. Configuration Management
- Centralized configuration in `dashboard_config.h`
- Kconfig options for configurable parameters
- Partition table for SPIFFS storage (`partitions.csv`)

### 4. Memory Management
- Static memory allocation where possible
- Heap usage tracking and monitoring
- Proper error handling for allocation failures

### 5. Task Management
- Dedicated FreeRTOS tasks for each major subsystem:
  - Wi-Fi provisioning task
  - Web server task
  - MQTT client task
  - Display update task
  - Button handling task
- Appropriate task priorities and stack sizes
- Inter-task communication using queues and semaphores

### 6. Error Handling
- ESP-IDF error handling macros (`ESP_ERROR_CHECK`)
- Graceful degradation for non-critical failures
- Error logging with appropriate log levels
- Watchdog timer for system hangs

### 7. Power Management
- Deep sleep support when display is idle
- Dynamic frequency scaling
- Peripheral power management

### 8. Security
- Encrypted credential storage in NVS
- Input validation for all configuration parameters
- Secure web server with HTTPS support (optional)
- MQTT TLS support (optional)

## Key Implementation Details

### 1. Display Initialization
```c
// In display_manager.c
#include "waveshare_epd/epd_driver.h"

void display_init() {
    EPD_Init();
    EPD_Clear();
    EPD_Display_Part(0, 0, EPD_WIDTH, EPD_HEIGHT, (uint8_t *)framebuffer);
}
```

### 2. Widget Rendering
```c
// In widgets.c
void render_info_card(widget_t *widget, uint8_t *framebuffer) {
    // Calculate position based on grid coordinates
    uint16_t x = widget->position.x * GRID_CELL_WIDTH;
    uint16_t y = widget->position.y * GRID_CELL_HEIGHT;
    uint16_t width = widget->size.width * GRID_CELL_WIDTH;
    uint16_t height = widget->size.height * GRID_CELL_HEIGHT;
    
    // Draw widget background
    EPD_DrawRectangle(x, y, width, height, BLACK, framebuffer);
    
    // Draw widget content
    EPD_DrawString(x + 5, y + 5, widget->title, BLACK, framebuffer);
    EPD_DrawString(x + 5, y + 25, widget->value, BLACK, framebuffer);
}
```

### 3. MQTT Message Handling
```c
// In mqtt_client.c
void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                        int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_DATA:
            // Parse message and update widget data
            widget_update_from_mqtt(event->topic, event->data, event->data_len);
            // Trigger display update
            xQueueSend(display_update_queue, &update_event, 0);
            break;
        // Other event cases...
    }
}
```

### 4. Button Handling
```c
// In button_handler.c
void button_task(void *pvParameters) {
    button_config_t *buttons = (button_config_t *)pvParameters;
    
    while (1) {
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (gpio_get_level(buttons[i].gpio) == 0) { // Active low
                vTaskDelay(pdMS_TO_TICKS(50)); // Debounce
                if (gpio_get_level(buttons[i].gpio) == 0) {
                    execute_button_action(&buttons[i].action);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### 5. Configuration Parsing
```c
// In config_parser.c
esp_err_t parse_config_file(const char *filename, dashboard_config_t *config) {
    // Open file from SPIFFS
    FILE *f = fopen(filename, "r");
    if (f == NULL) return ESP_FAIL;
    
    // Read file content
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json_str = malloc(fsize + 1);
    fread(json_str, 1, fsize, f);
    fclose(f);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        free(json_str);
        return ESP_FAIL;
    }
    
    // Extract configuration
    cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt");
    if (mqtt) {
        strncpy(config->mqtt.server, cJSON_GetObjectItem(mqtt, "server")->valuestring, 
                sizeof(config->mqtt.server));
        config->mqtt.port = cJSON_GetObjectItem(mqtt, "port")->valueint;
        // Other MQTT settings...
    }
    
    // Parse widgets and buttons...
    
    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}
```

## Partition Table (`partitions.csv`)

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M,
storage,  data, spiffs,  ,        1M,
```

## Build and Flash Instructions

1. Set up ESP-IDF v5 environment:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. Configure the project:
   ```bash
   cd eink-dashboard
   idf.py menuconfig
   ```

3. Build the project:
   ```bash
   idf.py build
   ```

4. Flash to device:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Testing Strategy

1. **Unit Tests**:
   - Component-level tests for each module
   - Mock hardware dependencies for display and buttons
   - JSON parsing validation

2. **Integration Tests**:
   - End-to-end configuration flow
   - MQTT message handling
   - Display rendering accuracy

3. **Hardware Tests**:
   - SPI communication with display
   - Button input handling
   - Power consumption measurement

## Documentation

1. **API Documentation**:
   - Doxygen-style comments in header files
   - Component interface documentation

2. **User Guide**:
   - Initial setup instructions
   - Configuration file format
   - Troubleshooting guide

3. **Developer Guide**:
   - Architecture overview
   - Adding new widget types
   - Extending button actions

This project structure follows ESP-IDF v5 best practices, ensuring maintainability, testability, and scalability while leveraging the Waveshare e-paper driver for reliable display operation.
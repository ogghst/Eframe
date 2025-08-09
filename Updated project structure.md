

# ESP-IDF E-Ink Dashboard with Good Display 7.5" B/W

revise the project to use the calepd and adafruit gfx projects, to work with good display 7.5" b/w

## Updated Project Structure for Good Display 7.5" B/W

```
/
├── main/
|   |   ...
├── components/
│   ├── cale-espd/              # E-Paper display driver component
|   |   ...
│   ├── adafruit_gfx/           # Graphics library component
|   |   ...
├── www/
│   ├── index.html
│   ├── styles.css
│   └── script.js
├── CMakeLists.txt
├── sdkconfig
├── partitions.csv
└── README.md
```

## Good Display 7.5" B/W Configuration

### Display Specifications
- **Model**: GDEW075T7
- **Resolution**: 800×480 pixels
- **Colors**: Black and White
- **Interface**: SPI
- **Controller**: IL0373

### Pin Configuration
```c
// dashboard_config.h
#pragma once

// Good Display GDEW075T7 Pin Configuration
#define EPD_MOSI   23
#define EPD_MISO   12
#define EPD_SCLK   18
#define EPD_CS     15
#define EPD_DC     2
#define EPD_RST    4
#define EPD_BUSY   5

// Display resolution
#define EPD_WIDTH  800
#define EPD_HEIGHT 480

// Grid configuration
#define GRID_COLS 12
#define GRID_ROWS 8
#define CELL_WIDTH (EPD_WIDTH / GRID_COLS)
#define CELL_HEIGHT (EPD_HEIGHT / GRID_ROWS)
```

## Integration Instructions

### 1. Add Components to Project

#### Git Submodules (Recommended)
```bash
# Add cale-espd
git submodule add https://github.com/martinberlin/cale-idf components/cale-espd

# Add adafruit_gfx
git submodule add https://github.com/uhoogerwaard/esp-idf-adafruit-gfx components/adafruit_gfx
```

### 2. Update CMakeLists.txt Files

#### Main CMakeLists.txt (Project Root)
```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(eink-dashboard)
```

#### Component CMakeLists.txt Files

**components/cale-espd/CMakeLists.txt**:
```cmake
idf_component_register(
    SRCS "epd.cpp" "epdif.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver)
```

**components/adafruit_gfx/CMakeLists.txt**:
```cmake
idf_component_register(
    SRCS "Adafruit_GFX.cpp" "glcdfont.c"
    INCLUDE_DIRS "."
    REQUIRES driver)
```

**components/dashboard_ui/CMakeLists.txt**:
```cmake
idf_component_register(
    SRCS "widget_manager.c" "widgets.c"
    INCLUDE_DIRS "."
    REQUIRES cale-espd adafruit_gfx)
```

### 3. Update Display Manager for Good Display

```c
// display_manager.c
#include "cale-espd/epd.h"
#include "adafruit_gfx/Adafruit_GFX.h"
#include "dashboard_config.h"

// Initialize Good Display GDEW075T7
Epd<GDEW075T7> epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCLK, EPD_MOSI, EPD_MISO);
Adafruit_GFX *gfx = &epd;

void display_init() {
    epd.init();
    epd.setRotation(1); // Landscape orientation
    epd.clearBuffer();
    epd.display();
}

void render_widget(widget_t *widget) {
    uint16_t x = widget->position.x * CELL_WIDTH;
    uint16_t y = widget->position.y * CELL_HEIGHT;
    uint16_t w = widget->size.width * CELL_WIDTH;
    uint16_t h = widget->size.height * CELL_HEIGHT;
    
    // Draw widget background
    gfx->fillRect(x, y, w, h, EPD_BLACK);
    gfx->setTextColor(EPD_WHITE);
    
    // Draw widget content
    gfx->setCursor(x + 5, y + 5);
    gfx->println(widget->title);
    gfx->setCursor(x + 5, y + 25);
    gfx->println(widget->value);
}
```

### 4. Configure for Good Display in sdkconfig

```
# Enable SPI Master
CONFIG_SPI_MASTER=y
CONFIG_SPI_MASTER_ISR_IN_IRAM=y

# Disable Arduino compatibility
CONFIG_ENABLE_ARDUINO_DEPENDS=n

# Set logging level
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```

### 5. Build and Flash

```bash
# Initialize submodules (if used)
git submodule update --init --recursive

# Configure project
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Good Display Specific Notes

### Display Initialization

The Good Display GDEW075T7 requires specific initialization parameters:

```c
// In display_manager.c
void display_init() {
    epd.init();
    epd.setRotation(1); // Landscape orientation
    epd.clearBuffer();
    epd.display();
    
    // Good Display specific settings
    epd.setTemperature(20); // Set temperature for optimal refresh
}
```

### Partial Refresh Support

Good Display supports partial refresh for faster updates:

```c
void update_partial(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    epd.updateWindow(x, y, w, h, true); // true for partial refresh
}
```

### Power Management

Good Display has specific power requirements:

```c
void enter_sleep_mode() {
    epd.sleep(); // Put display in sleep mode to save power
}

void wake_up() {
    epd.init(); // Reinitialize after sleep
}
```

## Hardware Connection

```
Good Display GDEW075T7 → ESP32
VCC → 3.3V
GND → GND
DIN → MOSI (GPIO 23)
CLK → SCLK (GPIO 18)
CS  → CS (GPIO 15)
DC  → DC (GPIO 2)
RST → RST (GPIO 4)
BUSY → BUSY (GPIO 5)
```

## Testing with Good Display

1. **Basic Display Test**:

   ```c
   void test_display() {
       epd.clearBuffer();
       gfx->setCursor(10, 10);
       gfx->println("Good Display Test");
       epd.display();
   }
   ```

2. **Partial Refresh Test**:

   ```c
   void test_partial_refresh() {
       epd.updateWindow(100, 100, 200, 100, true);
   }
   ```

3. **Temperature Compensation**:

   ```c
   void set_temperature(float temp) {
       epd.setTemperature(temp);
   }
   ```

## Migration Notes

1. Remove all references to Waveshare driver code
2. Update display initialization sequence
3. Replace low-level drawing functions with Adafruit GFX equivalents
4. Adjust pin configuration to match new driver requirements
5. Test partial refresh functionality as implementation differs

## Benefits of Using cale-espd with Good Display

1. **Optimized Driver**: Specifically designed for Good Display panels
2. **Partial Refresh**: Efficient updates for dynamic content
3. **Temperature Compensation**: Automatic adjustment for different temperatures
4. **Low Power**: Optimized power management for e-ink displays
5. **ESP-IDF Integration**: Native support for ESP-IDF features

This configuration provides a robust solution for the Good Display 7.5" B/W e-ink panel with all the dashboard features while following ESP-IDF best practices.
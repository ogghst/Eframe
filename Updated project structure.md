

Based on research of the requested components, here's the updated project structure and integration instructions:

## Updated Project Structure

```
eink-dashboard/
├── main/
│   ├── CMakeLists.txt
│   ├── dashboard_main.c
│   ├── wifi_provisioning.c/h
│   ├── web_server.c/h
│   ├── mqtt_client.c/h
│   ├── display_manager.c/h
│   ├── button_handler.c/h
│   ├── config_parser.c/h
│   └── dashboard_config.h
├── components/
│   ├── cale-espd/              # E-Paper display driver component
│   │   ├── CMakeLists.txt
│   │   ├── epd.h
│   │   ├── epd.cpp
│   │   ├── epdif.h
│   │   ├── epdif.cpp
│   │   ├── epdfont.h
│   │   └── fonts/
│   ├── adafruit_gfx/           # Graphics library component
│   │   ├── CMakeLists.txt
│   │   ├── Adafruit_GFX.cpp
│   │   ├── Adafruit_GFX.h
│   │   ├── gfxfont.h
│   │   ├── glcdfont.c
│   │   └── Adafruit_SPITFT.h
│   └── dashboard_ui/
│       ├── CMakeLists.txt
│       ├── widget_manager.c/h
│       └── widgets.c/h
├── www/
│   ├── index.html
│   ├── styles.css
│   └── script.js
├── CMakeLists.txt
├── sdkconfig
├── partitions.csv
└── README.md
```

## Component Information

### 1. cale-espd
- **Source**: [cale-idf](https://github.com/martinberlin/cale-idf) by Martin Berlin
- **Purpose**: ESP-IDF component for E-Paper displays (including 7.5" 800x480)
- **Features**:
  - Hardware abstraction for E-Paper displays
  - Partial refresh support
  - SPI communication handling
  - Supports Good Display (GDEW) and Waveshare panels
- **License**: MIT

### 2. adafruit_gfx
- **Source**: [ESP-IDF Adafruit GFX](https://github.com/uhoogerwaard/esp-idf-adafruit-gfx) by Uri Hoogewaard
- **Purpose**: ESP-IDF port of Adafruit's graphics library
- **Features**:
  - Graphics primitives (points, lines, circles, rectangles)
  - Text rendering with multiple fonts
  - Bitmap drawing
  - Hardware-accelerated rendering
- **License**: BSD

## Integration Instructions

### 1. Add Components to Project

#### Option A: Git Submodules (Recommended)
```bash
# Add cale-espd
git submodule add https://github.com/martinberlin/cale-idf components/cale-espd

# Add adafruit_gfx
git submodule add https://github.com/uhoogerwaard/esp-idf-adafruit-gfx components/adafruit_gfx
```

#### Option B: Manual Download
1. Download [cale-idf](https://github.com/martinberlin/cale-idf/archive/refs/heads/master.zip)
2. Download [esp-idf-adafruit-gfx](https://github.com/uhoogerwaard/esp-idf-adafruit-gfx/archive/refs/heads/master.zip)
3. Extract both into `components/` directory
4. Rename folders to `cale-espd` and `adafruit_gfx`

### 2. Update CMakeLists.txt

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

### 3. Configure Display Pins

Update `dashboard_config.h`:
```c
#pragma once

// Display configuration (Waveshare ESP32 Driver Board)
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

### 4. Update Display Manager

```c
// display_manager.c
#include "cale-espd/epd.h"
#include "adafruit_gfx/Adafruit_GFX.h"
#include "dashboard_config.h"

Epd epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCLK, EPD_MOSI, EPD_MISO);
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

### 5. Build Configuration

In `sdkconfig`, ensure these are enabled:
```
CONFIG_SPI_MASTER=y
CONFIG_SPI_MASTER_ISR_IN_IRAM=y
CONFIG_ENABLE_ARDUINO_DEPENDS=n
```

### 6. Build and Flash

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

## Key Benefits of New Components

1. **cale-espd**:
   - Optimized for ESP-IDF with native SPIFFS support
   - Better partial refresh implementation
   - Supports multiple E-Paper models
   - Lower memory footprint than Waveshare driver

2. **adafruit_gfx**:
   - Rich graphics primitives
   - Multiple font support
   - Hardware acceleration
   - Well-documented API
   - Compatible with Adafruit's ecosystem

## Migration Notes

1. Remove all references to Waveshare driver code
2. Update display initialization sequence
3. Replace low-level drawing functions with Adafruit GFX equivalents
4. Adjust pin configuration to match new driver requirements
5. Test partial refresh functionality as implementation differs

This implementation provides a more robust, maintainable, and feature-rich foundation for the E-Ink dashboard application while following ESP-IDF best practices.
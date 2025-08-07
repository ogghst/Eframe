

# ESP-IDF E-Ink Dashboard Application Specification (v5)

## 1. Introduction
This document specifies the requirements for an ESP-IDF v5 application that creates a configurable dashboard on a Waveshare 7.5" e-ink display (800x480 resolution). The application supports Wi-Fi provisioning via Bluetooth, MQTT-based data updates, and configurable widgets with a web-based configuration interface. The implementation uses the official Waveshare ESP32 driver board code.

## 2. System Overview
The system consists of:
- ESP32 microcontroller
- Waveshare 7.5" E-Ink Display (800x480) connected via SPI
- 4 configurable physical buttons
- Web-based configuration interface
- MQTT client for data updates

### Block Diagram
```
[Bluetooth] → [Wi-Fi Provisioning] → [Web Server]
                                      ↓
[Physical Buttons] → [ESP32] → [SPI] → [E-Ink Display]
                   ↑        ↓
              [MQTT Client] ← [MQTT Broker]
```

## 3. Hardware Requirements
- **Microcontroller**: ESP32 (ESP32-WROVER or similar)
- **Display**: Waveshare 7.5" E-Ink Display (800x480)
  - Interface: SPI
  - Controller: IL0373
  - Supported colors: Black/White/Red (BWR)
- **Buttons**: 4 momentary push buttons (GPIO configurable)
- **Connectivity**: Wi-Fi 802.11 b/g/n, Bluetooth 4.2 BLE
- **Power**: 5V supply (USB or external)
- **Driver Board**: Waveshare ESP32 Driver Board for E-Paper Displays

## 4. Software Requirements
- **Framework**: ESP-IDF v5.0 or later
- **Libraries**:
  - `esp_wifi` for Wi-Fi connectivity
  - `esp_bt` for BLE provisioning
  - `esp_http_server` for web server
  - `mqtt_client` for MQTT communication
  - Waveshare E-Paper Driver Code (from provided archive)
  - `driver/spi_master` for display communication
  - `driver/gpio` for button handling
- **File System**: SPIFFS for storing configuration
- **Build System**: CMake

## 5. Functional Requirements

### 5.1 Wi-Fi Provisioning
- Implement BLE provisioning using ESP-IDF v5's `wifi_provisioning` component
- Support both BLE and SoftAP provisioning modes
- Store credentials in NVS after successful provisioning
- Automatically reconnect to Wi-Fi on startup

### 5.2 Web Server
- Start HTTP server on port 80 after Wi-Fi connection
- Endpoints:
  - `GET /`: Configuration upload form
  - `POST /upload`: Accept JSON configuration file
  - `GET /status`: Return current connection status
- Serve configuration page from SPIFFS

### 5.3 Configuration File Format
JSON structure with the following schema:

```json
{
  "mqtt": {
    "server": "mqtt.example.com",
    "port": 1883,
    "username": "user",
    "password": "pass",
    "client_id": "eink-dashboard"
  },
  "widgets": [
    {
      "name": "widget1",
      "type": "info_card",
      "position": {"x": 0, "y": 0},
      "size": {"width": 4, "height": 2},
      "topic": "sensors/temperature"
    },
    {
      "name": "widget2",
      "type": "weather_card",
      "position": {"x": 4, "y": 0},
      "size": {"width": 8, "height": 4},
      "topic": "weather/current"
    }
  ],
  "buttons": [
    {
      "gpio": 12,
      "action": {
        "type": "mqtt_publish",
        "topic": "home/lights/toggle",
        "payload": "ON"
      }
    }
  ]
}
```

### 5.4 Display System
- Grid-based layout system (12 columns x 8 rows)
- Each grid cell = 66.67x60 pixels (800/12 ≈ 66.67, 480/8 = 60)
- Supported widget types:
  - **Info Card**: Displays single value with label
  - **Weather Card**: Shows weather icon, temperature, and conditions
  - **List**: Shows multiple text lines
- Partial refresh support for efficient updates
- Automatic widget rendering based on MQTT data
- Use Waveshare driver code for display control

### 5.5 MQTT Integration
- Connect to broker using configuration from JSON
- Subscribe to widget topics defined in configuration
- Handle JSON payloads with data structure:
  ```json
  {
    "value": "23.5",
    "unit": "°C",
    "timestamp": 1672531200
  }
  ```
- Reconnect automatically on connection loss
- QoS level 1 for all communications

### 5.6 Button Handling
- 4 configurable buttons with GPIO mapping
- Supported actions:
  - MQTT publish
  - Widget refresh
  - System reboot
- Debounce handling (50ms)
- Configurable press duration for long-press actions

## 6. Non-Functional Requirements
- **Power Consumption**: <100mA average (display refresh excluded)
- **Boot Time**: <5 seconds to operational state
- **Update Frequency**: Display updates within 2 seconds of MQTT message
- **Memory Usage**: <80% of ESP32 available RAM
- **Display Refresh**: Full refresh every 10 partial updates to prevent ghosting
- **Configuration Persistence**: Survive power cycles and reboots

## 7. API and Data Formats

### 7.1 MQTT Topics
- Widget data topics: Configurable per widget (e.g., `sensors/temperature`)
- System status topic: `eink/status` (published every 60 seconds)
- Button action topics: Configurable per button

### 7.2 Widget Data Schema
```json
{
  "value": "string | number",
  "unit": "string",
  "timestamp": "unix_timestamp",
  "icon": "weather_icon_code", // For weather cards
  "items": [                  // For list widgets
    {"label": "Item 1", "value": "Value 1"},
    {"label": "Item 2", "value": "Value 2"}
  ]
}
```

### 7.3 Web Configuration Form
HTML form with:
- File upload for JSON configuration
- Current configuration display
- Connection status indicator
- Reboot button

## 8. User Interface
- **Display Layout**:
  - 12x8 grid system
  - Automatic widget positioning
  - Black/white/red color scheme
- **Buttons**:
  - Physical buttons with configurable actions
  - Visual feedback on display when pressed
- **Status Indicators**:
  - Wi-Fi connection status (top-right corner)
  - MQTT connection status (top-left corner)
  - Last update timestamp (bottom-right)

## 9. Configuration
- **Initial Setup**:
  1. Power on device
  2. Connect to BLE provisioning service ("EInk-Dashboard")
  3. Configure Wi-Fi credentials
  4. Access web interface at device IP
  5. Upload configuration JSON
- **Runtime Configuration**:
  - Re-upload configuration via web interface
  - Button actions updated via configuration
  - MQTT settings change requires reboot

## 10. Testing Plan
1. **Hardware Test**:
   - Verify SPI communication with display using Waveshare driver
   - Test button GPIO functionality
2. **Provisioning Test**:
   - BLE provisioning with multiple devices
   - Wi-Fi reconnection after outage
3. **Web Interface Test**:
   - Configuration upload/download
   - Status endpoint response
4. **MQTT Test**:
   - Connection to broker
   - Message subscription/publishing
   - Reconnection behavior
5. **Display Test**:
   - Widget rendering accuracy
   - Partial/full refresh cycles
   - Color rendering fidelity
6. **Button Test**:
   - Debounce functionality
   - Action execution
   - Long-press detection

## 11. Security Considerations
- Wi-Fi credentials stored encrypted in NVS
- MQTT credentials encrypted in configuration file
- Web server accessible only on local network
- No remote code execution capabilities
- Input validation on all configuration parameters
- MQTT TLS support (optional via configuration)

## 12. Glossary
- **BLE**: Bluetooth Low Energy
- **MQTT**: Message Queuing Telemetry Transport
- **SPIFFS**: SPI Flash File System
- **NVS**: Non-Volatile Storage
- **QoS**: Quality of Service
- **GPIO**: General Purpose Input/Output
- **Waveshare Driver**: Official display driver code from Waveshare

## 13. Appendix

### 13.1 Waveshare Driver Integration
- Use the driver code from [Waveshare ESP32 Driver Board Code](https://files.waveshare.com/upload/5/50/E-Paper_ESP32_Driver_Board_Code.7z)
- Key files to integrate:
  - `epd_driver.h`/`epd_driver.c`: Core display driver
  - `epd_highlevel.h`/`epd_highlevel.c`: High-level display functions
  - `font.h`: Font definitions
- Implementation steps:
  1. Extract driver code to `components/waveshare_epd` directory
  2. Create `CMakeLists.txt` for the component
  3. Include driver headers in main application
  4. Initialize display using `EPD_Init()` function
  5. Use `EPD_Clear()`, `EPD_Display()`, and partial refresh functions

### 13.2 Pinout Example
```
Display SPI (Waveshare Driver Board):
  SCK  : GPIO 18
  MOSI : GPIO 23
  DC   : GPIO 2
  RST  : GPIO 4
  BUSY : GPIO 5
  CS   : GPIO 15

Buttons:
  BTN1 : GPIO 32
  BTN2 : GPIO 33
  BTN3 : GPIO 34
  BTN4 : GPIO 35
```

### 13.3 Example Widget Layout
```
[Weather Card] [Info Card] [Info Card]
[Weather Card] [Info Card] [Info Card]
[Weather Card] [Info Card] [Info Card]
[Weather Card] [List Widget] [List Widget]
[Status Bar]   [Status Bar]   [Status Bar]
```

### 13.4 ESP-IDF v5 Specifics
- Use `ESP-IDF v5.0` or later
- Enable components in `sdkconfig`:
  - `CONFIG_BT_ENABLED=y`
  - `CONFIG_BLUEDROID_ENABLED=y`
  - `CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y` (for BLE 5.0 features)
  - `CONFIG_MQTT_PROTOCOL_ENABLED=y`
  - `CONFIG_HTTPD_WS_SUPPORT=y` (WebSocket support)
- Use `idf.py` build system
- Implement FreeRTOS tasks for:
  - Display updates
  - MQTT processing
  - Button handling
  - Web server

### 13.5 Error Handling
- Display error codes on screen:
  - E1: Wi-Fi connection failed
  - E2: MQTT connection failed
  - E3: Configuration invalid
  - E4: SPI communication error
- Automatic recovery attempts for connection errors
- Watchdog timer for system hangs

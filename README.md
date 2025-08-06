# Eframe


An ESP32 E-Ink Dashboard

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.0-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

A configurable e-ink dashboard for ESP32 that displays real-time data via MQTT on a Waveshare 7.5" display. Features Wi-Fi provisioning via Bluetooth, web-based configuration, and customizable widgets.

## Features

- **E-Ink Display**: 800×480 Waveshare 7.5" display with partial refresh support
- **Wi-Fi Provisioning**: Simple Bluetooth setup for Wi-Fi credentials
- **Web Configuration**: Upload JSON layouts to customize widgets and settings
- **MQTT Integration**: Real-time data updates for all dashboard elements
- **Widget System**: Grid-based layout (12×8) with info cards, weather cards, and lists
- **Configurable Buttons**: 4 physical buttons with customizable actions
- **Power Efficient**: Optimized for low power consumption with periodic updates

## Hardware Requirements

- ESP32 development board
- Waveshare 7.5" E-Ink Display (800×480)
- 4 momentary push buttons
- USB power supply

## Quick Start

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/eink-dashboard.git
   cd eink-dashboard
   ```

2. Install ESP-IDF v5 and set up the environment:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

3. Configure and build:
   ```bash
   idf.py menuconfig
   idf.py build
   ```

4. Flash to your device:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

5. Connect to "EInk-Dashboard" via Bluetooth to configure Wi-Fi

## Configuration

Access the web interface at the device's IP address to upload a JSON configuration file defining:

- MQTT server settings
- Widget types, positions, and topics
- Button actions and GPIO mappings

Example configuration:
```json
{
  "mqtt": {
    "server": "mqtt.example.com",
    "port": 1883
  },
  "widgets": [
    {
      "name": "temperature",
      "type": "info_card",
      "position": {"x": 0, "y": 0},
      "size": {"width": 4, "height": 2},
      "topic": "sensors/temperature"
    }
  ]
}
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
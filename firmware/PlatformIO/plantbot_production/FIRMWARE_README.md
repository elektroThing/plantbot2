# PlantBot2 Production Firmware

Production-ready firmware for ESP32-C6 with simplified HTTP-based data upload.

## Changes from Original

- **Removed InfluxDB dependency** - uses simple HTTP POST requests
- **Simplified configuration** - only server IP/port needed
- **Reduced memory usage** - fewer libraries and dependencies
- **Better error handling** - clearer status reporting

## Configuration

Edit `include/credentials.h`:
```cpp
#define SERVER_HOST "192.168.1.100"  // Your dashboard server IP
#define SERVER_PORT 3000             // Dashboard server port
#define DATA_ENDPOINT "/api/data"    // API endpoint for sensor data
```

## Build Instructions

```bash
# Install PlatformIO
pip install platformio

# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Data Format

The device sends JSON data via HTTP POST:
```json
{
  "device_id": "AA:BB:CC:DD:EE:FF",
  "timestamp": 12345,
  "temperature": 22.5,
  "humidity": 65.2,
  "battery_voltage": 3.85,
  "light_level": 1250,
  "moisture_level": 2100,
  "moisture_percent": 45.5,
  "boot_count": 15,
  "rssi": -45,
  "low_battery": false,
  "sleep_minutes": 120,
  "charging": false
}
```

## Power Consumption

Same ultra-low power characteristics as original:
- **Average**: ~0.7mA during normal operation
- **Sleep**: ~10μA deep sleep current
- **Battery Life**: 120+ days on 2000mAh Li-ion
- **Solar Compatible**: Indefinite operation with 6V 1W panel

## Status Indicators

LED blink patterns:
- **2 blinks**: Data uploaded successfully
- **3 blinks**: Sensor reading failed
- **4 blinks**: Data upload failed
- **6 blinks**: WiFi connection failed
- **7 blinks**: Critical battery (24hr sleep)
- **10 fast blinks**: UVLO protection active
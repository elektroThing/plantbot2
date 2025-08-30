# PlantBot2 Firmware

Open-source firmware for the PlantBot2 solar-powered plant monitoring system.

## Quick Start

### 1. Hardware Testing (Required First)
Validate your PlantBot2 PCB with the bring-up firmware:

```bash
cd firmware/PlatformIO/plantbot2_bringup
pio run --target upload
pio device monitor
```

**Expected**: All 7 hardware tests pass ✓

### 2. Production Monitoring
Deploy the main monitoring application:

```bash
cd firmware/PlatformIO/plantbot_app
pio run --target upload
pio device monitor
```

**Expected**: Hourly sensor data collection with deep sleep power management.

## Firmware Options

| Firmware | Purpose | Power | Use Case |
|----------|---------|-------|-----------|
| **plantbot2_bringup** | Hardware validation | ~40mA continuous | PCB testing, debugging |
| **plantbot_app** | Production monitoring | ~0.7mA average | Long-term deployment |
| **plantbot_production** | Cloud deployment | ~0.8mA average | Remote monitoring |

## Hardware Requirements

- **ESP32-C6-MINI-1-N4** microcontroller
- **AHT20** temperature/humidity sensor
- **3.7V Li-ion battery** with solar charging
- **Capacitive soil moisture sensor**
- **Light sensor (TEMT6000)**

## Pin Configuration

```cpp
#define PIN_STATUS_LED     0   // Debug LED
#define PIN_BATTERY_READ   1   // Battery voltage (ADC)
#define PIN_LIGHT_SENSOR   2   // Light level (ADC)
#define PIN_I2C_POWER      3   // Sensor power control
#define PIN_MOISTURE_SENS  4   // Soil moisture (ADC)
#define PIN_PUMP_CONTROL   5   // Pump MOSFET
#define PIN_I2C_SDA       14   // I2C data
#define PIN_I2C_SCL       15   // I2C clock
```

## Development Workflow

1. **Hardware Validation**: Use `plantbot2_bringup` to verify PCB functionality
2. **Power Testing**: Measure current consumption in deep sleep mode  
3. **Production Deploy**: Flash `plantbot_app` for autonomous monitoring
4. **Dashboard Setup**: Configure data collection endpoint

## Build System

All projects use **PlatformIO** with ESP32-C6 support:

```ini
[env:esp32-c6-devkitc-1]
platform = espressif32
board = esp32-c6-devkitc-1
framework = arduino
monitor_speed = 115200
```

## Power Management

### Bringup Firmware
- Always active for continuous testing
- LED heartbeat for status indication
- ~40mA power consumption

### Production Firmware  
- Deep sleep between measurements
- Wake every hour for sensor reading
- ~0.7mA average power consumption
- Solar powered operation

## Getting Started

1. **Install PlatformIO**: Follow [PlatformIO installation guide](https://platformio.org/install)
2. **Clone repository**: Get the source code
3. **Connect hardware**: USB-C for programming
4. **Run tests**: Start with bringup firmware
5. **Deploy production**: Flash monitoring firmware

## Troubleshooting

### Build Issues
- Verify PlatformIO installation
- Check ESP32-C6 platform support
- Ensure USB drivers installed

### Hardware Issues  
- Run bringup firmware for diagnosis
- Check power connections
- Verify I2C sensor wiring

### Power Issues
- Measure battery voltage
- Test solar charging circuit
- Confirm deep sleep current draw

## License

CC0 1.0 Universal - Public domain dedication. Use this code however you want!

---

*For detailed hardware documentation, see HARDWARE.md*
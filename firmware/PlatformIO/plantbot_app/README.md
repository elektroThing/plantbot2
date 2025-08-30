# PlantBot2 Solar Monitoring Application

## Purpose

This is the production firmware for PlantBot2 designed for long-term, solar-powered plant monitoring. It features ultra-low power consumption through deep sleep cycles and automatic data upload to a cloud dashboard.

## Key Features

### Power Optimization
- **Deep Sleep Cycles**: 1-hour sleep intervals between sensor readings
- **Average Current**: ~0.7mA (30 seconds active, 3599 seconds sleep)
- **Battery Life**: 120+ days on 2000mAh Li-ion (without solar)
- **Solar Compatible**: Indefinite operation with properly sized solar panel
- **Low Battery Protection**: Extended 12-hour sleep when battery < 3.4V

### Sensor Monitoring
- **Temperature & Humidity**: AHT20 I2C sensor with validation
- **Light Level**: TEMT6000X01 phototransistor with averaging
- **Soil Moisture**: Capacitive sensor via 555 timer circuit
- **Battery Voltage**: Precise monitoring with power source detection
- **Boot Counting**: Session tracking for system reliability

### Connectivity
- **WiFiManager**: Easy wireless configuration via captive portal
- **JSON Data Upload**: Structured sensor data transmission
- **Automatic Retry**: Up to 3 attempts with exponential backoff
- **Connection Management**: Minimal WiFi active time for power savings

## Building and Deployment

### Prerequisites
- PlatformIO CLI or IDE
- PlantBot2 hardware validated with bring-up firmware
- WiFi network for data transmission
- Railway.app dashboard deployed (see `railway-dashboard/README.md`)

### Configuration

#### 1. Update Server Settings
Edit `include/plantbot2_pins.h` with your dashboard URL:
```cpp
#define SERVER_HOST "your-app-name.railway.app"  // Your Railway deployment
#define SERVER_PORT 80     // HTTP port
#define DATA_ENDPOINT "/api/data"  // API endpoint
```

#### 2. Build and Upload
```bash
# Navigate to project directory
cd firmware/PlatformIO/plantbot_app

# Install dependencies and build
pio run

# Upload to device
pio run --target upload

# Monitor first boot
pio device monitor
```

### First Boot Setup

#### WiFi Configuration
1. **Power on device** - will create WiFi access point
2. **Connect to "PlantBot2-Setup"** WiFi network
3. **Open browser** - captive portal should appear automatically
4. **Enter WiFi credentials** for your network
5. **Save configuration** - device will restart and connect

#### Verification
Monitor serial output to confirm:
```
=== PlantBot2 Starting ===
Boot count: 1
🔄 Wakeup: Power on reset
🔧 Initializing hardware...
✅ Hardware initialized
📊 Reading sensors...
✅ All sensors read successfully
📡 Connecting to WiFi...
✅ Connected to YourWiFi
IP: 192.168.1.123
RSSI: -45 dBm
📤 Uploading data...
JSON payload: {"device_id":"AA:BB:CC:DD:EE:FF","timestamp":12345,"temperature":22.5,...}
✅ Data uploaded successfully
🔧 Configuring GPIOs for sleep...
✅ GPIOs configured for minimal power consumption
💤 Entering deep sleep for 1 hour
Going to sleep now...
```

## Operation Cycle

### Normal Operation
Every hour, the device follows this sequence:

1. **Wake Up** (ESP32 timer or button press)
   - Print wake-up reason and boot count
   - Initialize GPIO pins and I2C bus

2. **Hardware Setup** (2 seconds)
   - Configure output pins to safe states
   - Enable sensor power (GPIO3 = HIGH)
   - Initialize I2C communication
   - Wait for sensor stabilization

3. **Sensor Reading** (5-10 seconds)
   - Read AHT20 temperature and humidity
   - Average light sensor readings (5 samples)
   - Average moisture sensor readings (5 samples)
   - Read battery voltage (10 samples for accuracy)
   - Validate all readings for reasonable ranges

4. **WiFi Connection** (10-20 seconds)
   - Try stored credentials first
   - Fall back to configuration portal if needed
   - Connect with 30-second timeout

5. **Data Upload** (5-10 seconds)
   - Create JSON payload with all sensor data
   - POST to dashboard API endpoint
   - Retry up to 3 times on failure
   - Disconnect WiFi immediately after success

6. **Sleep Preparation** (1 second)
   - Turn off sensor power (GPIO3 = LOW)
   - Configure all GPIO pins as inputs with pull-ups
   - Set wakeup timer for 1 hour (or 12 hours if low battery)

7. **Deep Sleep** (3599 seconds / 43199 seconds)
   - ESP32 enters deep sleep mode
   - Current consumption drops to ~10μA
   - GPIO states held by hardware

### Power States

#### Normal Battery Operation (3.4V - 4.2V)
- 1-hour sleep cycles
- All features enabled
- Normal retry logic

#### Low Battery Mode (< 3.4V)
- 12-hour sleep cycles  
- Reduced retry attempts
- Status indication via LED blinks

#### USB Power Detection (> 4.5V)
- Normal operation continues
- Battery charging indicator
- No power-saving restrictions

## Data Format

### JSON Payload Structure
```json
{
  "device_id": "AA:BB:CC:DD:EE:FF",    // WiFi MAC address
  "timestamp": 1625097600,              // Device uptime in milliseconds
  "temperature": 22.5,                  // Celsius from AHT20
  "humidity": 65.2,                     // % RH from AHT20
  "battery_voltage": 3.85,              // Volts from ADC with scaling
  "light_level": 1250,                  // Raw ADC reading (0-4095)
  "moisture_level": 2100,               // Raw ADC reading (0-4095)
  "boot_count": 15,                     // Session counter
  "rssi": -45,                          // WiFi signal strength
  "low_battery": false                  // Battery status flag
}
```

### Dashboard Integration
Data is sent via HTTP POST to the configured endpoint. The Railway dashboard automatically:
- Stores readings in SQLite database
- Associates data with device ID
- Updates device "last seen" timestamp
- Provides REST API for data retrieval
- Displays real-time dashboard with charts

## Power Consumption Analysis

### Detailed Current Measurements

| Phase | Duration | Current | Description |
|-------|----------|---------|-------------|
| Wake-up | 1s | 40mA | System initialization |
| Sensor reading | 5s | 15mA | I2C communication, ADC sampling |
| WiFi connection | 15s | 80mA | WiFi radio active, authentication |
| Data upload | 5s | 80mA | HTTP POST transmission |
| Sleep preparation | 1s | 15mA | GPIO configuration |
| Deep sleep | 3599s | 0.01mA | Ultra-low power mode |

**Average calculation:**
- Active energy: (40×1 + 15×5 + 80×15 + 80×5 + 15×1) = 1370 mA·s
- Sleep energy: 0.01×3599 = 35.99 mA·s  
- Total cycle: 1405.99 mA·s / 3600s = **0.39mA average**

**Real-world factors increase to ~0.7mA:**
- WiFi connection time variability
- Sensor warm-up requirements
- GPIO leakage currents
- Environmental temperature effects

### Battery Life Calculations

#### Without Solar Panel
- **2000mAh Li-ion**: 2000mAh ÷ 0.7mA = **2857 hours** = **119 days**
- **3000mAh Li-ion**: 3000mAh ÷ 0.7mA = **4286 hours** = **178 days**

#### With Solar Panel
- **Required solar current**: 0.7mA average
- **Recommended panel**: 6V, 1W (167mA peak) provides 20× margin
- **Cloudy day operation**: 5× margin allows for poor weather
- **Result**: **Indefinite operation** with proper solar sizing

## Configuration Options

### Timing Configuration (`plantbot2_pins.h`)
```cpp
#define SLEEP_DURATION_MINUTES 60        // Normal sleep duration
#define LOW_BATTERY_SLEEP_HOURS 12       // Low battery sleep duration
#define WIFI_TIMEOUT_MS 30000            // WiFi connection timeout
#define SENSOR_WARMUP_MS 2000            // Sensor stabilization time
#define MAX_RETRIES 3                    // Upload retry attempts
```

### Power Thresholds
```cpp
#define BATTERY_LOW_VOLTAGE 3.4          // Low battery threshold
#define BATTERY_MIN_VOLTAGE 3.2          // Minimum safe voltage
#define USB_DETECT_VOLTAGE 4.5           // USB power detection
```

### Server Configuration
```cpp
#define SERVER_HOST "your-dashboard.railway.app"
#define SERVER_PORT 80
#define DATA_ENDPOINT "/api/data"
```

## Troubleshooting

### Common Issues

#### Device Not Waking Up
**Symptoms**: No serial output, no WiFi activity
**Causes**: Deep sleep current too high, battery depleted
**Solutions**:
- Check battery voltage with multimeter
- Verify deep sleep current <50μA
- Reset device with boot button

#### WiFi Connection Failures
**Symptoms**: "❌ WiFi connection failed" in serial output
**Causes**: Weak signal, wrong credentials, router issues
**Solutions**:
- Check WiFi signal strength (RSSI in logs)
- Reset WiFi config by holding boot button during startup
- Verify 2.4GHz network compatibility

#### Data Upload Failures
**Symptoms**: "❌ Data upload failed" despite WiFi connection
**Causes**: Wrong server URL, network firewall, server down
**Solutions**:
- Verify dashboard URL in configuration
- Test dashboard accessibility from browser
- Check Railway.app deployment status

#### High Power Consumption
**Symptoms**: Battery draining faster than expected
**Causes**: WiFi not disconnecting, GPIO leakage, sensor power
**Solutions**:
- Verify GPIO configuration in sleep preparation
- Check sensor power switching (GPIO3)
- Measure current with multimeter in sleep mode

### Debug Output Analysis

#### Normal Operation Logs
```
=== PlantBot2 Starting ===
Boot count: 42
⏰ Wakeup: Timer                    # Expected for hourly cycles
🔧 Initializing hardware...
✅ Hardware initialized
📊 Reading sensors...
Temperature: 23.1°C                 # Should be reasonable room temp
Humidity: 58.2%                     # Should be 20-80% typically
Battery: 3.92V                      # Should be >3.4V for normal operation
Light: 1456                         # Varies with ambient light
Moisture: 2234                      # Varies with soil conditions
✅ All sensors read successfully
📡 Connecting to WiFi...
✅ Connected to MyNetwork
IP: 192.168.1.100
RSSI: -52 dBm                       # Should be >-70 for good connection
📤 Uploading data...
HTTP Response: 200                  # 200 = success, 4xx/5xx = error
✅ Data uploaded successfully
🔧 Configuring GPIOs for sleep...
✅ GPIOs configured for minimal power consumption
💤 Entering deep sleep for 1 hour
```

#### Error Indicators
```
❌ Sensor reading failed             # Hardware issue
❌ WiFi connection failed            # Network issue  
❌ Data upload failed                # Server/connectivity issue
🔋 Low battery detected             # <3.4V, switching to 12h cycles
```

### Recovery Procedures

#### WiFi Reset
1. Hold boot button while pressing reset
2. Keep holding until "WiFi configuration portal" appears
3. Connect to "PlantBot2-Setup" network
4. Reconfigure wireless settings

#### Factory Reset
1. Reflash firmware with PlatformIO
2. Device will start fresh with no stored settings
3. Reconfigure WiFi through setup portal

#### Hardware Reset
1. Power cycle device (disconnect battery and USB)
2. Wait 10 seconds for complete discharge
3. Reconnect power and monitor startup

## Development and Customization

### Adding New Sensors
1. **Hardware**: Connect to I2C bus or available GPIO pins
2. **Firmware**: Add reading code in `readSensors()` function
3. **Data Structure**: Extend `SensorData` struct and JSON payload
4. **Dashboard**: Update API to handle new data fields

### Modifying Sleep Duration
```cpp
// In plantbot2_pins.h, change:
#define SLEEP_DURATION_MINUTES 30   // 30-minute cycles instead of 60
```

### Custom Server Integration
```cpp
// Replace Railway dashboard with custom server
#define SERVER_HOST "your-custom-server.com"
#define DATA_ENDPOINT "/custom/api/endpoint"
```

### Power Optimization
```cpp
// Add custom GPIO power control
void configureGPIOForSleep() {
    // Turn off all unnecessary peripherals
    digitalWrite(PIN_CUSTOM_POWER, LOW);
    
    // Configure pins for minimal leakage
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    // Configure additional pins...
}
```

## Performance Monitoring

### Key Metrics to Track
- **Boot count**: Should increment each hour
- **Battery voltage**: Should remain stable or increase (solar)
- **Upload success rate**: Should be >95% with good WiFi
- **RSSI**: Should be >-70 dBm for reliable operation
- **Wake-up time**: Should be consistent (Timer vs External)

### Dashboard Monitoring
Use the web dashboard to track:
- Device online/offline status
- Historical sensor trends
- Battery level over time
- Upload frequency and reliability
- Multi-device fleet management

### Long-term Validation
For production deployment, monitor for:
- **Week 1**: Initial connectivity and power consumption
- **Month 1**: Battery degradation and solar charging
- **Season 1**: Environmental stress and reliability
- **Year 1**: Long-term component stability

## Production Deployment

### Pre-deployment Checklist
- [ ] Hardware validated with bring-up firmware
- [ ] Dashboard deployed and accessible
- [ ] Firmware configured with correct server URL
- [ ] WiFi credentials configured and tested
- [ ] Power consumption measured and verified
- [ ] Solar panel sized and connected
- [ ] Weatherproof enclosure installed
- [ ] Initial data upload confirmed

### Installation Guidelines
1. **Location**: Choose spot with good WiFi signal and solar exposure
2. **Solar Panel**: Size for local conditions (6V, 1W minimum recommended)
3. **Enclosure**: Use IP65-rated weatherproof housing
4. **Mounting**: Secure against wind and theft
5. **Monitoring**: Check dashboard weekly for first month

### Maintenance Schedule
- **Weekly**: Check dashboard for connectivity
- **Monthly**: Verify battery voltage and solar charging
- **Seasonally**: Clean solar panel and check enclosure seals
- **Annually**: Firmware updates and component inspection

This firmware provides a robust foundation for long-term plant monitoring with minimal maintenance requirements and excellent power efficiency for solar-powered operation.
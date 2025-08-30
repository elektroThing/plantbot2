# PlantBot2 Hardware Bring-up Firmware

## Purpose

This firmware provides systematic hardware validation for the PlantBot2 PCB. It tests all peripherals, sensors, and interfaces to ensure the hardware is functioning correctly before deploying production firmware.

## Features

### Comprehensive Hardware Testing
- **GPIO Output Testing**: LED control, pump MOSFET switching
- **Power Management**: Battery voltage monitoring and USB detection
- **I2C Bus Validation**: Automatic device scanning and sensor detection
- **Sensor Testing**: AHT20 temperature/humidity sensor validation
- **Analog Input Testing**: Light sensor and moisture sensor readings
- **Pass/Fail Reporting**: Clear summary of all test results

### Test Sequence
The firmware runs continuously, cycling through test phases every 3 seconds:

1. **GPIO Output Test**: LED PWM, pump control pulse, user GPIO validation
2. **Power Management Test**: Battery voltage reading and power source detection
3. **I2C Bus Scan**: Device detection at all addresses with device identification
4. **AHT20 Sensor Test**: Temperature/humidity reading with range validation
5. **Analog Input Test**: All ADC channels with percentage calculations
6. **Light Sensor Test**: Phototransistor response testing
7. **Moisture Sensor Test**: Capacitive sensor with response validation
8. **Test Summary**: Overall pass/fail status with recommendation

## Building and Uploading

### Prerequisites
- PlatformIO CLI or IDE
- PlantBot2 hardware with ESP32-C6 module
- USB-C cable for programming

### Build Commands
```bash
# Navigate to project directory
cd firmware/PlatformIO/plantbot2_bringup

# Install dependencies and build
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### PlatformIO Configuration
```ini
[env:esp32-c6-devkitc-1]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/54.03.20/platform-espressif32.zip
board = esp32-c6-devkitc-1
framework = arduino
board_build.arduino.usb_cdc=enable
build_flags = -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
debug_tool = esp-builtin
upload_protocol = esptool
lib_deps = 
    adafruit/Adafruit BusIO
    adafruit/Adafruit_VL53L0X
    adafruit/Adafruit AHTX0
    tzapu/WiFiManager
monitor_speed = 115200
```

## Expected Test Results

### Successful Hardware Validation
When all hardware is functioning correctly, you should see:

```
================================================
PlantBot2 Board Bring-up Test
Hardware Validation Firmware v1.0
ESP32-C6-MINI-1-N4
================================================

=== GPIO Initialization ===
✓ GPIO initialization complete
✓ I2C sensor power enabled

=== I2C Initialization ===
✓ I2C bus initialized
  - SDA: GPIO14
  - SCL: GPIO15
  - Frequency: 100 kHz

=== TEST 1: GPIO Output Test ===
Testing pump control MOSFET... ✓ Pump control pulse sent
  - Check pump status LED (D2) flashed briefly
Testing LED PWM control... ✓ LED PWM test complete
Testing user GPIO (GPIO8)... ✓ Read state: HIGH (pulled up)

=== TEST 2: Power Management Test ===
Battery monitoring:
  - ADC raw value: 2456
  - Calculated voltage: 3.850 V
Power source detection: ✓ Battery power detected
  - Battery level: ~65%

=== TEST 3: I2C Bus Scan ===
Scanning I2C bus...
  - I2C device found at address 0x38 → AHT20 Temperature/Humidity Sensor

Total devices found: 1
✓ AHT20 sensor detected at expected address 0x38

=== TEST 4: AHT20 Temperature/Humidity Test ===
✓ AHT20 initialization successful
Temperature: 22.1 °C
Humidity: 45.3 %RH
✓ AHT20 readings within valid range

=== TEST 5: Analog Input Test ===
ADC Channel Readings:
  - Battery (GPIO1): 2456 (60.0%)
  - Light (GPIO2): 1234 (30.1%)
  - Moisture (GPIO4): 2100 (51.3%)
✓ All ADC channels responsive

=== TEST 6: Light Sensor Test ===
Taking 10 readings...
Average light level: 1250 ADC (30%)
  - Normal indoor lighting

💡 Test: Shine a flashlight on the sensor
   The reading should increase significantly

=== TEST 7: Moisture Sensor Test ===
⚠ Note: 555 timer may need hardware fix
  (NE555 requires >4.5V, but runs on 3.3V)
  Consider replacing with TLC555 or LMC555

Taking 10 readings...
Average ADC reading: 2100
Calculated moisture: 45%

💧 Test procedure:
  1. Touch sensor plates with dry material
  2. Touch with damp cloth/sponge
  3. Readings should change significantly

================================================
BOARD BRING-UP TEST SUMMARY
================================================
GPIO Outputs        : ✓ PASS
Power Management     : ✓ PASS
I2C Bus             : ✓ PASS
AHT20 Sensor        : ✓ PASS
Analog Inputs       : ✓ PASS
Light Sensor        : ✓ PASS
Moisture Sensor     : ✓ PASS
------------------------------------------------
Overall Result: 7/7 tests passed 🎉

ALL TESTS PASSED!
Board is ready for user firmware.

--- System Information ---
Chip Model: ESP32-C6
Chip Revision: 0
CPU Frequency: 160 MHz
Free Heap: 327 KB
Flash Size: 8 MB
================================================
```

## Test Descriptions

### GPIO Output Test
- **LED Control**: Tests PWM brightness control from 0-255
- **Pump Control**: Sends 100ms pulse to verify MOSFET switching
- **User GPIO**: Reads pull-up state on expansion pin

### Power Management Test  
- **Battery Voltage**: Reads voltage via resistor divider
- **Power Detection**: Identifies USB vs battery power
- **Calculation**: Uses formula: `(ADC/4095) * 3.3V * 1.51`

### I2C Bus Scan
- **Device Discovery**: Scans addresses 0x01-0x7F
- **Device Identification**: Recognizes AHT20 at 0x38
- **Bus Validation**: Confirms SDA/SCL functionality

### AHT20 Sensor Test
- **Initialization**: Verifies I2C communication
- **Reading Validation**: Checks temperature (-20°C to 60°C) and humidity (0-100%)
- **Response Time**: Tests sensor measurement timing

### Analog Input Test
- **ADC Validation**: Tests all three analog channels
- **Range Check**: Verifies 0-4095 ADC range
- **Percentage Display**: Shows readings as percentages

### Light Sensor Test
- **Averaging**: Takes 10 readings for stability
- **Range Detection**: Categorizes light levels (dark/normal/bright)
- **Interactive**: Prompts user to test with flashlight

### Moisture Sensor Test
- **555 Timer**: Tests capacitive frequency measurement
- **Hardware Warning**: Notes voltage compatibility issue
- **Calibration**: Maps ADC reading to moisture percentage
- **Interactive**: Prompts user to test with wet/dry materials

## Troubleshooting

### Common Issues

#### No I2C Devices Found
```
Total devices found: 0
✗ AHT20 sensor not found at address 0x38
```
**Solutions**:
- Check I2C wiring (GPIO14/15)
- Verify sensor power (GPIO3 should be HIGH)
- Inspect solder joints on AHT20

#### Low Battery Voltage
```
⚠ Low battery voltage detected (<3.2V)
```
**Solutions**:
- Connect USB-C power source
- Check battery connection
- Verify charging circuit operation

#### AHT20 Communication Failure
```
✗ AHT20 initialization failed
  - Check wiring and power supply
```
**Solutions**:
- Verify I2C pull-up resistors (10kΩ)
- Check sensor power switching (GPIO3)
- Test with multimeter on SDA/SCL lines

#### Moisture Sensor Not Responding
```
✗ Sensor may not be connected properly
  - Check connections to J2
  - Verify 555 timer circuit
```
**Solutions**:
- Check J2 connector and cable
- Verify 555 timer has correct supply voltage
- Consider hardware modification (NE555 → TLC555)

### Hardware Modifications

#### 555 Timer Voltage Issue
The current design uses NE555 which requires >4.5V, but runs on 3.3V:
- **Symptom**: Moisture sensor readings unstable or non-responsive
- **Solution**: Replace NE555 with TLC555 or LMC555 for 3.3V operation
- **Impact**: More reliable moisture sensing

#### Timing Capacitor Adjustment
Current C2 (470pF) may produce frequency too high for soil sensing:
- **Symptom**: Moisture readings not sensitive to soil moisture changes
- **Solution**: Increase C2 to 1-10nF for lower frequency
- **Impact**: Better sensitivity and measurement range

## Interactive Testing

### Manual Verification Steps
1. **Visual LED Test**: Watch status LED cycle through brightness levels
2. **Pump LED Test**: Observe pump status LED (D2) flash during pump test
3. **Light Response**: Use flashlight to verify light sensor response
4. **Moisture Response**: Touch sensor probes with dry/wet materials
5. **Button Test**: Press boot button to skip to test summary

### Serial Monitor Usage
- **Baud Rate**: 115200
- **Real-time Updates**: Tests cycle every 3 seconds
- **Interactive Control**: Press boot button anytime to jump to summary
- **Continuous Operation**: Tests run indefinitely for extended validation

## Pin Configuration Reference

| GPIO | Function | Test Purpose |
|------|----------|--------------|
| GPIO0 | Status LED | PWM brightness testing |
| GPIO1 | Battery ADC | Voltage monitoring validation |
| GPIO2 | Light Sensor | Phototransistor response |
| GPIO3 | Sensor Power | I2C power control |
| GPIO4 | Moisture ADC | Capacitive sensor testing |
| GPIO5 | Pump Control | MOSFET switching test |
| GPIO8 | User GPIO | Pull-up state verification |
| GPIO9 | Boot Button | Test control input |
| GPIO14 | I2C SDA | Bus communication |
| GPIO15 | I2C SCL | Clock signal |

## Next Steps

After successful bring-up testing:
1. **Deploy Main Firmware**: Upload plantbot_app for production monitoring
2. **Setup Dashboard**: Deploy Railway dashboard for data collection
3. **Configure WiFi**: Use WiFiManager portal for network setup
4. **Monitor Operation**: Use dashboard to verify sensor data upload

## Development Notes

This firmware is designed for:
- **Hardware validation** before production deployment
- **Manufacturing testing** for quality assurance
- **Troubleshooting** hardware issues during development
- **Educational purposes** to understand system operation

The modular test structure makes it easy to add new hardware components or modify test procedures as the hardware design evolves.
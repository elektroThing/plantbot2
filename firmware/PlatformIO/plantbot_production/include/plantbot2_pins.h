/*
 * PlantBot2 Pin Definitions
 * 
 * Hardware pin mapping for ESP32-C6-MINI-1-N4
 * Version: 1.0
 */

#ifndef PLANTBOT2_PINS_H
#define PLANTBOT2_PINS_H

// GPIO Pin Assignments
#define PIN_STATUS_LED     0   // GPIO0 - Status LED (boot strapping pin)
#define PIN_BATTERY_READ   1   // GPIO1 - Battery voltage monitor (ADC1_CH1)
#define PIN_LIGHT_SENSOR   2   // GPIO2 - Light sensor input (ADC1_CH2)
#define PIN_I2C_POWER      3   // GPIO3 - I2C sensor power control
#define PIN_MOISTURE_SENS  4   // GPIO4 - Soil moisture sensor (ADC1_CH4)
#define PIN_PUMP_CONTROL   5   // GPIO5 - Pump control output
// GPIO6 - BAT pin (not used in firmware)
// GPIO7 - Reserved
#define PIN_USER_GPIO      8   // GPIO8 - User expansion pin
#define PIN_BOOT_BUTTON    9   // GPIO9 - Boot/User button
// GPIO10-11 - Reserved
// GPIO12 - USB D-
// GPIO13 - USB D+
#define PIN_I2C_SDA       14   // GPIO14 - I2C data line
#define PIN_I2C_SCL       15   // GPIO15 - I2C clock line

// I2C Configuration
#define I2C_FREQUENCY     100000  // 100kHz standard mode
#define I2C_ADDR_AHT20    0x38    // AHT20 temperature/humidity sensor

// ADC Configuration
#define ADC_RESOLUTION    12      // 12-bit ADC (0-4095)
#define ADC_MAX_VALUE     4095.0  // Maximum ADC reading
#define ADC_REF_VOLTAGE   3.3     // ADC reference voltage

// Battery Monitoring - Linear calibration values (y = mx + c)
// Calculated from measurements: 3.0V→3168, 3.2V→3182, 3.8V→3374, 4.2V→3444
#define BATTERY_CALIB_SLOPE    0.003944  // m: voltage per ADC unit
#define BATTERY_CALIB_INTERCEPT -9.436   // c: voltage offset
#define BATTERY_MIN_VOLTAGE    3.0   // Minimum safe battery voltage (UVLO threshold)
#define BATTERY_MAX_VOLTAGE    4.2   // Maximum battery voltage
#define USB_DETECT_VOLTAGE     4.5   // Voltage threshold for USB detection
#define BATTERY_LOW_VOLTAGE    3.7   // Low battery warning threshold - start scaling
#define BATTERY_CRITICAL_VOLTAGE 3.7 // Critical battery - deep sleep below this
#define BATTERY_UVLO_VOLTAGE   3.6   // Under voltage lockout - absolute minimum
#define CHARGING_DETECT_VOLTAGE 4.0  // Voltage threshold for charging detection
#define BATTERY_TREND_SAMPLES  10    // Number of samples for trend analysis
#define BATTERY_ADC_SAMPLES    20    // Number of ADC samples to average

// Power Management
#define SLEEP_DURATION_MINUTES 120  // Default sleep duration (2 hours)
#define SLEEP_DURATION_US      (SLEEP_DURATION_MINUTES * 60 * 1000000ULL)
#define WIFI_TIMEOUT_MS        30000 // 30 second WiFi connection timeout
#define HTTP_TIMEOUT_MS        30000 // 30 second HTTP timeout (normal)
#define CLOUD_WAKEUP_DELAY_MS  90000 // 90 second delay for cloud service wake-up
#define SENSOR_WARMUP_MS       2000  // 2 second sensor warmup time
#define CRITICAL_BATTERY_SLEEP_HOURS 24  // Sleep 24 hours if battery critical
#define UVLO_SLEEP_HOURS       48    // Sleep 48 hours if under voltage lockout

// Dynamic Sleep Configuration
#define CHARGING_LIGHT_THRESHOLD 2000  // Light level indicating charging conditions
#define MIN_SLEEP_MINUTES       120    // Minimum sleep time (2 hours)
#define MAX_SLEEP_MINUTES       360    // Maximum sleep time (6 hours) when battery low
#define NORMAL_SLEEP_MINUTES    120    // Normal sleep time (2 hours)
#define CHARGING_SLEEP_MINUTES  120    // Sleep time when charging detected (2 hours)
#define CRITICAL_SLEEP_MINUTES  1440   // Critical battery sleep (24 hours)
#define UVLO_SLEEP_MINUTES      2880   // UVLO sleep (48 hours)

// Moisture Sensor Calibration
#define MOISTURE_WET_VALUE    1300   // ADC value for 100% moisture (fully wet)
#define MOISTURE_DRY_VALUE    1850   // ADC value for 0% moisture (fully dry)

// Application Configuration
#define SERIAL_BAUD_RATE      115200
#define MAX_RETRIES           2      // Maximum upload retry attempts

#endif // PLANTBOT2_PINS_H
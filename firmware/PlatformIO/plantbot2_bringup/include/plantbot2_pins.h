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

// Battery Monitoring
#define BATTERY_DIVIDER_RATIO  1.51  // (R37 + R38) / R38 = (51k + 100k) / 100k
#define BATTERY_MIN_VOLTAGE    3.2   // Minimum safe battery voltage
#define BATTERY_MAX_VOLTAGE    4.2   // Maximum battery voltage
#define USB_DETECT_VOLTAGE     4.5   // Voltage threshold for USB detection

// Timing Constants
#define SENSOR_WARMUP_MS       100   // Time for sensors to stabilize after power-on
#define PUMP_MAX_DURATION_MS   15000 // Maximum pump run time (safety)
#define PUMP_MIN_INTERVAL_MS   28800000 // Minimum time between watering (8 hours)

// Test Configuration
#define SERIAL_BAUD_RATE      115200
#define TEST_PUMP_PULSE_MS    100    // Brief pump test pulse duration

#endif // PLANTBOT2_PINS_H
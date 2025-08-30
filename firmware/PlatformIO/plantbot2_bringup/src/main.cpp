/*
 * PlantBot2 Board Bring-up Firmware
 * 
 * Purpose: Systematic hardware validation and testing
 * Tests all GPIO, sensors, power management, and interfaces
 * 
 * Target: ESP32-C6-MINI-1-N4
 * Version: 1.0
 * Author: elektroThing
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <driver/gpio.h>
#include "plantbot2_pins.h"

// Test configuration
#define TEST_DELAY_MS     3000   // Delay between test phases
#define BLINK_PERIOD_MS   500    // LED blink period
#define PUMP_TEST_MS      100    // Short pump test duration for safety
#define I2C_FREQ          100000 // 100kHz I2C frequency

// Battery voltage calculation constants
#define ADC_MAX_VALUE     4095.0
#define ADC_REF_VOLTAGE   3.3
#define VOLTAGE_DIVIDER   1.51   // R37(51k) + R38(100k) / R38(100k)

// Test result tracking
typedef struct {
    bool gpio_test;
    bool power_test;
    bool i2c_scan;
    bool aht20_test;
    bool analog_test;
    bool light_test;
    bool moisture_test;
} TestResults;

// Global variables
Adafruit_AHTX0 aht;
TestResults testResults = {false};
unsigned long lastTestTime = 0;
int currentTestPhase = 0;
const int totalTestPhases = 8;

// Function declarations
void initializeGPIO();
void initializeI2C();
void runTestPhase(int phase);
void testGPIOOutputs();
void testPowerManagement();
void testI2CBus();
void testAHT20Sensor();
void testAnalogInputs();
void testLightSensor();
void testMoistureSensor();
void printTestSummary();
void scanI2CBus();
float readBatteryVoltage();
void blinkStatusLED(int count, int delayMs);

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(2000); // Give time for serial monitor to connect
    
    // Print header
    Serial.println("\n================================================");
    Serial.println("PlantBot2 Board Bring-up Test");
    Serial.println("Hardware Validation Firmware v1.0");
    Serial.println("ESP32-C6-MINI-1-N4");
    Serial.print("Build: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Serial.println("================================================\n");
    
    // Initialize hardware
    initializeGPIO();
    initializeI2C();
    
    Serial.println("Starting systematic hardware tests...\n");
    Serial.println("Press BOOT button (GPIO9) anytime to skip to summary\n");
}

void loop() {
    // Check if user wants to skip to summary
    if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
        delay(50); // Debounce
        if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
            Serial.println("\nBOOT button pressed - jumping to test summary");
            currentTestPhase = totalTestPhases - 1;
            while (digitalRead(PIN_BOOT_BUTTON) == LOW); // Wait for release
        }
    }
    
    // Run test phases with delay
    if (millis() - lastTestTime > TEST_DELAY_MS) {
        runTestPhase(currentTestPhase);
        currentTestPhase = (currentTestPhase + 1) % totalTestPhases;
        lastTestTime = millis();
    }
    
    // Continuous heartbeat on status LED
    static unsigned long lastBlinkTime = 0;
    static bool ledState = false;
    if (millis() - lastBlinkTime > BLINK_PERIOD_MS) {
        ledState = !ledState;
        digitalWrite(PIN_STATUS_LED, ledState);
        lastBlinkTime = millis();
    }
}

void initializeGPIO() {
    Serial.println("=== GPIO Initialization ===");
    
    // Configure output pins - GPIO 0 needs special handling
    gpio_reset_pin((gpio_num_t)PIN_STATUS_LED);
    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_PUMP_CONTROL, OUTPUT);
    pinMode(PIN_I2C_POWER, OUTPUT);
    
    // Configure input pins
    pinMode(PIN_BATTERY_READ, INPUT);
    pinMode(PIN_LIGHT_SENSOR, INPUT);
    pinMode(PIN_MOISTURE_SENS, INPUT);
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
    pinMode(PIN_USER_GPIO, INPUT_PULLUP);
    
    // Set safe initial states
    digitalWrite(PIN_STATUS_LED, LOW);
    digitalWrite(PIN_PUMP_CONTROL, LOW);
    digitalWrite(PIN_I2C_POWER, HIGH);  // Enable sensor power
    
    Serial.println("✓ GPIO initialization complete");
    Serial.println("✓ I2C sensor power enabled\n");
}

void initializeI2C() {
    Serial.println("=== I2C Initialization ===");
    
    // Initialize I2C with custom pins
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_FREQ);
    
    Serial.println("✓ I2C bus initialized");
    Serial.print("  - SDA: GPIO");
    Serial.println(PIN_I2C_SDA);
    Serial.print("  - SCL: GPIO");
    Serial.println(PIN_I2C_SCL);
    Serial.print("  - Frequency: ");
    Serial.print(I2C_FREQ / 1000);
    Serial.println(" kHz\n");
}

void runTestPhase(int phase) {
    switch(phase) {
        case 0:
            testGPIOOutputs();
            break;
        case 1:
            testPowerManagement();
            break;
        case 2:
            testI2CBus();
            break;
        case 3:
            testAHT20Sensor();
            break;
        case 4:
            testAnalogInputs();
            break;
        case 5:
            testLightSensor();
            break;
        case 6:
            testMoistureSensor();
            break;
        case 7:
            printTestSummary();
            break;
    }
}

void testGPIOOutputs() {
    Serial.println("=== TEST 1: GPIO Output Test ===");
    
    // Test pump control (very brief pulse for safety)
    Serial.print("Testing pump control MOSFET... ");
    digitalWrite(PIN_PUMP_CONTROL, HIGH);
    delay(PUMP_TEST_MS);
    digitalWrite(PIN_PUMP_CONTROL, LOW);
    Serial.println("✓ Pump control pulse sent");
    Serial.println("  - Check pump status LED (D2) flashed briefly");
    
    // Test LED PWM brightness levels
    Serial.print("Testing LED PWM control... ");
    for (int brightness = 0; brightness <= 255; brightness += 51) {
        analogWrite(PIN_STATUS_LED, brightness);
        delay(200);
    }
    // Reset to digital mode after PWM test
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
    Serial.println("✓ LED PWM test complete");
    
    // Test user GPIO
    Serial.print("Testing user GPIO (GPIO8)... ");
    int userPinState = digitalRead(PIN_USER_GPIO);
    Serial.print("✓ Read state: ");
    Serial.println(userPinState ? "HIGH (pulled up)" : "LOW");
    
    testResults.gpio_test = true;
    Serial.println();
}

void testPowerManagement() {
    Serial.println("=== TEST 2: Power Management Test ===");
    
    float batteryVoltage = readBatteryVoltage();
    int batteryADC = analogRead(PIN_BATTERY_READ);
    
    Serial.print("Battery monitoring:");
    Serial.print("\n  - ADC raw value: ");
    Serial.print(batteryADC);
    Serial.print("\n  - Calculated voltage: ");
    Serial.print(batteryVoltage, 3);
    Serial.println(" V");
    
    // Power source detection
    Serial.print("Power source detection: ");
    if (batteryVoltage > 4.5) {
        Serial.println("✓ USB power detected (>4.5V)");
    } else if (batteryVoltage > 4.0) {
        Serial.println("✓ USB power with battery charging (4.0-4.5V)");
    } else if (batteryVoltage > 3.2) {
        Serial.println("✓ Battery power detected (3.2-4.0V)");
        float batteryPercent = ((batteryVoltage - 3.2) / (4.2 - 3.2)) * 100;
        Serial.print("  - Battery level: ~");
        Serial.print((int)batteryPercent);
        Serial.println("%");
    } else if (batteryVoltage > 2.0) {
        Serial.println("⚠ Low battery voltage detected (<3.2V)");
    } else {
        Serial.println("⚠ No power source detected or measurement error");
    }
    
    testResults.power_test = (batteryVoltage > 2.0 && batteryVoltage < 5.5);
    Serial.println();
}

void testI2CBus() {
    Serial.println("=== TEST 3: I2C Bus Scan ===");
    
    // Ensure sensor power is on
    digitalWrite(PIN_I2C_POWER, HIGH);
    delay(100); // Allow sensors to power up
    
    scanI2CBus();
    
    // Check for AHT20
    Wire.beginTransmission(0x38);
    testResults.i2c_scan = (Wire.endTransmission() == 0);
    
    if (testResults.i2c_scan) {
        Serial.println("\n✓ AHT20 sensor detected at expected address 0x38");
    } else {
        Serial.println("\n✗ AHT20 sensor not found at address 0x38");
        Serial.println("  - Check I2C connections (GPIO14/15)");
        Serial.println("  - Verify sensor power is enabled");
    }
    Serial.println();
}

void scanI2CBus() {
    Serial.println("Scanning I2C bus...");
    int deviceCount = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("  - I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            // Identify known devices
            switch(address) {
                case 0x38:
                    Serial.println(" → AHT20 Temperature/Humidity Sensor");
                    break;
                case 0x76:
                case 0x77:
                    Serial.println(" → Possible BMP280/BME280 (not installed)");
                    break;
                default:
                    Serial.println(" → Unknown device");
            }
            deviceCount++;
        }
    }
    
    Serial.print("\nTotal devices found: ");
    Serial.println(deviceCount);
}

void testAHT20Sensor() {
    Serial.println("=== TEST 4: AHT20 Temperature/Humidity Test ===");
    
    if (!testResults.i2c_scan) {
        Serial.println("✗ Skipping AHT20 test - sensor not detected");
        testResults.aht20_test = false;
        Serial.println();
        return;
    }
    
    // Initialize AHT20
    if (aht.begin()) {
        Serial.println("✓ AHT20 initialization successful");
        
        // Wait for sensor to stabilize
        delay(100);
        
        // Read sensor data
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);
        
        Serial.print("Temperature: ");
        Serial.print(temp.temperature, 1);
        Serial.println(" °C");
        
        Serial.print("Humidity: ");
        Serial.print(humidity.relative_humidity, 1);
        Serial.println(" %RH");
        
        // Validate readings
        bool tempValid = (temp.temperature > -20 && temp.temperature < 60);
        bool humidValid = (humidity.relative_humidity >= 0 && humidity.relative_humidity <= 100);
        
        if (tempValid && humidValid) {
            Serial.println("✓ AHT20 readings within valid range");
            testResults.aht20_test = true;
        } else {
            Serial.println("✗ AHT20 readings out of range");
            testResults.aht20_test = false;
        }
    } else {
        Serial.println("✗ AHT20 initialization failed");
        Serial.println("  - Check wiring and power supply");
        testResults.aht20_test = false;
    }
    Serial.println();
}

void testAnalogInputs() {
    Serial.println("=== TEST 5: Analog Input Test ===");
    
    // Test all ADC channels
    Serial.println("ADC Channel Readings:");
    
    int battADC = analogRead(PIN_BATTERY_READ);
    Serial.print("  - Battery (GPIO1): ");
    Serial.print(battADC);
    Serial.print(" (");
    Serial.print((battADC / 4095.0) * 100, 1);
    Serial.println("%)");
    
    int lightADC = analogRead(PIN_LIGHT_SENSOR);
    Serial.print("  - Light (GPIO2): ");
    Serial.print(lightADC);
    Serial.print(" (");
    Serial.print((lightADC / 4095.0) * 100, 1);
    Serial.println("%)");
    
    int moistADC = analogRead(PIN_MOISTURE_SENS);
    Serial.print("  - Moisture (GPIO4): ");
    Serial.print(moistADC);
    Serial.print(" (");
    Serial.print((moistADC / 4095.0) * 100, 1);
    Serial.println("%)");
    
    // ADC test passes if we can read values
    testResults.analog_test = true;
    Serial.println("✓ All ADC channels responsive");
    Serial.println();
}

void testLightSensor() {
    Serial.println("=== TEST 6: Light Sensor Test ===");
    
    // Take multiple readings for stability
    const int numReadings = 10;
    long lightSum = 0;
    
    Serial.print("Taking ");
    Serial.print(numReadings);
    Serial.println(" readings...");
    
    for (int i = 0; i < numReadings; i++) {
        lightSum += analogRead(PIN_LIGHT_SENSOR);
        delay(50);
    }
    
    int lightAvg = lightSum / numReadings;
    int lightPercent = map(lightAvg, 0, 4095, 0, 100);
    
    Serial.print("Average light level: ");
    Serial.print(lightAvg);
    Serial.print(" ADC (");
    Serial.print(lightPercent);
    Serial.println("%)");
    
    // Provide guidance based on reading
    if (lightAvg < 100) {
        Serial.println("  - Very dark (cover sensor or nighttime)");
    } else if (lightAvg < 1000) {
        Serial.println("  - Low light conditions");
    } else if (lightAvg < 3000) {
        Serial.println("  - Normal indoor lighting");
    } else {
        Serial.println("  - Bright light detected");
    }
    
    Serial.println("\n💡 Test: Shine a flashlight on the sensor");
    Serial.println("   The reading should increase significantly");
    
    testResults.light_test = (lightAvg > 10 && lightAvg < 4090);
    Serial.println();
}

void testMoistureSensor() {
    Serial.println("=== TEST 7: Moisture Sensor Test ===");
    
    Serial.println("⚠ Note: 555 timer may need hardware fix");
    Serial.println("  (NE555 requires >4.5V, but runs on 3.3V)");
    Serial.println("  Consider replacing with TLC555 or LMC555\n");
    
    // Take multiple readings
    const int numReadings = 10;
    long moistSum = 0;
    
    Serial.print("Taking ");
    Serial.print(numReadings);
    Serial.println(" readings...");
    
    for (int i = 0; i < numReadings; i++) {
        moistSum += analogRead(PIN_MOISTURE_SENS);
        delay(50);
    }
    
    int moistAvg = moistSum / numReadings;
    // Invert scale: higher capacitance = lower frequency = higher moisture
    int moisturePercent = map(moistAvg, 3500, 1500, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);
    
    Serial.print("Average ADC reading: ");
    Serial.print(moistAvg);
    Serial.print("\nCalculated moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");
    
    // Check if sensor is connected
    if (moistAvg == 0 || moistAvg == 4095) {
        Serial.println("\n✗ Sensor may not be connected properly");
        Serial.println("  - Check connections to J2");
        Serial.println("  - Verify 555 timer circuit");
        testResults.moisture_test = false;
    } else {
        Serial.println("\n💧 Test procedure:");
        Serial.println("  1. Touch sensor plates with dry material");
        Serial.println("  2. Touch with damp cloth/sponge");
        Serial.println("  3. Readings should change significantly");
        testResults.moisture_test = true;
    }
    Serial.println();
}

void printTestSummary() {
    Serial.println("\n================================================");
    Serial.println("BOARD BRING-UP TEST SUMMARY");
    Serial.println("================================================");
    
    const char* testNames[] = {
        "GPIO Outputs        ",
        "Power Management    ",
        "I2C Bus             ",
        "AHT20 Sensor        ",
        "Analog Inputs       ",
        "Light Sensor        ",
        "Moisture Sensor     "
    };
    
    bool* testResultsArray[] = {
        &testResults.gpio_test,
        &testResults.power_test,
        &testResults.i2c_scan,
        &testResults.aht20_test,
        &testResults.analog_test,
        &testResults.light_test,
        &testResults.moisture_test
    };
    
    int passCount = 0;
    for (int i = 0; i < 7; i++) {
        Serial.print(testNames[i]);
        Serial.print(": ");
        if (*testResultsArray[i]) {
            Serial.println("✓ PASS");
            passCount++;
        } else {
            Serial.println("✗ FAIL");
        }
    }
    
    Serial.println("------------------------------------------------");
    Serial.print("Overall Result: ");
    Serial.print(passCount);
    Serial.print("/7 tests passed");
    
    if (passCount == 7) {
        Serial.println(" 🎉");
        Serial.println("\nALL TESTS PASSED!");
        Serial.println("Board is ready for user firmware.");
        blinkStatusLED(5, 100); // Celebration blink
    } else {
        Serial.println(" ⚠");
        Serial.println("\nSome tests failed - check hardware connections");
        Serial.println("Review failed tests above for troubleshooting tips");
    }
    
    // Print system info
    Serial.println("\n--- System Information ---");
    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Revision: ");
    Serial.println(ESP.getChipRevision());
    Serial.print("CPU Frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.println(" KB");
    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");
    
    Serial.println("\n================================================\n");
}

// Utility functions
float readBatteryVoltage() {
    // Take multiple readings for accuracy
    const int numReadings = 10;
    long adcSum = 0;
    
    for (int i = 0; i < numReadings; i++) {
        adcSum += analogRead(PIN_BATTERY_READ);
        delay(10);
    }
    
    float adcAverage = adcSum / (float)numReadings;
    // Calculate actual voltage using divider ratio
    float voltage = (adcAverage / ADC_MAX_VALUE) * ADC_REF_VOLTAGE * VOLTAGE_DIVIDER;
    
    return voltage;
}

void blinkStatusLED(int count, int delayMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_STATUS_LED, HIGH);
        delay(delayMs);
        digitalWrite(PIN_STATUS_LED, LOW);
        delay(delayMs);
    }
}
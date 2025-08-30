/*
 * PlantBot2 Application Firmware
 * 
 * Solar-powered plant monitoring system with WiFi data upload
 * Features deep sleep for power efficiency and hourly sensor readings
 * 
 * Target: ESP32-C6-MINI-1-N4
 * Version: 1.0
 * Author: elektroThing
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiMulti.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_pm.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "plantbot2_pins.h"
#include "credentials.h"

// HTTP client for dashboard
HTTPClient http;

// WiFi clients for HTTP/HTTPS requests
WiFiClient client;
WiFiClientSecure clientSecure;

// RTC memory variables (survive deep sleep)
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool wifiConfigured = false;
RTC_DATA_ATTR uint32_t failedUploads = 0;
RTC_DATA_ATTR float batteryHistory[BATTERY_TREND_SAMPLES] = {0};
RTC_DATA_ATTR int batteryHistoryIndex = 0;
RTC_DATA_ATTR bool batteryHistoryFull = false;
RTC_DATA_ATTR uint32_t lastSleepDuration = SLEEP_DURATION_MINUTES;

// Global objects
Adafruit_AHTX0 aht;
WiFiManager wifiManager;

// Sensor data structure
struct SensorData {
    float temperature;
    float humidity;
    float batteryVoltage;
    int lightLevel;
    int moistureLevel;
    float moisturePercent;
    uint32_t timestamp;
    bool lowBattery;
};

// Function declarations
void setupHardware();
void configureGPIOForSleep();
void initializeRadio();
bool readSensors(SensorData &data);
bool connectWiFi();
bool uploadData(const SensorData &data, uint32_t sleepMinutes);
void displaySetupInformation();
String fetchDeviceAccessKey();
void enterDeepSleep(uint64_t sleepTimeUs);
float readBatteryVoltage();
void blinkStatusLED(int count, int delayMs = 200);
void printWakeupReason();
void updateBatteryHistory(float voltage);
bool isCharging();
uint32_t calculateDynamicSleepTime(float batteryVoltage, int lightLevel);
float calculateMoisturePercent(int moistureReading);

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    bootCount++;
    
    Serial.println("\n=== PlantBot2 Starting ===");
    Serial.printf("Boot count: %d\n", bootCount);
    printWakeupReason();
    
    // Setup hardware
    setupHardware();
    
    // Initialize radio stack (needed after deep deinit)
    initializeRadio();
    
    // HTTP client setup handled in uploadData()
    
    // Read sensors
    SensorData sensorData;
    bool sensorsOK = readSensors(sensorData);
    
    if (!sensorsOK) {
        Serial.println("❌ Sensor reading failed, entering sleep");
        blinkStatusLED(3, 100); // Error indication
        enterDeepSleep(SLEEP_DURATION_US);
    }
    
    // Update battery history for trend analysis
    updateBatteryHistory(sensorData.batteryVoltage);
    
    // Calculate dynamic sleep time based on battery and light levels
    uint32_t sleepMinutes = calculateDynamicSleepTime(sensorData.batteryVoltage, sensorData.lightLevel);
    lastSleepDuration = sleepMinutes;
    
    // Check for UVLO (Under Voltage Lock Out) - critical safety check
    if (sensorData.batteryVoltage <= BATTERY_UVLO_VOLTAGE) {
        Serial.printf("⚠️ UVLO triggered at %.2fV - entering extended sleep\n", sensorData.batteryVoltage);
        Serial.println("🚫 WiFi disabled to prevent brownout");
        blinkStatusLED(10, 50); // Fast blink for UVLO
        configureGPIOForSleep();
        enterDeepSleep(UVLO_SLEEP_HOURS * 60 * 60 * 1000000ULL);
    }
    
    // Check for low battery - enter deep sleep below 3.7V
    if (sensorData.batteryVoltage < BATTERY_CRITICAL_VOLTAGE) {
        Serial.printf("🔋 Low battery at %.2fV - entering deep sleep\n", sensorData.batteryVoltage);
        Serial.println("🚫 WiFi disabled to conserve power");
        blinkStatusLED(7, 100); // Low battery indication
        configureGPIOForSleep();
        enterDeepSleep(CRITICAL_BATTERY_SLEEP_HOURS * 60 * 60 * 1000000ULL);
    }
    
    // Print sensor readings
    Serial.println("\n=== Sensor Readings ===");
    Serial.printf("Temperature: %.1f°C\n", sensorData.temperature);
    Serial.printf("Humidity: %.1f%%\n", sensorData.humidity);
    Serial.printf("Battery: %.2fV\n", sensorData.batteryVoltage);
    Serial.printf("Light: %d\n", sensorData.lightLevel);
    Serial.printf("Moisture: %d (%.1f%%)\n", sensorData.moistureLevel, sensorData.moisturePercent);
    
    // UVLO protection before WiFi - prevent brownout at low voltage
    if (sensorData.batteryVoltage <= BATTERY_UVLO_VOLTAGE) {
        Serial.printf("🚫 Skipping WiFi - voltage too low (%.2fV)\n", sensorData.batteryVoltage);
        Serial.printf("💤 Entering extended sleep for %d hours\n", UVLO_SLEEP_HOURS);
        configureGPIOForSleep();
        enterDeepSleep(UVLO_SLEEP_HOURS * 60 * 60 * 1000000ULL);
    }
    
    // Connect to WiFi and upload data
    if (connectWiFi()) {
        Serial.println("📡 WiFi connected");
        
        if (uploadData(sensorData, sleepMinutes)) {
            Serial.println("✅ Data uploaded successfully");
            failedUploads = 0;
            blinkStatusLED(2, 200); // Success indication
        } else {
            Serial.println("❌ Data upload failed");
            failedUploads++;
            blinkStatusLED(4, 100); // Upload failed indication
        }
        
        // Disconnect WiFi to save power
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("❌ WiFi connection failed");
        failedUploads++;
        blinkStatusLED(6, 100); // WiFi failed indication
    }
    
    Serial.printf("Failed uploads: %d\n", failedUploads);
    Serial.printf("💤 Entering deep sleep for %d minutes\n", sleepMinutes);
    Serial.printf("🔋 Battery trend: %s\n", isCharging() ? "Charging" : "Discharging");
    
    // Configure GPIOs for minimal power consumption
    configureGPIOForSleep();
    
    // Enter deep sleep with calculated duration
    enterDeepSleep(sleepMinutes * 60 * 1000000ULL);
}

void loop() {
    // Should never reach here due to deep sleep
    delay(1000);
}

void setupHardware() {
    Serial.println("🔧 Initializing hardware...");
    
    // Configure output pins
    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_PUMP_CONTROL, OUTPUT);
    pinMode(PIN_I2C_POWER, OUTPUT);
    
    // Configure input pins
    pinMode(PIN_BATTERY_READ, INPUT);
    pinMode(PIN_LIGHT_SENSOR, INPUT);
    pinMode(PIN_MOISTURE_SENS, INPUT);
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
    
    // Ensure boot button has strong pull-up
    gpio_set_pull_mode((gpio_num_t)PIN_BOOT_BUTTON, GPIO_PULLUP_ONLY);
    pinMode(PIN_USER_GPIO, INPUT_PULLUP);
    
    // Set safe initial states
    digitalWrite(PIN_STATUS_LED, LOW);
    digitalWrite(PIN_PUMP_CONTROL, LOW);
    digitalWrite(PIN_I2C_POWER, HIGH); // Power on sensors
    
    // Wait for sensor power stabilization
    delay(SENSOR_WARMUP_MS);
    
    // Initialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_FREQUENCY);
    
    Serial.println("✅ Hardware initialized");
}

void initializeRadio() {
    Serial.println("📡 Initializing radio stack...");
    
    // Initialize WiFi stack - ignore errors as it might already be initialized
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // Initialize Bluetooth controller - ignore errors as it might already be initialized  
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    
    Serial.println("✅ Radio stack initialized");
}

void setupInfluxDB() {
    Serial.println("⚡ Setting up InfluxDB...");
    
    // Skip time sync - use server timestamps only for power efficiency
    Serial.println("Using server timestamps (no client time sync)");
    
    // Skip connection validation to avoid time-dependent crashes
    Serial.println("InfluxDB client ready (skipping validation for power efficiency)");
}

void configureGPIOForSleep() {
    Serial.println("🔧 Configuring GPIOs for sleep...");
    
    // Explicitly deinitialize I2C first to ensure proper shutdown
    Wire.end();
    Serial.println("✅ I2C bus deinitialized");
    
    // Turn off all outputs to minimize current draw
    digitalWrite(PIN_STATUS_LED, LOW);
    digitalWrite(PIN_PUMP_CONTROL, LOW);
    digitalWrite(PIN_I2C_POWER, LOW); // Power down sensors
    
    // Add delay to ensure sensors are properly powered down
    delay(100);
    Serial.println("✅ Sensors powered down");
    
    // Configure all GPIOs as inputs with pull-ups to prevent floating
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_STATUS_LED) |
                          (1ULL << PIN_PUMP_CONTROL) |
                          (1ULL << PIN_I2C_POWER) |
                          (1ULL << PIN_USER_GPIO) |
                          (1ULL << PIN_I2C_SDA) |
                          (1ULL << PIN_I2C_SCL);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    
    // Configure ADC pins as inputs with pull-ups (they're already inputs but ensure low power)
    gpio_config_t adc_conf = {};
    adc_conf.intr_type = GPIO_INTR_DISABLE;
    adc_conf.mode = GPIO_MODE_INPUT;
    adc_conf.pin_bit_mask = (1ULL << PIN_BATTERY_READ) |
                           (1ULL << PIN_LIGHT_SENSOR) |
                           (1ULL << PIN_MOISTURE_SENS);
    adc_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    adc_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&adc_conf);
    
    Serial.println("✅ GPIOs and peripherals configured for minimal power consumption");
}

bool readSensors(SensorData &data) {
    Serial.println("📊 Reading sensors...");
    
    // Initialize sensor data
    memset(&data, 0, sizeof(data));
    data.timestamp = millis();
    
    // Read battery voltage with improved accuracy
    data.batteryVoltage = readBatteryVoltage();
    data.lowBattery = (data.batteryVoltage < BATTERY_LOW_VOLTAGE && data.batteryVoltage > BATTERY_UVLO_VOLTAGE);
    
    // Read light sensor
    long lightSum = 0;
    for (int i = 0; i < 5; i++) {
        lightSum += analogRead(PIN_LIGHT_SENSOR);
        delay(10);
    }
    data.lightLevel = lightSum / 5;
    
    // Read moisture sensor
    long moistSum = 0;
    for (int i = 0; i < 5; i++) {
        moistSum += analogRead(PIN_MOISTURE_SENS);
        delay(10);
    }
    data.moistureLevel = moistSum / 5;
    data.moisturePercent = calculateMoisturePercent(data.moistureLevel);
    
    // Read AHT20 temperature and humidity with retries
    bool ahtSuccess = false;
    for (int retry = 0; retry < 3 && !ahtSuccess; retry++) {
        if (retry > 0) {
            Serial.printf("AHT20 retry %d/3\n", retry + 1);
            // Power cycle the sensor on retries
            digitalWrite(PIN_I2C_POWER, LOW);
            delay(100);
            digitalWrite(PIN_I2C_POWER, HIGH);
            delay(500);
        }
        
        if (aht.begin()) {
            delay(200); // Extended stabilization time
            
            sensors_event_t humidity, temp;
            if (aht.getEvent(&humidity, &temp)) {
                data.temperature = temp.temperature;
                data.humidity = humidity.relative_humidity;
                
                // Validate readings
                if (data.temperature >= -20 && data.temperature <= 60 &&
                    data.humidity >= 0 && data.humidity <= 100) {
                    ahtSuccess = true;
                } else {
                    Serial.println("❌ AHT20 readings out of range");
                }
            } else {
                Serial.println("❌ Failed to read AHT20");
            }
        } else {
            Serial.println("❌ AHT20 initialization failed");
        }
    }
    
    if (!ahtSuccess) {
        Serial.println("❌ AHT20 failed after all retries");
        return false;
    }
    
    Serial.println("✅ All sensors read successfully");
    return true;
}

bool connectWiFi() {
    Serial.println("📡 Connecting to WiFi...");
    
    // Disable WiFi sleep mode for faster connection
    WiFi.setSleep(false);
    
    // Try to connect with stored credentials first
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startTime < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✅ Connected to %s\n", WiFi.SSID().c_str());
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        
        // WiFi connected successfully
        
        return true;
    }
    
    Serial.println("\n❌ Stored credentials failed");
    
    // If this is the first boot or after many failures, start config portal
    if (bootCount <= 3 || failedUploads > 10) {
        Serial.println("🔧 Starting WiFi configuration portal...");
        
        // Add custom parameters to show device info
        String deviceId = WiFi.macAddress();
        String accessKey = "elektrothing";
        
        String deviceInfoHtml = "<h3>📱 Your PlantBot Information</h3>";
        deviceInfoHtml += "<p><strong>Device ID:</strong> " + deviceId + "</p>";
        deviceInfoHtml += "<p><strong>Access Key:</strong> " + accessKey + "</p>";
        deviceInfoHtml += "<p><strong>Your Dashboard URL:</strong></p>";
        
#ifdef USE_HTTPS
        deviceInfoHtml += "<p><a href='https://" + String(SERVER_HOST) + "/device/" + deviceId + "?key=" + accessKey + "' target='_blank'>";
        deviceInfoHtml += "https://" + String(SERVER_HOST) + "/device/" + deviceId + "?key=" + accessKey + "</a></p>";
#else
        deviceInfoHtml += "<p><a href='http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/device/" + deviceId + "?key=" + accessKey + "' target='_blank'>";
        deviceInfoHtml += "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/device/" + deviceId + "?key=" + accessKey + "</a></p>";
#endif
        
        deviceInfoHtml += "<p><em>📝 Save this URL! Bookmark it to access your plant data.</em></p>";
        
        // Add custom HTML to WiFiManager
        wifiManager.setCustomHeadElement(deviceInfoHtml.c_str());
        
        // Start config portal with timeout
        wifiManager.setConfigPortalTimeout(300); // 5 minutes
        wifiManager.setConnectTimeout(30);
        
        if (wifiManager.autoConnect("PlantBot2-Setup")) {
            Serial.println("✅ WiFi configured via portal");
            wifiConfigured = true;
            
            // Display setup information with access key and dashboard URL
            displaySetupInformation();
            
            return true;
        } else {
            Serial.println("❌ WiFi configuration portal timed out");
            return false;
        }
    }
    
    return false;
}

bool uploadData(const SensorData &data, uint32_t sleepMinutes) {
    Serial.println("📤 Uploading data to dashboard...");
    
    // Create JSON payload
    JsonDocument doc;
    
    String deviceId = WiFi.macAddress();
    doc["device_id"] = deviceId;
    doc["timestamp"] = millis();
    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["battery_voltage"] = data.batteryVoltage;
    doc["light_level"] = data.lightLevel;
    doc["moisture_level"] = data.moistureLevel;
    doc["moisture_percent"] = data.moisturePercent;
    doc["boot_count"] = bootCount;
    doc["rssi"] = WiFi.RSSI();
    doc["low_battery"] = data.lowBattery;
    doc["sleep_minutes"] = sleepMinutes;
    doc["charging"] = isCharging();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.printf("JSON payload: %s\n", jsonString.c_str());
    
    // Smart retry logic for cloud services with cold-start delays
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        Serial.printf("Upload attempt %d/%d\n", attempt, MAX_RETRIES);
        
#ifdef USE_HTTPS
        // Use HTTPS for cloud deployment
        clientSecure.setInsecure(); // Skip certificate validation for simplicity
        http.begin(clientSecure, SERVER_HOST, SERVER_PORT, DATA_ENDPOINT);
        http.setTimeout(HTTP_TIMEOUT_MS); // Normal timeout
#else
        // Use HTTP for local deployment
        http.begin(client, SERVER_HOST, SERVER_PORT, DATA_ENDPOINT);
#endif
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonString);
        
        if (httpResponseCode == 200) {
            Serial.println("✅ Data uploaded successfully");
            http.end();
            return true;
        } else {
            Serial.printf("❌ HTTP request failed: %d\n", httpResponseCode);
            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.printf("Response: %s\n", response.c_str());
            }
            
            http.end();
            
            // If first attempt failed and we have one more try, sleep to let cloud service wake up
            if (attempt == 1 && attempt < MAX_RETRIES) {
#ifdef USE_HTTPS
                Serial.printf("💤 Sleeping %d seconds for cloud service wake-up...\n", CLOUD_WAKEUP_DELAY_MS / 1000);
                delay(CLOUD_WAKEUP_DELAY_MS);
#else
                Serial.println("⏳ Waiting 2 seconds before retry...");
                delay(2000); // Shorter delay for local deployments
#endif
            }
        }
    }
    
    return false;
}

void enterDeepSleep(uint64_t sleepTimeUs) {
    uint32_t sleepMinutes = sleepTimeUs / 60000000ULL;
    Serial.printf("💤 Entering deep sleep for %d minutes\n", sleepMinutes);
    
    // Complete WiFi shutdown for maximum power savings
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    if (esp_wifi_stop() == ESP_OK) {
        Serial.println("✅ WiFi stopped");
    }
    if (esp_wifi_deinit() == ESP_OK) {
        Serial.println("✅ WiFi deinitialized");
    }
    
    // Complete Bluetooth shutdown
    if (esp_bt_controller_disable() == ESP_OK) {
        Serial.println("✅ Bluetooth disabled");
    }
    if (esp_bt_controller_deinit() == ESP_OK) {
        Serial.println("✅ Bluetooth deinitialized");
    }
    
    // Additional power management - disable unnecessary peripherals
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Configure wake up source (timer)
    esp_sleep_enable_timer_wakeup(sleepTimeUs);
    
    // Note: GPIO9 is not an RTC GPIO on ESP32-C6, so external wakeup is not available
    // Only timer wakeup is configured
    
    Serial.println("Going to sleep now...");
    Serial.flush();
    delay(100); // Ensure serial output completes
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

float readBatteryVoltage() {
    // Take multiple readings for accuracy with improved averaging
    long adcSum = 0;
    int validReadings = 0;
    
    // Take more samples and filter out obvious outliers
    for (int i = 0; i < BATTERY_ADC_SAMPLES; i++) {
        int reading = analogRead(PIN_BATTERY_READ);
        // Basic outlier filtering - reject readings at extremes
        if (reading > 50 && reading < 4000) {
            adcSum += reading;
            validReadings++;
        }
        delay(10); // Longer delay for more stable readings
    }
    
    if (validReadings == 0) {
        Serial.println("❌ No valid battery readings!");
        return 0.0;
    }
    
    float adcAverage = adcSum / (float)validReadings;
    
    // Linear calibration: voltage = m * adc + c
    float voltage = BATTERY_CALIB_SLOPE * adcAverage + BATTERY_CALIB_INTERCEPT;
    
    Serial.printf("Battery ADC: %.1f (from %d samples), Voltage: %.2fV\n", 
                  adcAverage, validReadings, voltage);
    
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

void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("🔘 Wakeup: External signal (BOOT button)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.printf("⏰ Wakeup: Timer (slept %d minutes)\n", lastSleepDuration);
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            Serial.println("🔄 Wakeup: Power on reset");
            break;
    }
}

void updateBatteryHistory(float voltage) {
    // Read current ADC for debugging
    float currentADC = analogRead(PIN_BATTERY_READ);
    
    batteryHistory[batteryHistoryIndex] = voltage;
    batteryHistoryIndex = (batteryHistoryIndex + 1) % BATTERY_TREND_SAMPLES;
    
    if (batteryHistoryIndex == 0) {
        batteryHistoryFull = true;
    }
    
    Serial.printf("ADC: %.0f, Battery history updated: %.2fV (index %d)\n", currentADC, voltage, batteryHistoryIndex);
}

bool isCharging() {
    if (!batteryHistoryFull && batteryHistoryIndex < 3) {
        return false; // Need at least 3 samples for reliable trend
    }
    
    int samples = batteryHistoryFull ? BATTERY_TREND_SAMPLES : batteryHistoryIndex;
    float currentVoltage = batteryHistory[(batteryHistoryIndex - 1 + BATTERY_TREND_SAMPLES) % BATTERY_TREND_SAMPLES];
    
    // Calculate recent trend (last 3 readings for quick response)
    float recentTrend = 0;
    int recentSamples = min(3, samples - 1);
    for (int i = 1; i <= recentSamples; i++) {
        int currentIdx = (batteryHistoryIndex - 1 - (i-1) + BATTERY_TREND_SAMPLES) % BATTERY_TREND_SAMPLES;
        int prevIdx = (batteryHistoryIndex - 1 - i + BATTERY_TREND_SAMPLES) % BATTERY_TREND_SAMPLES;
        recentTrend += batteryHistory[currentIdx] - batteryHistory[prevIdx];
    }
    recentTrend /= recentSamples;
    
    // Calculate overall trend for stability check
    float overallTrend = 0;
    for (int i = 1; i < samples; i++) {
        int currentIdx = (batteryHistoryIndex - 1 - i + BATTERY_TREND_SAMPLES) % BATTERY_TREND_SAMPLES;
        int prevIdx = (batteryHistoryIndex - 2 - i + BATTERY_TREND_SAMPLES) % BATTERY_TREND_SAMPLES;
        overallTrend += batteryHistory[currentIdx] - batteryHistory[prevIdx];
    }
    overallTrend /= (samples - 1);
    
    // Improved charging detection logic
    bool voltageRising = recentTrend > 0.015; // Recent upward trend
    bool stableRise = overallTrend > 0.005;   // Overall stable rise
    bool voltageInChargingRange = currentVoltage > 3.8; // Minimum voltage for charging
    bool highVoltage = currentVoltage > 4.1; // High voltage indicates charging
    
    // Charging if recent rise OR high voltage with stable trend
    bool charging = (voltageRising && stableRise && voltageInChargingRange) || 
                   (highVoltage && overallTrend > -0.01);
    
    Serial.printf("Battery trends - Recent: %.3fV, Overall: %.3fV, Current: %.2fV, Charging: %s\n", 
                  recentTrend, overallTrend, currentVoltage, charging ? "Yes" : "No");
    
    return charging;
}

uint32_t calculateDynamicSleepTime(float batteryVoltage, int lightLevel) {
    uint32_t sleepMinutes = NORMAL_SLEEP_MINUTES;
    
    // UVLO check - should not reach here but safety first
    if (batteryVoltage <= BATTERY_UVLO_VOLTAGE) {
        return UVLO_SLEEP_MINUTES;
    }
    
    // Critical battery check
    if (batteryVoltage <= BATTERY_CRITICAL_VOLTAGE) {
        Serial.printf("🔋 Critical battery (%.2fV): 24hr sleep\n", batteryVoltage);
        return CRITICAL_SLEEP_MINUTES;
    }
    
    // Enhanced charging detection with multiple indicators
    bool highLight = (lightLevel > CHARGING_LIGHT_THRESHOLD);
    bool chargingDetected = isCharging();
    bool voltageIndicatesCharging = (batteryVoltage > CHARGING_DETECT_VOLTAGE);
    
    // Primary: Voltage trend indicates charging
    // Secondary: High light + reasonable voltage (backup indicator)
    // Tertiary: High voltage alone (simple fallback)
    bool chargingConditions = chargingDetected || 
                             (highLight && batteryVoltage > 3.9) ||
                             (voltageIndicatesCharging && batteryVoltage > 4.1);
    
    if (chargingConditions) {
        // Charging conditions: use minimum sleep time
        sleepMinutes = MIN_SLEEP_MINUTES;
        String reason = chargingDetected ? "voltage trend" : 
                       (highLight && batteryVoltage > 3.9) ? "light + voltage" : 
                       "high voltage";
        Serial.printf("🔋 Charging detected via %s (%.2fV, light=%d): minimum sleep\n", 
                      reason.c_str(), batteryVoltage, lightLevel);
    } else if (batteryVoltage < BATTERY_LOW_VOLTAGE) {
        // Linear scaling from 2hrs (4.2V) to 6hrs (3.7V)
        // batteryVoltage ranges from 3.7V (low) to 4.2V (high)
        float voltageRange = BATTERY_MAX_VOLTAGE - BATTERY_LOW_VOLTAGE; // 4.2 - 3.7 = 0.5V
        float voltageRatio = (batteryVoltage - BATTERY_LOW_VOLTAGE) / voltageRange;
        voltageRatio = max(0.0f, min(1.0f, voltageRatio)); // Clamp to 0-1
        
        // Linear interpolation: 6hrs when ratio=0 (3.7V), 2hrs when ratio=1 (4.2V)
        sleepMinutes = MAX_SLEEP_MINUTES - (MAX_SLEEP_MINUTES - MIN_SLEEP_MINUTES) * voltageRatio;
        
        Serial.printf("🔋 Battery scaling: %.2fV → %.1f ratio → %d min\n", 
                      batteryVoltage, voltageRatio, sleepMinutes);
    } else {
        // Normal/high battery level: use minimum sleep time (2 hours)
        sleepMinutes = MIN_SLEEP_MINUTES;
        Serial.printf("🔋 Good battery (%.2fV): standard sleep\n", batteryVoltage);
    }
    
    // Ensure sleep time is within bounds
    sleepMinutes = max((uint32_t)MIN_SLEEP_MINUTES, min((uint32_t)MAX_SLEEP_MINUTES, sleepMinutes));
    
    Serial.printf("Final sleep decision: Battery=%.2fV, Light=%d, Sleep=%d min (%.1f hrs)\n", 
                  batteryVoltage, lightLevel, sleepMinutes, sleepMinutes/60.0);
    
    return sleepMinutes;
}

float calculateMoisturePercent(int moistureReading) {
    // Convert ADC reading to moisture percentage
    // Lower ADC values = more moisture (inverted scale)
    
    if (moistureReading <= MOISTURE_WET_VALUE) {
        return 100.0; // Saturated - 100% moisture
    }
    
    if (moistureReading >= MOISTURE_DRY_VALUE) {
        return 0.0; // Dry - 0% moisture
    }
    
    // Linear interpolation between wet and dry values
    float percent = 100.0 * (float)(MOISTURE_DRY_VALUE - moistureReading) / 
                    (float)(MOISTURE_DRY_VALUE - MOISTURE_WET_VALUE);
    
    // Ensure result is between 0-100%
    return max(0.0f, min(100.0f, percent));
}

String fetchDeviceAccessKey() {
    // Return hardcoded access key
    return "elektrothing";
}

void displaySetupInformation() {
    Serial.println("\n==================================================");
    Serial.println("🎉 PLANTBOT SETUP COMPLETE!");
    Serial.println("==================================================");
    
    String deviceId = WiFi.macAddress();
    String accessKey = fetchDeviceAccessKey();
    
    Serial.printf("📱 Device ID: %s\n", deviceId.c_str());
    
    if (accessKey.length() > 0) {
        Serial.printf("🔑 Access Key: %s\n", accessKey.c_str());
        Serial.println();
        Serial.println("🌐 Your Personal Dashboard:");
        
#ifdef USE_HTTPS
        Serial.printf("   https://%s/device/%s?key=%s\n", SERVER_HOST, deviceId.c_str(), accessKey.c_str());
#else
        Serial.printf("   http://%s:%d/device/%s?key=%s\n", SERVER_HOST, SERVER_PORT, deviceId.c_str(), accessKey.c_str());
#endif
        
        Serial.println();
        Serial.println("📝 IMPORTANT: Save this information!");
        Serial.println("   • Bookmark the URL above");
        Serial.println("   • Share it with family/friends");
        Serial.println("   • Keep the access key secure");
    } else {
        Serial.println("⚠️  Could not retrieve access key");
        Serial.println("   Your device will still work, but you'll need to");
        Serial.println("   check the main dashboard for your access key.");
    }
    
    Serial.println();
    Serial.println("📊 What happens next:");
    Serial.println("   • Device uploads data every 2 hours");
    Serial.println("   • Check your dashboard for live readings");
    Serial.println("   • Monitor battery, temperature, humidity & soil");
    Serial.println();
    Serial.println("==================================================");
    
    // Blink LED to indicate setup complete
    blinkStatusLED(5, 300);
}
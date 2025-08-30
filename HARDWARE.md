# PlantBot2 Hardware Documentation

## Overview

This document provides the technical hardware specifications required for firmware development on the PlantBot2 system. It covers pin assignments, IC specifications, electrical connections, and interface details needed to write effective embedded software.

## Microcontroller Unit

**Primary MCU**: ESP32-C6-MINI-1-N4
- **CPU**: RISC-V single-core processor @ 160MHz
- **Memory**: 512KB SRAM, 4MB integrated flash
- **Connectivity**: WiFi 6 (802.11ax), Bluetooth 5, Zigbee 3.0, Thread
- **Package**: QFN-32 (5mm × 5mm)
- **Operating Voltage**: 3.0V - 3.6V
- **Operating Temperature**: -40°C to +85°C

## Pin Assignment Table

| GPIO | Function | Direction | Connection | Electrical Notes |
|------|----------|-----------|------------|------------------|
| GPIO0 | Status LED | Output | Green LED (D8) via R13 (1kΩ) | Boot strapping pin, pulled up on boot |
| GPIO1 | Battery Monitor | Input (ADC) | Voltage divider (R37: 51kΩ, R38: 100kΩ) | ADC1_CH1, 0-3.3V range |
| GPIO2 | Light Sensor | Input (ADC) | TEMT6000X01 output via R35 (10kΩ) | ADC1_CH2, phototransistor current |
| GPIO3 | Sensor Power Control | Output | I2C_PWR rail enable | Controls power to I2C sensors |
| GPIO4 | Soil Moisture | Input (ADC) | 555 timer output via signal conditioning | ADC1_CH4, capacitive sensor |
| GPIO5 | Pump Control | Output | DTC143ECA → DMP3017SFG MOSFET gate | PWM capable, controls water pump |
| GPIO8 | User Expansion | I/O | Pull-up to 3.3V (R21: 10kΩ) | Reserved for future use |
| GPIO9 | Boot/User Button | Input | Tactile switch S1 | Boot strapping pin with pull-up |
| GPIO12 | USB D- | I/O | USB-C connector data line | USB functionality |
| GPIO13 | USB D+ | I/O | USB-C connector data line | USB functionality |
| GPIO14 | I2C SDA | I/O | I2C data line with R17 (10kΩ) pull-up | Standard I2C interface |
| GPIO15 | I2C SCL | I/O | I2C clock line with R18 (10kΩ) pull-up | Standard I2C interface |

## Power System Architecture

### Primary Power Rails

1. **VDD Rail**: Input power from USB-C or battery (3.7V-5V)
2. **+3V3 Rail**: Regulated 3.3V from TPS63051RMWR buck-boost converter
3. **I2C_PWR Rail**: Switchable 3.3V for sensor power management (controlled by GPIO3)

### Power Management ICs

**BQ24073RGTR Battery Charger**
- **Function**: Li-ion/Li-Po battery charging with safety timer
- **Input Voltage**: 4.35V - 6.5V (USB-C compatible)
- **Charge Current**: ~650mA (set by R28, R29: 1.5kΩ resistors)
- **Safety Features**: Thermal regulation, timer, overvoltage protection
- **Status Outputs**: Power good (CHG_PG), charge status (CHG_STAT)

**TPS63051RMWR Buck-Boost Converter**
- **Function**: Maintains stable 3.3V output across wide input range
- **Input Range**: 1.8V - 5.5V
- **Output**: 3.3V ± 2% @ up to 500mA
- **Efficiency**: >85% typical
- **Enable**: Always enabled (EN tied high)

### Battery Monitoring Circuit

**Voltage Divider Configuration**:
- **R37**: 51kΩ (high side)
- **R38**: 100kΩ (low side, to ground)
- **Scaling Factor**: 0.662 (ADC_reading * 3.3V * 1.51 / 4095)
- **Measurable Range**: 0V - 4.98V (covers full Li-ion range)

**Firmware Calculation**:
```cpp
float batteryVoltage = (analogRead(BATTERY_READ_PIN) / 4095.0) * 3.3 * 1.51;
```

## Sensor Subsystems

### Temperature & Humidity Sensor

**AHT20 Digital Sensor (IC4)**
- **Interface**: I2C (address: 0x38)
- **Power**: I2C_PWR rail (switchable via GPIO3)
- **Measurement Range**: 
  - Temperature: -40°C to +85°C (±0.3°C accuracy)
  - Humidity: 0-100% RH (±2% accuracy)
- **Resolution**: 20-bit temperature, 20-bit humidity
- **Power Consumption**: 0.25μA standby, 550μA measuring

**I2C Connection**:
- **SDA**: GPIO14 with 10kΩ pull-up (R17)
- **SCL**: GPIO15 with 10kΩ pull-up (R18)
- **Bypass Capacitor**: C19 (100nF) for power stability

### Light Sensor

**TEMT6000X01 Phototransistor (Q2)**
- **Type**: NPN silicon phototransistor
- **Spectral Range**: 400nm - 1100nm (peak at 570nm)
- **Power**: I2C_PWR rail
- **Output**: Current proportional to light intensity
- **Load Resistor**: R35 (10kΩ) converts current to voltage

**Circuit Configuration**:
```
I2C_PWR → TEMT6000X01 collector
TEMT6000X01 emitter → R35 (10kΩ) → GND
                   ↓
                 GPIO2 (ADC input)
```

**Firmware Reading**:
```cpp
int lightADC = analogRead(LIGHT_SENSOR_PIN);
int lightPercent = map(lightADC, 0, 4095, 0, 100);
```

### Soil Moisture Sensor

**Capacitive Sensing Circuit**
- **Primary IC**: NE555DR timer (should be TLC555 for 3.3V operation)
- **Signal Conditioning**: LM358DR2G dual op-amp
- **External Sensor**: Capacitive plates connected via J2 connector

**555 Timer Configuration (IC1)**:
- **R3**: 1.5kΩ (timing resistor)
- **R4**: 2.4kΩ (timing resistor)  
- **R6**: 1MΩ (timing resistor)
- **C2**: 470pF (timing capacitor - **NOTE**: May need increase to 1-10nF)
- **Output Frequency**: ~8.6MHz (too high - needs component adjustment)

**Signal Processing Path**:
1. 555 timer generates frequency based on soil capacitance
2. Op-amp buffer (LM358DR2G) conditions the signal
3. Diode rectifier (D1) with filter capacitor (C6: 4.7μF)
4. Final analog voltage to GPIO4 (ADC1_CH4)

**Firmware Reading**:
```cpp
int moistureADC = analogRead(SOIL_MOISTURE_PIN);
// Invert reading: higher capacitance = lower frequency = higher moisture
int moisturePercent = map(moistureADC, 3500, 1500, 0, 100);
moisturePercent = constrain(moisturePercent, 0, 100);
```

## Pump Control System

### High-Side Switch Circuit

**Control Path**: GPIO5 → R10 (220Ω) → DTC143ECA → R12 (1kΩ) → DMP3017SFG gate

**DTC143ECA Pre-driver (Q1)**:
- **Type**: NPN transistor with built-in base resistor
- **Base Resistor**: Internal 4.7kΩ
- **Function**: Level shifting and current gain

**DMP3017SFG Power MOSFET (Q3)**:
- **Type**: P-channel MOSFET
- **Voltage Rating**: -20V
- **Current Rating**: -2.3A continuous
- **RDS(on)**: 70mΩ @ VGS = -4.5V
- **Gate Protection**: R11 (10kΩ) pull-down ensures off state

**Load Connection**:
- **Terminal Block J3**: Screw terminals for pump connection
- **Maximum Load**: 12V, 2A
- **Status LED**: D2 (green) indicates pump active state

**Firmware Control**:
```cpp
// Activate pump
digitalWrite(PUMP_CONTROL_PIN, HIGH);  // Turns on MOSFET

// Deactivate pump  
digitalWrite(PUMP_CONTROL_PIN, LOW);   // Turns off MOSFET
```

## USB-C Interface

**GT-USB-7010 Connector (J1)**
- **Type**: USB-C receptacle with full pin complement
- **Power Delivery**: VBUS supports up to 3A @ 5V
- **Configuration**: Device mode with 5.1kΩ CC resistors (R1, R2)
- **Data Lines**: D+/D- connected to ESP32-C6 USB pins (GPIO12/13)
- **Shield**: Connected to PCB ground plane

**USB Detection**:
```cpp
// No hardware USB detect pin - use voltage monitoring method
bool isUSBConnected() {
    return batteryVoltage > 4.0;  // Simple heuristic
}
```

## Expansion Interfaces

### QWIIC I2C Connector (J4)

**Pin Assignment**:
1. **Black**: GND
2. **Red**: 3.3V (I2C_PWR rail)
3. **Blue**: SDA (GPIO14)
4. **Yellow**: SCL (GPIO15)

**Compatible Devices**: Any QWIIC/Stemma QT sensor modules

### External Sensor Connector (J2)

**2-pin connector for capacitive moisture sensor plates**
- **Pin 1**: Sensor drive signal
- **Pin 2**: Sensor return/sense signal
- **Cable Length**: Up to 1 meter recommended
- **Wire Gauge**: 22-26 AWG suitable

## Reset and Boot Control

### Reset Circuit
- **Reset Button**: S2 tactile switch
- **Pull-up**: R8 (10kΩ) on EN pin
- **Bypass Capacitor**: C10 (100nF) for debouncing

### Boot Control
- **Boot Button**: S1 tactile switch (also user button)
- **GPIO9**: Boot strapping pin with internal pull-up
- **Function**: Hold during reset to enter download mode

## Status Indication

### LEDs and Functions

| LED | Color | Function | Control |
|-----|-------|----------|---------|
| D8 | Green | System Status/Debug | GPIO0 |
| D2 | Green | Pump Active | Connected to pump control circuit |
| D10 | Green | Power Good | BQ24073 charge controller |
| D11 | Green | Charge Status | BQ24073 charge controller |

## Critical Design Notes

### Hardware Issues to Address

1. **555 Timer Voltage Compatibility**
   - **Issue**: NE555DR requires >4.5V, but I2C_PWR is 3.3V
   - **Solution**: Replace with TLC555 or LMC555 for low-voltage operation
   - **Impact**: Current design may not work reliably

2. **Moisture Sensor Frequency**
   - **Issue**: Calculated frequency (~8.6MHz) too high for soil sensing
   - **Solution**: Increase C2 from 470pF to 1-10nF
   - **Impact**: Better sensitivity and more reasonable frequency range

### Power Consumption Optimization

**GPIO Configuration for Low Power**:
```cpp
void configureGPIOForSleep() {
    // Turn off sensor power
    digitalWrite(I2C_POWER_PIN, LOW);
    
    // Configure unused pins to minimize leakage
    gpio_pullup_en(GPIO_NUM_8);    // User GPIO
    gpio_pulldown_dis(GPIO_NUM_8);
    
    // Hold critical pin states
    gpio_hold_en(GPIO_NUM_3);      // Sensor power pin
    gpio_hold_en(GPIO_NUM_5);      // Pump control pin
}
```

## Firmware Development Guidelines

### ADC Usage Notes
- **ADC1 Channels**: Used for analog sensors (compatible with WiFi)
- **Reference Voltage**: 3.3V (VDD rail)
- **Resolution**: 12-bit (0-4095 range)
- **Sample Time**: Allow settling time between readings

### I2C Bus Management
```cpp
void powerOnSensors() {
    digitalWrite(I2C_POWER_PIN, HIGH);
    delay(100);  // Allow sensor startup time
}

void powerOffSensors() {
    digitalWrite(I2C_POWER_PIN, LOW);
    // Saves ~1mA in sleep mode
}
```

### Pump Safety Implementation
```cpp
#define MAX_PUMP_TIME 15000  // 15 second safety timeout
#define MIN_PUMP_INTERVAL 28800000  // 8 hours minimum between waterings

void safetyCheckPump() {
    static unsigned long pumpStartTime = 0;
    static unsigned long lastWateringTime = 0;
    
    if (pumpActive) {
        if (millis() - pumpStartTime > MAX_PUMP_TIME) {
            // Emergency pump shutoff
            digitalWrite(PUMP_CONTROL_PIN, LOW);
            pumpActive = false;
            Serial.println("SAFETY: Pump timeout!");
        }
    }
}
```

## Electrical Specifications

### Power Requirements
- **Input Voltage**: 5V ± 0.5V (USB-C) or 3.7V Li-ion battery
- **Current Consumption**:
  - Active (WiFi on): ~80mA
  - Sensors only: ~15mA  
  - Deep sleep: <1mA
  - Pump active: +2A max

### Environmental Ratings
- **Operating Temperature**: 0°C to +50°C (recommended)
- **Storage Temperature**: -20°C to +70°C
- **Humidity**: 5% to 85% RH, non-condensing
- **IP Rating**: IP20 (indoor use only)

### Signal Levels
- **Digital I/O**: 3.3V CMOS logic levels
- **ADC Input Range**: 0V to 3.3V
- **I2C Logic Levels**: 3.3V with 10kΩ pull-ups

---

*This hardware documentation is intended for firmware developers working with the PlantBot2 embedded system. Always verify pin assignments and electrical specifications against the actual PCB before making connections.*

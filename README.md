# PlantBot2 - Solar-Powered Plant Monitoring System

## Overview

PlantBot2 is an intelligent, solar-powered IoT plant monitoring system built around the ESP32-C6 microcontroller. This prototype system combines environmental sensing, battery management, and wireless data transmission to monitor plant conditions and upload data to a cloud dashboard.

## 🚀 Current Implementation Status

This repository contains a **working prototype** with:
- ✅ **Hardware validation firmware** (board bring-up)
- ✅ **Solar-powered monitoring firmware** with 1-hour sleep cycles
- ✅ **Web dashboard** for data visualization (Railway.app deployable)
- ✅ **Power-efficient design** optimized for battery operation
- ✅ **WiFiManager integration** for easy wireless setup

## Features

### Environmental Monitoring
- **Soil moisture sensing** via capacitive sensor with 555 timer
- **Temperature and humidity monitoring** (AHT20 I2C sensor)
- **Light level detection** (TEMT6000X01 phototransistor)
- **Battery voltage monitoring** with power source detection

### Power Management
- **Solar charging** with USB-C input and Li-ion battery backup
- **Deep sleep cycles** (1 hour intervals) for maximum battery life
- **Adaptive power management** with low-battery protection
- **GPIO optimization** to minimize current leakage

### Connectivity & Data
- **WiFi data upload** to cloud dashboard every hour
- **JSON API** for sensor data transmission
- **WiFiManager** for easy wireless configuration
- **Real-time web dashboard** with historical charts

## Hardware Platform

- **MCU**: ESP32-C6-MINI-1-N4 (WiFi 6, Bluetooth 5)
- **Sensors**: AHT20 (temp/humidity), TEMT6000X01 (light), capacitive moisture
- **Power**: USB-C input, Li-ion charging, solar panel compatible
- **Connectivity**: I2C expansion, WiFi data transmission

## Repository Structure

```
plantbot2/
├── README.md                           # This file - project overview
├── HARDWARE.md                         # Hardware documentation and pin assignments
├── PLAN.md                            # Original software development plan
├── CHANGELOG.md                       # Version history and changes
├── dashboard/                         # Web dashboard for data visualization
│   ├── server.js                     # Node.js backend with PostgreSQL/SQLite
│   ├── database.js                   # Database abstraction layer
│   ├── public/                       # Static web assets
│   │   ├── index.html               # Main dashboard interface
│   │   └── device.html              # Device-specific monitoring
│   ├── package.json                  # Dependencies and deployment config
│   └── .env.example                  # Environment configuration template
├── production/                        # Production documentation and configs
│   ├── README.md                     # Production deployment guide
│   ├── CLOUD_QUICKSTART.md          # Cloud deployment instructions
│   ├── DEPLOYMENT_STATUS.md          # Current deployment status
│   └── render.yaml                   # Render.com deployment configuration
├── PlatformIO/                       # All PlatformIO firmware projects
│   ├── plantbot2_bringup/           # Hardware bring-up and validation
│   │   ├── platformio.ini           # Build configuration
│   │   ├── src/main.cpp            # Systematic hardware testing
│   │   └── include/plantbot2_pins.h
│   ├── plantbot_app/                # Main solar monitoring application
│   │   ├── platformio.ini           # Build configuration
│   │   ├── src/main.cpp            # Power-efficient monitoring firmware
│   │   └── include/plantbot2_pins.h
│   └── plantbot_production/         # Production firmware with cloud integration
│       ├── platformio.ini           # Production build configuration
│       ├── src/main.cpp            # Cloud-enabled monitoring firmware
│       └── include/                 # Production headers and credentials
├── firmware/                         # Legacy Arduino IDE projects
│   ├── Arduino/                     # Original Arduino IDE implementations
│   │   └── plantbot2_bringup/       # Legacy hardware validation
│   └── README.md                    # Firmware development guide
└── pcb/                             # KiCad PCB design files
    ├── plantbot2.kicad_pcb         # Main PCB layout
    └── production/                  # Manufacturing files and gerbers
```

## 🛠 Quick Start Guide

### 1. Hardware Validation (Recommended First Step)

Test your hardware before deploying the main application:

```bash
# Navigate to bringup firmware
cd PlatformIO/plantbot2_bringup

# Build and upload (requires PlatformIO)
pio run --target upload

# Monitor serial output
pio device monitor
```

The bring-up firmware will systematically test:
- GPIO outputs (LED, pump control)
- Power management (battery monitoring)
- I2C bus and AHT20 sensor
- Analog inputs (light, moisture sensors)
- System validation with pass/fail summary

### 2. Deploy Web Dashboard

Set up the data collection dashboard on Render.com:

```bash
# Navigate to dashboard
cd dashboard

# Install dependencies
npm install

# Test locally (optional)
npm start

# Deploy to Render.com
# 1. Push this repository to GitHub
# 2. Connect Render to your repository
# 3. Deploy using render.yaml configuration
# Your dashboard: https://your-app.onrender.com
```

### 3. Configure and Deploy Main Firmware

```bash
# Navigate to production firmware
cd PlatformIO/plantbot_production

# Update server configuration
# Edit include/credentials.h:
# #define SERVER_HOST "your-app.onrender.com"

# Build and upload
pio run --target upload

# Monitor operation
pio device monitor
```

### 4. WiFi Configuration

On first boot, the PlantBot creates a WiFi configuration portal:
1. Connect to **"PlantBot2-Setup"** WiFi network
2. Open browser (captive portal should appear automatically)
3. Enter your WiFi credentials
4. Device will save settings and connect

## 📊 System Operation

### Normal Operation Cycle
1. **Wake up** from deep sleep (every 1 hour)
2. **Power sensors** and read all environmental data
3. **Connect to WiFi** using stored credentials
4. **Upload data** to Railway dashboard via JSON API
5. **Configure GPIO** for minimal power consumption
6. **Enter deep sleep** for 1 hour

### Power Management
- **Active time**: ~30 seconds (sensor reading + WiFi upload)
- **Sleep time**: 3599 seconds (deep sleep)
- **Average current**: ~0.7mA (excellent for solar/battery)
- **Low battery mode**: 12-hour sleep cycles when battery < 3.4V

### Data Upload Format
```json
{
  "device_id": "AA:BB:CC:DD:EE:FF",
  "temperature": 22.5,
  "humidity": 65.2,
  "battery_voltage": 3.85,
  "light_level": 1250,
  "moisture_level": 2100,
  "boot_count": 15,
  "rssi": -45,
  "low_battery": false
}
```

## 🌐 Web Dashboard Features

Access your deployed dashboard to view:
- **Real-time sensor readings** from all connected devices
- **Historical charts** showing 24-hour trends
- **Device status** indicators (online/offline)
- **Battery monitoring** with low-battery alerts
- **Auto-refresh** every 5 minutes
- **Mobile responsive** design

### Dashboard API Endpoints
- `GET /api/dashboard` - Summary statistics
- `GET /api/devices` - List all devices
- `GET /api/devices/:id/latest` - Latest reading for device
- `GET /api/devices/:id/history` - Historical data
- `POST /api/data` - Data ingestion (used by firmware)

## ⚡ Power Consumption Analysis

| Mode | Current | Duration | Description |
|------|---------|----------|-------------|
| Active (WiFi) | ~80mA | 30 sec | Sensor reading + data upload |
| Deep Sleep | ~10μA | 3599 sec | Low-power sleep mode |
| **Average** | **~0.7mA** | **1 hour cycle** | **Excellent for solar** |

**Expected battery life:**
- 2000mAh Li-ion: ~120 days continuous operation
- With solar panel: Indefinite (panel keeps battery charged)

## 🔧 Development

### Prerequisites
- **PlatformIO** (recommended) or Arduino IDE with ESP32 support
- **Node.js** for dashboard development
- **PlantBot2 hardware** with ESP32-C6 module

### Building Firmware
```bash
# Install PlatformIO CLI
pip install platformio

# Build bringup firmware
cd PlatformIO/plantbot2_bringup
pio run

# Build main application
cd ../plantbot_app
pio run

# Build production firmware
cd ../plantbot_production
pio run

# Upload and monitor
pio run --target upload
pio device monitor
```

### Testing Hardware
The bring-up firmware provides comprehensive hardware validation:
- Automatically tests all GPIO pins and peripherals
- Validates sensor communication and power management
- Provides clear pass/fail results for each subsystem
- Use this before deploying production firmware

### Dashboard Development
```bash
cd dashboard
npm install
npm start  # Local development server
```

## 📈 Monitoring and Debugging

### Serial Output
The firmware provides detailed serial logging:
```
=== PlantBot2 Starting ===
Boot count: 15
⏰ Wakeup: Timer
🔧 Initializing hardware...
✅ Hardware initialized
📊 Reading sensors...
✅ All sensors read successfully
📡 Connecting to WiFi...
✅ Connected to MyWiFi
📤 Uploading data...
✅ Data uploaded successfully
💤 Entering deep sleep for 1 hour
```

### Power Debugging
Monitor power consumption:
- Check battery voltage in serial output
- Verify deep sleep current with multimeter
- Use dashboard to track battery levels over time

### WiFi Issues
If WiFi connection fails:
- Check signal strength (RSSI in serial output)
- Reset WiFi credentials by holding boot button during startup
- Verify router compatibility (2.4GHz required)

## 🚀 Production Deployment

### For Prototype Testing
1. Flash bring-up firmware first to validate hardware
2. Deploy Render.com dashboard for data collection
3. Configure production firmware with your dashboard URL
4. Deploy in garden with solar panel and battery

### For Long-term Operation
- Use weatherproof enclosure for outdoor deployment
- Size solar panel for local conditions (recommend 6V, 1W minimum)
- Monitor dashboard for connectivity and battery health
- Consider multiple devices for garden-wide monitoring

## 📝 Known Issues & Future Improvements

### Hardware Considerations
- **555 Timer**: Current NE555 may need replacement with TLC555 for 3.3V operation
- **Moisture Sensor**: Frequency may be too high, consider increasing timing capacitor
- See HARDWARE.md for detailed electrical specifications

### Firmware Enhancements (Future)
- Automatic watering control based on moisture thresholds
- Local sensor data logging for offline operation
- Multiple WiFi network support
- Over-the-air firmware updates

### Dashboard Improvements (Future)
- User authentication and device management
- Alert notifications for critical conditions
- Data export functionality
- Mobile push notifications

## 📄 Documentation

- **[HARDWARE.md](HARDWARE.md)** - Complete hardware specifications and pin assignments
- **[PLAN.md](PLAN.md)** - Original software development roadmap
- **[production/README.md](production/README.md)** - Production deployment guide
- **[production/CLOUD_QUICKSTART.md](production/CLOUD_QUICKSTART.md)** - Cloud deployment quickstart
- **[CHANGELOG.md](CHANGELOG.md)** - Version history and updates

## 🤝 Contributing

This is a prototype system designed for hardware testing and proof-of-concept. Contributions welcome for:
- Power optimization improvements
- Additional sensor integrations
- Dashboard feature enhancements
- Hardware design improvements

## 📜 License

This project is open source. See LICENSE file for details.

---

*PlantBot2 - Solar-powered intelligence for your garden 🌱☀️*
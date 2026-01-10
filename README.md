# Smart Home IoT System with Edge AI

This project is a Smart Home IoT system designed to improve **home safety and monitoring**
using sensors, local intelligence (Edge AI), and cloud analytics.

The system monitors temperature, humidity, gas, and flame conditions and reacts locally
to dangerous situations without waiting for cloud decisions.

---

## Problem Statement

Fire and gas accidents are a major risk in smart homes.
Cloud-only systems can introduce delays and privacy concerns.

This project solves that by:
- Making safety decisions **at the edge**
- Triggering **immediate local actions**
- Using the cloud only for monitoring and analytics

---

## System Overview

- **Arduino UNO R4**  
  Reads sensors, controls garage door (servo), buzzer, LCD, and communicates via BLE

- **Raspberry Pi**  
  Acts as an edge AI brain and cloud gateway

- **ThingSpeak Cloud**  
  Stores and visualizes sensor data

---

## Features

- Temperature & humidity monitoring
- Gas detection with buzzer alert
- Flame detection with **edge-based danger scoring**
- Automatic garage door control
- Emergency panic mode
- BLE communication (Arduino ↔ Raspberry Pi)
- Cloud visualization using ThingSpeak

---

## Hardware Components

- Arduino UNO R4
- Raspberry Pi
- DHT11 (Temperature & Humidity)
- MQ-2 Gas Sensor
- Flame Sensor
- Ultrasonic Distance Sensor
- Servo Motor (used as garage door)
- Buzzer
- I2C LCD Display

---

## Software & Dependencies

### Arduino Dependencies
Install these libraries using **Arduino Library Manager**:

- `DHT sensor library`
- `LiquidCrystal_I2C`
- `Servo`
- `ArduinoBLE`
- `Wire` (built-in)

Arduino IDE version: **2.x recommended**

---

### Raspberry Pi Dependencies

Python version: **Python 3.9+**

Install required packages:

```bash
pip install bleak requests asyncio

# 1. Create a folder for the project
mkdir ~/SmartHome && cd ~/SmartHome

# 2. Create the virtual environment
python -m venv env

# 3. Activate the environment
source env/bin/activate

#4. if bleak and request not install use this
# Install libraries globally
pip install bleak --break-system-packages
pip install requests --break-system-packages

# Install the core Bluetooth system packages
sudo apt update
sudo apt install bluez bluetooth pi-bluetooth

# Enable and start the Bluetooth service
sudo systemctl enable bluetooth
sudo systemctl start bluetooth

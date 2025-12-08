# ESP32 SPI LED Control - Master & Slave Communication

## Project Overview
This project demonstrates SPI communication between two ESP32 boards:
- **Master**: Uses HSPI to control the Slave
- **Slave**: Uses VSPI to receive commands from Master

## Wiring Diagram

ESP32 Master (HSPI) ESP32 Slave (VSPI)
GPIO15 (SS) ------> GPIO5 (SS)
GPIO14 (SCK) ------> GPIO18 (SCK)
GPIO12 (MISO) <------ GPIO19 (MISO)
GPIO13 (MOSI) ------> GPIO23 (MOSI)
GND ------> GND


**Optional**: Connect 3.3V to 3.3V for common power supply.

## Windows Setup Instructions

### 1. Install PlatformIO Extension in VSCode
- Open VSCode
- Go to Extensions (Ctrl+Shift+X)
- Search for "PlatformIO IDE" and install

### 2. Open Project
- Clone or extract this project to a folder
- In VSCode: File → Open Folder → Select the project folder

### 3. Configure COM Ports (Windows)
1. Connect both ESP32 boards via USB
2. Open Device Manager (Win + X → Device Manager)
3. Check under "Ports (COM & LPT)" for:
   - USB Serial Device (usually COM3, COM4, etc.)
   - Silicon Labs CP210x USB to UART Bridge
4. Note the COM port numbers for each ESP32

### 4. Update platformio.ini
Edit `platformio.ini` and change:
```ini
[env:master]
upload_port = COM3  ; Change to your Master ESP32 COM port

[env:slave]
upload_port = COM4  ; Change to your Slave ESP32 COM port


# Build and Upload

## Build and upload Master
pio run -e master --target upload

## Build and upload Slave
pio run -e slave --target upload


# For Master (HSPI):

## Build Master
pio run -e master

## Upload Master (replace COM3 with your actual port)
pio run -e master --target upload --upload-port COM3

## Alternative: Clean build and upload
pio run -e master -t clean && pio run -e master && pio run -e master -t upload --upload-port COM3


# For Slave (VSPI):

## Build Slave
pio run -e slave

## Upload Slave (replace COM4 with your actual port)
pio run -e slave --target upload --upload-port COM4

## Alternative: Clean build and upload
pio run -e slave -t clean && pio run -e slave && pio run -e slave -t upload --upload-port COM4


# Monitor Master Output:
## Open serial monitor for master
pio device monitor --port COM3 --baud 115200

# Monitor Slave Output
## Open serial monitor for slave
pio device monitor --port COM4 --baud 115200


# Monitor Generic Output
pio device monitor --project-dir .


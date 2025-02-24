# ESP32-CAM Web Server

## Overview
This project implements a web server for the AI-Thinker ESP32-CAM module, providing functionalities like streaming live camera feed, capturing images, controlling onboard LEDs, and logging interactions.

### Features:
- Live MJPEG streaming
- Capture images in JPEG format
- Control front and back LEDs
- OTA updates over WiFi
- Remote logging and counters
- API for interaction with the ESP32-CAM

## Hardware Requirements
- AI-Thinker ESP32-CAM module
- FTDI Programmer (for flashing firmware)
- Power supply (e.g., 5V 2A adapter)

## Software Requirements
- Arduino IDE with ESP32 board package
- Required libraries:
  - ArduinoOTA
  - WiFi
  - HTTPClient
  - WebServer
  - OV2640

## API Endpoints
The following endpoints are available for interaction:

| Endpoint            | Method | Description                     |
|--------------------|--------|---------------------------------|
| `/`                | GET    | API overview                   |
| `/status`         | GET    | Returns system status           |
| `/jpg`            | GET    | Captures and returns an image  |
| `/flash`          | GET    | Captures an image with flash   |
| `/stream`         | GET    | Starts MJPEG streaming         |
| `/reboot`         | GET    | Reboots the ESP32-CAM          |
| `/back-led-on`    | GET    | Turns on the back LED          |
| `/back-led-off`   | GET    | Turns off the back LED         |
| `/front-led-on`   | GET    | Turns on the front LED         |
| `/front-led-off`  | GET    | Turns off the front LED        |

## Setup Instructions
1. **Flash the ESP32-CAM**
   - Connect the ESP32-CAM to an FTDI programmer
   - Open the Arduino IDE and select the correct board (AI Thinker ESP32-CAM for ESP32-CAM) and port
   - Upload the firmware

2. **Connect to WiFi**
   - Modify `login.h` with your WiFi credentials

3. **Access the Web Server**
   - Once connected, access `http://<ESP32_IP>/` in a browser

## Future Improvements
- Implement WiFi sleep mode for power saving
- Create directory on startup for logging
- Enhance security with authentication

## Troubleshooting
- **Camera initialization failed**: Ensure correct wiring and sufficient power supply.
- **Cannot connect to WiFi**: Double-check SSID and password in `login.h`.
- **Flashint the code faild**: Try use the ESP Bord Manager library ESP32 Version: 2.0.14 from espresif.
- **Can't find the board AI Thinker**: Search on Youtube for a vido that shows how to upload code to the ESP32-CAM.
- **OTA updates fail**: Ensure the device is on the same network as the uploader. It coud alsow be the partition Scheme, I recomend the Minimal SPIFFS.

## License
This project is open-source and free to edit or distribute.

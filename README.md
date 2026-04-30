# cse525-final

> Group 6: Malcolm Alexander, Brittney Garcia Camarillo, Ernest Harris, and Austyn Reichback

Brief Overview: Multiple ESP32-C3 nodes that connect to a central Raspberry Pi controller node. Each node can control either one or several sensors/outputs.

## Hardware

### Sensors/Inputs

- PIR Motion Sensor [HC-SR501](datasheets/SR501%20Motion%20Sensor.pdf)
- Normally Open Magnetic Reed Switch [insert link here]()
- Temperature via 10k Thermistor. [Adafruit Guide](https://learn.adafruit.com/thermistor/using-a-thermistor)

### Outputs

- LED via transistor
- Addressable RGB LED
  - This one is optional, but could show another output if we want.

### API

The API documentation can be [found here](docs/API.md).

## Table of Contents

- [cse525-final](#cse525-final)
  - [Hardware](#hardware)
    - [Sensors/Inputs](#sensorsinputs)
    - [Outputs](#outputs)
    - [API](#API)
  - [Table of Contents](#table-of-contents)
  - [Project Structure](#project-structure)
  - [Project Setup](#project-setup)
    - [PlatformIO](#platformio)
    - [Arduino](#arduino)
  - [System Architecture](#system-architecture)
  - [Troubleshooting](#troubleshooting)
    - [Device Not Connecting to WiFi](#device-not-connecting-to-wifi)
    - [Device connected to WiFi but not server](#device-connected-to-wifi-but-not-server)
    - [No Serial Output](#no-serial-output)

## Project Structure

The project has the following file structure:

```plaintext
<project root>/
├─ datasheets/
├─ examples/
│  ├─ arduino/
│  └─ platformio/
├─ src/
│  ├─ client/
│  │  └─ <folder for each client type>/
│  ├─ server/
│  │  ├─ flask/
│  │  └─ webserver/
│  └─ README.md
└─ README.md
```

## Project Setup

The board can be developed in either [PlatformIO](https://platformio.org/) or in regular [Arduino IDE](https://docs.arduino.cc/software/ide/). This guide will cover both.

### PlatformIO

1. Install [PlatformIO](https://platformio.org/)
   - This is available on CLion as well as VSCode.
2. Create Project
   - **Board:** `esp32-c3-devkitm-1`
   - **Framework:** Arduino
3. Open the generated `platformio.ini` file, and replace the contents with the contents from [the example](examples/platformio/platformio.ini).
4. Open `src/main.cpp` and replace the contents with the contexts from [the example](examples/platformio/src/main.cpp).

### Arduino

1. Install [ArduinoIDE](https://docs.arduino.cc/software/ide/)
2. Install ESP32 board support
   - The IDE will likely prompt you to install this if you have the dev board plugged in. If not, navigate the `Board Manager` window and find `ESP32 by Espressif Systems`.
3. Select `ESP32C3 Dev Module` as the board
4. Set `Tools -> USB CDC On Boot` to `Enabled`.

## System Architecture

The architecture of the network has the following components:

- ESP32 Nodes (clients)
- Raspberry Pi GPIO (client)
- Raspberry Pi running Flask (server)

This is the general flow:

1. Client boots
2. Client "registers" with the server
   - Each node is assigned a UUID, and each function that that node provides is assigned a UUID. The server keeps a list of the nodes connected, and keeps track of the UUIDs.
3. Server returns the relevant UUIDs for the client.
4. The client periodically sends updates to the servers, using the UUIDs given by the server.
   - The client will also check for any commands it receives from the server

The server will provide a simple web interface.

## Troubleshooting

### Device Not Connecting to WiFi

These ESP32 modules can be very picky when it comes to connecting to WiFi.
They can have trouble connecting to certain routers and certain networks, depending on the WiFi version and security protocol.

This is my recommended order for troubleshooting them:

1. Triple check the credentials
2. Make sure you are using a 2.4GHz wifi network, they will not connect to 5Ghz networks.
   - Connecting to a WPA or WPA2 Personal network should work. WPA3 is *very* hit or miss.

### Device connected to WiFi but not server

1. Ensure it's connected to the wifi.
2. Ensure the Raspberry Pi IP is correct
3. Ensure both are connected to the same network.

### No Serial Output

1. Ensure USB CDC is enabled.
2. Reconnect the serial terminal after uploading

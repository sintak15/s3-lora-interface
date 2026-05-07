# Heltec LoRa Interface (s3-lora-interface)

Welcome to the Heltec LoRa Interface project! This project allows you to build your own handheld, off-grid communication device. It pairs an ESP32-S3 microcontroller (which provides a screen and WiFi capabilities) with a Heltec LoRa radio (running the popular Meshtastic firmware). 

Together, they create a powerful tool for sending text messages, viewing GPS maps, and sharing data over long distances without needing cellular service or the internet.

## How It Works

1. **The Radio (Heltec LoRa):** Handles sending and receiving messages over the air using LoRa (Long Range) radio frequencies.
2. **The Interface (ESP32-S3):** Acts as the "brains" for the user interface. It connects to the radio to read messages, displays information on its screen, and broadcasts a local WiFi network so you can connect to it with your smartphone or computer.
3. **The GPS:** Provides your current location for mapping.

## Hardware Wiring Guide

To get started, you will need to connect the different components together using jumper wires. 

### 1. Connecting the Interface to the Radio (Serial Communication)
These connections allow the two boards to talk to each other.
*   **ESP32-S3 `GPIO14` (RX)** connects to **Heltec `GPIO39` (TX)**
*   **ESP32-S3 `GPIO21` (TX)** connects to **Heltec `GPIO38` (RX)**
*   **ESP32-S3 `GND` (Ground)** connects to **Heltec `GND` (Ground)**

### 2. Connecting the GPS Module
The GPS module needs to share its location data with *both* boards. You will need to split the "TX" (transmit) wire from the GPS so it goes to both devices.
*   **GPS TX** splits and connects to both **Heltec `GPIO41` (RX)** AND **ESP32-S3 `GPIO3` (RX)**
*   **GPS RX** connects to **Heltec `GPIO42` (TX)**
*   **GPS VCC (Power)** connects to **Heltec 3V3** (or another suitable 3.3V power pin)
*   **GPS GND (Ground)** connects to a **GND** pin on both the Heltec and the ESP32-S3

*Note: The ESP32-S3 display reads the raw GPS data directly on `GPIO3`. It does not rely on the Heltec radio to send it location packets for its screen or built-in maps.*

### 3. MicroSD Card Setup (Optional)
If your ESP32-S3 board has a built-in MicroSD card slot (like the LCDWiki board), it uses the following internal connections. You generally do not need to wire these yourself if the slot is built into the screen, but they are configured in `constants.h` as follows:
*   **SD CLK:** `GPIO38`
*   **SD CMD:** `GPIO40`
*   **SD D0-D3:** `GPIO39`, `GPIO41`, `GPIO48`, `GPIO47`

You can use the SD card to store offline map tiles (as raw RGB565 files) and to save device logs.

## Software Configuration

### Heltec Meshtastic Settings
You must configure your Heltec LoRa radio to communicate properly with the ESP32-S3. Using the Meshtastic app or web interface, apply these settings to the Heltec's "Serial" module configuration:
*   **Serial enabled:** Yes
*   **Mode:** PROTO (or Protobuf)
*   **Baud rate:** `115200`
*   **RX pin:** `38`
*   **TX pin:** `39`

Next, configure the GPS settings on the Heltec:
*   **GPS RX pin:** `41`
*   **GPS TX pin:** `42`

## How to Use the Device

1.  **Install the Software:** Flash (upload) the `s3-lora-interface.ino` program to your ESP32-S3 board using the Arduino IDE or your preferred tool.
2.  **Connect via WiFi:** Once powered on, the ESP32-S3 will create its own WiFi network. Open your smartphone or computer's WiFi settings and connect to the network named **`Heltec-LoRa-Interface`**.
3.  **Enter Password:** The default WiFi password is **`12345678`**.
4.  **Open the Web Dashboard:** Open a web browser (like Chrome or Safari) and go to the address **`http://192.168.4.1/`**.

### Features of the Web Dashboard

The web interface is a comprehensive control panel for your radio. It includes:
*   **Live Statistics:** View the health and status of your radio connection.
*   **Chat Rooms:** Separate panels for public broadcasts and private messages. Each chat has its own send box and can be opened in a dedicated browser window.
*   **Mapping:** An OpenStreetMap viewer that plots nodes on the network. If you don't have internet access, it will use an offline fallback plot.
*   **File Browser:** If you have an SD card installed, you can browse folders, download logs, create/delete folders, and upload files directly through the web page.
*   **Network Roster:** A list of all known nodes (other radios) on your network.

## Experimental Features (Game Boy Emulator)

This project is currently experimenting with a Game Boy emulator (`peanut_gb`) built into the device, specifically on the `test-gameboy-submenu` branch. 

**Important Note for Testers:** 
There is currently **no on-device menu or screen interface** available to play or test games directly on the ESP32-S3 physical display yet.

The Game Boy functionality is currently **"web-first"**. This means:
*   You must use the Web Dashboard (`http://192.168.4.1/`) to interact with games.
*   Through the web interface, you can upload Game Boy ROMs (`.gb` or `.gbc` files) to a `/roms` folder on your SD card.
*   The web interface can scan the SD card, let you select a ROM, and read the game cartridge metadata.
*   The actual game emulator engine is included in the project's code, but the runtime to play the games on the screen is not yet enabled.

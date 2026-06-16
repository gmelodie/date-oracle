# date-oracle

Random romantic date chooser for **Heltec WiFi LoRa 32 V2** (ESP32 + onboard 128x64 SSD1306 OLED). No external wiring — uses only what's on the board.

## Controls

The V2 has two onboard buttons: **RST** (just resets the chip, don't press it during use) and **PRG** (GPIO 0), the one user input.

- **Tap PRG** -> pick from current category
- **Hold PRG >0.7s** -> next category

## Setup (once)

Put the sketch in a folder of the same name (Arduino requirement):

```sh
mkdir -p date_oracle && mv date_oracle.ino date_oracle/
```

Install `arduino-cli`, then add the ESP32 core and the two display libraries:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306"
```

Plug the board in and find its port:

```sh
arduino-cli board list
```

## Build + upload

```sh
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V2 --upload -p /dev/ttyUSB0 date_oracle
```

Swap `/dev/ttyUSB0` for whatever `board list` showed (macOS is usually `/dev/cu.SLAB_USBtoUART` or `/dev/cu.usbserial-*`).

## Arduino IDE alternative

Open `date_oracle/date_oracle.ino`, select **Tools -> Board -> WiFi LoRa 32(V2)**, pick the port, hit Upload.

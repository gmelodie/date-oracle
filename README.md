# date-oracle

Random romantic date chooser for **Heltec WiFi LoRa 32 V2** (ESP32 + onboard 128x64 SSD1306 OLED). No external wiring — uses only what's on the board.

Flash online using [the **web flasher**](https://gmelodie.com/date-oracle)

## Controls

The V2 has two onboard buttons: **RST** (just resets the chip, don't press it during use) and **PRG** (GPIO 0), the one user input.

- **Tap PRG** -> pick from current category
- **Hold PRG >0.7s** -> next category

## Editing the date ideas

All categories and items live in [`date_oracle/date_ideas.txt`](date_oracle/date_ideas.txt). Format is `[Category Name]` headers with one item per line; blank lines are ignored:

```
[WHAT TO EAT]
Sushi
Pizza, obviously

[DATE IDEA]
Movie on couch
Stargazing drive
```

Up to 8 categories, 32 items each, ~4KB total. A pre-build step bakes the file into the firmware as a fixed-size blob with a magic header so the (upcoming) web flasher can patch it in place without recompiling.

## Setup (once)

Install `arduino-cli`, the ESP32 core, and the two display libraries:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306"
```

Plug the board in and find its port:

```sh
arduino-cli board list
```

## Build + flash

```sh
make flash PORT=/dev/ttyUSB0
```

`make` regenerates `date_ideas_data.h` from `date_ideas.txt` whenever the txt changes, then compiles and uploads. Other targets: `make gen` (only regenerate the header), `make build` (compile, no upload), `make clean` (drop the generated header).

Port hints: macOS is usually `/dev/cu.SLAB_USBtoUART` or `/dev/cu.usbserial-*`; Linux is `/dev/ttyUSB0`.

## Arduino IDE alternative

Run `make gen` once, then open `date_oracle/date_oracle.ino`, select **Tools -> Board -> WiFi LoRa 32(V2)**, pick the port, hit Upload.


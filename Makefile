SKETCH := date_oracle
FQBN   := esp32:esp32:heltec_wifi_lora_32_V2
PORT   ?= /dev/ttyUSB0

GENERATED := $(SKETCH)/date_ideas_data.h
BUILD_DIR := build
FW_OUT    := docs/firmware

.PHONY: gen build flash pages-bin clean

gen: $(GENERATED)

$(GENERATED): $(SKETCH)/date_ideas.txt $(SKETCH)/gen_ideas.py
	python3 $(SKETCH)/gen_ideas.py

build: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

flash: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) --upload -p $(PORT) $(SKETCH)

pages-bin: $(GENERATED)
	mkdir -p $(BUILD_DIR) $(FW_OUT)
	arduino-cli compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) $(SKETCH)
	cp $(BUILD_DIR)/$(SKETCH).ino.merged.bin $(FW_OUT)/firmware.bin
	@printf "wrote %s (%s bytes)\n" $(FW_OUT)/firmware.bin "$$(stat -c%s $(FW_OUT)/firmware.bin 2>/dev/null || stat -f%z $(FW_OUT)/firmware.bin)"

clean:
	rm -rf $(GENERATED) $(BUILD_DIR) $(FW_OUT)

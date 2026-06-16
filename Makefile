SKETCH := date_oracle
FQBN   := esp32:esp32:heltec_wifi_lora_32_V2
PORT   ?= /dev/ttyUSB0

GENERATED := $(SKETCH)/date_ideas_data.h
BUILD_DIR := build
FW_OUT    := docs/firmware

.PHONY: gen build flash pages-bin clean

gen: $(GENERATED)

$(GENERATED): ideas.txt gen_ideas.py
	python3 gen_ideas.py

build: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

flash: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) --upload -p $(PORT) $(SKETCH)

pages-bin: $(GENERATED)
	mkdir -p $(BUILD_DIR) $(FW_OUT)
	arduino-cli compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) $(SKETCH)
	cp $(BUILD_DIR)/$(SKETCH).ino.bin             $(FW_OUT)/firmware.bin
	cp $(BUILD_DIR)/$(SKETCH).ino.bootloader.bin  $(FW_OUT)/bootloader.bin
	cp $(BUILD_DIR)/$(SKETCH).ino.partitions.bin  $(FW_OUT)/partitions.bin
	cp "$$(find $${HOME}/.arduino15 -name boot_app0.bin 2>/dev/null | head -1)" $(FW_OUT)/boot_app0.bin
	@ls -la $(FW_OUT)

clean:
	rm -rf $(GENERATED) $(BUILD_DIR) $(FW_OUT)

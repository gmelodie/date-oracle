SKETCH := date_oracle
FQBN   := esp32:esp32:heltec_wifi_lora_32_V2
PORT   ?= /dev/ttyUSB0

GENERATED := $(SKETCH)/date_ideas_data.h

.PHONY: gen build flash clean

gen: $(GENERATED)

$(GENERATED): $(SKETCH)/date_ideas.txt $(SKETCH)/gen_ideas.py
	python3 $(SKETCH)/gen_ideas.py

build: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

flash: $(GENERATED)
	arduino-cli compile --fqbn $(FQBN) --upload -p $(PORT) $(SKETCH)

clean:
	rm -f $(GENERATED)

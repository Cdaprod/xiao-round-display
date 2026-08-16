SHELL := /bin/bash

PIO  ?= pio
ENV  ?= seeed_xiao_esp32c3_compat
PORT ?= /dev/cu.usbmodem2101
BAUD ?= 115200

.DEFAULT_GOAL := help

.PHONY: help ports check-port clean build upload flash monitor flash-monitor size

help: ; @echo "XIAO ESP32-C3 Round Rig Controller"; \
echo ""; \
echo "make ports          List serial devices"; \
echo "make clean          Clean build artifacts"; \
echo "make build          Compile firmware"; \
echo "make upload         Upload firmware"; \
echo "make flash          Clean, build, and upload"; \
echo "make monitor        Open serial monitor"; \
echo "make flash-monitor  Flash and open monitor"; \
echo "make size           Display memory usage"; \
echo ""; \
echo "Override the port with:"; \
echo "make flash PORT=/dev/cu.usbmodem1101"

ports: ; $(PIO) device list

check-port: ; @test -e "$(PORT)" || { \
echo "Serial port not found: $(PORT)"; \
echo "Run: make ports"; \
echo "Then: make flash PORT=/dev/cu.usbmodemXXXX"; \
exit 1; \
}

clean: ; $(PIO) run -e "$(ENV)" --target clean

build: ; $(PIO) run -e "$(ENV)"

upload: check-port ; $(PIO) run -e "$(ENV)" --target upload --upload-port "$(PORT)"

flash: ; @set -e; \
$(MAKE) clean ENV="$(ENV)"; \
$(MAKE) build ENV="$(ENV)"; \
$(MAKE) upload ENV="$(ENV)" PORT="$(PORT)"

monitor: check-port ; $(PIO) device monitor --port "$(PORT)" --baud "$(BAUD)" --filter time --filter esp32_exception_decoder

flash-monitor: ; @set -e; \
$(MAKE) flash ENV="$(ENV)" PORT="$(PORT)"; \
$(MAKE) monitor PORT="$(PORT)" BAUD="$(BAUD)"

size: ; $(PIO) run -e "$(ENV)" --target size

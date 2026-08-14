SHELL := /bin/bash
.RECIPEPREFIX := >

PIO      ?= pio
ENV      ?= seeed_xiao_esp32c3_compat
PORT     ?= /dev/cu.usbmodem2101
BAUD     ?= 115200

.DEFAULT_GOAL := help

.PHONY: help ports clean build upload flash monitor flash-monitor size

help:
> @echo "XIAO ESP32-C3 Round Rig Controller"
> @echo
> @echo "  make ports          List connected serial devices"
> @echo "  make clean          Clean PlatformIO build artifacts"
> @echo "  make build          Compile compatibility firmware"
> @echo "  make upload         Build if needed and upload firmware"
> @echo "  make flash          Clean, build, and upload"
> @echo "  make monitor        Open the serial monitor"
> @echo "  make flash-monitor  Flash, then open the serial monitor"
> @echo "  make size           Display firmware memory usage"
> @echo
> @echo "Overrides:"
> @echo "  make flash PORT=/dev/cu.usbmodem1101"
> @echo "  make build ENV=seeed_xiao_esp32c3"
> @echo "  make monitor BAUD=115200"

ports:
> $(PIO) device list

clean:
> $(PIO) run -e $(ENV) --target clean

build:
> $(PIO) run -e $(ENV)

upload:
> @if [ ! -e "$(PORT)" ]; then \
>   echo "Serial port not found: $(PORT)"; \
>   echo "Run 'make ports' and provide PORT=/dev/cu.usbmodemXXXX"; \
>   exit 1; \
> fi
> $(PIO) run -e $(ENV) \
>   --target upload \
>   --upload-port $(PORT)

flash:
> $(MAKE) clean ENV=$(ENV)
> $(MAKE) build ENV=$(ENV)
> $(MAKE) upload ENV=$(ENV) PORT=$(PORT)

monitor:
> @if [ ! -e "$(PORT)" ]; then \
>   echo "Serial port not found: $(PORT)"; \
>   echo "Run 'make ports' and provide PORT=/dev/cu.usbmodemXXXX"; \
>   exit 1; \
> fi
> $(PIO) device monitor \
>   --port $(PORT) \
>   --baud $(BAUD) \
>   --filter time \
>   --filter esp32_exception_decoder

flash-monitor:
> $(MAKE) flash ENV=$(ENV) PORT=$(PORT)
> $(MAKE) monitor PORT=$(PORT) BAUD=$(BAUD)

size:
> $(PIO) run -e $(ENV) --target size

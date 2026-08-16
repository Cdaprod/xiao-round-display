# PlatformIO build, flash, and monitor loop

Copy these files into the root of the PlatformIO project, beside
`platformio.ini`.

## Linux: everything in Docker

Connect the XIAO ESP32-C3, identify its port, then run:

```bash
SERIAL_DEVICE=/dev/ttyACM0 docker compose up --build
```

The service performs an initial build/upload, starts the serial monitor, and
watches `platformio.ini`, `src/`, `include/`, `lib/`, `data/`, and test files.
On a change it stops the monitor, incrementally rebuilds and uploads, then
reconnects the monitor.

If Linux reports permission denied, add your user to the serial-device group
(commonly `dialout`) and reconnect the board. Do not use `privileged: true` just
to bypass a permissions problem.

## macOS: Docker build, host flash/monitor

Docker Desktop runs containers inside a Linux VM. A path such as
`/dev/cu.usbmodem2101` is a macOS device node and cannot be used by the simple
Compose `devices` mapping above.

Use Docker for a clean build:

```bash
docker compose --profile build run --rm build-only
```

For the fast edit/flash/monitor loop, use the same supervisor natively where
PlatformIO already has access to the USB device:

```bash
PIO_ENV=xiao_esp32c3 \
PIO_PORT=/dev/cu.usbmodem2101 \
PIO_BAUD=115200 \
python3 tools/pio-dev.py
```

This is the recommended Mac workflow. Docker Desktop USB/IP can expose USB
hardware to a container, but it adds a privileged helper and attachment steps,
so it is intentionally not part of this basic setup.

## Configuration

Copy `.env.example` to `.env` when you want persistent Linux/Compose values.
The defaults match the current project environment and monitor command:

- Environment: `xiao_esp32c3`
- Container port: `/dev/ttyACM0`
- Baud: `115200`
- Filters: `time,esp32_exception_decoder`

Stop the loop with `Ctrl+C`.

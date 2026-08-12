# Changelog

## 2.1.0 - 2026-08-11

- Corrected the hardware target to the Seeed Studio XIAO ESP32-C3.
- Removed ESP32-S3 native-USB build flags; the C3 board uses its serial bridge.
- Changed the network worker to an unpinned FreeRTOS task for the single-core C3.
- Set the default halo target to 30 FPS and compatibility mode to 24 FPS.
- Retained the 80 MHz SPI DMA display path with a 40 MHz non-DMA fallback.
- Corrected the on-screen Device page and all build/upload documentation.

## 2.0.0 - 2026-08-11

- Split the original monolithic `main.cpp` into firmware modules.
- Added the animated Kinetic Status Halo.
- Added cubic-Bezier orbital acceleration with continuous baseline movement.
- Added independent breathing, color drift, and state palette crossfades.
- Added an SPI DMA display backend at 80 MHz.
- Moved blocking HTTP requests to a FreeRTOS worker task.
- Replaced blocking Wi-Fi connection waits with a retrying state machine.
- Replaced blocking touch-release waits with event polling.
- Added Network, Device, and Controls pages for operation without media.
- Changed connected/no-session state to green `RIG READY` while keeping offline states amber.
- Preserved `/rig.cfg`, Media Sync API routes, device headers, battery display, and session controls.
- Added a 40 MHz non-DMA compatibility build environment.

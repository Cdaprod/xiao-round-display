# Changelog

## 2.0.0 - 2026-08-11

- Split the original monolithic `main.cpp` into firmware modules.
- Added the animated Kinetic Status Halo.
- Added cubic-Bezier orbital acceleration with continuous baseline movement.
- Added independent breathing, color drift, and state palette crossfades.
- Added ESP32-S3 SPI DMA display backend at 80 MHz.
- Moved blocking HTTP requests to a FreeRTOS worker task.
- Replaced blocking Wi-Fi connection waits with a retrying state machine.
- Replaced blocking touch-release waits with event polling.
- Added Network, Device, and Controls pages for operation without media.
- Changed connected/no-session state to green `RIG READY` while keeping offline states amber.
- Preserved `/rig.cfg`, Media Sync API routes, device headers, battery display, and session controls.
- Added a 40 MHz non-DMA compatibility build environment.

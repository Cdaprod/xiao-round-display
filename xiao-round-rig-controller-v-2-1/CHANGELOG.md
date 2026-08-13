# Changelog

# Unreleased

- Added continuous, testable gesture recognition and inertial panel scrolling.
- Added expandable category navigation, action panels, keyboard editing, and confirmations.
- Added Wi-Fi retry diagnostics, asynchronous scanning, and NVS configuration overrides.
- Pinned Arduino_GFX 1.6.0 and made the 40 MHz compatibility environment the default.
- Split the Device page's SD-card and rig-configuration status indicators.
- Close the status halo at the 360-degree boundary without removing segment overlap.

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

## Unreleased compositor redesign

- Repaired dirty-row transfers, coalesced halo deadlines, bounded 24 FPS touch feedback, and nonblocking Wi-Fi attempt diagnostics.
- Fixed ESP32 GCC 8 compilation by explicitly constructing circular spans and normalizing span-width arithmetic to `int`.
- Removed the persistent bottom category bar; each circular summary is now its category.
- Added horizontal summary navigation, title/swipe/hold expansion, full circular panels, pinned back headers, and restrained action rows.
- Replaced visible direct erase/redraw with a one-time allocated indexed compositor and circular scanline transfers.
- Added shared round viewport geometry for rendering and hit testing, granular invalidation, render counters, and tactile perimeter feedback.
- Integrated keyboard and confirmation modes into the same compositor while preserving panel scroll state.

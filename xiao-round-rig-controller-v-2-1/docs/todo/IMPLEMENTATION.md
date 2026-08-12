# Interaction and configuration milestone

## Completed in this milestone

- [x] Separate raw CHSC6X sampling from deterministic gesture recognition.
- [x] Recognize press, drag, swipe, tap, live hold, release, and cancellation without blocking.
- [x] Add direct category selection, selected-category expansion, center return, and explicit UI modes.
- [x] Add deterministic inertial scrolling, resistance, bounds recovery, and pull-down collapse.
- [x] Add scrollable Status, Network, Device, and Controls action panels with safe recording action gating.
- [x] Add asynchronous Wi-Fi scan/retry controls, failure dwell, countdown, and reason diagnostics.
- [x] Apply configuration precedence of defaults, SD bootstrap, then NVS overrides.
- [x] Add NVS editors for SSID, password, API URL, node ID, and bearer token.
- [x] Add an on-demand masked round keyboard with layouts, clear, space, backspace, reveal, cancel, and confirm.
- [x] Add reusable confirmation UI for Wi-Fi forget, override clear, and reboot.
- [x] Keep the closed, elapsed-time-driven status halo and make compatibility mode the default.
- [x] Add host tests for gesture, scrolling, configuration policy, and Wi-Fi retry behavior.

## Physical validation remaining

- [ ] Complete the 20-step on-device acceptance checklist in `README.md` using compatibility mode.
- [ ] Confirm CHSC6X coordinate orientation and tune tab/key hit boxes if the installed display is rotated.
- [ ] Validate optional 80 MHz DMA mode only after compatibility mode passes without artifacts.

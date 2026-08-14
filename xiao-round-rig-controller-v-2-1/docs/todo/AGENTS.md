- [x] Replace `.RECIPEPREFIX` recipes with a tab-free GNU Make 3.81-compatible lowercase makefile.
- [x] Verify help, clean, build, upload, flash, monitor, size, overrides, and missing-port failure paths.

- [x] Add project-root Make targets for clean, build, upload, flash, monitor, and size.
- [x] Add a 40 ms release-stabilized input barrier around menu open/close transitions.
- [ ] Verify the top-header open/close bounce fix on physical touch hardware.

- [x] Make halo palettes category-owned with localized operational overlays.
- [x] Add Entering/Ambient/Touching/Holding/Success/Failure/Exiting/Hidden lifecycle.
- [x] Stop halo scheduling and transfers while menus and keyboards own the view.
- [x] Preserve phase across category crossfades and stop counting intentional idle as dropped.
- [ ] Migrate UI composition/input to pinned LVGL 8.3 after compatibility build dependencies are available.
- [ ] Convert the halo into an LVGL-owned custom object before enabling LVGL display flush ownership.

- [x] Assign a sanitized DHCP hostname before Wi-Fi association.
- [x] Start an independent port-80 HTTP and mDNS service after DHCP succeeds.
- [x] Separate LAN readiness from Media Sync API readiness.
- [x] Add redacted status/config endpoints and authenticated nonblocking retry requests.
- [ ] Resolve physical AP authentication reason 202, then verify DHCP, mDNS, and HTTP on cda_Lab.

- [x] Separate menu highlighting, scrolling, and 600 ms hold activation.
- [x] Prevent scroll/tap release from opening editors or executing actions.
- [x] Add circular-safe BACK/CANCEL, transactional keyboard controls, and modal input barriers.
- [x] Add DEL, DONE validation, safe-area geometry checks, and software recovery.
- [ ] Physically validate circular control bounds, hold thresholds, and all recovery paths.

- [x] Lock each touch session to its starting layer and a single gesture owner.
- [x] Remove generic release-to-collapse behavior and retain menus after scrolling.
- [x] Add deliberate header/at-top pull-down close rules and action tap cancellation.
- [x] Add coordinate filtering, 30 ms invalid-sample grace, and arbitration host coverage.
- [ ] Validate touch thresholds and menu persistence on physical XIAO ESP32-C3 hardware.
- [x] Debounce touch into two-sample press and three-sample release sequences.
- [x] Make holds one-shot and prevent live touches crossing menu transitions.
- [x] Restrict category opening to the center control and row execution to a fresh 600 ms hold.
- [ ] Physically verify top-edge holds, menu persistence, and single row execution.


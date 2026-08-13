# Round compositor milestone

## Completed

- [x] Make semantic row invalidation and bounded span transfers operational; coalesce expired halo/feedback frames.
- [x] Add Wi-Fi attempt/status/stage/elapsed/reason diagnostics without restarting active attempts.
- [x] Restore ESP32 GCC 8 compatibility with explicit circular-span construction and type-safe span-width arithmetic.
- [x] Remove the persistent bottom category bar and make each summary the full circular category.
- [x] Add tap cycling, horizontal previous/next swipes, title/swipe/hold expansion, pinned panel headers, and universal center return.
- [x] Replace direct center erase/redraw with a one-time allocated indexed compositor and atomic circular span presentation.
- [x] Share precomputed circular span geometry between drawing and hit testing.
- [x] Clear the complete owned radius across summary, panel, keyboard, confirmation, cancellation, and Status transitions.
- [x] Replace saturated rectangular action stacks with clipped labels, markers, separators, disabled reasons, and danger accents.
- [x] Preserve full-panel inertial scrolling and scroll position across snapshot and modal updates.
- [x] Add immediate contact, hold, drag cancellation, opposite-edge echo, disabled recoil, and outcome feedback without restarting halo phase.
- [x] Add granular invalidation and redraw/composition/transfer/touch/heap counters.
- [x] Add deterministic circular geometry, clipping, scroll reachability, invalidation, and ownership tests.
- [x] Document the old and new pixel ownership models, fallback, controls, and validation procedure.

## Hardware validation remaining

- [ ] Run the physical compositor acceptance checklist in `README.md` using compatibility mode.
- [ ] Record startup heap before/after indexed allocation plus PlatformIO RAM/flash totals.
- [ ] Confirm contact presentation is under 20 ms and tune touch orientation/hit spans if required.
- [ ] Validate experimental 80 MHz DMA only after compatibility mode is artifact-free.

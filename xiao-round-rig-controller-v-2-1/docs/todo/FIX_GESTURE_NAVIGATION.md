# Gesture navigation simplification

The summary uses one deterministic grammar: horizontal movement navigates,
center tap opens, and vertical movement or holding does nothing. Horizontal
ownership begins at 18 px with 1.15 axis dominance; 30 px commits, with a
20 px and 300 px/s fast path. The center control radius is 72 px and permits
14 px tap drift.

Menus use vertical movement only for scrolling. Pressing a settled valid row
focuses it and begins its 600 ms hold; release never executes. Movement beyond
8 px cancels activation for that sequence. The fixed header tap closes one
level, while a stationary 1200 ms header hold returns to Status.

Touch stability uses elapsed time rather than sample counts: 20 ms press,
70 ms release, and 40 ms unreadable-sample grace. Layer transitions retain the
release barrier, and menus/modal views keep the halo hidden.

## Physical acceptance

- [ ] Verify 30 px and slightly diagonal horizontal swipes cycle exactly once.
- [ ] Verify center taps open reliably and vertical summary drags do nothing.
- [ ] Verify a new row can be held directly for one execution at 600 ms.
- [ ] Verify menu scrolling never executes or closes on release.
- [ ] Verify header tap goes back and 1200 ms header hold returns to Status.
- [ ] Verify menu, keyboard, and confirmation views schedule no halo frames.

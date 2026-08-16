# Menu input capture repair

The input lifecycle now debounces contact into stable touch sequences, emits a
hold only once per sequence, and treats failed reads as unknown rather than as
release. Menu opening and closing are idempotent and establish a release
barrier so the destination view cannot inherit a live pointer.

Category opening is limited to the center control. The fixed menu header owns
back navigation. Menu rows capture the row at touch-down, scrolling cancels
activation, an eight-pixel displacement cancels holding, and a settled
highlighted row executes once after a stationary 600 ms hold.

Transition telemetry is emitted only for down, hold/navigation, row execution,
and resolution events. No application modes are part of this repair.

## Physical acceptance

- [ ] Hold the top edge continuously and verify the menu never flashes.
- [ ] Open from the center, keep holding, and verify the menu remains open.
- [ ] Tap a row to focus it, then hold it for 600 ms and verify one execution.
- [ ] Drag the menu and verify release executes and closes nothing.
- [ ] Verify the halo completes exit once and stays hidden in the menu.

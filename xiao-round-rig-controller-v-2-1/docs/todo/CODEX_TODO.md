Paste this entire task into Codex. It explicitly tells it not to stop after the first ledger item.



Work exclusively in:

/workspace/xiao-round-display/xiao-round-rig-controller-v-2-1

Continue from commit:

69fa6e7 — Fix device config status and halo seam

This is a Seeed Studio XIAO ESP32-C3 attached to the Seeed 1.28-inch 240×240 GC9A01 capacitive Round Display. It is not an ESP32-S3.

Complete the entire interaction and configuration milestone in one uninterrupted implementation pass. Do not stop after completing only the next unchecked ledger entry. Implement every requirement below, test the integrated system, update documentation, and commit the completed milestone.

Current hardware and build constraints:

- MCU: Seeed Studio XIAO ESP32-C3
- Display: GC9A01, 240×240, SPI
- Touch: CHSC6X, I²C address 0x2E
- LCD CS: D1
- LCD DC: D3
- LCD backlight: D6
- Touch interrupt: D7
- SD CS: D2
- Battery ADC: D0
- Primary development environment: `seeed_xiao_esp32c3_compat`
- Compatibility renderer: conventional SPI, 40 MHz, 24 FPS
- Experimental renderer: SPI DMA, 80 MHz, 30 FPS
- Arduino_GFX must remain pinned to 1.6.0 while using the current PlatformIO Espressif 32 platform. Do not reintroduce a version requiring `esp32-hal-periman.h`.
- Preserve the existing Media Sync API contract.
- Preserve the working animated status halo.
- Do not target or introduce ESP32-S3 configurations.
- Do not migrate the project to LVGL during this milestone.
- Do not hardcode `/dev/cu.usbmodem1101` or `/dev/cu.usbmodem2101`; macOS may enumerate either port.
- Do not commit real Wi-Fi passwords, bearer tokens, or other credentials.

# Objective

Turn the existing four read-only information pages into a responsive, expandable, gesture-controlled interface with:

- Reliable continuous touch gesture recognition
- Direct category selection
- Expandable category action panels
- Fluid inertial scrolling
- Useful Wi-Fi failure diagnostics
- Wi-Fi scanning and reconnection controls
- Persistent on-device configuration editing
- A custom on-demand round-screen keyboard
- Safe action execution
- Clear feedback, animations, and disabled-action explanations

The four categories remain:

1. Status
2. Network
3. Device
4. Controls

# Required interaction contract

Implement the following behavior exactly and consistently.

## Collapsed summary mode

- A normal center tap cycles:
  `Status → Network → Device → Controls → Status`
- Swiping upward starting on a category tab selects that category directly.
- Tapping the already-selected category expands its action panel.
- Holding any category tab for approximately 450 ms selects and expands it immediately.
- Long-pressing the center for approximately 700 ms returns directly to the collapsed Status page.
- A drag or swipe must cancel pending tap and hold recognition.

## Expanded category mode

- Vertical drag scrolls the category’s content 1:1 with the finger.
- Releasing after a drag produces inertial scrolling.
- Scrolling decelerates smoothly using time-based friction.
- Scrolling beyond the top or bottom uses soft resistance rather than abruptly stopping.
- Swiping down while already at the top collapses the expanded panel.
- Pulling down at least 36 pixels beyond the top collapses the panel when released.
- Tapping an action executes it.
- Tapping an editable row opens its editor or keyboard.
- Long-pressing the center closes all overlays and returns to collapsed Status.
- The halo should dim and may temporarily reduce to 12–15 FPS while a panel is actively moving.
- When interaction stops, restore the configured 24/30 FPS target.

## Keyboard/editing mode

- The keyboard exists only while editing.
- It must not consume display or interaction state during ordinary use beyond its static object storage.
- Provide lowercase, uppercase, and number/symbol layouts.
- Provide Backspace, Space, Cancel, Clear, layout switching, and Confirm.
- Password and token values must be masked.
- Allow a temporary reveal/hide control without printing the value to Serial.
- Swipe down on the keyboard header to cancel/dismiss.
- Confirm saves the value and returns to the originating expanded panel.
- Long-press center cancels editing and returns to Status.
- Never print passwords or bearer tokens to Serial, logs, error messages, or the display.

The keyboard must support editing:

- Wi-Fi SSID
- Wi-Fi password
- API base URL
- Node ID
- Bearer token

Use a compact round-safe button matrix. Favor usable touch targets over reproducing a desktop keyboard exactly. Ensure all interactive keys remain within the usable circular display region.

# Gesture-recognition architecture

Separate raw CHSC6X sampling from gesture recognition so the recognizer can be tested using injected timestamped samples without physical hardware.

Introduce a pure state-machine component, for example:

- `TouchController`: hardware/I²C sampling
- `GestureRecognizer`: platform-independent recognition
- `TouchEvent`: recognized event sent to the controller/UI

Use names consistent with the existing project where practical.

Extend the gesture event model to represent at least:

- `PressStarted`
- `DragStarted`
- `DragMoved`
- `SwipeUp`
- `SwipeDown`
- `Tap`
- `HoldStarted`
- `Released`
- `Cancelled`

Each event must provide enough information for scrolling and hit-testing:

- Initial X/Y
- Current X/Y
- Previous X/Y where useful
- Total delta X/Y
- Incremental delta X/Y
- Velocity X/Y
- Press duration
- Timestamp

Use these thresholds as the initial defaults:

- Maximum tap movement: 8 px
- Drag activation distance: 14 px
- Category hold: 450 ms
- Center return hold: 700 ms
- Minimum swipe velocity: approximately 180 px/s
- Pull-to-collapse overscroll: 36 px

Requirements:

- Continuously sample coordinates while touch interrupt is active.
- Emit `HoldStarted` once while the finger is still down.
- Do not wait until release to indicate a hold.
- Once dragging begins, cancel tap and hold candidates.
- Do not emit both Tap and Swipe for one interaction.
- Emit Released after a drag or hold.
- Handle `millis()` rollover correctly.
- Avoid `String`, heap allocation, and blocking waits inside sampling and recognition loops.
- Keep raw touch sampling failures from generating false taps.
- Preserve the existing center safety region for recording controls.

# Category navigation UI

Create an explicit, visible radial category rail or tab system suitable for the round screen.

The tabs must make the four categories discoverable without consuming the entire center panel. Use concise labels or symbols such as:

- STATUS
- NET
- DEVICE
- CTRL

Requirements:

- Clearly indicate the selected category.
- Provide pressed, holding, selected, disabled, and expanded visual states.
- Show radial hold progress around or behind a tab.
- Animate selection and expansion with time-based cubic-Bézier easing.
- Do not use frame-count-dependent animation.
- Keep all actionable areas inside the reliable circular touch region.
- Do not leave the old accidental three-o’clock halo seam.
- If a deliberate notch is introduced for navigation, it must be visually intentional, stable, symmetric, and documented. Otherwise keep the halo closed.
- Preserve the animated multicolor status palette around the interface.

Introduce explicit navigation state, for example:

- `Summary`
- `TabHolding`
- `Expanding`
- `Expanded`
- `Scrolling`
- `Collapsing`
- `Editing`
- `Keyboard`

Do not use one large collection of loosely related booleans if a state enum can represent the UI mode safely.

# Fluid scrolling

Implement deterministic, time-based vertical scrolling for expanded panels.

Requirements:

- Finger drag follows content directly.
- Track release velocity from recent touch samples.
- Apply inertial motion after release.
- Apply friction based on elapsed time.
- Clamp or spring content back into valid bounds.
- Apply resistance during overscroll.
- Support tap hit-testing after accounting for scroll offset.
- Cancel action activation if the finger transitions into a drag.
- Do not allocate heap memory per frame.
- Do not require a full-screen RGB565 framebuffer; the C3 has limited RAM.
- Redraw only the central content region or dirty rows where practical.
- Avoid full-screen clears during animation.
- Prevent text smearing when rows move.
- Avoid visible flicker as much as the non-framebuffer renderer allows.

# Wi-Fi state-machine correction

Fix the current `JOINING` behavior.

The present implementation times out after approximately 15 seconds, but the retry interval is already overdue, so the next retry immediately overwrites the failure state with another `JOINING`. A user never gets a useful failure explanation.

Introduce a separate Wi-Fi connection stage/status model rather than forcing every phase into the top-level `RigState`.

Represent at least:

- Unconfigured
- Scanning
- Connecting
- Authenticating or association pending where detectable
- Waiting for DHCP/IP
- Connected
- No access point found
- Authentication failure
- Connection timeout
- DHCP failure where detectable
- Disconnected
- Retry countdown

Requirements:

- Capture the ESP32 Wi-Fi disconnect reason when the installed Arduino-ESP32 version exposes it.
- Map numeric reasons into short user-readable text.
- Preserve the raw numeric reason in diagnostics.
- After a failure, show the failure for a minimum dwell period.
- Set an explicit `nextRetryAt` when the failure occurs.
- Do not immediately overwrite failure text with `JOINING`.
- Display a visible retry countdown.
- Allow `RETRY NOW` to bypass the countdown.
- Avoid blocking connection loops.
- Keep the halo and touch UI running during scans, connection attempts, API tests, and retries.
- Make all background results cross task boundaries using POD messages or another safe mechanism.
- Do not mutate UI-owned state directly from an asynchronous callback.

# Persistent configuration

Runtime editing cannot depend on writing the SD card while the display owns the shared SPI bus.

Use ESP32 `Preferences`/NVS as the persistent store for settings edited on-screen.

Configuration precedence:

1. Built-in safe defaults
2. `/rig.cfg` imported from SD at boot when available
3. Values previously saved to NVS override imported/default values

Requirements:

- Keep `/rig.cfg` support for bootstrapping and removable-card configuration.
- On-screen edits save to NVS.
- Rebooting must retain on-screen edits.
- Provide `RELOAD SD CONFIG`, which intentionally re-imports the SD configuration.
- Provide `CLEAR SAVED OVERRIDES` with confirmation.
- Never log secrets.
- Avoid unnecessary repeated NVS writes.
- Validate values before saving.
- Trim accidental leading/trailing whitespace except where a password intentionally contains it; do not silently mutate password contents.
- Validate API URLs as `http://` or `https://`.
- Provide explicit configuration status fields:
  - SD card mounted
  - `/rig.cfg` found
  - `/rig.cfg` parsed
  - Wi-Fi configured
  - Node ID configured
  - Device token configured
  - API URL configured
  - NVS overrides active
- Do not use a single `configLoaded` boolean to imply all fields are valid.

# Expanded Status panel

Implement a scrollable Status panel containing:

- Current top-level rig state
- Current Wi-Fi stage
- API state
- Session state
- Battery voltage and estimated percentage
- Uptime
- Free heap
- Halo presented FPS
- Halo dropped frames
- Renderer mode: DMA or compatibility
- LCD SPI frequency
- Firmware version
- Last error

Actions:

- `RETRY FAILED STAGE`
- `REFRESH API`
- `CLEAR LAST ERROR`
- `RETURN TO SUMMARY`

Actions that cannot currently run must be visibly disabled with a concise reason.

# Expanded Network panel

Implement a scrollable Network panel containing:

- Current SSID, without exposing password
- Connection stage
- Last disconnect reason and raw reason code
- IP address
- Gateway
- DNS
- RSSI
- Retry countdown
- API base URL
- API reachability/result

Actions:

- `RETRY NOW`
- `SCAN NETWORKS`
- `SELECT SSID`
- `EDIT SSID`
- `EDIT PASSWORD`
- `EDIT API URL`
- `TEST API`
- `DISCONNECT`
- `FORGET WIFI` with confirmation

Network scanning must be asynchronous.

Display scan results with:

- SSID
- RSSI
- Security/lock indication
- Selected state
- A safe representation for hidden SSIDs

Selecting a network should populate the SSID and then open the password editor when authentication is required.

Do not automatically overwrite a saved password merely by selecting a network with the same SSID.

# Expanded Device panel

Keep the summary-page corrections from commit `69fa6e7`.

Expanded information:

- `SD CARD`
- `RIG.CFG FOUND`
- `RIG.CFG PARSED`
- `NVS OVERRIDES`
- `NODE ID`
- `DEVICE TOKEN` as only `SET` or `NOT SET`
- `API URL`
- `FIRMWARE`
- `UPTIME`
- `FREE HEAP`
- `DISPLAY MODE`
- `TOUCH STATUS`

Actions:

- `EDIT NODE ID`
- `EDIT DEVICE TOKEN`
- `EDIT API URL`
- `RELOAD SD CONFIG`
- `CLEAR SAVED OVERRIDES`
- `DISPLAY TEST`
- `TOUCH TEST`
- `REBOOT DEVICE` with confirmation

Display and touch tests must have an obvious exit path and must not permanently alter settings.

# Expanded Controls panel

Expanded information:

- Whether a live session exists
- Session ID
- Session status
- Desired action
- Chunk count
- Whether a valid device token exists
- Last control response

Actions:

- `POLL SESSION`
- `START RECORDING`
- `STOP RECORDING`
- `ACKNOWLEDGE ERROR`
- `RETURN TO STATUS`

Safety requirements:

- Never execute recording start/stop because of a drag, swipe, release after scrolling, or cancelled touch.
- A recording command requires a clean tap on the action row.
- Keep unavailable actions visible but disabled.
- Display the reason beneath or alongside a disabled action:
  - `NO LIVE SESSION`
  - `TOKEN REQUIRED`
  - `ALREADY RECORDING`
  - `NOT RECORDING`
  - `REQUEST IN PROGRESS`
- Preserve authoritative API state; do not pretend a command succeeded before polling confirms it.

# Confirmation overlay

Implement a reusable confirmation overlay for destructive or disruptive actions:

- Forget Wi-Fi
- Clear NVS overrides
- Reboot device

Requirements:

- Clear action name
- Cancel and Confirm targets
- Default focus/visual emphasis on Cancel
- Swipe down cancels
- Long-press center returns to Status without executing
- No destructive action on initial press; execute only from a clean confirmed tap

# Rendering and performance

Preserve the existing time-based Kinetic Status Halo:

- Cubic-Bézier orbital variation
- Continuous baseline motion
- Independent breathing
- Color drift
- State palette crossfade
- Closed 360-degree seam from commit `69fa6e7`

Requirements:

- No heap allocation in the halo render loop.
- No full-screen clear per frame.
- Static summary text redraws only when its model changes.
- Expanded panels redraw only as needed.
- Animation position remains elapsed-time-based.
- Collect and display measured FPS and dropped-frame statistics.
- Keep network and API work from blocking the renderer.
- Compatibility mode must remain the safe default for physical validation until DMA is proven stable.
- Do not claim 150 FPS. Measure and report actual presentation rate.
- If DMA remains unstable, preserve it as an optional environment without breaking compatibility mode.

# Tests

Add deterministic host-side tests for the pure gesture and scrolling components.

Gesture tests must cover:

1. Clean tap
2. Small movement still recognized as tap
3. Movement beyond threshold becomes drag
4. Drag cancels pending tap
5. Hold emits once while finger remains down
6. Hold followed by release
7. Fast upward swipe
8. Fast downward swipe
9. Slow drag is not misclassified as swipe
10. No Tap plus Swipe double emission
11. Failed coordinate read does not create a false gesture
12. Center long press
13. Category hold
14. Timestamp rollover
15. Release after scrolling does not activate an action

Scrolling tests must cover:

1. Direct drag offset
2. Inertial continuation
3. Friction decay
4. Top overscroll resistance
5. Bottom overscroll resistance
6. Pull-down collapse threshold
7. Scroll-coordinate-adjusted row hit-testing
8. Bounds recovery

Configuration tests should cover:

- SD/default/NVS precedence
- Password preservation
- API URL validation
- Secret redaction
- Clearing overrides
- Invalid configuration status separation

Wi-Fi state tests should cover:

- Timeout is visible before retry
- Retry countdown
- Retry-now behavior
- Failure does not immediately become Joining
- Disconnect reason mapping
- Connected state clears retry countdown

Use pure C++ components wherever possible so these tests can run without Arduino hardware.

# Repository instructions and ledger cleanup

Inspect all applicable repository instructions before editing.

The file:

docs/todo/AGENTS.md

is currently being used as a changing implementation ledger. That is not the correct long-term purpose for an `AGENTS.md` file.

Perform this cleanup:

- Preserve any existing root-level `AGENTS.md`; do not overwrite repository instructions.
- Move the implementation checklist into:
  `docs/todo/IMPLEMENTATION.md`
- Remove `docs/todo/AGENTS.md` after preserving its checklist.
- Update the checklist to include every milestone in this task.
- Check off items only after implementation and testing.
- Document any physical-device validation that remains necessary.

Update:

- `README.md`
- `CHANGELOG.md`
- `docs/todo/IMPLEMENTATION.md`

Document:

- Interaction model
- Gesture thresholds
- Configuration precedence
- Keyboard operation
- Wi-Fi states and actions
- Build environments
- Physical validation procedure
- Known limitations

# Compatibility and regression requirements

Preserve:

- Existing API routes
- Existing authorization headers
- Existing live-session behavior
- Existing recording timer
- Existing battery telemetry
- Existing SD boot configuration
- Existing status palette meanings
- Existing compatibility environment
- Existing C3 board target
- Arduino_GFX 1.6.0 compatibility
- Existing halo seam fix
- Device-page SD/config separation

Do not:

- Introduce ESP32-S3 flags or board environments
- Reintroduce `esp32-hal-periman.h`
- Add blocking Wi-Fi loops
- Add blocking touch-release loops
- Log secrets
- Store secrets in source control
- Use LVGL in this milestone
- Replace the Media Sync API contract
- Remove the original safety behavior around recording actions
- Assume the upload port number

# Validation commands

Run all applicable checks from:

/workspace/xiao-round-display/xiao-round-rig-controller-v-2-1

At minimum:

- `git diff --check`
- Host-side gesture tests
- Host-side scrolling tests
- Host-side configuration/state tests
- `pio run -e seeed_xiao_esp32c3_compat`
- `pio run -e seeed_xiao_esp32c3`

If PlatformIO cannot download dependencies because of the execution environment:

- Do not treat that as a source-code success.
- Run all available host-side compilation and tests.
- Record the exact unavailable dependency/network failure.
- Provide the exact physical Mac commands required for final verification.

Physical upload command pattern:

`pio run -e seeed_xiao_esp32c3_compat --target upload --upload-port <detected-port>`

Port discovery:

`pio device list`

Physical acceptance checklist:

1. Boot visibly enters a specific Wi-Fi stage.
2. Failed Wi-Fi connection displays a stable failure reason.
3. Retry countdown is visible.
4. Retry Now works.
5. Network scanning does not freeze the halo.
6. Tap cycles summaries.
7. Swipe-up selects a category.
8. Tapping the selected category expands it.
9. Holding a category expands it.
10. Expanded lists scroll fluidly.
11. Swipe-down at the top collapses.
12. Center long press returns to Status.
13. Keyboard opens only when editing.
14. Password and token remain masked.
15. Edited configuration survives reboot.
16. Device page distinguishes card, file, parse, and field states.
17. Recording cannot be triggered by scrolling.
18. Halo remains closed and animated.
19. Compatibility mode remains stable.
20. Serial logs contain no secrets.

# Completion behavior

Implement this entire milestone before reporting completion. Do not stop after the gesture recognizer, radial tabs, Network panel, or keyboard individually.

You may create multiple logical commits while working, but deliver one integrated branch/PR containing the complete milestone.

If a specific feature proves impossible because of an actual hardware or library limitation:

- Implement the safest functional fallback.
- Document the exact limitation.
- Continue completing all unrelated requirements.
- Do not abandon the remaining milestone.

At completion, report:

- Summary of the finished interaction system
- Architecture and state-machine changes
- Every changed or added file
- Test results
- PlatformIO results
- Remaining physical-device checks
- Commit hashes
- PR title and description
- Exact Mac commands to build, flash, and monitor
- The next genuinely unfinished milestone, if any
# CDAProd XIAO Round Rig Controller

Native firmware for the Seeed Studio XIAO ESP32-C3 mounted on the Seeed XIAO Round Display:

- 240x240 GC9A01 LCD
- CHSC6X capacitive touch controller at `0x2E`
- microSD configuration through `/rig.cfg`
- Media Sync live-session polling and start/stop control
- animated, state-aware **Kinetic Status Halo**
- information pages that remain useful when no live session or media exists

## What changed

The original single-file sketch has been separated into display, UI, touch, storage, API, and application-controller modules. The runtime now keeps three different schedules independent:

- the halo is presented at a target of 30 FPS;
- visible status content redraws only when its data changes;
- blocking HTTP work runs on a separate FreeRTOS task.

The C3 has one CPU core. The API worker therefore prevents network calls from
blocking the UI loop through FreeRTOS scheduling; it does not run on a second
physical core.

The ring uses a continuously moving baseline plus a cubic-Bezier orbital lap. It never fully stops at the loop seam. Brightness breathes independently, colors drift, and palettes crossfade when state changes.

## Status colors

| State | Halo treatment |
| --- | --- |
| Boot | blue, cyan, white |
| Joining Wi-Fi | orange, amber, gold |
| Wi-Fi/API offline | dim orange with moving gold energy |
| Connected with no session | green, teal, cyan |
| Standby | green, teal, cyan |
| Recording | red, orange, magenta |
| Sending a command | blue, cyan, violet |
| Error | red, amber, magenta |

This means the screen visibly transitions from orange to green only after the rig is genuinely connected and the API has answered.

## SD card configuration

Format the microSD card as FAT32. Copy `data/rig.cfg.example` to the root of the card and rename it `rig.cfg`:

```ini
wifi_ssid=cda_Lab
wifi_password=YOUR_WIFI_PASSWORD
api_base=http://192.168.0.25:8787
node_id=YOUR_CAMERA_NODE_ID
bearer_token=YOUR_DEVICE_TOKEN
poll_ms=1000
```

If `/rig.cfg` is missing, the firmware creates a template automatically. Power down, remove the card, edit the template, insert it again, and reboot.

The card is intentionally used during boot only. It is unmounted before LCD initialization so the GC9A01 can exclusively own the shared SPI host in DMA mode. A boot event is appended to `/rig-events.csv`; runtime diagnostics continue over USB serial.

## Touch controls

With a live session:

- tap the center while standing by to request `start_recording`;
- tap while recording to request `stop_recording`;
- the display waits for the authoritative API state instead of pretending the command already completed.

Without a live session:

- tap to cycle through Status, Network, Device, and Controls pages;
- long-press to return to the Status page.

## Build and flash

From the project root:

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

The normal environment uses:

- 160 MHz ESP32-C3 CPU through the board platform defaults;
- 80 MHz LCD SPI;
- `Arduino_ESP32SPIDMA`;
- 30 FPS halo presentation target;
- `-O3` compiler optimization.

If the particular display/cable/build is unstable at 80 MHz, use the included compatibility environment:

```bash
pio run -e seeed_xiao_esp32c3_compat
pio run -e seeed_xiao_esp32c3_compat --target upload
```

That environment uses conventional SPI at 40 MHz and a 24 FPS target.

## API contract

The controller preserves the existing Media Sync API contract:

```text
GET  {api_base}/api/live_sessions
POST {api_base}/api/live_sessions/{session_id}/control
```

Control JSON:

```json
{"action":"start_recording"}
```

or:

```json
{"action":"stop_recording"}
```

Optional device headers are sent when configured:

```text
Authorization: Bearer {bearer_token}
X-Media-Sync-Node-Id: {node_id}
```

The session endpoint may return either a top-level JSON array or `{ "items": [...] }`.

## Runtime diagnostics

Every five seconds the serial monitor prints the measured presentation rate:

```text
[halo] 30.0 presented fps, 0 dropped, dma=on
```

This is the meaningful embedded measurement. The animation position is time-based, so a missed deadline advances to the correct point instead of making the motion run slowly.

## Source layout

```text
src/
├── app/         application state machine and background API worker
├── display/     GC9A01 and SPI/DMA ownership
├── input/       non-blocking CHSC6X touch events
├── model/       shared POD state and configuration types
├── network/     Media Sync HTTP and JSON contract
├── storage/     boot-time SD configuration
├── ui/          status pages and animated halo
└── main.cpp     object wiring plus setup/loop
```

## Performance rules retained by the implementation

- no Wi-Fi connection loop blocks the UI;
- no HTTP request runs on the display task;
- no SD access occurs while DMA owns the display bus;
- no full-screen clear occurs during halo frames;
- status text redraws only when the screen model changes;
- ring motion uses elapsed time rather than frame count;
- the render loop performs no heap allocation.

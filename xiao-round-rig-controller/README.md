# CDAProd XIAO Round Rig Controller

Flashable PlatformIO firmware for the **Seeed Studio XIAO ESP32-C3** mounted to the **Seeed Studio 1.28-inch Round Display for XIAO**.

The dial is the independent camera-rig controller we planned—not the Raspberry Pi monitor. It reads your existing `media-sync-api` live-session state, shows `STANDBY`/`REC`, and sends the existing `start_recording` or `stop_recording` control action when the center is touched.

## Hardware contract

| Function | Part / pin |
| --- | --- |
| LCD | GC9A01, 240×240, SPI |
| LCD CS | XIAO D1 |
| LCD DC | XIAO D3 |
| Backlight | XIAO D6 |
| Touch | CHSC6X, I2C address `0x2E` |
| Touch interrupt | XIAO D7 |
| microSD CS | XIAO D2 |
| Shared SPI | D8 SCK, D9 MISO, D10 MOSI |
| Battery read | XIAO D0 |

On newer Round Display boards, put both small `KE` switches in the **ON** position so D6 controls the backlight and D0 can read battery voltage.

## 1. Prepare the microSD card

The Round Display officially supports cards up to 32 GB formatted as FAT. Use **FAT32**, not exFAT.

Copy `sd-card/rig.cfg` to the card root so its final path is:

```text
/rig.cfg
```

Edit these required values:

```ini
wifi_password=YOUR_WIFI_PASSWORD
node_id=THE_CAMERA_NODE_ID
bearer_token=THE_CAMERA_NODE_DEVICE_TOKEN
```

The supplied API base already matches your authority host:

```ini
api_base=http://192.168.0.25:8787
```

The token must belong to the same `node_id` that owns the live session. That is required by the current `POST /api/live_sessions/{session_id}/control` authorization boundary.

## 2. Build and flash

Open this directory in VS Code with PlatformIO, connect the XIAO by a USB-C **data** cable, then run:

```bash
pio run -t upload
pio device monitor -b 115200
```

If upload does not start, enter bootloader mode. With the USB connector on the right as shown in the board photo, the upper tiny button marked `B` is **BOOT** and the other tiny button is **RESET**:

1. Hold **BOOT**.
2. Tap **RESET**.
3. Release **BOOT**.
4. Run the upload command again.

## 3. Operation

- `STANDBY`: the camera node has an active preview session; tap the center to request recording.
- `REC`: authoritative session status is recording; tap the center to request stop.
- `NO SESSION`: the configured camera node has no active `/api/live_sessions` entry.
- `API OFFLINE`: Wi-Fi works, but `192.168.0.25:8787` is unreachable or returned a non-200 response.
- `ERROR / TOKEN REQUIRED`: put the camera node's device token in `/rig.cfg`.

Every SD-backed event is appended to:

```text
/rig-events.csv
```

The firmware intentionally does not claim that recording has started merely because the touch request returned. It polls the authority and changes the screen to `REC` only after the live-session status becomes `recording`.

## Current media-sync API contract used

```http
GET /api/live_sessions

POST /api/live_sessions/{session_id}/control
Authorization: Bearer <device-token>
X-Media-Sync-Node-Id: <camera-node-id>
Content-Type: application/json

{"action":"start_recording"}
```

Stop uses the same endpoint with `{"action":"stop_recording"}`.

## Confirmed board

The photographed module is labeled `Model: XIAO-ESP32-C3`. This project therefore targets PlatformIO board ID `seeed_xiao_esp32c3`; do not upload an ESP32-S3 build to it.

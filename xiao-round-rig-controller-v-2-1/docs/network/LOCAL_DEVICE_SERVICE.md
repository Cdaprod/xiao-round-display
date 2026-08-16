# Local device service

The XIAO ESP32-C3 joins Wi-Fi as an independent LAN device. Media Sync is an upstream API dependency; an unavailable desktop changes API readiness but does not tear down Wi-Fi, DHCP, mDNS, or the local HTTP service.

## Addressing and discovery

Before association, the firmware sanitizes the node ID into `cda-rig-<node-id>` and passes it to `WiFi.setHostname()`. After DHCP succeeds it starts HTTP on port 80 and advertises `_http._tcp` through mDNS as `http://<hostname>.local/`. Serial reports both the DHCP address and mDNS URL. A router DHCP reservation is recommended for a predictable numeric address.

## Endpoints

- `GET /` — embedded management landing page with no external assets.
- `GET /health` — unauthenticated firmware liveness and uptime.
- `GET /api/status` — LAN, upstream API, storage, battery, UI, and rendering status.
- `GET /api/config` — SSID and non-secret configuration; password and token are booleans only.
- `POST /api/config` — authenticated bounded single-field update using `{"field":"ssid","value":"..."}`; persistence occurs once in the controller loop.
- `POST /api/wifi/retry` — authenticated request through the existing retry state machine.
- `POST /api/api/retry` — authenticated upstream API retry without disconnecting Wi-Fi.

Mutating requests require `Authorization: Bearer <device-token>`. Authorization headers longer than 160 bytes are rejected. Password and bearer-token bytes are never serialized. Destructive reset, reboot, override clearing, and OTA are not exposed.

Example:

```sh
curl http://cda-rig-controller.local/health
curl -H 'Authorization: Bearer DEVICE_TOKEN' \
  -X POST http://cda-rig-controller.local/api/api/retry
```

HTTP handling runs from the main nonblocking loop, enqueues semantic retry requests, and never calls display rendering or changes UI navigation.

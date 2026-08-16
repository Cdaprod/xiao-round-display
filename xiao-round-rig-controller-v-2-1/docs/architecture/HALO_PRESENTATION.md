# Event-driven halo presentation

The halo is owned by the visible category rather than by the current Wi-Fi/API state. Category palettes are Status gold/orange/coral, Network cyan/blue/indigo, Device violet/magenta/cyan, and Controls lime/emerald/teal. Operational state adds only localized overlays.

The presentation lifecycle is `Entering -> Ambient -> Exiting -> Hidden`, with touch, hold, success, and failure transient states. Opening a menu exits and clears the annulus; expanded menus and keyboards remain Hidden and schedule no frames. Closing returns through Entering without resetting angular phase.

Settled category summaries do not animate unless a semantic pulse is active. Transitions and feedback schedule at 30 FPS. Intentionally idle periods are not counted as dropped/coalesced frames.

The optimized annular geometry remains in use. A later LVGL migration must convert this renderer into an LVGL-owned custom object before LVGL becomes the display flush owner; independent halo LCD writes must not survive that migration.

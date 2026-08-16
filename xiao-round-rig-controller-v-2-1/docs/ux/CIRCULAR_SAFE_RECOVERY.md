# Circular-safe recovery

Critical controls use the 106 px interactive radius rather than square framebuffer corners. Menus provide a fixed `< BACK` header; editors and confirmations provide a fixed centered `CANCEL` header.

The keyboard edits a bounded draft and never persists per key. `CANCEL` discards it, `DONE` validates and persists once, and `DEL` supports tap and stationary hold-repeat. Keys, `DEL`, and `DONE` remain inside circular scanline spans. Modal input barriers prevent opening and closing releases from reaching adjacent layers.

A stationary three-second hold in the safe upper-center header discards transient state and returns to the selected category summary. No recovery operation saves configuration, resets credentials, or reboots.

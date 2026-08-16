# Round UI compositor

## Root cause

The former UI had two unrelated render paths. `StatusHalo` owned the annulus, while `RigUi` compared the entire snapshot, cleared an inner circle, and immediately repainted text and rectangular controls on the LCD. Fast-changing telemetry invalidated the complete center. LCD users could therefore see the erase phase, and controls outside the undersized inner mask left stale pixels.

## Ownership

The display now has explicit ownership:

1. the GC9A01 background is initialized once;
2. `StatusHalo` owns only the 110–117 px annulus;
3. `UiCompositor` owns the 108 px circular content viewport;
4. contact and hold feedback are composed over content and echoed in the halo;
5. keyboard and confirmation modes replace or composite the content layer.

`Arduino_Canvas_Indexed` allocates one 240×240 indexed surface once (57,600 bytes). A 240-pixel RGB565 conversion line is fixed storage. Changed states are completed offscreen and transferred as precomputed circular scanline spans, so an erase phase is never presented. If indexed allocation fails, firmware logs `span-fallback` and uses the same clipped spans directly rather than crashing.

`CircularViewport` is the single geometry source for rendering and hit testing. Summary content uses radius 103; all other modes use radius 108, leaving the animated halo independent and closed. Every mode transition clears and transfers the complete expanded ownership radius, eliminating stale pixels.

## Invalidation

`UiInvalidation` distinguishes background, header, summary, viewport, row, scroll, overlay, and feedback changes. Snapshot comparison considers only values visible in the current mode. Halo frames never invalidate content. Scroll position is held by `ScrollModel` and is not reset by snapshot updates.

## Performance telemetry

Rate-limited Serial diagnostics report full, viewport, row and feedback redraw counts, composition and transfer time, maximum observed touch interval, and minimum heap. Status diagnostics retain halo frame/drop counters. No compositor or feedback allocation occurs after startup.

## Dirty-span repair

Semantic snapshot comparison now maps retry-only changes to their visible text rows. `UiInvalidation` retains the union of dirty scanlines, and `UiCompositor::present()` transfers only those circular spans. Contact feedback is capped at the configured 24 FPS and transfers a bounded 36-pixel-high region. Transition frames remain full compositions and pause ambient halo presentation; idle and halo-only ticks perform no content transfer. Expired halo deadlines are coalesced into the newest elapsed-time phase rather than queued for catch-up.

## Halo LUT renderer

The halo no longer invokes arc rasterization during animation. `HaloGeometry` precomputes compact framebuffer offsets, angular indices, and the two annular spans for every scanline once. Each frame builds one 256-color RGB565 palette, maps approximately 5,000 annular pixels by `(angle + phase) & 0xff`, and transfers only the left/right halo spans. Halo logs separately report raster time, transfer time, coalesced deadlines, and bytes per second. Compatibility performance remains subject to physical measurement.

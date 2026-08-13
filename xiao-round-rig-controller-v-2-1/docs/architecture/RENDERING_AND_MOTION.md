# Rendering and motion architecture contract

## Delivery gate

Do not change visual design, gesture behavior, display ownership, or the rendering driver until both gates pass:

1. `seeed_xiao_esp32c3_compat` builds successfully with PlatformIO `espressif32@6.12.0`, Arduino core 2.0.17, GCC 8.4, and Arduino_GFX 1.6.0.
2. The current precomputed halo renderer is flashed and its output is recorded for 60 seconds on physical hardware.

The baseline record must include presented/coalesced halo frames, raster and transfer average/maximum, bytes per second, compositor transfer timing, full/viewport/row counters, touch interval, and free/minimum heap. Halo performance is not accepted from host tests alone.

## Single presentation owner

The target architecture has exactly one physical presentation path:

```text
UiPresentationScheduler -> UiCompositor -> DisplayDevice
```

The halo, gestures, transitions, semantic state, countdown, and telemetry may advance models and invalidate regions. They must not independently flush, push pixels, call `startWrite()`/`endWrite()`, or schedule LCD work. A repository check must enforce that only the presentation path calls physical transfer APIs.

## Frame classes and priority

| Priority | Class | Work |
| --- | --- | --- |
| 1 | Urgent | touch-down, action confirmation, error feedback |
| 2 | Interactive | category drag, menu depth transition, scrolling, collapse, settle |
| 3 | Semantic | category, Wi-Fi, recording, configuration state |
| 4 | Ambient | halo phase, breathing, palette drift |
| 5 | Background | countdown, telemetry, diagnostics |

Pending work is merged into one presentation. Lower-priority work never causes a second flush when a higher-priority frame is already pending. Touch preempts ambient preparation; the halo phase advances mathematically without presenting the discarded intermediate state.

## Presentation cadence

- Input sampling remains independent and targets 80–150 Hz.
- Urgent feedback targets at most 33 ms from touch-down to presentation.
- Interactive and settle frames target 24 FPS (41,667 microseconds).
- Ambient halo targets 18–24 FPS according to measured headroom.
- Static UI schedules no presentation.
- Semantic and background changes are event-driven and coalesced.

The scheduler uses monotonic deadlines aligned to the original cadence. At an eligible deadline it reads the newest state, computes elapsed-time progress, discards expired intermediate states, merges invalid regions, composes once, presents once, and records completion. Frames are never queued.

## Interruptible transition model

Finger-driven motion is controlled by normalized gesture progress, not a private animation timer. Horizontal displacement drives category progress. Upward displacement drives menu-open progress; downward overscroll at scroll position zero drives the inverse.

On release, settling begins at the current displayed progress and uses monotonic cubic easing. A new press may interrupt the settle and becomes authoritative immediately. Cancellation reverses from current progress and restores the previous authoritative state. Navigation gestures cannot execute action rows.

## Layer and halo ownership

- `CategorySummary` owns the ambient halo.
- `CategoryDraggingHorizontal` owns category content and halo impulse in the same frame; independent ambient presentation is paused.
- `OpeningMenu` and `ClosingMenu` own the halo transition.
- `MenuOpen` owns the full menu viewport and contains no halo pixels.
- Returning to `CategorySummary` restores the halo in the closing transition and resumes ambient scheduling only after completion.

Entering `MenuOpen` must clear the annulus in the authoritative framebuffer. No display operation may reveal an erase step.

## Dirty-region cost model

The scheduler evaluates correct candidate transfers using:

```text
estimated cost = bytes / measured bus bytes per microsecond
               + transaction count * measured transaction overhead
```

Benchmark on compatibility hardware:

1. two annular spans per halo row;
2. one center-preserving span per row;
3. merged annular bands;
4. a full frame for major transitions.

Selection uses complete raster, setup, and transfer duration—not pixel count alone. Dirty regions remain inside 240×240 and are merged before composition.

## Buffer and execution rules

- Maintain one authoritative RGB565 framebuffer in the target architecture.
- Compose replacement pixels before touching the LCD.
- Never present an intermediate erase or partial layer.
- Allocate no memory and construct no `String` objects in steady-state frame execution.
- Precompute geometry and lookup tables; perform no per-frame geometry generation or pixel trigonometry.
- Keep Arduino_GFX pinned to 1.6.0.

## Instrumentation

Record separately:

- input samples per second and longest UI-loop blocking operation;
- touch-down-to-first-feedback latency;
- requested, presented, coalesced, and missed frames;
- inter-present average, maximum, p95, and p99;
- raster and transfer average/maximum;
- bytes per frame/second and transactions per frame;
- full, viewport, and dirty-row presentations;
- current and minimum heap.

## Regression gates

Host tests must prove unchanged state causes no invalidation, only one presentation owner exists, multiple pending layers produce one presentation, urgent work overrides ambient work, expired frames coalesce, progress is deterministic, menu frames contain no halo, cancellation restores authoritative state, steady-state presentation allocates nothing, and every dirty region remains in bounds.

Physical acceptance requires stable heap for 10 minutes, no idle full-redraw growth, touch feedback within 33 ms when measurable, stable 24 FPS interaction, no growing backlog, no flicker/clearing, no navigation from network polling, no menu halo artifacts, and no touch-action safety regression.

## Incremental implementation order

Each numbered step is a separate measured commit:

1. obtain compatibility build success;
2. flash the current halo LUT build and capture the 60-second baseline;
3. add only the single presentation scheduler;
4. validate idle, halo, touch, and network updates;
5. add horizontal category dragging;
6. add upward opening and downward closing;
7. add halo entrance/retraction;
8. enable action interaction last.

Record before/after physical telemetry for each step. Do not combine scheduler, gesture, driver, configuration, and Wi-Fi changes in one patch.

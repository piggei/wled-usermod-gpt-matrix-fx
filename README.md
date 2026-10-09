# GPT Matrix FX — 25 experimental 2D effects for WLED

**Release:** v0.2.2 (experimental)  
**Effects:** 20 standalone matrix effects (`GPT01`–`GPT20`) + 5 AudioReactive effects (`GPTA01`–`GPTA05`)  
**Target:** WLED with a 2D matrix; designed around 64×64 on an ESP32-S3 / HUB75 setup  
**Integration:** external PlatformIO usermod library, placed next to the WLED source tree. No changes to WLED's core source files.

The WLED build, display and matrix configuration, preset handling, palettes, and audio processing remain owned by WLED. This project only registers additional WLED effects.

## Effect catalog

### Standalone effects

| UI name | Description | Origin |
| --- | --- | --- |
| GPT01 - Caustic Atlas | Orbiting virtual emitters refract and interfere to produce luminous optical caustics. | Preserved from v0.1.1 |
| GPT02 - Voronoi Bloom | Animated Voronoi membranes and pulsing seeds. | Preserved |
| GPT03 - Origami Reactor | Polygonal paper-fold geometry with moving creases and facets. | Preserved |
| GPT04 - Ink Tectonics | Excitable cellular automaton, propagating fronts and refractory trails. | Preserved |
| GPT05 - Event Horizon | Gravitationally distorted streams and a ringed accretion disk. | Preserved |
| **GPT06 - Clockwork Cathedral** | **Moving epicyclic mechanism**: two gear-meshed satellites orbit the rotating central gear while the entire assembly drifts and subtly breathes in size; moving spokes, enamel glitter and luminous pins remain attached to their respective gears. | Animated in v0.2.2 |
| GPT07 - Impossible Lattice | Nested folded tessellations and geometric mirrors. | Preserved |
| GPT08 - Aether Topography | Heightfield isolines and drifting escarpments. | Preserved |
| GPT09 - Mandelglass | Glassy cells traced through an animated complex Julia system. | New |
| GPT10 - Glass Circuit | Electrically animated traces, signal packets, and glowing junctions. | New |
| GPT11 - Aurora Loom | Undulating interwoven luminescent threads with alternating over/under crossings. | New |
| GPT12 - Lagrange Ballet | Three orbiting bodies sculpt a gravitational equipotential contour map. | New |
| GPT13 - Chromatic Kintsugi | Moving faceted surfaces stitched together by luminous cracks. | New |
| GPT14 - Phantom Elevator | Depth-projected perspective corridor with seemingly endless ascending beams. | New |
| GPT15 - Ferrofluid Theatre | Two orbiting magnetic poles bend spikes and liquid-metal field contours. | New |
| GPT16 - Riemann Current | Counterwinding logarithmic vortices with intersecting streamlines. | New |
| GPT17 - Constellation Engine | A moving astronomical chart of points connected by rotating chords. | New |
| GPT18 - Vitreous Labyrinth | Multi-scale reflective corridors and luminous paths. | New |
| GPT19 - Celestial Runewheel | Four nested rotating dials, orbiting markings and a sweeping indicator. | New |
| GPT20 - Zenith Quasicrystal | Five-axis quasiperiodic interference, revealing wandering star-like nodes. | New |

### AudioReactive effects

| UI name | Audio response | Origin |
| --- | --- | --- |
| GPTA01 - Bass Cartographer | Sixteen spectral sectors reshape a luminous polar coastline. | Previously GPT9 |
| GPTA02 - Harmonic Wormholes | Sixteen frequency-driven portals; rising sample peaks add shockwaves. | Previously GPT10 |
| GPTA03 - Spectral Loom | Each of the 16 bands drives a separate braided luminous ribbon. | New |
| GPTA04 - Resonant Tessellator | Hexagonal facets brighten, shift hue and deform in response to assigned FFT bands. | New |
| GPTA05 - Synaptic Reef | Branching coral arms grow with the spectrum; detected peaks send outward pulses. | New |

All effects are procedurally implemented in this package. They are **not copies of WLED's built-in effects**, although some explore established mathematical concepts such as Voronoi diagrams, Julia sets, and contour maps.

## Installation

### Directory layout

Place the extracted `wled-usermod-gpt-matrix-fx` directory **beside** your WLED checkout:

```text
/home/pj/repo/
├── wled_17_dev/
│   ├── platformio.ini
│   └── platformio_override.ini
└── wled-usermod-gpt-matrix-fx/
    ├── gpt_matrix_fx.cpp
    ├── library.json
    ├── README.md
    ├── TEST_REPORT.md
    ├── examples/
    └── tests/
```

### PlatformIO: add one line, not a new configuration

In the existing `custom_usermods` list for the WLED environment you actually use (for example, `[env:waveshare_spotify]`), **append only one** of these entries. Keep any existing usermods and the rest of the PlatformIO settings unchanged.

**Option A — relative path (recommended with sibling directories):**

```ini
  symlink://../wled-usermod-gpt-matrix-fx
```

The path is relative to the WLED project root (the directory containing its `platformio.ini`). The `..` selects `/home/pj/repo/`, and then enters the package directory. This survives moving both repositories together.

**Option B — absolute path (equivalent on this host):**

```ini
  symlink:///home/pj/repo/wled-usermod-gpt-matrix-fx
```

This points to an exact filesystem location. It needs editing when moving to a different machine or changing directory names. Do not add both entries.

If `custom_usermods` already uses `${...}` inheritance, preserve it; do not replace the setting. In particular, do **not** remove AudioReactive, Spotify, buzzer, matrix, or any other usermods already listed. See `examples/platformio_override.example.ini` for the same add-only instructions.

### Build

```sh
cd /home/pj/repo/wled_17_dev
pio run -e waveshare_spotify
```

After flashing, use the WLED effect selector. Names sort lexicographically as `GPT01` ... `GPT20`, followed by `GPTA01` ... `GPTA05`. Effect IDs are dynamically allocated by WLED (`strip.addEffect(255, ...)`) and may change between firmware builds. Verify `/json/eff` and update any saved mode IDs or external automations after installation or removal of effects.

**Effect-count caution:** WLED uses a limited effect-ID space; adding 25 modes may approach that limit if a build already includes a large number of custom effects. Check that all 25 entries appear in your effect list after compilation and flashing.

## GPT06 — Clockwork Cathedral (moving epicyclic mechanism)

GPT06 now has **three simultaneous motions**:

1. **Gear spin:** the 14-tooth center gear retains the previous v0.2.1 speed curve and can approach one revolution per second at maximum WLED Speed.
2. **Planetary revolution:** the two 10-tooth satellites orbit the center on opposite ends of a common carrier, with a pitch-center separation of `0.385 + 0.275` normalized units. The planet gears rotate according to `ωplanet = (1 + 14/10) × ωcarrier − (14/10) × ωsun`, rather than independently. This keeps a tooth opposite a gap at both contacts even as the carrier turns.
3. **Assembly drift:** the entire mechanism follows a gentle, dual-frequency 2D trajectory, with a small scale-breathing component. This prevents the central axis from appearing fixed to the screen and keeps the assembly within a typical 64×64 view.

The two satellites are meshed with the central gear, not with each other. Gear radii are proportional to tooth counts (14:10), and the half-tooth offset ensures compatible contact phases. To avoid cumulative errors, the phase of each rotating component is derived directly from the device timestamp in double precision before being reduced to one revolution. This also avoids jumps when the central angle wraps at 2π.

The existing tooth shapes, metallic body, perforations, moving spokes, rotating rim pins and surface glitter remain. Their geometry is transformed with the gears, so ornamental details move along with the mechanism instead of staying painted to screen coordinates.

**Controls:** WLED **Speed** sets the central spin and influences orbital-carrier speed; **Intensity** controls metallic highlights; **Custom 1** adjusts tooth depth; **Custom 2** adjusts color. No new controls or stored segment settings are required. The effect retains its name and its original `GPT06` registration position.

**Performance:** the nominal GPT06 frame interval remains 22 ms. The effect uses no additional segment memory, but its trigonometric per-pixel renderer can still be limited by ESP32-S3 workload or HUB75 refresh throughput. This is a stylized geometric model, not a CAD-level involute/contact-force simulation.

## AudioReactive contract

The five audio effects read from the running WLED AudioReactive usermod via `UsermodManager::getUMData(..., USERMOD_ID_AUDIOREACTIVE)`; they do not create audio interfaces or configure I2S.

- FFT data: the normal 16-band `fftResult` array at the usermod's published data pointer.
- Smoothed volume and sample-peak flag are also used by appropriate effects.
- Every effect maintains a small **per-segment** attack/release smoothing state (approximately 24 bytes), and falls back to subdued rendering when audio is not available.
- The external AudioReactive usermod must already be built, enabled, and supplied with sound for real audio responsiveness.
- Input values are sanitized before use, including non-finite volume values.

## Size, memory, performance, and limitations

- All effects draw through `SEGMENT.setPixelColorXY()` and scale to the current 2D segment dimensions. They are designed visually around 64×64, but host-tested on 8×8, 16×32, 64×64, 96×48 and 128×128. Fine details can disappear on tiny matrices.
- Non-2D segments safely render the configured fallback color.
- GPT04 maintains two simulation buffers. Its logical grid is capped at 64×64, requiring at most **8,192 bytes** of per-segment effect memory; larger matrices scale the simulation.
- Other standalone effects use procedural drawing without allocating extra per-effect buffers. Audio modes each allocate the approximately 24-byte smoothing state.
- Effects have different frame-rate throttles between about 22 ms and 47 ms, but **host timings are not hardware FPS estimates**. Some procedural modes use significant per-pixel trigonometry and should be profiled on the real ESP32-S3.
- Palette transitions, brightness, 2D segment arrangement and matrix output are handled by WLED. Each GPT effect samples the selected palette through WLED; the WLED Default palette is deliberately requested as a real gradient (`mcol=255`) instead of substituting the segment primary color.
- If a selected palette is completely black (including a Primary Color palette configured with a black primary), GPT Matrix FX applies a temporary rainbow fallback so the procedural effect remains visible. Black *entries within a non-black palette* remain black. The fallback changes only the rendered result, not the configured WLED palette or primary color. The fallback check adds up to 16 palette lookups per effect invocation; no persistent extra segment buffers are allocated.
- This package has **not** been compiled or flashed against the user's customized `waveshare_spotify` environment. Host compilation is not a substitute for the full firmware build.

## Test commands

From the package directory:

```sh
./tests/run_host_tests.sh
./tests/run_sanitizers.sh
```

These compile and execute the **real C++ effects source** with a minimal host WLED API double. Checks cover effect naming and count; all 25 effects over five resolutions; all 25 effects across nine palette changes with a black-primary regression and dark-entry preservation; frame changes over time; the epicyclic gear mesh, spatial movement and long-uptime phase stability; sound disabled or incomplete; non-finite audio volume; missing matrix support; and a differential audio test for every audio effect. The host double intentionally defines Arduino's `sq(x)` macro to detect accidental helper-name collisions. C++11 compilation, AddressSanitizer and UndefinedBehaviorSanitizer are included.

## Files

- `gpt_matrix_fx.cpp`: all effect implementations and dynamic WLED registration.
- `library.json`: PlatformIO local-library definition (`libArchive: false`).
- `examples/platformio_override.example.ini`: add-only integration options.
- `tests/`: runnable host compile, regression and sanitizer tests, excluded from firmware compilation.
- `TEST_REPORT.md`: verification record and known limitations.
- `LICENSE`: MIT.

## v0.2.2 change summary

- GPT06 has two orbiting planet gears and a smooth, floating whole-assembly translation with subtle scale variation.
- Maintained physical pitch-center distances and tooth-to-gap engagement throughout the carrier revolution. Planet spin is derived from carrier motion and center spin.
- Retained tooth geometry, metallic spokes, enamel glitter, custom color, existing speed range, and palette behavior.
- Added numerical/visual regression tests for orbital geometry, mesh phase, viewport bounds, and visible travel at five speed levels and eleven timestamps, including near-millis-wrap uptime.
- No changes to the other 24 effects, palette handling, AudioReactive data plumbing, or effect registration order.

## v0.2.1 change summary

- Fixed palette 0 (Default) rendering: WLED can return the primary segment color, potentially black, when `color_from_palette()` receives `mcol=0`. GPT now passes `mcol=255` and requests moving/wrapped palette interpolation, keeping the selected palette active.
- Added an all-black palette safety fallback, without modifying non-black palette slots or the segment settings.
- Added an in-place nine-state palette-change regression for every one of the 25 effects. The test reproduces a blackout with the v0.2.0 source and passes with v0.2.1.
- No intentional changes to procedural geometry, effect names, registration order, or AudioReactive processing.

## v0.2.0 change summary

- Preserved GPT01–GPT05 and GPT07–GPT08 visuals from v0.1.1.
- Rebuilt GPT06 with kinematically consistent gearing, faster speed and ornamental metal/glitter detailing.
- Added twelve standalone effects (GPT09–GPT20).
- Renamed former GPT9 and GPT10 to GPTA01 and GPTA02 while preserving their visuals.
- Added three new AudioReactive effects (GPTA03–GPTA05).
- Switched all user-facing effect names to sortable, zero-padded `GPT## - Name` and `GPTA## - Name` forms.
- Updated host test coverage to 25 effects, and kept all packaged materials in English.

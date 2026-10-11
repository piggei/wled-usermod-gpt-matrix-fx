# Preliminary renderer cost audit — GPT Matrix FX v0.3.0-dev.8

## Scope and limitations

All 30 effects were inspected in the dev.7/dev.8 source. The only algorithm
modified in dev.8 is GPT24b. This is an initial *static* cost review, supported
by an exploratory x86-64 benchmark of 64x64 host mocks at common controls.
It is **not** an ESP32-S3 benchmark: the host has different floating-point,
math-library, palette, display and task scheduling costs. Host ranking is a
candidate list, not a measured ESP32 speed classification. The physical
ESP32-S3 baseline comes from the maintainer's `/json/info` recordings.

## Verified GPT24b finding

In dev.7, one pixel requested `atan2f()` twice in the two-layer loop and once
for palette indexing; `sqrtf()` twice for layer radius and once for central
core glow. Each request had the same argument pair for that pixel.

In dev.8, angle and radius are computed **once per pixel**, and those results
are used for the two layers, core glow and palette. Frame outputs matched the
frozen dev.7 reference byte-for-byte in seven host configurations. No
changes were made to `tick(40)`, UI parameters, color logic or other effect
functions.

Measured in an interleaved 120-pair x86-64 benchmark at -O2 / 64x64:

| Build | Median elapsed render | Host-only change |
| --- | ---: | ---: |
| Original dev.7 GPT24b | ~538 us | reference |
| Optimized dev.8 GPT24b | ~375 us | ~30% reduction |

This improvement is not guaranteed on Xtensa; FreeRTOS scheduling and BLE may
still be significant. On the maintainer's device dev.7 showed ~65 ms/render
with BLE advertising and ~129 ms cumulative/render after BLE connection. The
sliding-window dev.8 telemetry will provide a cleaner comparison.

## Other effects: initial optimization candidates

The following are *potential costs detected in source*, not device-measured
slowdowns. The table gives x86-64 exploratory **host median** elapsed
microseconds per 64x64 render under a shared set of controls; use them only
for a rough ordering on that host.

| Priority | Effect | Host median (us) | Expensive pattern and low-risk candidate |
| --- | --- | ---: | --- |
| High | GPT17 - Constellation Engine | 677 | 11 point distances and 11 point-to-segment distances per pixel. Precompute segment direction/length outside the pixel loop; avoid `sqrtf` when squared distance already exceeds the illuminated threshold. |
| High | GPT20 - Zenith Quasicrystal | 482 | Five axes evaluate `sinf/cosf` of axis angles on *every pixel*; cache axis directions once per frame. |
| High | GPT19 - Celestial Runewheel | 458 | Four dial evaluations including trigonometry per pixel; examine outer-bounds rejection and shared angular terms. |
| High | GPT07 - Impossible Lattice | 428 | 2-4 repeated `sinf/cosf(t * ... + j * ...)` values per pixel, invariant by recursion level; cache rotations per frame. |
| High | GPT18 - Vitreous Labyrinth | 424 | Four nested folds; the `sinf(t*.38f+k)` terms depend on frame/level only and can be precomputed. |
| Moderate | GPT25b - Tidal Glyphs | 333 | `sinf/cosf(theta)` each computed twice in coordinate rotation; cache per pixel, and consider per-cell cached tidal orientation later. |
| Moderate | GPTA02 - Harmonic Wormholes | 337 | Multiple trigonometric ridges per pixel; examine whether field invariants can be moved outside loops. |
| Moderate | GPTA05 - Synaptic Reef | 313 | Nested trig expressions and distance/angle per pixel; consider common-subexpression reuse before approximations. |
| Moderate | GPT08 - Aether Topography | 275 | Several `sinf/cosf` terms per pixel; minimal obvious reuse without changing the mathematical field. |
| Moderate | GPT16 - Riemann Current | 266 | Two `logf`, two polar angles and two radii per pixel; more invasive optimizations may affect output and require reference testing. |
| Moderate | GPT13 - Chromatic Kintsugi | 275 | Multiple periodic crack deformations per pixel; consider phase precomputation. |
| Low | GPT21b / GPT22b - Sparse Ripple | 19 / 27 | Sparse per-dot drawing; already low-cost on host, preserve these during optimization. |
| Low | GPT04 - Ink Tectonics | 59 | Uses a 2-page segment simulation buffer, no heavy trig in automaton step; monitor scratch RAM and scaling. |

All other effects were considered at a first pass, with no demonstrated
blocking issue on the ESP32-S3. The complete host-only results are included as `PERFORMANCE_HOST_DEV8.csv`.
For reproducibility, run `tests/profile_host.cpp` using the command below;
the benchmark also includes all five audio modes. Do not use its numbers as real WLED FPS.

## Suggested next sequence (no implementation in this build)

1. Collect GPT24b dev.8 real-device telemetry at identical palette, speed,
   intensity, petals and Size, first BLE advertising, then BLE connected;
   wait for 32 frames before each capture.
2. Check `recentAvgUs` versus `rowMaxUs` and `coreStart/coreEnd` to look for
   latency spikes or core migrations. Wall-clock samples cannot separately
   report CPU busy time, interrupts, BLE traffic or FreeRTOS preemption.
3. If GPT24b still falls to very low FPS, instrument task run-time or trace
   scheduling on a dedicated diagnostic build before changing NimBLE/RTOS.
4. Prepare *independent* pixel-identical optimization prototypes for GPT17,
   GPT20, GPT07 and GPT18 (in that order for source invariants), with per-effect
   host references and a hardware performance gate.
5. Evaluate memory and CPU tradeoffs on actual ESP32-S3 before introducing
   LUTs or cross-frame caches. Do not modify stable v0.2.2.

## Reproducing exploratory host timings

From the usermod source root:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -Itests tests/profile_host.cpp -o /tmp/gptfx-profile
/tmp/gptfx-profile
```

The benchmark uses the existing host stub of WLED, a simulated palette and
framebuffer, synthetic FFT, 72 timed frames per effect, and an alternating
120-pair dev.7-vs-dev.8 GPT24b test. It is optional and is not built by
PlatformIO for WLED.

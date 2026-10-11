# GPT Matrix FX v0.3.0-dev.12 — Full Temporal Trigonometry Audit

**Date:** 2026-10-11  
**Baseline:** frozen v0.3.0-dev.12 source, no rendering modifications  
**Source SHA-256 (`gpt_matrix_fx.cpp`):** `f9732e8144e83ef9b8f0600921059c386980a4aa6a69cdb579f30484f6addc09`  
**Scope:** remaining 28 effects; GPT18 and GPT24b already received phase-bounding corrections and have real ESP32-S3 duration evidence.  
**Hardware context:** WLED 17.0.0-devV5 Kagayaki on Waveshare ESP32-S3, HUB75 64×64, 240 MHz.  
**Important:** Classification is from source code; it is not a claim that all high-risk effects have already exhibited hardware slowdown.

## Executive findings

- **18 HIGH:** unbounded time enters a native `sinf()` / `cosf()` (often through the shared `ridge()` helper) in the per-pixel renderer. The same large-input slow path observed in GPT24b / GPT18 may manifest over longer uptime, depending on per-effect phase rates and ESP-IDF math implementation.
- **2 PER_DOT:** GPT21b / GPT22b share one native `cosf(dist*ringWave-t*3)` evaluation per lattice dot rather than per panel pixel; likely smaller absolute impact at 64×64 but a genuine temporal-phase exposure.
- **4 FRAME_ONLY:** GPT06, GPT07, GPT12, GPT17 use unbounded temporal phases only in precomputed orbit / transformation terms. Some pixel-dependent trig may still exist (e.g. GPT06 gear spokes) but its input is spatially bounded; the time-growing expensive argument is not repeated per pixel.
- **2 LUT_WATCH:** GPT08 and GPT20 route their growing time terms through the shared 1024-sample interpolated LUT, not through libm under its normal accepted input range. However `gptfx_trig::linear()` falls back to native `sinf/cosf` when `abs(input) >= 100000`, so multi-day runtime and large-angle LUT precision deserve protection.
- **2 NO_TRIG:** GPT03 and GPT04 lack temporal `sinf/cosf` paths; time-related floating-point precision, fract/rotation drift and render scaling are separate concerns.

All 28 effects have been accounted for. **Total: 18 + 2 + 4 + 2 + 2 = 28.** This is not a broad proof of stability: the host benchmark does not reproduce the ESP32-S3 libm behavior.

## Per-effect inventory and proposed action

| Effect | Class | Source line | Relevant path | Recommended action |
|---|---|---:|---|---|
| GPT01 — Caustic Atlas | HIGH | 296 | sinf(warp spatial+t*.7), ridge(g with -t*1.4) | Wrap t*.7 and -t*1.4 independently; keep palette drift. |
| GPT02 — Voronoi Bloom | HIGH | 319 | ridge(14*sqrt(first)-t*1.7+id*.4) | Precompute -t*1.7 modulo 2pi; separately consider frame-only seed phase bounding. |
| GPT03 — Origami Reactor | NO_TRIG | 347 | No sinf/cosf/ridge; time enters frac and angular folding | Do not change frac/angle arithmetic under this audit; long-duration precision is separate. |
| GPT04 — Ink Tectonics | NO_TRIG | 369 | Cellular automaton; no trig | Exclude from trigonometric remediation. |
| GPT05 — Event Horizon | HIGH | 420 | ridge(ring -t*.8), sin(r*6-t*.6), ridge(spiral*2.7+...) | Bound -t*.8, -t*.6 and -t*2.7 separately; do not wrap entire spiral before multiplying. |
| GPT06 — Clockwork Cathedral | FRAME_ONLY | 488 | gearPose sinf/cosf(t*...) only once per frame; per-pixel trig uses bounded angles | Consider frame-only bounding of gearPose translation oscillations, preserving gear teeth and ratios. |
| GPT07 — Impossible Lattice | FRAME_ONLY | 549 | foldCos/foldSin(t*.09+j*.75) precomputed per frame | Optionally bound each fold angle before frame-only trig; avoid touching fractal transforms. |
| GPT08 — Aether Topography | LUT_WATCH | 579 | extraSine/extraCosine use LUT by default with growing t offsets | Before multi-day use, prewrap the time-only term for each of the six LUT calls. |
| GPT09 — Mandelglass | HIGH | 601 | sinf(t*.18) redundantly per pixel; ridge(trap*16+t) | Hoist sine(t*.18) once/frame; wrap t for ridge separately. |
| GPT10 — Glass Circuit | HIGH | 624 | sinf(ix*127.1+iy*311.7) hash seed; ridge(lineCoord*5-t*4.1+seed*8) | Wrap -t*4.1 for ridge; DO NOT normalize/rewrite sin(hash) without hash-equivalence validation. |
| GPT11 — Aurora Loom | HIGH | 645 | sinf(3.2*y+t), cosf(3.6*x-t*.71), sinf(...+t*.3) | Prewrap t, -t*.71, t*.3 independently; preserve warp/weft coordinates. |
| GPT12 — Lagrange Ballet | FRAME_ONLY | 662 | body orbital sin/cos evaluated 3 times/frame; ridge(potential*6) time-bounded | Optionally phase-bound body angles once/frame; no change to potential contours. |
| GPT13 — Chromatic Kintsugi | HIGH | 683 | multiple sinf/cosf(spatial +/- t*...), ridge(8*f+t) | Prewrap independent time coefficients and preserve frac crack/facet behavior. |
| GPT14 — Phantom Elevator | HIGH | 702 | ridge(yy+t*.4), yy=spatial+t*.7 | Prewrap t*1.1 for ridge; leave yy for stripeY and hue unchanged. |
| GPT15 — Ferrofluid Theatre | HIGH | 721 | ridge(vector*frequency + potential*.65-t*.42) | Prewrap -t*.42 before ridge; optionally bound frame-only pole orbits. |
| GPT16 — Riemann Current | HIGH | 742 | ridge(8*a0+8*log(r0)-t*1.6), ridge(8*a1-8*log(r1)+t*1.6) | Prewrap +t*1.6 once/frame; subtract/add independently, preserve logarithms. |
| GPT17 — Constellation Engine | FRAME_ONLY | 766 | star sinf/cosf angles computed for N=11 before pixels | If required, bound star orbital phase once per frame; keep star links unchanged. |
| GPT19 — Celestial Runewheel | HIGH | 824 | sinf((8+3*k)*(a +/- t*(.25+.12*k))) four times + sinf(a-t*.64) | Prewrap full multiplied temporal phase per ring, NOT the base time offset before multiplication. |
| GPT20 — Zenith Quasicrystal | LUT_WATCH | 847 | extraCosine LUT with timePhase[k] growing for each of five axes | Prewrap five timePhase[k] before per-pixel cosine only; keep hue drift. |
| GPT21b — Sparse Ripple Brightness | PER_DOT | 873 | cosf(dist*ringWave - t*3) once per lattice dot | Prewrap -t*3 once/frame; ensure same shared implementation for GPT22b. |
| GPT22b — Sparse Ripple Size | PER_DOT | 873 | cosf(dist*ringWave - t*3) once per lattice dot | Prewrap -t*3 once/frame; keep radius modulation unchanged. |
| GPT23b — Photon Loom | HIGH | 927 | two sinf(spatial+t coeff), two ridge(spatial+t coeff) | Prewrap t*.8,-t*.7,-t*2.8,+t*2.5 separately; keep crossing order. |
| GPT25b — Tidal Glyphs | HIGH | 1056 | sinf(cell coords-t*1.8) and sinf(angle*2+t*.7+cell coords) | Prewrap -t*1.8 and +t*.7; investigate per-cell reuse of theta separately. |
| GPTA01 — Bass Cartographer | HIGH | 1115 | sinf(9*a-t*1.1) per pixel | Prewrap -t*1.1; preserve audio state and palette. |
| GPTA02 — Harmonic Wormholes | HIGH | 1144 | ridge(13*r+8*a+bend-t*1.6) and ridge(24*r-2.5*bend+t) | Prewrap -t*1.6 and +t independently; preserve audio envelope. |
| GPTA03 — Spectral Loom | HIGH | 1174 | sinf(p.y*frequency+t*(1+k*.035)+k*1.8), ridge(p.y*19-t*3.4+k) | Prewrap 16 band-specific phases once/frame and -t*3.4 for ridge. |
| GPTA04 — Resonant Tessellator | HIGH | 1199 | ridge(atan2(ry,rx)*3+e*4-t*.5) | Prewrap -t*.5 once/frame; retain e-dependent facet modulation. |
| GPTA05 — Synaptic Reef | HIGH | 1227 | nested sinf(r*7+t), sinf(r*4-t), ridge(r*21-t*3) | Prewrap t, -t and -t*3 separately; outer sinf inputs remain geometry+bounded modulation. |

## Important special cases: do NOT normalize blindly

1. **`ridge(x)` uses `sinf(x)`.** A call to `ridge(spatial + omega*t)` is exposed just like `sinf(spatial + omega*t)`. Bound the **time-only periodic contribution** once per frame and leave the spatial value unchanged.
2. **Frequency multiplication matters.** In GPT19, `sinf((8+3*k)*(a + signed*t*omega))` must prewrap the entire temporal term `(8+3*k)*signed*t*omega` separately for each ring. Do not assume that wrapping before multiplying is safe in general: it is mathematically non-equivalent for noninteger multipliers, and even for integer multipliers can amplify finite-precision differences. Preserve each complete sine phase and test pixels.
3. **GPT10's `seed=frac(sinf(ix*127.1+iy*311.7)*43758.5453)` acts as a spatial hash.** Altering its argument reduction or replacing it with an interpolated LUT can completely change cell assignments. Treat hash sine separately from the genuinely periodic `ridge(lineCoord*5-t*4.1+seed*8)`.
4. **Never wrap palette hue, `frac()`-driven scroll positions, phase-to-grid integer indices, spatial fractal offsets, or noise hashes just because they contain time.** They are not equivalent to trigonometric modulo-2π operations.
5. **Nested trigonometry:** GPT05, GPTA05 and others use one sine to modulate a second waveform; normalize only the periodic component of each call. Keep the inner waveform's amplitude and spatial dependence intact.
6. **Per-frame trig:** GPT06/07/12/17 still have a slow-input path, but only a small fixed number of times per frame. Fixing it may improve numerical continuity but is unlikely to explain a 4× whole-frame slowdown by itself.
7. **Shared LUT input bound:** `gptfx_trig::withinRange()` accepts input absolute value below 100000 rad; outside this domain it falls back to standard math. GPT08/GPT20 are not fully protected for multi-day device uptimes despite currently being fast.

## Reproducible host experiment

`tests/profile_uptime_audit.cpp` runs all 30 effects with `strip.now` at 20, 600, 1800 and 7200 seconds, at speeds 135 and 235. Each scenario renders a 64×64 frame 24 times (6 warm-up + 18 samples), logging median and p90 to `PERFORMANCE_UPTIME_HOST_AUDIT.csv`.

**Interpretation:** the x86 host does not reproduce the ESP32-S3's expensive large-angle `sinf/cosf` path. Small host timing differences come from input geometry, branch behavior, cache state and host load; they cannot validate or reject embedded slowdown. The purpose is a reproducible comparative baseline and a guard against gross host regressions.

## Recommended implementation sequence (not yet applied)

**Stage 1 — highest per-pixel exposure:** GPT19, GPT23b, GPT11, GPTA05, GPT13 and GPT05. They run several native trigonometric operations per pixel and have conspicuous time coefficients, ring multipliers or nested waves. Prototype a *phase-bounding-only* patch, with the existing backend unchanged. Do not replace math functions with LUT at the same time.

**Stage 2 — remaining native per-pixel cases:** GPT01, GPT02, GPT09, GPT10 (hash guarded), GPT14, GPT15, GPT16, GPT25b, GPTA01, GPTA02, GPTA03, GPTA04. The audio effects should be exercised both with live AudioReactive input and its disconnected/dimmed mode.

**Stage 3 — per-dot and LUT longevity:** GPT21b and GPT22b bound `-t*3` once per frame. GPT08 and GPT20 prewrap each time-only LUT term so inputs remain modest regardless of uptime; do not wrap their palette/height/iso drift.

**Stage 4 — low exposure:** Optional per-frame phase bounding for GPT06, GPT07, GPT12 and GPT17, with attention to gear engagement in GPT06. No trig work for GPT03 or GPT04.

### Acceptance gates for each stage

- Preserve effect registration, UI parameters, defaults, logical geometry, `tick` intervals, speed scaling, hue and segment API.
- Add source-level and pixel-level baseline comparisons across speed (slow/default/fast), intensity, custom controls, palettes, 64×64/128×64/128×128, and uptimes 0/300/1800/7200 s. Include high-uptime cases and near phase wrap boundaries.
- Reject large coherent visual changes even if average RGB error is low. Record max per-channel error, number of changed pixels, >16/255 outliers and geometry-dependent clustering.
- On ESP32-S3 measure actual effect render times via `micros()`, recent average (last 32), max recent, skips, width/height and original/wrapped phase maxima. Do not use WLED `leds.fps` as a substitute for actual completed renders.
- Verify at boot, 5 min and >=35 min. Compare at matched effect settings. Keep old backend compile-time reversible; no core WLED or BLE changes.
- Avoid promotion until WLED/PlatformIO full build and device runtime qualify. Keep stable v0.2.2 unchanged.

## Validation of audit material

- The source was extracted directly from the user-tested dev.12 archive and left byte-for-byte unchanged; its SHA-256 is recorded above.
- Original host test script: PASS (`HOST_TEST_PASS` for all 30 effects across five resolutions and nine palette states, plus audio cases).
- Original AddressSanitizer and UndefinedBehaviorSanitizer test script: PASS.
- Audit uptime profiler: PASS, 30 effects × four simulated uptimes × two speeds = 240 scenarios (24 render invocations per scenario, including warm-up).
- PlatformIO/WLED embedded compilation: not performed for this audit; it contains no firmware modification. No embedded runtime or long-duration bench was performed on the remaining 28 effects.

The dev.10 ESP32-S3 microbenchmark reported native small-input trigonometry around 1.17 microseconds and large-input trigonometry around 12.2 microseconds (same firmware family and user-provided device telemetry); this supports *plausibility* of the source-level risks, not proof that each effect will reach the slow path under identical settings.

## Historical qualified cases — out of scope for the 28

- **GPT24b:** dev.9 phase normalization prevented progressive slowdown; dev.10 LUT reduced render time to about 34.8 ms and stayed stable through 754 s on the 64×64 Waveshare.
- **GPT18:** dev.11 worsened to `recentAvgUs=190948` at 1806 s. Dev.12 phase-only correction, using `labyrinthBackend=libm`, measured `recentAvgUs=39601` at 2094 s (34 min 54 s) with raw temporal phase about 340.7 rad and wrapped phase about 3.0 rad.

## Scope and version discipline

This package is an **audit only**. `gpt_matrix_fx.cpp`, `gptfx_trig_lut.h` and all other runtime files from dev.12 remain byte-for-byte unchanged. The source will **not** be labeled dev.13 until a specific implementation has been made, tested and packaged independently. This prevents confusion with the hardware-qualified dev.12 baseline.

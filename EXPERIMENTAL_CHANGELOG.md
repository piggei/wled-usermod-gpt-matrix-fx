# Experimental changelog

## v0.3.0-dev.2

- Renamed experimental modes to GPT21b and GPT22b.
- Fixed GPT22b Expansion slider: Intensity now controls maximum dot radius rather than peak brightness; radius remains capped by lattice pitch.
- Kept wave phase, grid centers, palette and speed unchanged.
- Added a deterministic slider regression (64x64, fixed wave phase): lit pixels increase from 484 at minimum Expansion to 1484 at maximum Expansion in the host mock. This is a coverage check, not a measured real-world brightness result.
- Documented the initial hardware smoke test separately from pending controlled A/B validation.
- All existing 25 effect algorithms remain unchanged.

Credit: Milo (1human/JM) suggested the original CC0 Dot ripple reference and identified the Expansion control mismatch in the experimental code.

## v0.3.0-dev.3

- Added GPT23b Photon Loom, GPT24b Kinetic Rosettes and GPT25b Tidal Glyphs.
- Retained GPT21b and GPT22b and all 25 original effects.
- New effects are experimental only; hardware testing pending.

## v0.3.0-dev.4

- Enlarged GPT24b Kinetic Rosettes by 22%, targeting approximately 90-95% matrix occupancy without changing its rotation, petal count or palette behavior.
- Recorded the maintainer's successful GPT22b Expansion hardware smoke test on 2026-10-10, while explicitly marking unreported firmware revision details as unknown.
- No other effect algorithm changed. Public v0.2.2 remains untouched.

## v0.3.0-dev.5
- Added runtime JSON info metadata for installed build identification.
- Replaced both Sparse Ripple `Reserved` controls with shared `Dot Spacing`.
- Replaced GPT24b `Hue` with a geometric `Size` slider.
- All stable and audio effects unchanged.

## v0.3.0-dev.7 — GPT24b diagnostic instrumentation

- Added cumulative low-overhead GPT24b render timing and internal tick-skip counters to `/json/info` under `u.gptMatrixFX.performance`.
- Kept the GPT24b rendering algorithm, controls, and timing gate unchanged. All other effects are unchanged.
- Added host regression coverage for the counters and timing gate.
- Hardware build and measurement pending.

## v0.3.0-dev.8 — GPT24b common-subexpression elimination + BLE-aware diagnostics

- Moved GPT24b polar angle and radius outside the two-layer loop and reused
  them for central glow and palette mapping. The color output, Size slider,
  rendering schedule and other 29 effects are unchanged.
- Added a 32-render rolling wall-clock window, latest render time, per-row
  elapsed-time aggregate and the start/end FreeRTOS core IDs to the existing
  GPT24b telemetry. All diagnostics are read-only and fixed-allocation.
- Added pixel-for-pixel equivalence tests against a frozen dev.7 renderer and
  an experimental host-only performance benchmark for all effects.
- Generated a preliminary cross-effect performance audit. Device-specific
  timings and BLE behavior remain subject to hardware validation.
- Stable v0.2.2 remains unchanged; development branch only.

## v0.3.0-dev.9 — GPT24b time-phase normalization

- Follow-up to the hardware investigation showing a time-dependent GPU-like
  workload regression **without** the iDotMatrix and Buzzer usermods.
- Bounded five independent time-only trigonometric arguments modulo 2*pi,
  one reduction per oscillator per rendered frame. Preserved layer frequencies,
  distance fields, palette timing, controls and frame gate.
- Added the last rendered frame's `phaseRawAbs` and `phaseWrappedAbs`
  diagnostics to `u.gptMatrixFX.performance`.
- Added frozen dev.8 reference and 48 long-uptime comparison cases spanning
  0 to 2 hours with four speed settings; results differ by at most 1/255 per
  channel on the tested host settings. Strict dev.7/dev.8 identity checks stay.
- Added an optional interleaved host A/B benchmark at seven simulated uptimes.
- All 29 other effects, WLED registration order and user controls unchanged.
- Stable v0.2.2 unchanged. ESP32-S3 firmware build and hardware qualification pending.

## v0.3.0-dev.10 — GPT24b experimental trigonometry LUT

- Added a read-only 1024-sample sine table (4096 bytes in flash) used as
  sine/cosine LUT for GPT24b only. C++11-compatible, no heap allocations.
- Added compile-time backend choice: 0 native libm, 1 nearest LUT,
  2 linear-interpolated LUT (default), preserving dev.9 time-phase bounding.
- Kept the original `ridge()` untouched for all other 29 effects; introduced
  a GPT24b-specific ridge helper for the selected trigonometry backend.
- Added one-shot `setup()` microbenchmark and JSON reporting at
  `u.gptMatrixFX.trigonometry` (native small/large, nearest and linear).
- 120 frozen-dev.9 frame comparisons for each of three backends; the
  default linear mode shows <=1/255 channel difference, <=2 changed pixels
  per tested 64x64 frame. Nearest mode is less accurate and experimental.
- Host tests, C++11 compatibility, ASan/UBSan pass. Real ESP32-S3 gain is
  pending hardware verification; do not extrapolate host results.
- All other effects, registration order, palettes, timing gate, UI controls,
  BLE/FreeRTOS settings and stable release v0.2.2 remain unchanged.

## v0.3.0-dev.11 — Selective multi-effect LUT extension

- Kept dev.10 GPT24b code, LUT selection, phase normalization and timing diagnostics unchanged.
- GPT07 Impossible Lattice: precompute static per-frame fold rotations instead of per-pixel sin/cos (frozen dev.10 framebuffer checks byte-identical).
- GPT08 Aether Topography: use the existing interpolated LUT for six trig evaluations per pixel, controlled by `GPTFX_EXTRA_TRIG_BACKEND` (default 2).
- GPT18 Vitreous Labyrinth: hoist four time-only sine terms per frame; keep per-pixel `sinf` native by default; add opt-in `GPTFX_GPT18_TRIG_BACKEND=2`.
- GPT20 Zenith Quasicrystal: hoist the five sine/cosine direction pairs and temporal offsets per frame; use shared LUT for five pixel-local cosine calls.
- Add `extraPerformance` read-only rolling timings in `/json/info` for the four candidate effects; add selected-backend identifiers under `trigonometry`.
- Differential host visual tests for three matrix resolutions, backend A/B tests, host performance profiling, and ASan/UBSan passed. See `PERFORMANCE_EXTENSION_DEV11.md` for caveats and hardware qualification plan.
- Firmware compilation and physical ESP32-S3 qualification pending; public v0.2.2 unchanged.

## v0.3.0-dev.12 — GPT18 temporal trigonometric phase bounding

- Diagnose physical GPT18 slowdown from ~44.7 ms/render at 1048 s to ~190.9
  ms/render at 1806 s on the same ESP32-S3 64x64 setup, even without
  iDotMatrix/Buzzer.
- Bound time-only inputs of four per-pixel wave sines and four fold-offset
  sines once per layer per rendered frame. Native libm remains the default.
- Add independent `GPTFX_GPT18_PHASE_BOUNDING` compile switch (default 1),
  retain `GPTFX_GPT18_TRIG_BACKEND` (default 0), for controlled hardware A/B.
- Add latest GPT18 `phaseRawAbs`, `phaseWrappedAbs` temporal diagnostics and
  boolean `labyrinthPhaseBounding` in `/json/info`.
- Preserve GPT24b, GPT07, GPT08, GPT20, all other effects, names, sliders,
  timing gates, audio paths, palette mapping and dynamic effect registration.
- Add frozen-reference visual tests at 64x64, 128x64 and 128x128 across 324
  combinations per backend configuration; preserve full 30-effect suite.
- Host test success does not establish ESP32-S3 throughput; device qualification
  remains pending. See `PERFORMANCE_INVESTIGATION_DEV12.md`.

## v0.3.0-dev.13 — Stage-1 temporal trigonometry remediation

- Started from frozen hardware-tested dev.12 without changing its 24 other renderers, effect ID registration order, sliders, palettes, scheduling or audio sampling.
- Bound time-only sine/ridge phase inputs once per rendered frame for GPT05, GPT11, GPT13, GPT19, GPT23b and GPTA05 (no new LUT). GPT05 reduces the multiplied spiral phase and GPT19 independently bounds all four frequency-multiplied rune-dial phases.
- `GPTFX_STAGE1_PHASE_BOUNDING=1` by default; `=0` restores original dev.12 expressions pixel-for-pixel for all six effects.
- Extended the existing rolling `extraPerformance` telemetry to monitor the six effects and report last-frame raw/bounded temporal phase maxima. Added `trigonometry.stage1PhaseBounding` and `stage1Effects`.
- Frozen dev.12 reference: 636 visual host scenarios in each mode, 180 audio scenarios per mode; bounded graphical differences quantified; all reference-off comparisons byte-identical. Host tests, GPT18 regression and sanitizers pass.
- Performance CSV is **host only**. Physical ESP32-S3 dev.13 qualification and complete WLED firmware compilation remain pending. Stable v0.2.2 unchanged.

See `PERFORMANCE_REMEDIATION_DEV13.md`.


## dev.14 — Stage-2A temporal phase bounding (experimental)

Correct native-libm time-phase exposure in GPT01, GPT02, GPT09, GPT10, GPT14 and GPT16. Prewrap only the periodic temporal contribution once per frame, preserve GPT10 spatial hash and GPT14 nonperiodic stripe/hue scroll, and retain a reversible `GPTFX_STAGE2_PHASE_BOUNDING` flag. Extend existing `extraPerformance` telemetry. Frozen dev.13 comparisons, independent old/new visual comparisons, multi-day diagnostic, regression and sanitizers are included in `PERFORMANCE_REMEDIATION_DEV14.md`. All other 24 renderers are byte-for-byte unchanged. WLED hardware qualification pending.

## v0.3.0-dev.14a — GPT10 late-uptime remediation

- GPT10-only exact cell-hash cache avoids repeated native large-angle
  `sinf()` for all pixels belonging to the same integer circuit cell.
  Original float hash expression and all seeds remain unchanged.
- Independently bound the *combined* temporal packet phase after accounting
  for the moving x/y grid, rather than bounding only `-4.1*t`.
- Two reversible flags: `GPTFX_GPT10_CELL_CACHE` and
  `GPTFX_GPT10_PACKET_BOUNDING`, enabled by default when Stage-2A is on.
- Added diagnostic counts and sampled packet argument maxima in
  `/json/info.u.gptMatrixFX.gpt10Diagnostics`.
- 204 host scenarios x 4 gate modes passed. Exact hash-only rendering
  matches dev.14 pixel-for-pixel in all 204 cases (up to 48h).
- Other 30 renderer bodies unchanged; complete firmware build and physical
  hardware validation pending. Legacy 48h dev.13 comparison still has a
  GPT10 outlier above its former threshold, explicitly documented.

See `PERFORMANCE_REMEDIATION_DEV14A.md` for details and evidence.

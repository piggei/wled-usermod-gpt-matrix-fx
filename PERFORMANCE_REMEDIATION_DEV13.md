# GPT Matrix FX v0.3.0-dev.13 — Stage-1 Temporal Trigonometry Remediation

**Date:** 2026-10-11  
**Baseline:** user-tested `v0.3.0-dev.12` (original `gpt_matrix_fx.cpp` SHA-256 `f9732e8144e83ef9b8f0600921059c386980a4aa6a69cdb579f30484f6addc09`).  
**Scope:** six high-priority effects from `TRIG_PHASE_AUDIT_DEV12.md`.  
**Release status:** experimental source candidate; full WLED/PlatformIO compile and ESP32-S3 runtime validation pending. Public v0.2.2 is untouched.

## Why this patch exists

The hardware-qualified GPT18 and GPT24b fixes demonstrated that ESP32-S3 native trigonometric functions can become costly when a time-dependent argument grows without bound. Device-side microbenchmark previously reported approximately 1.17 us for bounded inputs versus 12.2 us for large native inputs. A code audit identified multiple effects with large native `sinf/cosf` input computed once or more per pixel.

**This six-effect patch changes only the time-dependent periodic contributions in those calls.** It does not add a new LUT, alter the global `timePhase()` function, change WLED's scheduler or task affinities, or wrap hue, `frac`, spatial coordinates, audio state, or pseudo-random hash inputs.

## Implemented effects

| Effect | Original hot path | dev.13 adjustment | Temporal phases bounded per frame |
|---|---|---|---:|
| GPT05 — Event Horizon | `ridge(ring-t*0.8)`, `sinf(r*6-t*0.6)`, nested `ridge(2.7*spiral+...)` | Wrap `-0.8*t`, `-0.6*t`, **`-2.7*t` after spiral frequency multiplication** | 3 |
| GPT11 — Aurora Loom | `sinf(3.2*y+t)`, `cosf(3.6*x-0.71*t)`, `sinf(woven+0.3*t)` | Wrap `t`, `-0.71*t`, `0.3*t` | 3 |
| GPT13 — Chromatic Kintsugi | Four spatial wave sines/cosines and one `ridge(8*f+t)` | Wrap `0.31*t`, `-0.43*t`, `0.42*t`, `-0.34*t`, `t` independently | 5 |
| GPT19 — Celestial Runewheel | Four `sinf((8+3*k)*(a+signed*omega*t))`, one `sinf(a-0.64*t)` | **Wrap the full frequency-multiplied temporal term per dial**, plus pointer offset | 5 |
| GPT23b — Photon Loom | Two thread warp sines, two per-pixel `ridge()` pulses | Wrap `0.8*t`, `-0.7*t`, `-2.8*t`, `2.5*t` | 4 |
| GPTA05 — Synaptic Reef | Two nested wave sines and one `ridge(21*r-3*t)` | Wrap `t`, `-t`, `-3*t`; keep outer waves and audio modulation untouched | 3 |

Temporal phases are centered in [-pi, pi] using `fmodf`, **outside pixel loops**. Original native `sinf/cosf` and shared `ridge()` remain unchanged. The same six effects are wrapped for wall-clock telemetry, using the existing 32-render `extraPerformance` buffer shared with GPT07/08/18/20. There is no additional framebuffer or heap allocation.

## Reversible build flag

`GPTFX_STAGE1_PHASE_BOUNDING=1` is default. Set `-DGPTFX_STAGE1_PHASE_BOUNDING=0` in the existing WLED `build_flags` to build the exact original dev.12 rendering expressions for all six effects. The legacy behavior was validated as **pixel-identical across all 636 test scenarios**. Other existing flags (`GPTFX_GPT18_PHASE_BOUNDING`, `GPTFX_EXTRA_TRIG_BACKEND`, `GPTFX_GPT24_TRIG_BACKEND`, etc.) retain their meanings and defaults.

## Frozen-reference visual proof

`tests/gpt_stage1_dev12_reference.h` contains verbatim dev.12 render functions with different function names, so output comparisons do not depend on maintaining a second code branch manually.

`tests/test_stage1_phase.cpp`: 106 scenarios per effect (636 total) with three matrix sizes (64x64, 128x64, 128x128), speeds 0/135/255 or 0/225, palettes 0/6/8, two settings for controls/intensity, and simulated uptimes from 0 to 7200 seconds. Each scenario checks pixel channel differences, changed pixels, outliers above 16/255, max connected component of outliers, and per-frame phase bounds.

| Effect | Max mean RGB channel delta (0..255 scale) | Max changed pixels, any tested frame | Max >16/255 pixels | Largest contiguous >16/255 cluster | Worst individual delta |
|---|---:|---:|---:|---:|---:|
| GPT05 | 0.02930 | 747 | 0 | 0 | 1 |
| GPT11 | 0.01733 | 351 | 1 | 1 | 208 |
| GPT13 | 0.01058 | 156 | 1 | 1 | 103 |
| GPT19 | 0.04370 | 468 | 0 | 0 | 10 |
| GPT23b | 0.01546 | 390 | 0 | 0 | 2 |
| GPTA05 (no audio) | 0.00114 | 8 | 0 | 0 | 1 |

**Caveat:** Although the overall mean errors are very low, the GPT11 and GPT13 palette discontinuities can create single-pixel full-color changes. Physical visual review is required; this does not establish bitwise equivalence with bounding enabled. The 747 changed pixels on GPT05 generally differ by only a single channel level.

`tests/test_stage1_audio.cpp`: additional 180 scenarios per build setting (90 no-audio and 90 connected-AudioReactive), 64x64 and 128x128, speed/palette sweeps and simulated uptime up to 7200 seconds. In the audio-connected cases the largest channel difference was 1, with no >16/255 outliers in this test. All tests passed in enabled and disabled builds.

## Host profiling

`tests/benchmark_stage1_host.cpp` measures original frozen dev.12 and the bounded dev.13 variant at 64x64, 20/600/1800/7200 simulated seconds, speeds 135 and 235. It records median host frame time in `PERFORMANCE_STAGE1_HOST_DEV13.csv`.

At simulated 7200 s and speed 135, the interleaved host comparison approximately measured:

| Effect | dev.12 reference (us) | dev.13 (us) | Time reduction |
|---|---:|---:|---:|
| GPT05 | 210.74 | 185.52 | 12.0% |
| GPT11 | 155.44 | 136.70 | 12.1% |
| GPT13 | 239.34 | 227.55 | 4.9% |
| GPT19 | 420.53 | 353.89 | 15.8% |
| GPT23b | 205.56 | 189.87 | 7.6% |
| GPTA05 | 267.88 | 256.81 | 4.1% |

Other host cases show neutral or slightly negative gains (e.g. GPT05 at 20 s). **These measurements do not predict the ESP32-S3 improvement.** Real-hardware evaluation should measure `recentAvgUs` at matching effect settings before and after the time thresholds. Changing the clocks, BLE state, other usermods, or audio input would confound comparisons.

## Added `/json/info` instrumentation

- `u.gptMatrixFX.build = "dev.13"`
- `u.gptMatrixFX.trigonometry.stage1PhaseBounding = true` (or `false` for the compile-time A/B)
- `u.gptMatrixFX.trigonometry.stage1Effects = "GPT05,GPT11,GPT13,GPT19,GPT23b,GPTA05"`
- `u.gptMatrixFX.extraPerformance.effect` shows the last monitored effect, including the six new cases and the previously monitored four effects.
- `...extraPerformance.recentAvgUs`, `recentMaxUs`, `rendered`, `skipped`, `width`, `height` are measured by the existing last-32-frames rolling implementation.
- `...extraPerformance.phaseRawAbs` and `phaseWrappedAbs` show **the most recent completed frame's temporal-only phase magnitude** for GPT18 or one of the six stage-1 effects. Raw values may grow; wrapped values should remain <= pi for the six new effects.

`extraPerformance` resets when the measured effect changes, or at its first segment invocation. It is **not a simultaneous multi-effect profiling table**. `leds.fps` is WLED's general frame metric, not the per-effect render rate; rely on the recorded render count and `recentAvgUs` for the comparisons.

## Test record and hardware acceptance

- Existing `tests/run_host_tests.sh`: **PASS**, 30 effects, palette, audio, geometry, scheduler telemetry and compatibility.
- Existing `tests/run_labyrinth_phase_tests.sh`: **PASS**, both phase-bounding variants and both native/LUT GPT18 modes, at three matrix sizes.
- New `tests/run_stage1_phase_tests.sh`: **PASS**, both stage-1 flag settings, C++11 Arduino compatibility, 636 visual and 180 audio scenarios per setting.
- Existing `tests/run_sanitizers.sh`: **PASS**, AddressSanitizer + UndefinedBehaviorSanitizer on host.
- WLED/PlatformIO full integrated build: **NOT PERFORMED** in this environment.
- On-device six-effect firmware test: **PENDING**. No claims of device FPS speedup or device-phase stability until actual measurement.

**Recommended device qualification:** compile with default flags (stage1=1); run GPT19, GPT23b, GPT11, GPTA05, GPT13, GPT05, at unchanged settings with `/json/info` samples near 20 s, 5 min and >=35 min *after boot* for each effect. Since uptime is global, changing effects without reboot is acceptable for phase exposure, but each effect's recent window needs enough completed renders. To isolate the growing-time effect, a warm start after 35 minutes of uptime is a useful stress test. A complete six-effect 35-minute soak for each effect is stronger but not a prerequisite for a first diagnosis. Test GPTA05 with live AudioReactive input and then with missing input, and inspect physical visual artifacts on GPT11/GPT13.

## Still pending

Stage 2: GPT01/02/09/10/14/15/16/25b/A01/A02/A03/A04; Stage 3: shared per-dot GPT21b/22b and long-runtime LUT fallbacks GPT08/20; Stage 4: per-frame trig in GPT06/07/12/17. GPT10's sine spatial hash must remain untouched. Stable v0.2.2 and frozen dev.12 source archive are not modified.

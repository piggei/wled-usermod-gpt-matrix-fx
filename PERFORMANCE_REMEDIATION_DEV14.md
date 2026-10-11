# GPT Matrix FX v0.3.0-dev.14 — Stage-2A Temporal Phase Remediation

**Date:** 2026-10-11  
**Baseline:** source archive `wled-usermod-gpt-matrix-fx-v0.3.0-dev.13-temporal-phase-stage1.zip`, previously verified on the real 64×64 Waveshare ESP32-S3.  
**Scope:** six additional effects, stage-2A: GPT01 / GPT02 / GPT09 / GPT10 / GPT14 / GPT16.  
**Status:** host-tested experimental source candidate, NOT qualified on hardware; does not change stable public v0.2.2.

## Motivation and scope discipline

The device exposed severe large-angle native `sinf/cosf` slowdown over uptime in GPT18 and GPT19. Bounds on periodic temporal sine inputs resolved their *observed* duration regressions. The `TRIG_PHASE_AUDIT_DEV12.md` audit identifies other effects which repeatedly call native trigonometry with growing time contributions. Stage 1, dev.13, was validated physically at elevated uptime for six effects: GPT05, GPT11, GPT13, GPT19, GPT23b, GPTA05.

This dev.14 stage-2A **bounds periodic time components ONLY**, once per effect frame using centered `fmodf` into `[-pi, pi]`. `libm` continues to be used for per-pixel trigonometry: no new LUT, change to the global timebase, WLED scheduler or AudioReactive path, extra framebuffer or heap allocation, effect ID/name/controls/palette changes.

## Exact changes

| Effect | Hot path | Stage-2A bounded terms | Critical preservation |
|---|---|---|---|
| GPT01 — Caustic Atlas | Per-pixel spatial warp sine and refracted ridge | `+0.7*t`, `-1.4*t` | Emitter orbits, prism/hue, lens remain identical. |
| GPT02 — Voronoi Bloom | Per-pixel distance-pulse ridge | `-1.7*t` | Voronoi seed orbits computed per frame; nearest-cell and hue unchanged. |
| GPT09 — Mandelglass | Per-pixel redundant Julia translation sine and glass ridge | `+0.18*t`, `+t` | When enabled, inner sine also hoisted once/frame; fractal iteration/escape/hue unchanged. Disabled branch reproduces original sine in each pixel. |
| GPT10 — Glass Circuit | Per-pixel moving packet ridge | `-4.1*t` | **Spatial sine hash, cell assignment, line selection, grid travel and palette unchanged.** No modifications to `sinf(ix*127.1f+iy*311.7f)` or `frac()`. |
| GPT14 — Phantom Elevator | Per-pixel illuminated corridor ridge | `0.7*t + 0.4*t`, as combined sine-only contribution | `yy = spatial + 0.7*t` continues to drive `stripeY`, perspective and hue *unbounded*, intentionally. |
| GPT16 — Riemann Current | Two per-pixel logarithmic ridge terms | `-1.6*t`, `+1.6*t` independently | `logf`, `atan2f`, coordinates, interference, hue retained. |

The disabled `GPTFX_STAGE2_PHASE_BOUNDING=0` build retains the exact dev.13 behavior for all six effects. The compile-time gate is **independent** of the dev.13 Stage-1, GPT18 and GPT24b flags. Stage-2 reduces only the inputs to periodic math functions; it must never be applied to unrelated `frac`, palette colors, pseudo-random hashes, spatial transformations or screen position calculations.

## Instrumentation

Version in `/json/info` is `u.gptMatrixFX.build="dev.14"`. Within `trigonometry`:

- `stage1PhaseBounding=true` remains on by default, **unchanged**.
- `stage1Effects="GPT05,GPT11,GPT13,GPT19,GPT23b,GPTA05"` unchanged.
- `stage2PhaseBounding=true` (unless compiled with flag `0`).
- `stage2Effects="GPT01,GPT02,GPT09,GPT10,GPT14,GPT16"`.

The preexisting, shared `extraPerformance` telemetry now tracks these six effects too; it resets on effect switches. Fields include `effect`, `calls`, `rendered`, `skipped`, `renderLastUs`, `renderMaxUs`, `recentCount` (last 32 renders), `recentAvgUs`, `recentMaxUs`, 2D dimensions and `phaseRawAbs` / `phaseWrappedAbs` (temporal terms, **not** all pixel-dependent trig arguments). With bounding enabled, `phaseWrappedAbs <= pi` should hold independently of uptime, without implying correctness of unrelated nonperiodic animation components. With stage-2 disabled, the two stage-2-specific phase diagnostics remain zero by design. Device-level `leds.fps` is not the same as effect-local render frequency.

## Reproducible tests

Files under `tests/`:

- `gpt_stage2_dev13_reference.h`: verbatim frozen dev.13 implementations, extracted **before** patching; test comparisons avoid manually reconstructing the baseline.
- `test_stage2_phase.cpp` + `run_stage2_phase_tests.sh`: 106 scenarios per effect, 636 per compiled mode (1,272 comparisons across enabled/disabled). Combinations of panel 64x64, 128x64, 128x128, speed, palette, intensity/custom controls and simulated uptimes 0–7200 seconds. Check channel mean, outlier pixel count, connected outlier clusters and finite/centered time phases. **Disabled: all 636 images pixel-exact. Enabled: all 636 pass strict visual limits.**
- `test_stage2_phase_long.cpp` + `run_stage2_long_tests.sh`: 150 scenarios per effect, **900 per mode (1,800 total)** with added 4h, 12h, 24h and 48h simulated uptimes. Disabled: 900 exact images. Enabled: 900 pass a relaxed **diagnostic** criterion, with documented floating-point precision warnings; **these tests do not qualify multiday visual fidelity.**
- `benchmark_stage2_host.cpp` + `run_stage2_host_benchmark.sh`: 48 host measurements, 4 simulated uptimes x 2 speeds x 6 effects, x86 median microseconds logged in `PERFORMANCE_STAGE2_HOST_DEV14.csv`. Host math timing is **not** predictive of ESP32-S3 libm performance.
- `verify_dev13_unchanged.py`: compares all 24 untouched renderer bodies against the dev.13 source archive. **24/24 byte-identical**, with the GPT10 spatial hash source string also asserted verbatim.
- Previously existing host, Stage-1, GPT18 dual backend and sanitizer tests also pass. WLED complete PlatformIO compile is not available here and has **not** been performed.

### Per-effect strict 0–2 h visual envelope

Maxima across 106 cases, with bounding enabled, comparing to frozen dev.13:

| Effect | Worst mean absolute RGB channel delta /255 | Maximum changed pixels (single frame) | Maximum pixels with channel delta >16/255 | Largest connected high-delta cluster | Worst individual channel delta |
|---|---:|---:|---:|---:|---:|
| GPT01 | 0.01676 | 402 | 0 | 0 | 1 |
| GPT02 | 0.01375 | 295 | 0 | 0 | 1 |
| GPT09 | 0.03003 | 115 | 3 | 1 | 200 |
| GPT10 | 0.05688 | 350 | 5 | 2 | 249 |
| GPT14 | 0.00684 | 51 | 0 | 0 | 1 |
| GPT16 | 0.02873 | 470 | 0 | 0 | 2 |

**Important:** rare full-color outliers remain in GPT09 and especially GPT10 near discontinuities in palette or geometric thresholds. They are isolated in tested 0–2 h frames; low average error **does not** prove visual identity. Prioritize physical inspection of these two effects.

### Beyond two hours: known precision limitation

The original renderer computes `timePhase()` as `float` and many expressions subsequently combine large time values with small spatial or palette contributions. Loss of float32 resolution occurs *even if* the trigonometric term is later reduced modulo 2pi. When compared with the original **already-imprecise** large-angle output, some phase-bounded images increasingly differ after 12–48 simulated hours. A dev.13 pixel-exact A/B is still possible with the bounding flag off, but that old path is susceptible to slow libm calls on the ESP32-S3.

The exploratory extended comparison has the following examples at 48h (speed 255, 64x64, palette 6): GPT10 mean channel difference about **1.08/255** and ~91 high-delta pixels; GPT16 about **0.45/255**, mostly small differences; GPT09 about **0.24/255** with scattered individual outliers. Across all extended cases GPT10 worst mean **1.27/255**, worst high-pixel count **176/16384** on a larger panel, and contiguous outlier clusters at most 4 pixels. Extended tests deliberately use separate more permissive diagnostic thresholds and log `STAGE2_LONG_PRECISION_WARNING`. They are NOT evidence of byte identity or qualification of 24/48-hour appearance. A future multi-day time precision project must be planned separately and must not silently alter stage-2A scope.

### Host speed, illustrative only

At 7200s simulated and speed 135, host measured reductions are roughly: GPT01 1.7%, GPT02 4.4%, GPT09 6.1%, GPT10 0.7%, GPT14 0.3%, GPT16 4.7%. Results vary by CPU load and scenario. ESP32-S3 hardware requires its own `/json/info` profiling (especially GPT10/14 where host improvement is tiny).

## Proposed ESP32-S3 hardware validation

1. Compile stock dev.14 with stage-1=1, stage-2=1, GPT18 libm and existing LUT defaults. Verify `build=dev.14`, `stage2PhaseBounding=true` and `stage2Effects` in `/json/info`.
2. Test **GPT10 first**, then GPT09, GPT16, GPT01, GPT02, GPT14. Leave other settings and usermods unchanged. After each switch, wait for `extraPerformance.recentCount=32`, and collect FPS, `recentAvgUs`, raw/bounded phases and RAM. Compare to dev.13 only if the *same effect*, speed, palette and hardware load were recorded there; do not infer speedup from nonmatching effects.
3. First test a newly installed effect at 15–30s and 5min. For long-uptime evidence, leave GPT10 or GPT09 running for 30+ minutes or start them at an already high uptime and monitor in multiple samples. A warm-start test proves high phase tolerance; a soak additionally tests time progression.
4. **Visually inspect GPT09 and GPT10** for isolated color anomalies, discontinuities, judder or unexpected hash/cell changes; monitor GPT14 moving stripes and GPT16 counterwinding vortex shapes. If abnormal behavior is observed, use stage2=0 as a *separate compilation A/B*, not a runtime setting.
5. Prefer stable previous dev.13 hardware baseline until the stage-2A candidate receives measured and visual acceptance. AudioReactive GPTA01..GPTA04 remain for a separate stage-2B; GPT08/GPT20 long-term LUT precision remains separately open.

## Explicit nonchanges and next stages

- **Byte-identical 24 renderer function bodies** in the dev.13 archive, including all six stage-1 effects, GPT18 and GPT24b. No changes to effect registration order/names, UI, WLED core or stable public v0.2.2.
- Stage-2B still pending: GPT15, GPT25b, GPTA01, GPTA02, GPTA03 and GPTA04 (the remaining six high-risk paths from the dev.12 audit).
- Per-dot GPT21b/GPT22b; LUT-watch GPT08/GPT20; per-frame GPT06/GPT07/GPT12/GPT17 will be treated independently, after validating current patch.

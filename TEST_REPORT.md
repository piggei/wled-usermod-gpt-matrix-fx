# GPT Matrix FX v0.3.0-dev.2 - Test report

Experimental branch build only. Public v0.2.2 remains unchanged.

- GPT21b: fixed-size dots; Intensity controls brightness contrast.
- GPT22b: Intensity controls actual maximum dot radius, capped by lattice pitch.
- Both variants retain shared lattice geometry, phase and palette behavior.
- Host regression: 30 effects across five resolutions, palette changes, audio fallback and motion checks.
- Dedicated slider regression checks that increasing Expansion increases lit-pixel footprint at fixed phase.
- Host AddressSanitizer and UndefinedBehaviorSanitizer: see local test execution.
- ESP32/WLED full compilation: not performed here.
- Hardware: maintainer reported initial visual smoke test of the prior experimental build on 64x64 HUB75, preferring GPT22. Exact firmware configuration/date not recorded. The Expansion fix was subsequently tested successfully on real hardware (maintainer confirmation, 2026-10-10).

## v0.3.0-dev.3 additions

- All 30 registered effects passed host regression at 8x8, 16x32, 64x64, 96x48 and 128x128.
- All 30 passed palette switching across nine palette states.
- AddressSanitizer and UndefinedBehaviorSanitizer passed.
- Full WLED PlatformIO firmware compilation and hardware visual evaluation are pending.


## v0.3.0-dev.4 - Hardware record and rosette scale

- GPT22b Expansion fix: **PASS**, maintainer-confirmed real-hardware smoke test on 2026-10-10. Expansion visibly changes dot radius across its range.
- Hardware: ESP32-S3 driving a 64x64 HUB75 RGB LED matrix.
- User's previously reported PlatformIO environment: `waveshare_spotify`; exact WLED firmware revision, compiler configuration, and flashed binary hash were not supplied and are **not independently verified**.
- This confirms functional slider behavior only; controlled brightness matching and pitch comparison remain pending.
- GPT23b and GPT25b: maintainer reported pleasing, glitter-like results on hardware; not yet formally qualified.
- GPT24b: maintainer reported good visual quality but approximately 75% screen occupancy in dev.3. Dev.4 increases the rosette geometry scale by 22% (target approximately 92% occupancy on 64x64). **Dev.4 hardware check pending**.
- Host tests: PASS, 30 effects x 5 resolutions, 9 palette states, 5 FFT checks and other regressions.
- AddressSanitizer / UndefinedBehaviorSanitizer: PASS on host.
- Full PlatformIO firmware build: not performed for dev.4.

## v0.3.0-dev.5 - Diagnostics and beta controls

- Added `gptMatrixFX` metadata to `/json/info` (version/build and effect counts).
- GPT21b/GPT22b: `Dot Spacing` replaces `Reserved` with shared pitch control.
- GPT24b: `Size` replaces `Hue`; slider midpoint preserves the dev.4 scale.
- Hardware confirmation of these dev.5 changes: **pending**.
- Full WLED PlatformIO build: **pending**.

## v0.3.0-dev.6 - JSON info namespace and macro compatibility

- Moved diagnostics from `/json/info.gptMatrixFX` to `/json/info.u.gptMatrixFX`, preserving other `u` entries.
- Renamed global constants to `GPTFX_VERSION` and `GPTFX_BUILD` to avoid WLED `VERSION` macro collision.
- Added host regression assertions for the nested JSON path and defined WLED `VERSION` in the test stub.
- dev.5 changes (JSON presence and new sliders) were reported working on physical hardware by the maintainer; dev.6 firmware compilation and hardware verification remain pending.

## v0.3.0-dev.7 performance instrumentation

- Host test: PASS (30 effects, five resolutions, palette/audio/gear regressions).
- GPT24b instrumentation test: PASS (two renders and one skipped call, dimension and timer assertions).
- AddressSanitizer/UndefinedBehaviorSanitizer: PASS.
- Full PlatformIO WLED build: NOT RUN in this environment.
- Real ESP32-S3 performance telemetry: PENDING.
- Diagnostic counters do not measure LED DMA or WLED scheduler time.

## v0.3.0-dev.8 — GPT24b optimization and static performance audit

- Baseline: original v0.3.0-dev.7 source, no modifications to any of the
  existing effect functions except GPT24b.
- GPT24b expected pixel output: reference comparison against frozen dev.7
  function at 7 resolutions/control/palette/time combinations (8x8, 16x32,
  64x64, 96x48, 128x128); **PASS / byte-identical host framebuffers**.
- All 30 effects: host regression, palette changes, FFT response and fallback,
  geometry, gear meshing and telemetry diagnostics: **PASS**.
- C++11 compile-only compatibility (Arduino macro simulation): **PASS**.
- Host x86-64 benchmark (72-frame median): dev.8 GPT24b **~381 us**;
  interleaved dev.7/dev.8 A/B median **~538 us vs ~375 us** (roughly 30%
  lower host elapsed time). **NOT an ESP32 performance prediction.**
- GPT24b counters: 32-sample rolling average, recent peak, row timings and
  core start/end exposed under `/json/info.u.gptMatrixFX.performance`.
- Host AddressSanitizer / UndefinedBehaviorSanitizer: **PASS** with the
  seven reference comparisons and the rolling-window telemetry tests.
- Full WLED PlatformIO build with the user's custom environment: **NOT RUN**.
- dev.8 on physical ESP32-S3 with BLE advertising vs connected: **PENDING**.
- Real-hardware dev.7 baseline, reported by maintainer 2026-10-10 on
  ESP32-S3 64x64 HUB75: GPT24b advertising 13 FPS / 64965 us cumulative
  render average; after BLE connection 4 FPS / 128856 us cumulative average.
  Times are elapsed wall-clock and the latter includes earlier frames.
  Disabling iDotMatrix at runtime without reboot is not equivalent to a
  clean startup without NimBLE.

### Detailed dev.8 validation (2026-10-10)

- `sh tests/run_host_tests.sh`: **PASS**, including a separate C++11
  compile-only guard for the ESP32 `xPortGetCoreID()` branch and Arduino macro
  compatibility.
- `sh tests/run_sanitizers.sh`: **PASS** under AddressSanitizer and
  UndefinedBehaviorSanitizer.
- GPT24b visual equivalence: **PASS**, 7 cases compared byte-for-byte against
  `tests/gpt24_dev7_reference.h`.
- GPT24b rolling-window telemetry regression: **PASS**, including >32
  complete frames and correct bounded ring behavior.
- Static mode comparison: the sole changed effect implementation is GPT24b;
  the other 29 effects and the shared Sparse Ripple implementation remain
  identical to dev.7.
- Full list of 64x64 x86 host effect timings: `PERFORMANCE_HOST_DEV8.csv`.
  Interpret this only as a preliminary host cost audit.

## v0.3.0-dev.9 — time-dependent phase fix candidate

- Baseline: v0.3.0-dev.8 (real hardware 16 FPS / 50.5 ms after boot and 2 FPS /
  345.9 ms after 697 s **without** iDotMatrix or Buzzer). Confirmed GPT24b
  `calls` increased and `skipped=0`. Spotify usermod installed but disabled.
- Changed only GPT24b's time-dependent sine/cosine arguments; reductions
  happen outside the pixel loop. Added two related telemetry values.
- Reference comparison: 7 settings/resolutions maintain dev.7 == dev.8
  bit identity, while dev.9 differs from dev.8 by at most 2 channel levels.
- Long-uptime reference test: PASS, 48 runs (4 speeds x 12 uptime points,
  0-7200 s). Observed worst channel difference 1/255, worst changed pixels
  67/4096 for a single tested 64x64 frame.
- Full host regression: PASS, all 30 effects at five matrix geometries,
  palette switching, audio response, fallback, geometry slider tests and
  GPT06 mechanical-meshing invariants.
- Arduino C++11 compile-compatibility probe: PASS (host mock).
- ASan + UBSan: PASS on the host test suite.
- Interleaved host benchmark: `PHASE_HOST_DEV9.csv`. The host did **not**
  reproduce the severe time-dependent slowdown. This is unsurprising when
  using a different math library and CPU; do not extrapolate to ESP32-S3.
- Full WLED PlatformIO compilation: NOT PERFORMED in this environment.
- Real Waveshare 64x64 soak test: PENDING. No known evidence yet that dev.9
  prevents the 2 FPS condition or resolves any additional BLE interaction.

## v0.3.0-dev.10 — LUT experiment qualification

- Base: dev.9 phase-bounding candidate, with real ESP32-S3 checks at 16s,
  207s and 421s reporting ~50.4 ms/renderer and 16 FPS.
- 1024-entry read-only float sine table shared with cosine; default backend
  is interpolation (`GPTFX_GPT24_TRIG_BACKEND=2`).
- C++11 Arduino/API macro compatibility compilation: PASS, backends 0/1/2.
- 120 frozen dev.9 framebuffer comparisons per backend: PASS.
  - libm: zero pixel differences.
  - nearest: max 3/255 per channel, 254/4096 changed pixels.
  - interpolated: max 1/255 per channel, 2/4096 changed pixels.
- Numeric LUT sweep: PASS, nearest error <=0.00306758, linear <=0.00000745.
- Full 30-effect host/palette/audio tests: PASS (default mode).
- Host AddressSanitizer and UndefinedBehaviorSanitizer: PASS (default mode).
- Host A/B benchmark: approximately 1.5x faster for linear LUT, **host only**.
- Full PlatformIO WLED/ESP32-S3 compilation: NOT RUN.
- Real ESP32-S3 LUT performance and visual smoke: PENDING.


## dev.11 additional verification

- 84 frozen dev.10 framebuffer comparisons **per effect** for GPT07, GPT08,
  GPT18, GPT20 at 64×64, with independent backend switches: pass.
- 9 additional comparisons per effect at 128×64 and at 128×128: pass.
- `extraPerformance` JSON, skipped/rendered accounting, 32-frame window: pass.
- Backend controls for GPT24b 0/1/2, GPT08/GPT20 0/2, GPT18 0/2: pass.
- Complete host functional suite (30 effects): pass.
- ASan + UBSan host suite: pass.
- Real-device WLED firmware: not built or tested here.

See `PERFORMANCE_EXTENSION_DEV11.md` and `PERFORMANCE_HOST_DEV11.csv`.


## dev.12 — GPT18 phase-bounding qualification (host only)

- Source base: dev.11. Only GPT18 phase computations and its diagnostics
  are modified, plus source/document version and new test files.
- Physical dev.11 comparison: at 1048 s GPT18 averaged 44,725 us across its
  recent 32 completed renders; at 1806 s it averaged 190,948 us.
- `tests/test_labyrinth_phase.cpp` compares the candidate against the frozen
  dev.10 GPT18 reference (same math as the dev.11 default). 324 frames per
  compile variant (three resolutions x nine uptimes x four speeds x three
  palettes). Four variants test phase reduction off/on and native/LUT.
- With phase reduction disabled and native libm: pixel-identical. With phase
  reduction enabled/native libm, at 64x64 maximum changed pixels 73/4096,
  high-channel-difference pixels at most 4 per frame, and maximum average
  channel difference 0.04175/255 across the sampled frames.
- The visual tests demonstrate bounded error on the sampled inputs, **not**
  an exact mathematical/palette equivalence for all possible configurations.
- Full host suite `sh tests/run_host_tests.sh`: **PASS**.
- Existing `run_trig_backend_tests.sh`: all six legacy backend variants passed
  (GPT24b 0/1/2, extra 0/2, GPT18 opt-in LUT=2); the final GPT18 variant
  was executed separately after the main runner reached the tool time limit.
- New `run_labyrinth_phase_tests.sh`: **PASS** in all 4 phase/backend combinations,
  including C++11 compile-compatibility and 1,296 total visual comparisons.
- Host AddressSanitizer + UndefinedBehaviorSanitizer: **PASS** on the full suite.
- Host benchmark: `PERFORMANCE_HOST_DEV12.csv` (x86-64 only), NOT ESP32 timing.
- Full WLED PlatformIO compilation with maintainer's custom environment: NOT RUN.
- ESP32-S3 dev.12 hardware qualification: PENDING.

See `PERFORMANCE_INVESTIGATION_DEV12.md`.


## v0.3.0-dev.13 — Stage-1 phase bounding test record

- Base: frozen dev.12 source with runtime changes limited to GPT05, GPT11, GPT13, GPT19, GPT23b and GPTA05.
- `tests/run_stage1_phase_tests.sh`: PASS at `GPTFX_STAGE1_PHASE_BOUNDING=0` and `=1`, including C++11 compatibility checks. For each mode: 636 reference-frame scenarios across 64x64/128x64/128x128; also 180 audio-connected/disconnected comparisons. Disabled mode is pixel-exact against frozen dev.12.
- Visual quality gates: bounded-mode maximum per-frame mean channel delta <=0.04370/255 for all six effects; GPT11/GPT13 each have at most 1 >16/255 outlier per frame, with no cluster larger than one pixel. See CSV and remediation report.
- `tests/run_host_tests.sh`: PASS, all 30 effects, nine palette states, controls, audio response, geometry and scheduling.
- `tests/run_labyrinth_phase_tests.sh`: PASS, all four GPT18 combinations across 64x64/128x64/128x128; no GPT18 renderer modifications.
- `tests/run_sanitizers.sh`: PASS, host AddressSanitizer and UndefinedBehaviorSanitizer.
- `tests/benchmark_stage1_host.cpp`: PASS; experimental x86/host profiling only, not ESP32-S3 hardware speedup evidence.
- Full WLED/PlatformIO custom firmware build and ESP32-S3 runtime tests: NOT PERFORMED.

For conditions, limitations and full metrics see `PERFORMANCE_REMEDIATION_DEV13.md`.


## dev.14 Stage-2A addendum (host only)

- `tests/run_host_tests.sh`: PASS 30 effects, baseline palettes, audio, geometry, registration and device mocks.
- `tests/run_stage2_phase_tests.sh`: PASS. 636 scenarios for `GPTFX_STAGE2_PHASE_BOUNDING=0`, pixel-exact dev.13; 636 for =1, strict visual envelope; six effects at 64x64/128x64/128x128 and 0–7200 simulated seconds.
- `tests/run_stage2_long_tests.sh`: PASS diagnostic thresholds at 4–48h, 900 scenarios per flag; **floating-point precision warnings** must be reviewed. Passing diagnostic tolerances does NOT qualify multiday graphics.
- `tests/verify_dev13_unchanged.py <dev13-zip>`: PASS 24 unchanged renderer bodies + protected GPT10 hash.
- `tests/run_stage1_phase_tests.sh`, `tests/run_labyrinth_phase_tests.sh`, `tests/run_sanitizers.sh`: PASS.
- GPT24b trig backends 0/1/2, default/opt-in GPT18 LUT, extra backend defaults: host suite variants PASS. Full chained backend shell suite was time-limited after most variants; individual remaining backends verified.
- `tests/run_stage2_host_benchmark.sh`: PASS; 48 comparative x86 median timings in `PERFORMANCE_STAGE2_HOST_DEV14.csv`.
- Integrated custom WLED PlatformIO compilation: **NOT PERFORMED**. Waveshare on-device dev.14: **NOT TESTED**.

See `PERFORMANCE_REMEDIATION_DEV14.md` for numeric visual differences, 24/48h limitation and recommended hardware test.

## v0.3.0-dev.14a — GPT10 long-uptime correction

- PASS: GPT10 dev.14 comparison, 204 scenarios x four gate configurations
  (816 comparisons); hash-only and fully disabled variants pixel-exact to
  dev.14 at all tested uptimes 0–48h.
- PASS: two-stage GPT10 default correction at strict 0–2h image thresholds.
  Beyond 2h, deviations in float32 phase numerics are documented and not
  considered multiday qualification.
- PASS: dev.13-vs-stage-2A host suite, 636 scenarios per stage-2 mode,
  including GPT10 at up to 2h.
- PASS: 30-effect general host suite, Stage-1, GPT18, and targeted GPT10
  AddressSanitizer/UndefinedBehaviorSanitizer tests.
- PASS: unchanged renderer-body audit, 30/30 untouched bodies vs dev.14.
- **KNOWN DIAGNOSTIC FAILURE**: existing dev.13-vs-stage-2A long suite has
  a GPT10 >48h outlier over the former threshold (mean 1.575/255 vs dev.13);
  this test is not used to claim multiday visual fidelity. dev.14a-vs-dev.14
  targeted long-uptime test passes its explicitly labeled diagnostic envelope.
- Host-only benchmark: at 64x64, 21–26% lower host renderer wall time in
  checked configurations; not a prediction of ESP32-S3 FPS.
- **PENDING**: full WLED PlatformIO compile, actual ESP32-S3 / HUB75
  performance/visual qualification at >2h; do not promote this dev yet.

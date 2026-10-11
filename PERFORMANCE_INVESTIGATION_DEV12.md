# GPT Matrix FX v0.3.0-dev.12 — GPT18 temporal-phase bounding

## Scope and qualification status

Experimental derivative of **v0.3.0-dev.11**. Only **GPT18 — Vitreous Labyrinth** changes its rendering math. All 30 registration positions, effect labels, slider defaults, palette and hue drift, tick threshold (`tick(35)`), GPT24b LUT/phase logic, GPT07/GPT08/GPT20 optimizations, AudioReactive paths and WLED integration remain unchanged. Public v0.2.2 remains untouched.

**This is a software/host-tested hardware candidate, not a qualified firmware release.** No complete PlatformIO/WLED ESP32-S3 build or physical test of dev.12 was possible in this environment.

## Real-device evidence from dev.11

Source: maintainer's Waveshare ESP32-S3 HUB75 64x64 WLED 17.0.0-devV5 JSON measurements, with iDotMatrix and Buzzer omitted, Spotify usermod installed but disabled:

| Uptime | WLED `leds.fps` | GPT18 `extraPerformance.recentAvgUs` | Rendered | Skipped | Free internal heap |
|---:|---:|---:|---:|---:|---:|
| 1048 seconds | 18 | 44,725 us | 81 | 0 | 152,676 B |
| 1806 seconds | 4 | 190,948 us | 7,795 | 0 | 152,676 B |

The effect slowed by **4.27x** across 758 seconds while heap remained unchanged. These are actual measurements for dev.11, not predictions for dev.12. `leds.fps` is WLED's reported frame rate; render timings are **wall-clock elapsed** measurements that also include any scheduler preemption.

The dev.11 GPT18 implementation calculates four `sinf()` values *per pixel*, once per nested corridor level:

```cpp
float wave = sinf(xx*f + yy*(1.7f+.3f*k) + t*(.4f+.1f*k));
```

`t = timePhase(.27f)` grows with WLED's millisecond clock. The different layer coefficients make the temporal arguments grow at 0.4 to 0.7 times `t`. A separate set of four fold-offset `sinf()` calls is already hoisted out of the pixel loops in dev.11, but that does not fix the costly 4 x width x height **spatial sine evaluations**.

The maintainer's ESP32-S3 startup microbenchmark recorded `nativeSmallNs=1166` and `nativeLargeNs=12206` per trigonometric function call in the same firmware family. Multiplying the large-input value by 4 × 4,096 evaluations gives about **200 ms**, very close to the observed 191 ms frame time. This is a strong mechanistic consistency check, **not** isolated CPU-cycle profiling or proof that all ~191 ms are consumed inside `sinf()`.

## Implementation

- Add compile-time switch `GPTFX_GPT18_PHASE_BOUNDING`, default `1` (enabled). Setting it to `0` restores the exact dev.11 GPT18 pixel math for controlled A/B builds.
- Hoist *both* phase components for each of GPT18's four layers: `waveRaw = t * (.4f+.1f*k)` and `foldRaw = t*.38f+k`.
- Reduce the time-only parts **once per layer, per frame**, using `fmodf(raw, 2*pi)` normalized to `[-pi, pi]`. Eight `fmodf()` calls per rendered frame, **zero inside the pixel loop**.
- Preserve pixel-space `xx`, `yy`, geometry, independent oscillator frequencies and the unbounded **palette drift** `t*7.0f`. Do not modulo the whole time variable or the entire per-pixel sine argument.
- Native `sinf()` remains the default for GPT18 (`GPTFX_GPT18_TRIG_BACKEND=0`); LUT interpolation (`=2`) stays opt-in, independent of temporal bounding.
- No heap allocation, additional frame buffer, task, interrupt or FreeRTOS change.

With the new bounded temporal term, the per-pixel angle stays close to the spatial domain rather than growing to hundreds of radians. Floating-point rounding can produce small differences compared with evaluating an unreduced large argument; strict pixel identity is **not** claimed in the default mode.

## Device diagnostics

Existing `/json/info.u.gptMatrixFX.extraPerformance` now also contains:

- `phaseRawAbs`: last GPT18 frame's largest absolute raw **temporal** input in radians (four wave and four fold phases).
- `phaseWrappedAbs`: largest absolute bounded **temporal** input, expected `<= pi ~= 3.141593` when bounding is enabled.

Both diagnostic values refer to **temporal components only**, not pixel-dependent angles, and are meaningful when `extraPerformance.effect` is `GPT18` and `rendered > 0`. They reset when another profiled effect becomes active. The existing `trigonometry` object gains `labyrinthPhaseBounding` (boolean). All prior JSON keys remain.

## Compile-time A/B controls

Append the chosen definitions to the existing PlatformIO `build_flags` without replacing WLED's inherited flags:

```ini
-DGPTFX_GPT18_PHASE_BOUNDING=1
-DGPTFX_GPT18_TRIG_BACKEND=0
```

These are the **defaults** and may be omitted. For the old dev.11 behavior, compile `-DGPTFX_GPT18_PHASE_BOUNDING=0` with `-DGPTFX_GPT18_TRIG_BACKEND=0`. Test `-DGPTFX_GPT18_TRIG_BACKEND=2` **only separately**, after establishing phase-bounding stability on the ESP32-S3.

## Host verification

1. Full 30-effect host suite, including palettes, FFT and matrix resolutions: `sh tests/run_host_tests.sh`.
2. All legacy backend variants: `sh tests/run_trig_backend_tests.sh`.
3. Four GPT18 combinations (bounding 0/1 × trig libm/LUT 0/2) with C++11 compatibility compilation: `sh tests/run_labyrinth_phase_tests.sh`.
4. Host AddressSanitizer and UndefinedBehaviorSanitizer: `sh tests/run_sanitizers.sh`.

The dedicated test compares with a **frozen dev.10 GPT18 renderer**, pixel-identical to the dev.11 default native implementation, at **three matrix geometries (64x64, 128x64, 128x128), four speeds, three palettes, and nine uptime values from 0 through 7200 s**: 324 frames per backend combination. With bounding=0/libm, comparison is pixel-perfect. With bounding enabled, isolated palette-edge pixels can differ noticeably, but across these cases the worst average channel difference remains below 0.05 levels/255 (64x64), and high-color-difference pixels are sparse (four at most for 64x64). Detailed results are printed by the test runner. Host measurements do **not** validate target timing.

The optional host benchmark `PERFORMANCE_HOST_DEV12.csv` is an interleaved dev.11-reference/dev.12 comparison. It cannot reproduce the ESP32-S3 library's severe long-angle slowdown and is only a functional performance sanity check.

## Suggested physical qualification

Keep the current firmware configuration **without iDotMatrix and Buzzer** to isolate the cause, and keep GPT18 selected at the same sliders/palette on 64x64. Take `/json/info` after approximately 20 s, 5 min, **20 min**, and 30 min **of device uptime**, with no reboot or effect change. In addition to `leds.fps`, compare `extraPerformance.recentAvgUs`, `renderLastUs`, `renderMaxUs`, `skipped`, `phaseRawAbs`, `phaseWrappedAbs`, and stable heap/PSRAM. The rolling 32 renders need enough time to refill at slow FPS. Time since boot matters because `strip.now` is WLED's device clock; note if the effect is selected substantially later.

A strong outcome would be `recentAvgUs` remaining near the early ~45 ms at 1806 seconds rather than rising to 191 ms, with `phaseRawAbs` increasing but `phaseWrappedAbs <= pi`. Do **not** claim this result until confirmed by the maintainer's firmware measurements. Then test with iDotMatrix/Buzzer restored and BLE connected, separately from the GPT18 math qualification.

## Related future audit

Other effects contain sine/cosine or `ridge()` arguments with time-varying terms in per-pixel loops (for example GPT01, GPT08, GPT16, GPT19, GPT23b). Similar long-uptime checks may be warranted, but dev.12 makes **no behavioral changes to those effects**. Some use independent internal bounds/LUTs; do not globally replace trig or alter timePhase until specific evidence demonstrates a problem.

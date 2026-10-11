# GPT Matrix FX v0.3.0-dev.11 — Selective multi-effect optimization

## Status and scope

Experimental continuation of the **physically measured dev.10** build. This is **not** a public release or a merge candidate until ESP32-S3 qualification. Public **v0.2.2 remains unchanged**. No WLED core modification, FreeRTOS priority change, dynamic buffer, or new trigonometric table is introduced. The 30 effect names and registration order are retained.

## Previous physical-device baseline (Waveshare ESP32-S3 64×64)

The maintainer's physical `/json/info` for **dev.10**, without iDotMatrix and Buzzer, reported:

| Uptime | WLED FPS | GPT24b `recentAvgUs` | `phaseRawAbs` | `phaseWrappedAbs` |
|---|---:|---:|---:|---:|
| 20 s | 22 | 34,933 | 32.46 | 3.56 |
| 754 s | 23 | 34,789 | 1,245.29 | 3.95 |

ESP32-S3 setup microbenchmark, in **elapsed** nanoseconds per call: `nativeSmallNs=1167`, `nativeLargeNs=12206`, `nearestNs=543`, `linearNs=578`. These numbers are the maintainer's hardware telemetry, not measurements performed by dev.11. No dev.11 physical performance claim is made here.

## Changes from dev.10

Four specific effects are changed and profiled:

- **GPT07 — Impossible Lattice:** calculate the two rotation coefficients for each of 2–5 folding levels **once per frame**, instead of once per pixel per level. Calls remain `sinf/cosf`, precisely the same inputs and arithmetic order. Host framebuffer comparison against frozen dev.10 is byte-identical.
- **GPT08 — Aether Topography:** use the **existing shared 1024-entry interpolated LUT** for the four `sinf` and two `cosf` calls per pixel (six evaluations × 4096 pixels at 64×64). Keep all other math and GUI controls unchanged.
- **GPT18 — Vitreous Labyrinth:** precompute four time-only sines per frame, rather than evaluating them on every pixel. Keep the four per-layer spatial sines **native by default**, as initial host measurements did not prove that the interpolated LUT was consistently faster for this effect. Optional independent LUT experimentation is supported.
- **GPT20 — Zenith Quasicrystal:** precompute the five sine/cosine axis pairs and five time offsets once per frame, rather than redundantly every pixel; use the existing interpolated LUT for five pixel-local cosine evaluations. The final `ridge` and color path remain native.

**GPT24b Kinetic Rosettes** is completely unchanged from dev.10, including phase bounding, LUT backend and telemetries. The other 25 effects also retain their original implementations. The only shared behavioral change is an additional simple `tick()` bookkeeping branch when the four profiled effects are selected.

## Compile-time backend controls

Append optional preprocessor definitions to the existing PlatformIO `build_flags` without deleting current flags:

```ini
-DGPTFX_GPT24_TRIG_BACKEND=2  ; existing rosette LUT, unchanged
-DGPTFX_EXTRA_TRIG_BACKEND=2  ; GPT08/GPT20: interpolated LUT (dev.11 default)
-DGPTFX_GPT18_TRIG_BACKEND=0  ; GPT18: native spatial sin (dev.11 default)
```

The values `0` (native libm) and `2` (LUT with linear interpolation) are supported for `GPTFX_EXTRA_TRIG_BACKEND` and `GPTFX_GPT18_TRIG_BACKEND`. They are independently selectable. GPT07 remains precomputed and does not need a backend flag. Do not paste the inline comments into real `build_flags` if the target parser is sensitive; use plain `-D...` flag lines.

**No second 4 KiB LUT is allocated.** The existing `gptfx_trig_lut.h` table is shared. These paths make no heap allocation.

## Telemetry

`/json/info.u.gptMatrixFX.trigonometry` retains the dev.10 benchmark and gains:

- `extraBackend`: selected GPT08/GPT20 backend (default `lut1024-linear`).
- `extraEffects`: `GPT08,GPT20` (the two effects with default extra LUT).
- `labyrinthBackend`: selected GPT18 backend (default `libm`).

New `/json/info.u.gptMatrixFX.extraPerformance` reports **the latest of GPT07, GPT08, GPT18 or GPT20 selected**:

- `effect` (e.g. `GPT08`), `calls`, `rendered`, `skipped`.
- `renderLastUs`, `renderMaxUs`, `recentCount`, `recentAvgUs`, `recentMaxUs` (a fixed 32-render rolling window).
- `width`, `height`.

Statistics reset on effect transition between the four observed effects, or when a fresh invocation (`SEGENV.call == 0`) starts. The measurements cover the effect entrypoint and are *elapsed wall time*, not isolated CPU time. They can include FreeRTOS preemption; WLED `leds.fps` also includes other costs. A different uninstrumented effect does not update `extraPerformance` until one of the four measured effects is entered again.

## Host visual regression versus frozen dev.10

There are **84 frame comparisons at 64×64** per effect (7 uptimes from 0–7200 s × 4 speeds × 3 palettes). The results below apply to the **dev.11 default backends**; they do not imply perfect visual equivalence:

| Effect | Max channel difference (0–255) | Max changed pixels / 4096 | Max pixels with >16 channel difference | Max mean absolute channel difference |
|---|---:|---:|---:|---:|
| GPT07 | 0 | 0 | 0 | 0 |
| GPT08 | 162 | 179 | 3 | 0.0464 |
| GPT18 | 0 | 0 | 0 | 0 |
| GPT20 | 216 | 30 | 1 | 0.0183 |

**Caveat:** Individual large differences occur near quantized palette-index boundaries. The small average differences must **not** be misrepresented as a strict worst-case 1/255 guarantee. Physical visual inspection is required before promotion.

A second set includes **nine frame comparisons at each of 128×64 and 128×128** per effect (3 uptimes × 3 speeds, palette 6). At 128×128, changed pixels were at most GPT08=174, GPT20=46, GPT07=GPT18=0 (out of 16,384). This is framebuffer fidelity and *not* a measured higher-resolution hardware FPS result.

All other pre-existing regression checks for all 30 effects remain enabled.

## Host performance — not ESP32-S3 FPS

Median duration per 64×64 effect frame, captured on x86_64 using `g++ -O3` with interleaved reference and candidate runs. Representative results from the **dev.11 default backends**; reference is dev.10, candidate dev.11, timings vary with host CPU scheduling and cache:

| Effect | dev.10 at 14 s | dev.11 at 14 s | dev.10 at 900 s | dev.11 at 900 s |
|---|---:|---:|---:|---:|
| GPT07 | 419 µs | 258 µs | 421 µs | 258 µs |
| GPT08 | 233 µs | 220 µs | 241 µs | 220 µs |
| GPT18 | 202 µs | 200 µs | 212 µs | 212 µs |
| GPT20 | 202 µs | 188 µs | 229 µs | 189 µs |

The full original host benchmark CSV is distributed as `PERFORMANCE_HOST_DEV11.csv`. This is a development profiler, not a prediction of ESP32-S3 cost. In particular, GPT18 is still effectively unchanged in host speed.

## Validation completed

- Host functional tests, palette/geometry/audio regressions for all 30 effects: **PASS**.
- Frozen dev.10 comparisons: **PASS**, default and libm-switchable modes.
- Host comparisons on 64×64, 128×64, 128×128: **PASS**.
- Independent GPT24b backends 0/1/2: **PASS**, unchanged dev.10 test cases.
- Extra backends 0/2, including GPT18 opt-in mode 2: **PASS**.
- Arduino-compatible C++11 compile-only tests: **PASS**.
- AddressSanitizer and UndefinedBehaviorSanitizer host run: **PASS**.
- **Full WLED PlatformIO build: NOT RUN** (the matching WLED tree/toolchain is not available in this work environment).
- **Physical ESP32-S3/FPS/visual qualification for dev.11: NOT RUN**, pending maintainer test.

## Recommended hardware qualification

1. Compile dev.11 with default flags on the same WLED 17.0.0-devV5 and hardware configuration as dev.10. Confirm `/json/info` reports `build=dev.11`, `trigonometry.extraBackend=lut1024-linear`, `trigonometry.labyrinthBackend=libm`, and no change to GPT24b `trigonometry.backend`.
2. Select **GPT07**, wait at least 32 rendered frames, record `leds.fps` and `u.gptMatrixFX.extraPerformance`. Observe animation geometry. Repeat for **GPT08**, **GPT18**, **GPT20** with the same speed/palette as previous builds.
3. For GPT08 and GPT20, compare against an otherwise identical build with `-DGPTFX_EXTRA_TRIG_BACKEND=0` if a definitive on-device A/B speed measurement is needed; validate that visual changes remain acceptable.
4. If desired, test GPT18 experimentally with `-DGPTFX_GPT18_TRIG_BACKEND=2`, **only if native GPT18 performance justifies the tradeoff**.
5. Recheck GPT24b after ~10 minutes to ensure dev.10 phase bounding/LUT remains untouched.
6. Repeat with iDotMatrix and Buzzer restored **after** the no-BLE baseline is qualified. Beware that BLE can affect wall-clock rendering time without changing the computation itself.

No code should be merged into `develop` until the device and visual test results have been reviewed.

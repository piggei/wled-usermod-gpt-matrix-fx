# GPT Matrix FX 0.3.0-dev.10 — Experimental trigonometry LUT

## Purpose and release status

Develop an **opt-in/reversible, targeted trigonometry experiment** based on the
hardware-qualified direction of 0.3.0-dev.9. The public v0.2.2 release is
unchanged. This package remains an experimental hardware-test candidate, not a
public stable release. No WLED core files are changed.

## Physical hardware evidence prior to dev.10

The maintainer's Waveshare ESP32-S3 64x64 HUB75, WLED 17.0.0-devV5,
without iDotMatrix or Buzzer, has reported these real values for GPT24b:

| Build | Uptime | FPS | `recentAvgUs` | `phaseRawAbs` | `phaseWrappedAbs` |
|---|---:|---:|---:|---:|---:|
| dev.8 | 14 s | 16 | 50,479 | Not available | Not available |
| dev.8 | 697 s | 2 | 345,919 | Not available | Not available |
| dev.9 | 16 s | 16 | 50,395 | 26.62 | 4.71 |
| dev.9 | 207 s | 16 | 50,367 | 342.55 | 6.12 |
| dev.9 | 421 s | 16 | 50,368 | 694.73 | 4.78 |

These are device-supplied observations, not device timings measured by this
package. The 421-second dev.9 measurement strongly supports phase bounding as
a correction for the progressive slowdown. Twelve to fifteen minutes of
continuous hardware operation and BLE connection stress remain outstanding.

## Implementation

`gptfx_trig_lut.h` defines **1024 float samples**, a total of **4096 bytes** of
static read-only flash data (`PROGMEM`); no RAM table and no runtime allocation.
Both sine and cosine use this same table; cosine adds a 256-sample quarter turn.
The table is generated deterministically by `generate_lut.py` (offline tool).

For input phase x, compute the scaled table index; reduce its signed integer
part modulo 1024 with a bit mask. This avoids calling `fmodf` for every pixel.
The dev.9 *time-only* phase reductions remain unchanged and occur five times
per rendered frame, never per pixel.

The code provides three **compile-time** backends, affecting **GPT24b only**:

| `GPTFX_GPT24_TRIG_BACKEND` | Name | Policy |
|---:|---|---|
| 0 | `libm` | Original dev.9 `sinf/cosf` baseline |
| 1 | `lut1024-nearest` | Nearest table sample; evaluation only, lower fidelity |
| 2 (default) | `lut1024-linear` | Two neighboring samples and linear interpolation |

The two rosette layers and their ridge/spark path use the selected backend.
Other effects use the original math functions, unchanged. Size, speed, petals,
brightness, color and `tick(40)` remain unchanged. A fallback to libm is used
for exceptional out-of-range inputs, avoiding invalid float-to-integer casts.
This code does not alter FreeRTOS task priorities, BLE or interrupt handling.

To select the original implementation, **append** this preprocessor definition
in the *existing* PlatformIO environment's `build_flags` (without removing its
other settings):

```ini
-DGPTFX_GPT24_TRIG_BACKEND=0
```

For the candidate interpolated LUT, use `-DGPTFX_GPT24_TRIG_BACKEND=2` or omit
the flag. No firmware runtime configuration is required. For a controlled A/B,
compile and flash the baseline and candidate against the *same* WLED source,
user-modules and effect settings.

## Benchmark telemetry

`/json/info.u.gptMatrixFX.trigonometry` provides:

- `effect`, `backend`, `tableSamples`, `tableBytes`
- `nativeSmallNs`: mean elapsed ns per native sine/cosine call on a fixed,
  bounded input sequence (~-61..+61 radians).
- `nativeLargeNs`: mean elapsed ns per native call on a positive sequence
  ~160..1455 radians, illustrating any slow large-input path.
- `nearestNs`: mean elapsed ns per nearest lookup on the bounded sequence.
- `linearNs`: mean elapsed ns per interpolated lookup on the bounded sequence.

Each figure comes from a **one-shot, 2048-operation microbenchmark** for that
backend during usermod setup. The bench is not repeated inside the renderer,
and has no per-pixel instrumentation or dynamic allocation. It adds a short,
one-time boot workload. Timings include measurement overhead, cache/memory
conditions and potential RTOS preemption; a cold-cache microbenchmark is not
an exclusive CPU-cycle count. For authoritative performance, compare the
existing GPT24b `recentAvgUs`, and `leds.fps`, at constant palette/sliders
and on the same full firmware configuration.

## Validation and numerical error

- Numeric sine/cosine sweep across +/-80 radians at 0.005-radian steps:
  - Nearest maximum observed absolute error: 0.00306758.
  - Linear maximum observed absolute error: 0.00000745.
- Frozen dev.9 framebuffer comparison: 120 cases (10 uptimes, 4 speeds,
  3 palettes, 64x64):
  - libm mode 0: byte-for-byte identical, zero changed pixels.
  - nearest mode 1: maximum channel delta 3/255; up to 254/4096 pixels changed.
  - linear mode 2: maximum channel delta 1/255; up to 2/4096 pixels changed.
- The original dev.7 vs dev.8 byte-identical checks and dev.8 vs dev.9
  long-uptime checks remain in the test suite.
- All 30 effects, color/palette and audio regressions: PASS on host.
- C++11/Arduino-macro compile-only tests: PASS for modes 0, 1 and 2.
- AddressSanitizer + UndefinedBehaviorSanitizer host suite: PASS (default 2).

### Host benchmark (NOT ESP32-S3 performance)

Median frame render microseconds, interleaved A/B, `-O3 -march=native` on the
available x86 host; this is only evidence that the implementation can be
faster on *that* architecture. Native runs fluctuate between builds due to
CPU cache, thermal/power state and scheduling.

| Host backend | dev.9 reference 14s | selected 14s | dev.9 reference 900s | selected 900s |
|---|---:|---:|---:|---:|
| Native libm | 312 | 314 | 312 | 313 |
| LUT nearest | 300 | 188 | 297 | 189 |
| LUT linear | 310 | 201 | 302 | 200 |

The linear-LUT build is approximately 1.5x faster than dev.9 in these host
conditions, with negligible frame differences. **No actual ESP32-S3 speedup is
yet confirmed; do not claim 1.5x on the Waveshare.**

## Follow-up qualification

1. Build 0.3.0-dev.10 with default backend 2. Verify `build=dev.10` and
   `trigonometry.backend=lut1024-linear` in `/json/info`.
2. Select GPT24b, with identical controls and palette to the dev.9
   measurement. At 15–30 seconds collect `/json/info`.
3. Without reboot or changing effects, collect at 5 minutes and 12–15 minutes.
4. Record nativeSmallNs, nativeLargeNs, nearestNs, linearNs, `leds.fps`,
   `recentAvgUs`, `recentMaxUs`, `phaseRawAbs`, `phaseWrappedAbs`.
5. Compare to the previous dev.9 baseline ~50.4 ms and 16 FPS in the
   no-iDotMatrix/no-Buzzer configuration. If improvement is insufficient or
   image fidelity is unacceptable, flash a backend-0 build for a direct A/B.
6. Only after stability is demonstrated, repeat with other usermods and BLE.

The 29 other effects are frozen for this experiment. Other opportunities
identified in the dev.8 static audit, including GPT07/GPT17/GPT18/GPT20,
remain for a future release with one-effect-at-a-time validation.

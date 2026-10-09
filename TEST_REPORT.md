# GPT Matrix FX v0.2.2 — Test and validation report

**Date:** 2026-10-09  
**Scope:** moving, physically meshed planetary mechanism for GPT06; all 25 effect, palette, and AudioReactive regressions retained.

## Test matrix

| Item | Coverage | Result |
| --- | --- | --- |
| Effect registration | 20 regular + 5 audio, dynamic ID=255, exact zero-padded names and 2D/audio metadata | PASS |
| Host C++ compilation | Actual effect source under C++11 with the Arduino `sq(x)` macro in scope | PASS |
| Frame rendering | All 25 algorithms on 8×8, 16×32, 64×64, 96×48, and 128×128 | PASS |
| Frame evolution | Six subsequent frames per effect; checks for an animated image at resolutions >=64 wide | PASS |
| Live palette selection | Nine successive palette selections on the same active segment, covering 25 effects (225 transitions), without resetting the effect | PASS |
| Default/black primary color | Palette 0 with an explicitly black primary color retains visible procedural color | PASS |
| All-black palette | Palette 2 with black primary and a simulated fully black palette safely fall back to a rainbow | PASS |
| Dark-entry preservation | A multicolored palette with one black region keeps that region black | PASS |
| Baseline v0.2.0 reproduction | Same palette test on v0.2.0 fails: GPT01, palette 0, single-color black frame | CONFIRMED |
| Audio differential | Each of five audio modes tested with all FFT bands at zero and at 255, held at fixed time | PASS |
| No audio | AudioReactive missing or disabled | PASS |
| Incomplete audio payload | Usermod present but payload too small | PASS |
| Nonfinite audio | NaN smoothed volume | PASS |
| Nonmatrix fallback | 25 modes on 64×1, without per-mode allocation | PASS |
| GPT06 epicyclic geometry | Moving planet centers maintain pitch distance; both tooth contacts align throughout orbital motion | PASS |
| GPT06 motion bounds | Five speed settings × eleven timestamps, including near-32-bit-millis wrap, check viewport envelope and nonzero spatial travel | PASS |
| GPT06 visual motion | 64×64 frames six seconds apart show changed gear geometry | PASS |
| AddressSanitizer + UndefinedBehaviorSanitizer | Whole host suite | PASS |
| Real WLED firmware build | `pio run -e waveshare_spotify` in the customized checkout | NOT RUN HERE (private checkout unavailable) |
| Hardware timing | ESP32-S3 64×64 HUB75 FPS measurements | NOT RUN |

## GPT06 orbital-motion revision

Version 0.2.2 changes only GPT06 rendering and its metadata label from `Rotation` to `Spin + Orbit` (effect name and registration remain `GPT06 - Clockwork Cathedral`). The two 10-tooth satellites revolve around the central 14-tooth gear on opposite sides of a moving carrier. The center gear's original angular speed formula is retained; the satellite angular velocity includes carrier rotation so the pitch contact does not slip. A dual-frequency center translation and subtle scale breathing move the assembly around the 64×64 screen while conserving relative gear radii.

The new host assertions cover five Speed settings (`0`, `30`, `90`, `150`, `255`) and eleven uptime snapshots (`0` through `4294967000` ms). They check pitch-radius separation, per-frame screen bounds, and tooth-to-gap phase at both engagement points; a frame difference confirms visible motion. The double-precision exact 14/10 tooth ratio is important to prevent cumulative phase drift over multi-day uptime. These tests check geometric invariants, not contact forces or real hardware frame rates.

## Palette fix

WLED interprets `color_from_palette(..., mcol=0, ...)` with palette 0 as the segment primary color. If that primary color is black, all 25 effects can go dark because they share one `chroma()` function. The corrected implementation specifies `mcol=255` (WLED party/default palette behavior) and requests moving palette interpolation. A lightweight full-palette sample detects only **completely black palettes** and switches to a local HSV-style fallback; valid palette black entries are preserved. This avoids changes to WLED core or segment configuration.

Repro before fix: run the new test suite with the original v0.2.0 `gpt_matrix_fx.cpp` and the updated tests. Expected failure: `Palette blackout: FX 1 pal=0 (pass 1), colors=1`. The v0.2.1 source passes all 225 in-place palette transitions. This is a host reproduction of WLED palette semantics, not a hardware measurement.

## FFT differential results

With the same frame timestamp, change all 16 FFT bands from `0` to `255`, volume from `0` to `220`, and sample peak from `0` to `1`:

| Mode | Changed color-channel bytes (64×64 frame) |
| --- | ---: |
| GPTA01 | 6,061 |
| GPTA02 | 11,276 |
| GPTA03 | 6,930 |
| GPTA04 | 11,008 |
| GPTA05 | 9,063 |

## Resource model

- GPT04 uses a maximum 64×64 two-page `SEGENV.allocateData` allocation (8192 bytes per active segment).
- The five audio modes each use a small per-segment AudioState (`sizeof(AudioState)`; 24 bytes in the host environment).
- Remaining normal modes do not allocate additional effect scratch memory.
- The procedural renderers use CPU-intensive math; smooth on-device frame pacing is **not guaranteed by host execution speed**.

## Reproduction

```sh
./tests/run_host_tests.sh
./tests/run_sanitizers.sh
```

## Limitations and acceptance gate

The source was compiled with a mock `wled.h`, *not* the user's private WLED fork. The palette mock reproduces WLED's documented `mcol` behavior and deterministic non-black palette samples; it does not implement the full hardware transition pipeline. Host tests cannot prove that the live WLED API, usermod registration order, ESP32-S3 toolchain, segment handling, or HUB75 driver will all work without issues. The next acceptance gate is `pio run -e waveshare_spotify` with this package, followed by a 64×64 hardware inspection of GPT06 motion at low, middle, and maximum Speed, plus palette changes (including Default and several fixed palettes). Verify AudioReactive behavior independently. Check that the expanded effect count fits within the configured firmware's available mode IDs.

The code and documentation are in English. The regular modes are renamed `GPT01` through `GPT20`; the former GPT9 and GPT10 are now GPTA01 and GPTA02 respectively. All 25 effect names and registration order remain stable; only GPT06 spatial mechanics have changed since v0.2.1. The palette fix introduced in v0.2.1 is retained.

## Public release assessment

The maintainer reports satisfactory results on the physical LED matrix. Host regression and sanitizer tests were rerun for the v0.2.2 release and passed. No new renderer code changes were introduced during packaging; the release-only changes are the `Zone.Identifier` ignore rule, English-language release documentation, and status wording. Hardware FPS, extended soak tests, and a locally reproduced build of the maintainer's private firmware remain unverified by the package author.

# GPT Matrix FX 0.3.0-dev.9 — GPT24b progressive slowdown investigation

## Scope and status

This is a narrow **experimental hardware-test candidate**. The v0.2.2 stable
release and the other 29 effects remain untouched. The diagnosis is supported
by real-device measurements, but the proposed solution has **not yet** been
confirmed on ESP32-S3 hardware.

## Real-device evidence from the maintainer

Hardware: Waveshare ESP32-S3 RGB Matrix 64x64 HUB75 with WLED 17.0.0-devV5
and GPT Matrix FX 0.3.0-dev.8. iDotMatrix and Buzzer were entirely excluded
from the second comparison firmware; Spotify Connect was present but disabled.

| Snapshot | Uptime | WLED FPS | GPT24b recent mean | Last |
|---|---:|---:|---:|---:|
| After reboot; no iDotMatrix or Buzzer | 14 s | 16 | 50,479 us | 50,577 us |
| Same effect, unchanged firmware | 697 s | 2 | 345,919 us | 346,682 us |

This experiment proves that BLE connectivity and the Buzzer task are not
necessary conditions for GPT24b's progressive slowdown. The elapsed time
measurement includes task preemption; it does not measure exclusive CPU time.
Heap and PSRAM were approximately stable, and telemetry reported zero
`tick(40)` drops in both snapshots.

## Technical hypothesis

The original `timePhase()` accumulates phase from `strip.now` in milliseconds.
GPT24b repeatedly evaluates `sinf` and `cosf` on phases such as
`a*22 +/- t*3` for 4096 pixels and two independent layers.

Some Newlib `sinf`/`cosf` implementations use a significantly more expensive
range-reduction path for large inputs (the 2018 fast-path implementation
handles angles up to approximately 120 radians using a single reduction).
This makes the increasing arguments a plausible cause of the regression.
The exact math implementation and cost on the installed ESP32 firmware have
not been independently profiled. This is a **hypothesis**, not yet a proven
root cause.

## dev.9 implementation

The five time-only phase components are calculated modulo 2*pi once per
rendered frame, before entering the 64x64 loop. The spatial angle remains
unchanged, and the phase terms then remain bounded regardless of uptime.

The time component in the secondary `sinf(phase*1.7 + t*0.4)` term must be
reduced at the combined rate: `t * (layerRate * 1.7 + 0.4)`. Wrapping the
base phase and multiplying by 1.7 would have changed the periodicity.

The spark term is reduced independently at `t*3`, retaining its opposite
layer signs. The palette index `a*35 + t*18` remains unchanged to preserve
its original movement. The `tick(40)` gate remains unchanged.

The JSON fields `u.gptMatrixFX.performance.phaseRawAbs` and
`phaseWrappedAbs` expose the latest temporal input peak magnitude before and
after phase wrapping, in radians; the latter must be less than 6.2832.

## Validation

- Frozen dev.7 vs frozen dev.8: byte-exact for 7 test cases, preserved.
- Frozen dev.8 vs dev.9: 48 combinations, 4 speeds x 12 uptimes (0-7200 s).
- At 64x64, maximum observed absolute channel difference: 1/255.
- Largest single-frame set of changed pixels: 67 out of 4096 (<1.7%).
- Color/geometry/timing-gate/palette/audio regression tests: PASS.
- AddressSanitizer and UndefinedBehaviorSanitizer: PASS.
- No full ESP32-S3 PlatformIO firmware build was possible in this environment.

Host x86-64 A/B median in microseconds (unrelated to ESP32 timing):

| Uptime | dev.8 | dev.9 |
|---|---:|---:|
| 14 s | 347.18 | 351.76 |
| 150 s | 368.81 | 360.23 |
| 300 s | 366.82 | 351.68 |
| 400 s | 373.85 | 352.71 |
| 700 s | 370.42 | 349.25 |
| 900 s | 369.64 | 349.36 |
| 1800 s | 369.23 | 348.75 |

This benchmark does not reproduce the ESP32's up-to-sevenfold slowdown,
which is compatible with a math library/architecture-dependent effect.

## Requested real-device qualification

Use the same WLED environment and 64x64 matrix. Keep the same GPT24b
Speed, Trace Glow, Petals, Size, and palette parameters between runs.

1. Reboot, select GPT24b, read `/json/info` after about 20 seconds.
2. Without changing anything, read again at 5 minutes.
3. Without rebooting, read again at 12-15 minutes.
4. Compare `leds.fps`, `recentAvgUs`, `recentMaxUs`, `rowMaxUs`,
   `phaseRawAbs`, and `phaseWrappedAbs`, and record `uptime`.
5. Only if stable, repeat with the full usermod set and then BLE connected
   and disconnected. Do not isolate BLE until the no-iDotMatrix/no-Buzzer
   baseline is established.

Desired qualification result: dev.9 rendering time stays near initial
50 ms on the real ESP32 after at least 12-15 min. Any significant slowdown
means the hypothesis is incomplete and needs deeper FreeRTOS/CPU timing
instrumentation; no architecture/interrupt changes should be made blindly.

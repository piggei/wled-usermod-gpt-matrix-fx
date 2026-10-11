# GPT Matrix FX v0.3.0-dev.14a — GPT10 Glass Circuit remediation

**Status: host-tested experimental source candidate, awaiting complete WLED PlatformIO build and real ESP32-S3 visual/performance qualification.** Public v0.2.2 and the validated dev.13 sources remain unchanged.

## Hardware trigger

The maintainer observed GPT10 at 64x64 on an ESP32-S3/Waveshare HUB75 using dev.14 with stage-2A phase bounding enabled:

| WLED uptime | WLED FPS | Last-32-frame average renderer | GPT10 frame count | Phase raw/wrapped |
|---|---:|---:|---:|---|
| 16 s | 12 | 70.752 ms | 79 | 35.467 / 2.232 rad |
| 137 s | 12 | 71.499 ms | 1,590 | 304.322 / 2.729 rad |
| 8,543 s | 6 | 136.103 ms | 66,569 | 18,949.780 / 0.312 rad |

The rendering time rose **92.4%** after 2 h 22 min, despite normal Stage-2A `phaseWrappedAbs` diagnostics. This points to time-growing arguments not included in the stage-2A phase telemetry. There were zero skipped frames and no evident memory exhaustion. The current build included iDotMatrix and Buzzer instead of the previous Spotify usermod; comparisons to the other build's absolute FPS therefore require caution.

## Root cause, specific to GPT10

1. The *cell hash* passes `ix*127.1f+iy*311.7f` to native `sinf()` **for every pixel**. Here `ix=floorf(gx)`, `iy=floorf(gy)`, `gx=p.x*scale+t*.11f`, `gy=p.y*scale-t*.06f`. Consequently, the hash angle grows with uptime even while `phaseWrappedAbs` stays small. Multiple pixels in a cell request the same hash value.
2. The packet ridge in dev.14 uses `(seed>.34f?gx:gy)*.65f*5.0f + bounded(-4.1f*t) + seed*8.0f`. Bounding only `-4.1*t` leaves the time-dependent grid translation in the selected spatial coordinate. Its `ridge()` sine angle still grows with uptime.

These are code-backed causes of **unbounded sine arguments**, not an experimentally isolated attribution of the entire 65 ms late-uptime regression. The physical split of hash/ridge costs must still be tested on ESP32-S3.

## Isolated changes to GPT10 only

### Flag A: exact cell hash cache (default 1)

`GPTFX_GPT10_CELL_CACHE=1` adds a **64-slot direct-mapped cell cache local to one effect frame**, keyed by the integer-valued `floorf(gx),floorf(gy)` coordinates. Cache misses evaluate the **identical** `frac(sinf(ix*127.1f+iy*311.7f)*43758.5453f)` expression from dev.14; cache hits reuse that exact `float` result. Collision handling checks both coordinates before reuse. A cache collision recomputes the original expression; the cache does not approximate or periodic-wrap the spatial pseudo-random hash.

- Fixed stack only, about 1 KiB at typical alignment; **no heap or PSRAM allocation**.
- No persistence across frames, effect changes, or segments; no cross-segment sharing.
- Hash, grid translation, `floorf`, `frac`, palette, controls, output color selection and effect tick unchanged.
- At 64x64, speed 120, custom1 128, 8,543 s simulated uptime, native hash `sinf` calls decreased from **4,096 to 240 per frame** (94.14% avoided, 3,856 cache hits).
- Across **204 test setups from 0 to 48 h**, enabling the cache *alone* produces **byte-identical framebuffer output to dev.14**. Disabling both new flags also reproduces dev.14 exactly.

### Flag B: full periodic packet phase bound (default follows stage-2A)

`GPTFX_GPT10_PACKET_BOUNDING=1` combines the **entire periodic time contribution** of the selected trace-axis packet before calling `ridge()`. For the horizontal coordinate, the time component is `0.11*t*0.65*5 - 4.1*t`; for the vertical coordinate it is `-0.06*t*0.65*5 - 4.1*t`. These two phases are wrapped once per frame to `[-pi,+pi]` via `stage2BoundPhase()`, and are then combined with spatial pixel terms and the unchanged cell `seed*8`.

- Grid coordinates themselves stay unbounded; this is essential to preserve cell travel and hash selection.
- Native `ridge()` and `sinf()` remain in use: **no new LUT**, no palette/geometry/brightness/user-setting change by design.
- Rearranging floating-point operations can introduce small pixel/phase differences. The corrected output is **not pixel-identical** to dev.14 when this flag is on.
- When `GPTFX_STAGE2_PHASE_BOUNDING=0`, packet bounding defaults to 0 as well, preserving the older *dev.13* compile-time comparison. A separately explicit `-DGPTFX_GPT10_PACKET_BOUNDING=1` overrides that default if desired.

Flags can be combined independently for controlled A/B diagnostics. To revert to **exact dev.14 GPT10** while retaining all other dev.14 functionality, compile with both new flags set to 0. To keep the exact dev.14 image with hash caching only, compile with cache=1 and packet=0.

## Runtime telemetry

`/json/info.u.gptMatrixFX.build = "dev.14a"` and `.trigonometry.gpt10CellCache`, `.gpt10PacketBounding` describe the compile-time configuration. Existing `.extraPerformance` remains unchanged and measures the effect's last 32 frame times. New `.gpt10Diagnostics` describes the **last rendered GPT10 frame**:

| Field | Meaning |
|---|---|
| `hashRequests` | Number of pixel hash lookups (4,096 at 64x64) |
| `hashEvaluations` | Actual native cell hash `sinf()` calls |
| `hashCacheHits` | Lookups that reused an exactly matching cached cell |
| `hashArgAbs` | Greatest `abs()` hash argument among **evaluated** cells |
| `packetRawSampleAbs` | Greatest legacy packet sine argument sampled at 1/32 pixels |
| `packetBoundedSampleAbs` | Greatest corrected packet sine argument sampled at the same pixels |

The last two fields are **sample maxima**, not exhaustive maxima. The bounded packet sine's final full argument need not be in `[-pi,+pi]`: the spatial+seed contribution is preserved, so its magnitude must instead remain bounded **independently of uptime**. Likewise, the cell hash still receives large arguments on cache misses; the fix drastically reduces their number, but does not make them small.

These diagnostics do not measure the individual hash/ridge CPU-time contribution and must not be presented as such. The full frame `recentAvgUs` remains the principal hardware performance measurement.

## Host verification, 2026-10-11

- **GPT10 matrix**: 204 cases per configuration, **816 total** across cache/packet flag combinations `(0,0)`, `(1,0)`, `(0,1)`, `(1,1)`. 64x64, 128x64, 128x128; three palettes; multiple speed/control values; 0–172,800 s uptime. All four configurations **PASS**. Both packet-disabled variants are pixel-exact to dev.14 on all 204 cases each. Packet-enabled variants meet the established *strict 0–2 h* visual thresholds and show bounded whole-packet arguments across the tested up-times.
- **GPT10 long-uptime diagnostic**: packet-enabled maximum mean RGB-channel difference across the entire 48h test is **1.06584/255** versus frozen dev.14; this is a **diagnostic**, not a 48-hour visual qualification. Outliers remain near discontinuities at large uptimes.
- **Stage-2A legacy tests** (dev.13 vs candidate): strict 0–2 h PASS, both compile-time stage-2A configurations, 636 scenarios each. At 24–48h the *legacy dev.13-comparison* long suite flags **one GPT10 case at 48h** (mean delta ~1.575/255 with speed 255), above its former diagnostic threshold. This failure is **not suppressed**; extended visual fidelity is **unqualified**.
- **Other effects**: extracted all 31 `mode_*()` bodies from dev.14 and dev.14a: **only `mode_gpt10()` differs**, all other 30 bodies are byte-identical.
- **General host suite**: PASS all 30 effects, palette/audio/resolution checks.
- **Stage-1 and GPT18 regressions**: PASS in the evaluated configurations.
- **GPT10 AddressSanitizer and UndefinedBehaviorSanitizer**: PASS for 204-case default-enabled comparison.
- **x86 benchmark**: 64x64, controlled speed 120/235, 16–172,800s; old renderer ~157–166 us/frame, candidate ~123–124 us/frame, about **21–26% less elapsed host time**. These figures **do not forecast ESP32-S3 timing**.
- **PlatformIO full WLED compilation and physical hardware validation**: NOT RUN in this environment.

## Hardware acceptance plan

Flash dev.14a with defaults (`stage2PhaseBounding=true`, `gpt10CellCache=true`, `gpt10PacketBounding=true`). Confirm these fields in JSON; verify the selected effect reports `GPT10`.

1. Keep the **same LED segment settings**, preferably the same usermod composition, and collect GPT10 `/json/info` at ~20s, ~2min, ~10min, ~30min, then >=2h uptime.
2. Compare `recentAvgUs` with the dev.14 known ~71ms early and ~136ms at 8,543s. Watch the `hashCacheHits/hashEvaluations` ratio and ensure `packetBoundedSampleAbs` does not increase continuously with uptime.
3. **Visually inspect** circuit shape, junction color, smooth packet motion and absence of occasional flashes or discontinuities, especially after >2h.
4. If any unexpected visual behavior is detected, first disable packet bounding (`-DGPTFX_GPT10_PACKET_BOUNDING=0`) **while keeping the exact cache on**, then repeat. Only if necessary, disable the cache separately. Distinguish a hardware slowdown from the known large-uptime `float` precision limit.
5. Keep dev.13 as qualified prior baseline and dev.14 as the comparison candidate until hardware results justify promoting dev.14a.

The physical success criteria are improved and stable `recentAvgUs` at long uptime, no loss of function, visually acceptable animation, no reboot or heap regression and negligible skipped frames. **Do not promote dev.14a as hardware-qualified before these gates.**

# GPT Matrix FX — WLED 2D matrix effects

**Current development snapshot:** `v0.3.0-dev.16-preview.7b` (experimental; not the final v0.3.0 release).  
**40 effects:** `GPT01`–`GPT30` (30 standard) and `GPTA01`–`GPTA10b` (10 AudioReactive).  
**Hardware target:** WLED on ESP32-S3 with a 2D matrix, especially a 64×64 HUB75 panel.  
**Last stable release:** v0.2.2 with 25 effects; keep the stable release separate from this development snapshot.

`GPT21`–`GPT30` have graduated from beta without changes to their accepted renderer algorithms. The five newest AudioReactive modes are still marked beta in the effect selector: `GPTA06b` Pulse Architecture, `GPTA07b` Harmonic Constellations, `GPTA08b` Spectral Vortex, `GPTA09b` **Spectral Ribbons**, and `GPTA10b` Resonance Cathedral. They retain the beta suffix in this snapshot; a final release requires explicit promotion and validation. The file [DEV16_PREVIEW7_NOTES.md](DEV16_PREVIEW7_NOTES.md) describes the current audio full-scale qualification.

This is an **external PlatformIO usermod**. Its effect registration uses `strip.addEffect(255, ...)`; no core WLED patch is required *by this library*. A WLED build already near its effect-ID limit may need a separate core-side registration filter, described below. Host tests and RGB simulator outputs do not replace firmware compilation and real hardware verification.

## Effect ID limit, missing effects, and reserved slots

**Symptom:** The final GPT effect is missing from the WLED effect list, or custom-effect registration stops working even though the firmware builds. A highly customized WLED build can exhaust the available effect IDs. Do not assume that a successful build guarantees that every effect is registered.

**Why:** Current WLED effect registration and related interfaces have an 8-bit effect-ID constraint. `255` is a special value used by `strip.addEffect(255, ...)` to request automatic assignment; it is **not** a normal extra user-effect slot. Dynamic registration is therefore constrained by the available ID slots, not merely by the number of effect functions in source code. Simply changing `fxcount` or hiding menu entries does not solve the problem. Exact allocation behavior can vary with the WLED fork and its other usermods.

**Preferred workaround for customized forks:** Exclude unused *built-in effect registrations* while leaving their renderer implementations untouched, so their original IDs can remain reserved (`RSVD`) and be reused by dynamically registered usermod effects.

1. **Back up** the WLED fork and export any presets / automations that reference effect IDs.
2. In the WLED checkout's `wled00/FX.cpp`, apply the optional **WLED FX Registration Filter** patch compatible with that exact core revision. This patch is maintained separately from GPT Matrix FX, because changing core registration is not the responsibility of this usermod. It uses an adjacent `wled00/wled_fx_exclusions.h` file to define which built-in effect IDs are excluded. **Do not blindly replace `FX.cpp` from a different WLED revision.**
3. Under your existing PlatformIO environment (e.g. `[env:waveshare]`), **append** `-D WLED_FX_CUSTOM_FILTER` to its existing `build_flags`, preserving the other flags. The macro works **only after applying the matching FX.cpp patch**; stock WLED does not implement it by itself.
4. Edit the `case FX_MODE_...: return false;` entries in `wled_fx_exclusions.h` to exclude only modes you do not use. **Every `case` must appear once**; duplicated cases cause a compiler `duplicate case value` error. This is a registration-only filter and does **not** remove renderer code or guarantee any firmware-size savings.
5. Rebuild, flash, verify that **all 40 GPT effects** appear in `/json/eff`, and check `/json/info` for the reported GPT Matrix FX build and effect count. `fxcount` does **not necessarily decrease** when slots are excluded: the reserved positions can remain in the effect vector.
6. Check old presets after every change to the exclusions list: **built-in IDs remain in place, but auto-assigned GPT/user effect IDs can change**, so previously saved preset IDs may select a different effect. Update affected presets/automations as necessary.

**Alternative:** Remove other optional usermod effect packs from your own firmware configuration if you do not need them. Avoid disabling WLED's 2D matrix infrastructure (`WLED_DISABLE_2D`) on a 2D/HUB75 configuration. Do not remove effects manually from the body of `FX.cpp` just to recover slots.

**Reverting the filter:** Remove its macro/build flag or restore the unmodified `FX.cpp` (and rebuild). GPT Matrix FX itself does not require this customization on WLED builds that have enough free slots.

## Current effect catalog (v0.3.0-dev.16-preview.7b)

**Additional standard effects:** `GPT21` Sparse Ripple Brightness; `GPT22` Sparse Ripple Size; `GPT23` Photon Loom; `GPT24` Kinetic Rosettes; `GPT25` Tidal Glyphs; `GPT26` Iridescent Membranes; `GPT27` Crystal Recursion; `GPT28` Luminous Mycelium; `GPT29` Prismatic Comets; `GPT30` Moiré Engine.

**Additional AudioReactive effects:** `GPTA06b` Pulse Architecture; `GPTA07b` Harmonic Constellations; `GPTA08b` Spectral Vortex; `GPTA09b` Spectral Ribbons; `GPTA10b` Resonance Cathedral. The five earlier `GPTA01`–`GPTA04` are unchanged. GPTA05 Synaptic Reef now launches independent, overlapping beat ripples rather than restarting a single ring. GPTA02 and GPTA07b no longer display the beat-triggered full-screen rings.

See [EXPERIMENTAL_SPARSE_RIPPLE.md](EXPERIMENTAL_SPARSE_RIPPLE.md) for the historical beta development and attribution of GPT21–GPT25.

---

## Historical development notes (dev.14a and later)


## Current dev.14a GPT10 correction

Dev.14a fixes the late-uptime native `sinf()` cost in **GPT10 Glass Circuit**
with an exactly faithful, frame-local cell hash cache and a separately
reversible bound on the combined temporal packet phase. **All other effects
are byte-for-byte unchanged from dev.14.** No LUT or user-facing effect
control was added. Full details, test limits and device acceptance protocol:
[PERFORMANCE_REMEDIATION_DEV14A.md](PERFORMANCE_REMEDIATION_DEV14A.md).

No extra build flag is needed for defaults. To compile a separate A/B image:

```ini
-DGPTFX_GPT10_CELL_CACHE=1
-DGPTFX_GPT10_PACKET_BOUNDING=0
```

With packet bounding disabled, the cell cache alone is pixel-identical to
dev.14 in host tests. To preserve the exact original dev.14 renderer,
disable **both** new flags. Run `sh tests/run_gpt10_remediation_tests.sh`
for all four compile-time variants.

**dev.14a is experimental and not yet ESP32-S3 hardware-qualified.**

## PlatformIO

Place this directory next to the WLED project and add to the existing environment's `custom_usermods` list (keep existing entries):

```ini
  symlink://../wled-usermod-gpt-matrix-fx
```

Alternatively, use `symlink:///absolute/path/to/wled-usermod-gpt-matrix-fx`. Build with `pio run -e YOUR_WLED_ENV` from the WLED project directory.

## Testing

```sh
sh tests/run_host_tests.sh
sh tests/run_sanitizers.sh
```

No WLED core modifications are required. This is a test candidate, not a release.

## Runtime diagnostics

`/json/info` includes `u.gptMatrixFX` with `version`, `build`, `effects`, `standard`, `audio`, and `experimental`. This confirms the compiled usermod version.

## Experimental controls (dev.6)

- GPT21b / GPT22b: `Dot Spacing` adjusts lattice pitch; midpoint preserves the six-pixel baseline at 64x64.
- GPT24b: `Size` replaces `Hue` and changes rosette scale. Midpoint preserves the dev.4 scale.
- Existing GPT24b presets will interpret the old Hue slider value as Size.

## JSON information layout (dev.6)

The usermod writes its diagnostics to `/json/info` under `u.gptMatrixFX` alongside other usermod information. This is separate from `/json/state` and its `u` usermod state/configuration object. The module does not replace the existing `u` object.

```json
{ "u": { "gptMatrixFX": { "version": "0.3.0", "build": "dev.14a", "effects": 30, "standard": 20, "audio": 5, "experimental": 5 } } }
```

## Experimental GPT24b performance telemetry (v0.3.0-dev.7)

The `/json/info` response includes `u.gptMatrixFX.performance` with `effect`, `calls`, `rendered`, `skipped`, `renderAvgUs`, `renderMaxUs`, `width`, and `height`. These counters are cumulative since boot and apply only to GPT24b. `renderAvgUs` and `renderMaxUs` measure the GPT24b effect function, including its pixel drawing, not HUB75 transfer or WLED's overall frame time. `skipped` counts only calls rejected by GPT24b's internal `tick(40)` gate. `calls` can be larger than `rendered + skipped` if rendering is unavailable for a non-matrix segment. No algorithm or timing threshold was changed.

To diagnose slow animation, select GPT24b, let it run for 20-30 seconds, then record `/json/info` together with `leds.fps`. Repeat after changing to another effect and back if necessary. A high render average relative to the frame budget suggests computational load; a high skipped count suggests the internal frame gate is active. The first reading after boot may show zeros until GPT24b has run.

## GPT24b equivalence-preserving optimization (v0.3.0-dev.8)

GPT24b calculates polar angle and radius once per pixel and reuses the same
values for both rosette layers, central glow and palette lookup. The tick
threshold, effect settings, colors, palette behavior, segment memory and the
other 29 effects remain unchanged. Seven deterministic host reference cases
compare every pixel to a frozen copy of the dev.7 renderer.

New fields in `/json/info.u.gptMatrixFX.performance`:

- `recentCount`, `recentAvgUs`, `recentMaxUs`: up to 32 latest completed
  renders (a sliding window), to compare BLE advertising and connected states.
- `renderLastUs`: most recent GPT24b completed renderer duration.
- `rowAvgUs`, `rowMaxUs`: **cumulative** row elapsed-time diagnostics.
  Up to 64 row samples per frame on a 64x64 segment; no per-pixel timestamps.
- `coreStart`, `coreEnd`, `coreChanges`: observed FreeRTOS core IDs at the
  beginning/end of a render, and the number of changes; host tests report -1.

All timings use elapsed wall-clock microseconds, so **they include task
preemption and contention**. They do NOT report CPU time or task interrupt
counts. Cumulative fields persist for the device uptime; compare
`recentAvgUs` after waiting for 32 complete frames following a BLE state change.
If the effect falls to 4 FPS, wait at least 8 seconds for 32 samples.

For an apples-to-apples comparison: keep GPT24b's palette, sliders, matrix
configuration and power constant. Record `/json/info` with BLE advertising,
then connect BLE without changing effects, wait for the window to refill, and
record `/json/info` again. `leds.fps` is the WLED system display rate, not
identical to the internal render invocation count.

See [PERFORMANCE_AUDIT_DEV8.md](PERFORMANCE_AUDIT_DEV8.md) for a preliminary,
non-hardware cost assessment of the remaining effects and next steps.

## GPT24b phase-bounding candidate (v0.3.0-dev.9)

The dev.8 rosette became progressively slower on a physical Waveshare
ESP32-S3 64x64 despite iDotMatrix and Buzzer being excluded: approximately
50 ms/render shortly after boot versus approximately 346 ms after 697 s.
This establishes that a Bluetooth connection is not required for the slowdown.

In dev.9, the time-only components of five rosette oscillations are reduced
modulo 2*pi **once per frame**, before drawing the pixels. Each oscillator
retains its exact temporal frequency; in particular the secondary sine's
`phase*1.7 + t*0.4` is reduced at the *combined* temporal frequency.
The spatial terms and palette drift are intentionally unchanged.

`performance.phaseRawAbs` and `performance.phaseWrappedAbs` report the
maximum magnitude (in radians) of these five temporal terms before and after
reduction, respectively. `phaseWrappedAbs` should always be less than 2*pi.
The remaining performance counters are unchanged from dev.8.

Numerical host regression across 48 simulated uptime/speed conditions up to
7200 s shows a maximum channel difference of 1/255 versus frozen dev.8,
with at most 67 of 4096 pixels showing any difference in one tested frame.
This is not strict byte-for-byte equivalence due to floating-point
rounding, but preserves the visual geometry to the measured tolerance.

The mechanism is motivated by known slow large-input argument-reduction
paths in some sinf/cosf libraries; the ESP32-S3 performance impact is
**unconfirmed until hardware testing**. Do not claim this build fixes the
2-FPS condition until a 10-15 minute real-device soak completes.

Optional host diagnostic:

```sh
sh tests/run_phase_profile.sh
```

See [PERFORMANCE_INVESTIGATION_DEV9.md](PERFORMANCE_INVESTIGATION_DEV9.md)
for the evidence, constraints, and controlled hardware procedure.

## GPT24b trigonometry LUT candidate (introduced in v0.3.0-dev.10)

An experimental 1024-entry, float, flash-resident sine lookup table is used
for sine and cosine in **GPT24b only**. No other effect's math is changed.
The default is linearly interpolated to preserve visual fidelity. Use the
existing PlatformIO environment's `build_flags` to choose:

- `-DGPTFX_GPT24_TRIG_BACKEND=0`: original dev.9 `sinf/cosf`.
- `-DGPTFX_GPT24_TRIG_BACKEND=1`: nearest LUT (comparison only).
- `-DGPTFX_GPT24_TRIG_BACKEND=2`: interpolated LUT (default).

`/json/info.u.gptMatrixFX.trigonometry` reports active backend, 1024 samples,
4096 bytes, and a one-time setup microbenchmark of native small/large angle
calls vs nearest and interpolated table accesses, in elapsed nanoseconds/call.
The existing `performance` telemetry remains unchanged. The `build_flags`
setting must be appended, not replace WLED's existing flags.

See [PERFORMANCE_EXPERIMENT_DEV10.md](PERFORMANCE_EXPERIMENT_DEV10.md) for
hardware evidence, precision measurements, host benchmarks and a qualification
procedure. **The dev.10 ESP32-S3 speedup has since been measured on a physical 64×64 Waveshare; see PERFORMANCE_EXTENSION_DEV11.md.**

```sh
sh tests/run_trig_backend_tests.sh
sh tests/run_trig_host_benchmark.sh
```

## Selective optimizations of other effects (v0.3.0-dev.11)

Experimental change targets GPT07, GPT08, GPT18 and GPT20 only. Default:
GPT07 hoists invariant rotation trigonometry; GPT08 and GPT20 use the same
1024-float interpolated sine table already included for GPT24b; GPT18
hoists four per-frame sines, but keeps its per-pixel sine functions native
unless explicitly enabled for testing. Other effects are unchanged.

Optional compile-time flags (append to existing `build_flags`):

```ini
-DGPTFX_EXTRA_TRIG_BACKEND=0
-DGPTFX_GPT18_TRIG_BACKEND=2
```

`0` selects native sin/cos, `2` selects linear LUT. **Defaults** are `2`
for GPT08/GPT20, and `0` for GPT18. GPT24b remains controlled by its
own existing `GPTFX_GPT24_TRIG_BACKEND` flag, unaffected by these options.

New `/json/info.u.gptMatrixFX.extraPerformance` exposes a rolling 32-render
window for the last selected effect among GPT07/GPT08/GPT18/GPT20. This
is elapsed time and not isolated CPU time. The `trigonometry` JSON object
also reports `extraBackend`, `extraEffects`, and `labyrinthBackend`.

For caveats, frozen-reference pixel comparisons (including 128×128), host-only
benchmarks, and hardware test steps, see [PERFORMANCE_EXTENSION_DEV11.md](PERFORMANCE_EXTENSION_DEV11.md).

```sh
sh tests/run_trig_backend_tests.sh
sh tests/run_multi_host_benchmark.sh
```

## GPT18 time-dependent phase-bounding candidate (v0.3.0-dev.12)

Physical dev.11 diagnostics demonstrated that GPT18 degraded from 44.73 ms/render
at uptime 1048 s to 190.95 ms/render at uptime 1806 s, despite unchanged free
heap. GPT18's four per-pixel `sinf` calls receive continuously increasing
time phases. The dev.12 experiment reduces each wave phase and its associated
fold phase to `[-pi, pi]` **once per layer per frame**. Spatial coordinates,
palette drift and `tick(35)` are unchanged. GPT18 still uses native `sinf()`
by default; LUT usage is an **independent experimental option**.

- `GPTFX_GPT18_PHASE_BOUNDING=1`: default phase bounding.
- `GPTFX_GPT18_PHASE_BOUNDING=0`: reproduce dev.11 phase behavior for A/B.
- `GPTFX_GPT18_TRIG_BACKEND=0`: native `sinf`, default.
- `GPTFX_GPT18_TRIG_BACKEND=2`: opt-in existing interpolated LUT.

`u.gptMatrixFX.trigonometry.labyrinthPhaseBounding` identifies the compiled
phase behavior. `u.gptMatrixFX.extraPerformance.phaseRawAbs` and
`phaseWrappedAbs` contain the latest GPT18 temporal phases in radians when
GPT18 is the monitored effect. With phase bounding enabled, the latter should
remain `<= pi`.

```sh
sh tests/run_labyrinth_phase_tests.sh
sh tests/run_host_tests.sh
sh tests/run_sanitizers.sh
```

See [PERFORMANCE_INVESTIGATION_DEV12.md](PERFORMANCE_INVESTIGATION_DEV12.md)
for on-device evidence, host regression limits and the 30-minute qualification
procedure. **Full target firmware build / physical dev.12 soak: pending.**


## dev.13 — Stage-1 long-uptime trigonometry remediation

Six audit-priority effects (GPT05, GPT11, GPT13, GPT19, GPT23b, GPTA05) now reduce only the **time-only periodic phases** of native sine/cosine/ridge calls before entering the pixel loops. The original renderer is restored with optional `-DGPTFX_STAGE1_PHASE_BOUNDING=0` in the existing PlatformIO `build_flags` (default is 1). The LUT and GPT18/GPT24b mechanisms are untouched.

`u.gptMatrixFX.trigonometry.stage1PhaseBounding` identifies the selected mode; `stage1Effects` gives the effect labels. `u.gptMatrixFX.extraPerformance` records the most recently selected measured effect, its recent 32-frame timing and raw/bounded temporal phase magnitudes. The effects share the existing low-overhead diagnostic buffer; each effect change resets that snapshot.

Tests: `sh tests/run_stage1_phase_tests.sh`, `sh tests/run_host_tests.sh`, `sh tests/run_labyrinth_phase_tests.sh`, and `sh tests/run_sanitizers.sh`. See [PERFORMANCE_REMEDIATION_DEV13.md](PERFORMANCE_REMEDIATION_DEV13.md) and included CSV logs for host visual comparisons and limitations. **PlatformIO/WLED integration and physical performance qualification are still pending for dev.13.**


## dev.14 — Stage-2A long-uptime trigonometry remediation

The dev.14 candidate bounds periodic time phases in GPT01, GPT02, GPT09, GPT10, GPT14 and GPT16. It leaves the 24 other renderers byte-identical to dev.13 and keeps all stage-1/GPT18/GPT24b fixes. No additional LUT is used. `GPTFX_STAGE2_PHASE_BOUNDING=1` by default; `0` returns exact dev.13 rendering for the six new candidates. Telemetry is available in `/json/info` (`trigonometry.stage2PhaseBounding`, `stage2Effects`, and shared `extraPerformance`). See [PERFORMANCE_REMEDIATION_DEV14.md](PERFORMANCE_REMEDIATION_DEV14.md). The complete WLED build and physical validation are pending. Extended 24/48-hour float precision remains unqualified.

# GPT Matrix FX release notes

## v0.3.0-rc.1 — release candidate (not final)

- 40 approved effects: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`). The five most recent audio effects are promoted from `GPTA06b`–`GPTA10b` without changing their renderers.
- `build=rc.1`, `experimental=0`, updated `library.json`, README and host metadata expectations.
- Retains the hardware-approved dev.16-preview.7b visuals and beat behavior: no full-screen beat ring in GPTA02/GPTA07; GPTA05 accumulates independent fading waves.
- WLED automatic effect IDs have an 8-bit limit. Refer to README for `RSVD` slot filtering, preset ID migration, and optional fork-only customization.
- Candidate passes host/sanitizers and Stage-1/Stage-2A tests. Historical Stage-2B golden checks for GPTA02 fail on this RC **and on dev.16-preview.7b**, due to the intentionally removed ring; see RC1_RELEASE_CHECKLIST.md. Full Kagayaki PlatformIO compilation, 40-effect registration confirmation and long-duration on-device stability are release gates. No claim of final hardware qualification is made.

---

# Historical releases

## GPT Matrix FX v0.2.2

First public source release of **GPT Matrix FX**, an external PlatformIO usermod providing **25 original effects for WLED 2D LED matrices**.

All effects, source code, automated tests, and documentation were designed and produced by **ChatGPT (OpenAI)**. The repository maintainer supplied the brief, feedback, hardware evaluation, and release publishing. This is an independent project, not an official OpenAI or WLED release.

## Highlights

- **20 standalone effects**, named `GPT01` through `GPT20`.
- **5 AudioReactive effects**, named `GPTA01` through `GPTA05`, using the WLED AudioReactive usermod's 16 FFT bands.
- A redesigned **GPT06 - Clockwork Cathedral** with gear-meshed orbiting satellites, drifting assembly motion, moving metallic details, and glitter.
- Palette-change handling and fallback for entirely black palettes, without erasing intentional dark regions in non-black palettes.
- Scales from small to larger 2D segments; tuned visually around a 64x64 HUB75 display.
- External symlink-based PlatformIO integration; no WLED core modifications.

## Installation

Unpack `wled-usermod-gpt-matrix-fx` alongside your WLED checkout. Append the following entry to your existing `custom_usermods` in the selected PlatformIO environment (preserving all other entries):

```ini
  symlink://../wled-usermod-gpt-matrix-fx
```

Alternatively, use an absolute `symlink:///absolute/path/to/wled-usermod-gpt-matrix-fx` path, replacing the placeholder with your actual filesystem location. Compile from the WLED project root with `pio run -e YOUR_WLED_ENV`, replacing `YOUR_WLED_ENV` with the selected PlatformIO environment name. See `README.md` for details. Include and enable the existing WLED AudioReactive usermod for the five sound-responsive effects.

## Validation

- Host suite: 25 effects x 5 matrix resolutions; 25 effects x 9 live palette configurations.
- Differential FFT tests for all five audio modes.
- GPT06 meshing/orbital-motion geometry tests.
- C++11 compatibility checks and AddressSanitizer/UndefinedBehaviorSanitizer passed.
- The maintainer reports satisfactory results on the real LED matrix. This package has not been independently compiled against the maintainer's custom WLED checkout; hardware FPS/soak tests have not been formally measured.

## Notes

- Effect IDs are dynamically allocated by WLED and can change when the effect set is rebuilt. Presets referencing numeric IDs may need review.
- The package includes an updated `.gitignore` that excludes `Zone.Identifier` filesystem artifacts (including Windows/WSL Mark-of-the-Web sidecars).
- The firmware environment is **not** bundled: this is a WLED usermod source library, not a prebuilt firmware image.
- License: MIT.


### v0.3.0-dev.11 (experimental, not a release)

- GPT07: hoist fold-angle sine/cosine pairs once per frame; byte-exact against dev.10 on tested cases.
- GPT08: experiment with shared interpolated 1024-value LUT for six trig calls per pixel.
- GPT18: hoist fixed temporal sine offsets; native trig remains default; optionally enable LUT.
- GPT20: hoist axis sine/cosine and time offsets; use existing LUT for per-pixel cosine.
- Add independent compile-time A/B controls and `extraPerformance` rolling telemetry.
- Preserve dev.10 GPT24b algorithm, phase bounding, LUT, and telemetry unchanged.
- Host reference comparisons: 64×64, 128×64, 128×128. Host/ASan/UBSan tests pass.
- The dev.11 firmware and physical performance are not yet qualified on ESP32-S3.

See `PERFORMANCE_EXTENSION_DEV11.md` for exact outcomes and limitations.


### v0.3.0-dev.12 (experimental, not a release)

- GPT18/Vitreous Labyrinth phase-bounding correction candidate for time-dependent
  native `sinf()` cost. Bound eight time-only inputs once per frame; no new LUT
  or pixel-loop argument reduction. Native `libm` remains the default.
- CMake/PlatformIO-independent compile flags permit exact dev.11 baseline
  (`GPTFX_GPT18_PHASE_BOUNDING=0`) and independent LUT A/B.
- Additional GPT18 phase telemetry and extensive long-uptime host tests.
- Others 29 effects unchanged; public v0.2.2 untouched. ESP32-S3 dev.12
  firmware build and on-device soak still required.


### v0.3.0-dev.13 (experimental, not a release)

- Stage-1 uptime phase bounding for GPT05, GPT11, GPT13, GPT19, GPT23b and GPTA05, with compile-time exact dev.12 fallback.
- Shared 32-render `extraPerformance` telemetry includes these six effects; raw/wrapped phase maxima and build flags are available in `/json/info`.
- No changes to existing LUT, GPT18 or GPT24b, user-facing effect names, sliders or WLED core.
- Frozen source visual regressions, native/disabled A/B, AudioReactive connected/disconnected scenarios, sanitizer and host tests pass. Device qualification/PlatformIO full build pending.

See `PERFORMANCE_REMEDIATION_DEV13.md` for exact test scope and limitations.


### v0.3.0-dev.14 (experimental, not a release)

- Stage-2A bounded periodic temporal phases for GPT01, GPT02, GPT09, GPT10, GPT14 and GPT16, with independent compile-time exact dev.13 A/B (`GPTFX_STAGE2_PHASE_BOUNDING=0`).
- GPT09's redundant time-only sine is hoisted once per frame *only* with phase bounding enabled. GPT10 retains its original spatial sine hash and cell assignment.
- Six new `extraPerformance` profiles; `/json/info` includes stage-2 flag and list. All other 24 renderer bodies remain byte-identical.
- Strict 0–2h host visual tests pass (636 enabled + 636 disabled), with rare isolated color outliers in GPT09/GPT10. Extended 48h diagnostic shows float-phase precision divergence and is **not** a multiday visual qualification. Host/sanitizer, stage-1/GPT18 and compatibility checks pass.
- Integrated WLED/PlatformIO build and Waveshare visual/timing acceptance pending. Stable public v0.2.2 untouched.

See `PERFORMANCE_REMEDIATION_DEV14.md` for exact tests and limitations.

### v0.3.0-dev.14a (experimental, not a release)

GPT10 Glass Circuit only: bounded temporal packet phase and exact per-cell
hash caching. Build metadata and diagnostic telemetry extended, with 30 effect
IDs, effect settings, existing rosette/LUT and all other renderer bodies
preserved. Reversible compile-time A/B switches. Tests are host-only so far;
physical hardware qualification pending. See
`PERFORMANCE_REMEDIATION_DEV14A.md`.

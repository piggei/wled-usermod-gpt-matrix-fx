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

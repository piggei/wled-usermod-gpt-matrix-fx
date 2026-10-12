# v0.3.0-dev.16-preview.7 — Audio full-scale qualification

Built from preview.6; modifies only the five new beta AudioReactive renderers GPTA06b–GPTA10b and their isolated helper. All 35 other renderer bodies remain unchanged.

## Audio processing
- Same AudioReactive 16-band FFT source and original peak/volume smoothing; no changes to the original GPTA01–05 processing.
- New beta-only nonlinear low/mid sensitivity curve `e*(1.52-.52*e)`; exact endpoints 0 and 255 preserved.
- GPTA06b height at full-scale reaches 99% independent of the breathing phase; no forced limit around 3/4 screen.
- GPTA07b brightens and expands constellation geometry and spectral connections.
- GPTA08b increases the musical modulation of the established vortex, retaining its general geometry.
- GPTA09b expands musical ribbon displacement and energy response.
- GPTA10b deletes the disconnected vertical rib lines. Spectral energy now affects vault geometry, rose size and brightness. The third control label changes from Columns to Arches.

## New exhaustive host qualification
`tests/test_new_audio_fullscale.cpp` exercises all five new renderers on 64x64 with FFT = 0, 26, 128, 204, 255 (all 16 bands), individual bass/mid/treble-only full scale, and 0->255->0. 45 frames per scenario allow attack/release smoothing to settle. Results: `FULLSCALE_FFT_5_EFFECTS_9_SCENARIOS_PASS`, RGB contact sheet and CSV.

At 255 for all bands A06 reaches RGB max 180 on top row and 3840 pixels above brightness threshold 56; at 0 FFT no pixel crosses that threshold. The other effects intentionally remain sparse in some regions and do not aim to light all LEDs.

Tests completed: general host test, Stage1, Stage2A, Stage2B (both modes), new full-scale FFT test. Kagayaki PlatformIO hardware compilation and real 64x64 ESP32-S3 timing have NOT been verified; preserve beta suffixes.

**Caution:** `tests/test_new_audio_fullscale.cpp` measures host RGB from the actual C++ renderer but its palette is a host mock, not necessarily identical to the Waveshare/WLED FastLED palette output. The host timing values in the CSV are mock timing and should NOT be used as ESP32 measurements.

## Documentation refresh (no renderer changes)
The README now covers the 40-effect inventory, current beta names, the WLED effect-ID ceiling, the optional external core-side registration filter, RSVD slot reuse, duplicate-case errors, and preset ID migration. This refresh does not change effect algorithms, registrations, or the firmware build identifier.

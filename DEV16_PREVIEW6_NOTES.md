# GPT Matrix FX v0.3.0-dev.16-preview.6

## Changes relative to dev.16-preview.5

- GPTA07b Harmonic Constellations: 16 FFT-modulated stars arranged across the full viewport, dynamic vertical/horizontal bridges, wider halos and beat wave. The renderer's controls, registration order, and profile identifier stay unchanged.
- GPTA09b replaces Quantum Fireflies with **Spectral Ribbons**: six full-width light streams responding to 12 FFT bands and bass, with musical modulation of amplitude/brightness/width and a transient streak. Same registration position and profile ID, but **changed name and complete visual identity**.
- GPTA06b, GPTA08b and GPTA10b unchanged. Other 35 effects unchanged.
- Build identifier advanced to `dev.16-preview.6` and host test expected strings updated for promoted GPT21-GPT30.

## Tests performed in host environment

- `sh tests/run_host_tests.sh`: PASS (40 FX / five resolutions / palette and FFT cases).
- `sh tests/run_stage1_phase_tests.sh`: PASS.
- `sh tests/run_stage2_phase_tests.sh`: PASS.
- `sh tests/run_stage2b_phase_tests.sh`: PASS.
- Static host visual frames: `visual/audio7_audio9_contact_sheet.png`. Colors use the mock host palette and are **not** an exact copy of the WLED rainbow palette.

## Hardware and slot warning

Source-only package, not a precompiled .bin. No Kagayaki PlatformIO compile or ESP32-S3 real-time benchmark has been performed. The WLED `FX.cpp` exclusion patch is a **separate** change maintained by the user and is **not included** in this usermod archive. Confirm enough free effect slots and verify the `GPTA10b` entry appears before approving it.

Test AudioReactive with actual I2S audio, checking `extraPerformance` when GPTA07b and GPTA09b are active. Preserve a known-working firmware build before flashing new firmware.

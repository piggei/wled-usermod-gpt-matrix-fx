# Sparse Ripple - experimental hardware evaluation

This is an experimental build derived from the public v0.2.2 source. The released v0.2.2 remains unchanged.

## New modes

- **GPT21b - Sparse Ripple Brightness**: constant-size dots; distance-phased radial wave modulates brightness.
- **GPT22b - Sparse Ripple Size**: dot brightness remains comparatively steady; the wave expands and contracts each dot using pixel-center coverage.

Both modes use the WLED palette and native Speed / Intensity controls. GPT21b Intensity controls brightness contrast; GPT22b Intensity (Expansion) controls the maximum dot radius up to the pitch-based cap, with fixed peak brightness. Wave phase, lattice centers, and pitch are shared. Their 64x64 dot spacing is about six pixels. A full frame is cleared between updates to preserve the dark gaps and prevent ghosting. No extra frame buffer is allocated. Existing 25 effect registrations are unchanged and the new modes are appended.

## Test procedure

1. Build using your existing PlatformIO environment. Do not replace your tagged v0.2.2 release.
2. Compare GPT21b and GPT22b on the same palette, brightness, speed, intensity, and viewing distance.
3. Check negative space, whether dim peaks disappear, whether expanded dots merge, and whether the wave appears to jump at cycle boundaries.
4. Check a slow speed and a fast speed.
5. Record perceived FPS and any problems with palettes.

## Hardware status and comparison protocol

The project maintainer reports a successful initial visual smoke test on a real 64x64 HUB75 matrix and prefers the size-based variant. The precise firmware build/configuration and test date have not yet been recorded; this is not a controlled A/B validation. Full firmware compilation has not been reproduced in this test environment.

For the next comparison, keep the same palette, speed, 6-pixel pitch, and viewing distance. Compare average light output before choosing a variant; size modulation can illuminate more pixels. Then repeat with a wider pitch to evaluate preserved negative space. Record firmware version, PlatformIO environment, matrix configuration, and test date.

## Status

Host C++ compatibility and five-resolution rendering tests pass, including palette changes and ASan/UBSan. Full WLED/ESP32 firmware compilation and real-matrix testing have not been performed in this test environment. The original reference was suggested by Milo (1human / JM), based on the CC0 Dot ripple study: https://reels.1human.tech/post/ef5b73d3-87fa-4726-9026-e5c7c567cea5 . The algorithms here are new implementations, not copied reference code.

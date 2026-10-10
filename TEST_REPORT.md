# Sparse Ripple Experimental Test Report

- Derived from the v0.2.2 release source; the 25 original effect implementations remain unchanged.
- 27 modes registered in total: 20 original normal + 5 audio + 2 sparse ripple experiments.
- Host C++ compilation and rendering tests: PASS.
- Resolution coverage: 8x8, 16x32, 64x64, 96x48, 128x128.
- Palette-switch regression, audio tests and gear tests: PASS.
- ASan/UBSan host suite: PASS.
- Full firmware build on actual WLED / ESP32-S3: NOT RUN.
- Real hardware visual validation: PENDING.

The two new modes are appended after the existing 25 to minimize effects-ID disruption within the same firmware configuration. Effect IDs may still change across different WLED builds.

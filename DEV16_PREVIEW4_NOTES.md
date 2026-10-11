# dev.16-preview.4 — GPT28b raster performance candidate

- GPT28b geometry and light transfer functions retain preview.3 formulas.
- Precompute segment bounding boxes and inverse-length once per frame.
- Scan only rectangle-covered pixels via row tiles (32 scratch pixels), rather than per-pixel per-segment AABB and division.
- Profiles GPT26b–GPT30b using existing extraPerformance (selected effect only).
- All other rendering functions unchanged.
- Host validation does not substitute for full PlatformIO build or ESP32-S3 performance and visual qualification.

## Validation recorded
- Host general test suite: PASS (35 effects x 5 matrix sizes, 9 palettes and regressions).
- GPT28b direct framebuffer A/B preview.3 vs preview.4: 30 configurations, 8/16/64/96/128-square matrices, **720384 bytes exactly identical**.
- 64x64 host measurements on 6 independent single-frame cases: mean old 469.2 us, new 327.47 us (30.2% less); these CPU times cannot predict ESP32-S3 FPS.
- Stage-1 and Stage-2A regressions pass; Stage-2B disabled branch passes. Enabled branch was not completed in the allotted test execution time; no failures observed before interruption.
- Compiling Kagayaki/WLED and real hardware FPS remain unverified.

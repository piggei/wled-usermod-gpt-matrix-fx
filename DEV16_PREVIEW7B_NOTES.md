# dev.16-preview.7b — Synaptic Reef independent ripples

Based on dev.16-preview.7a (which removed pulse ring overlays from GPTA02 and GPTA07b).

- GPTA05 Synaptic Reef: consecutive detected peaks launch independent outward-moving waves. No wave restarts when another peak occurs. Up to six wave starts are retained in 56 bytes of segment-owned effect state. Each wave expands for 1700ms and fades with a smooth quadratic envelope. Oldest records are overwritten under heavy peak density. No per-frame allocations or large stack buffers.
- Original 16 coral arms, FFT interpretation, palette use and bounded trigonometric phases unchanged.
- GPTA02 and GPTA07b circle removals from preview.7a retained.
- All other renderers unchanged.
- New host proof `tests/test_reef_multiwave.cpp` confirms independent start timestamps and bounded storage. Run it with `g++ -std=c++17 -O2 -Itests tests/test_reef_multiwave.cpp -o /tmp/test_reef && /tmp/test_reef`. General host and stage-1 regressions have passed.
- Firmware compilation on the ESP32-S3 Kagayaki fork and visual reaction to real music remain to be checked.

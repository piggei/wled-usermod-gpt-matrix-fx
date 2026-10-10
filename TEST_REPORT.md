# GPT Matrix FX v0.3.0-dev.2 - Test report

Experimental branch build only. Public v0.2.2 remains unchanged.

- GPT21b: fixed-size dots; Intensity controls brightness contrast.
- GPT22b: Intensity controls actual maximum dot radius, capped by lattice pitch.
- Both variants retain shared lattice geometry, phase and palette behavior.
- Host regression: 27 effects across five resolutions, palette changes, audio fallback and motion checks.
- Dedicated slider regression checks that increasing Expansion increases lit-pixel footprint at fixed phase.
- Host AddressSanitizer and UndefinedBehaviorSanitizer: see local test execution.
- ESP32/WLED full compilation: not performed here.
- Hardware: maintainer reported initial visual smoke test of the prior experimental build on 64x64 HUB75, preferring GPT22. Exact firmware configuration/date not recorded. The new Expansion fix still requires hardware testing.

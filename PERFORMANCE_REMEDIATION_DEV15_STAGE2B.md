# GPT Matrix FX v0.3.0-dev.15 — Stage-2B native trigonometry

**Status: experimental host-tested source candidate, NOT a compiled WLED firmware and NOT yet hardware-qualified.** Stable public v0.2.2 and source dev.14a are untouched.

## Exact scope

Six previously exposed native per-pixel renderers have periodic **time-only** sine inputs bounded once per frame with centered `fmodf` to [-π,π]. Compile-time switch `-DGPTFX_STAGE2B_PHASE_BOUNDING=0` restores the exact dev.14a renderer math for these six effects. No changes to palette progression, fract/noise/hash, geometry, effects registration order, time scheduling or AudioReactive sampling.

| Effect | Time-only periodic inputs |
|---|---|
| GPT15 Ferrofluid Theatre | `-t*0.42` in spike ridge; poles and contour untouched |
| GPT25b Tidal Glyphs | `-t*1.8`, `+t*0.7` in cell tide and opening; geometry unchanged |
| GPTA01 Bass Cartographer | `-t*1.1` in coastline sine |
| GPTA02 Harmonic Wormholes | `-t*1.6`, `+t` in two ridge waves |
| GPTA03 Spectral Loom | 16 independent `t*(1+k*.035)` phases plus `-t*3.4` for sparks, precomputed before pixels |
| GPTA04 Resonant Tessellator | `-t*0.5` in facet ridge |

**Telemetry:** `u.gptMatrixFX.trigonometry.stage2bPhaseBounding`, `.stage2bEffects`, and the existing `extraPerformance` values (`effect`, `recentAvgUs`, `phaseRawAbs`, `phaseWrappedAbs`, etc.) now support all six effects, with the same single-effect-reset behavior as dev.14a. Legacy GPT10 diagnostics deliberately retain last GPT10 observation when another effect is selected.

## Local validation

- Host functional test 30 modes / five resolutions / 9 palettes / 5 FFT differential proofs: PASS.
- ASan and UBSan general host suite: PASS.
- Stage-2B differential test: 6 effects x 133 parameter/time/resolution scenarios = 798 comparisons in each mode; **disabled=exact pixel equivalence** to the frozen dev.14a implementation; **enabled=PASS** within configured visual thresholds.
- Active-mode worst observations by effect (maximum delta, high-delta pixel count >16, mean RGB error per channel): GPT15 (115, 3, 0.05282); GPT25b (6, 0, 0.06177); GPTA01 (1, 0, 0.00773); GPTA02 (2, 0, 0.06193); GPTA03 (2, 0, 0.05086); GPTA04 (1, 0, 0.00456). The GPT15 discrepancy is sparse but nonzero; requires human review near temporal wrap boundaries if observed. Tests cover 0–24h in 64×64; 128×64 and 128×128 through 2h.
- Connected and disconnected AudioReactive scenarios included in the above differential test (zero-padded static FFT sample, no real audio input; on-board audio visual gate still required).
- Stage-1 and Stage-2A original differential suites pass against their original references.
- Pre-existing Stage-2A **48-hour simulated** suite remains FAIL in enabled mode due to known GPT10 and GPT16 numerical drift, including GPT10 >16/255 outliers at 48 h. This is an inherited known limitation, not proven related to Stage-2B; dev.15 does NOT claim multi-day image equivalence. Stage-2A disabled-mode 48h tests pass.
- Original 30 effect renderer function bodies were compared against dev.14a; **only the six Stage-2B effects differ**. GPT24b, GPT10 and all other 24 effects are byte-identical at the function-body level.

## Hardware acceptance

Use 64×64 Waveshare ESP32-S3 / WLED Kagayaki. At boot and uptime >35min, select GPT15, GPT25b and GPTA01–GPTA04; record `/json/info` per effect with recent rendering, skips, raw/wrapped phases, freeheap/PSRAM; visually confirm effects, especially GPT15. For GPTA01–04 test AudioReactive with real changing FFT and without input. To validate speed regression specifically, compare against dev.14a **at same uptime and settings**; aggregate WLED `leds.fps` alone cannot establish renderer performance.

Do not promote to stable or assert WLED firmware compilation until compiled/flashed and hardware visual/performance checks pass.

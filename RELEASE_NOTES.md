# GPT Matrix FX — Release notes

## v0.3.0 — Final release

**40 effects**: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`), all with final names.

- Promotes the approved RC4 to the v0.3.0 release: no changes to rendering algorithms, effect order, palette behavior, or AudioReactive processing.
- Keeps overlapping audio-triggered rings in GPTA05, with distracting concentric rings removed from GPTA02 and GPTA07.
- Keeps the default `/json/info` payload concise (`version`, `build`, `effects`, `standard`, `audio`); the existing diagnostic code and counters remain available with `GPTFX_ENABLE_JSON_DIAGNOSTICS=1`.
- README provides the YouTube demonstration, complete tables of effects, 64×64 recommendation (16×16 not recommended), and WLED effect-ID / `RSVD` slot guidance.
- Release package contains only the eight public files. No development CSV, tests, simulated previews or visual assets.
- Tested on a 64×64 Waveshare ESP32-S3 / HUB75 configuration during development. This does not imply qualification for all hardware, WLED versions or long-running deployments.

## v0.3.0-rc.4

- Made JSON diagnostics opt-in at build time, preserving instrumentation and the rendering code.
- Distribution remained limited to eight public files.

## v0.3.0-rc.3

Release candidate with **40 effects**: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`). All effect names are final, without beta suffixes.

- Rendering behavior is frozen from `dev.16-preview.7b`: no formula, palette, effect order, or animation change in RC3.
- Audio details include independent fading beat waves for GPTA05 and removal of distracting concentric rings from GPTA02 and GPTA07.
- The distribution contains a GitHub-ready effect catalog in table form, the official YouTube video directly below the title, 64×64 guidance, and an explanation of WLED's effect-ID/`RSVD` slot limits.
- Excluded all development-only documents, host tests, diagnostics, CSV data, visual assets, and generated previews from the release archive.
- RC3 packaging update documents that `rsync -a` retains stale files and legacy install scripts must not unconditionally run excluded development tests.
- RC3 updates source build metadata and PlatformIO library metadata only. Cleaned distribution is intentionally limited to eight public files; removing previously tracked development artifacts from the GitHub branch requires a separate Git commit.
- Host smoke tests were completed during development (test harness not distributed). Full Kagayaki PlatformIO compilation, all-40-effect registration and long-duration hardware qualification remain necessary before final v0.3.0 promotion.

## v0.2.2

Earlier public release with 25 effects (20 standard, 5 AudioReactive). The v0.3.0 series adds 15 effects and improves rendering performance and long-uptime phase handling.

# GPT Matrix FX — Release notes

## v0.3.0 — Final release

**40 effects**: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`), all with final names.

- Promotes the approved RC4 to the v0.3.0 release: no changes to rendering algorithms, effect order, palette behavior, or AudioReactive processing.
- Keeps overlapping audio-triggered rings in GPTA05, with distracting concentric rings removed from GPTA02 and GPTA07.
- Keeps the default `/json/info` payload concise (`version`, `build`, `effects`, `standard`, `audio`); the existing diagnostic code and counters remain available with `GPTFX_ENABLE_JSON_DIAGNOSTICS=1`.
- README provides the YouTube demonstration, complete tables of effects, 64×64 recommendation (16×16 not recommended), and WLED effect-ID / `RSVD` slot guidance.
- Release package contains only the eight public files. No development CSV, tests, simulated previews or visual assets.
- Tested on a 64×64 Waveshare ESP32-S3 / HUB75 configuration during development. This does not imply qualification for all hardware, WLED versions or long-running deployments.

## v0.2.2

Earlier public release with 25 effects (20 standard, 5 AudioReactive). The v0.3.0 series adds 15 effects and improves rendering performance and long-uptime phase handling.

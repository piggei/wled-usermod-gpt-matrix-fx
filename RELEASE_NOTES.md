# GPT Matrix FX — Release notes

## v0.3.0-rc.4 — Release candidate

- Default `/json/info` is concise: version, build, effects, standard, audio.
- All existing diagnostic counters and benchmark computations remain implemented; the complete JSON can be restored at compile time using `GPTFX_ENABLE_JSON_DIAGNOSTICS=1`.
- No renderer, effect-registration order, palette, or AudioReactive changes compared with the prior v0.3.0 release candidate.
- Distribution remains limited to the same eight public files.

## v0.3.0 — Planned final release

**40 effects**: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`), all with final names.

- Promotes the hardware-reviewed RC3 to the final v0.3.0 release without changes to the rendering algorithms, effect order, or palette behavior.
- Retains the independent, overlapping audio-triggered rings in GPTA05 and the removal of distracting concentric rings from GPTA02 and GPTA07.
- README includes the YouTube demonstration, two text-based effect tables, 64×64 recommendation (16×16 not recommended), and guidance for handling WLED's effect ID limit using reserved slots.
- Distribution consists only of the eight public source, metadata, example, license and documentation files. No development CSV, test artifacts, simulated previews or visual assets.
- Confirmed by the project maintainer that the RC3 source compiles in the Waveshare Kagayaki environment. Validation of extended runtime stability across all effects and hardware configurations is not implied.

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

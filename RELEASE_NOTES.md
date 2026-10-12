# GPT Matrix FX — Release notes

## v0.3.0-rc.2

Release candidate with **40 effects**: 30 standard (`GPT01`–`GPT30`) and 10 AudioReactive (`GPTA01`–`GPTA10`). All effect names are final, without beta suffixes.

- Rendering behavior is frozen from `dev.16-preview.7b`: no formula, palette, effect order, or animation change in RC2.
- Audio details include independent fading beat waves for GPTA05 and removal of distracting concentric rings from GPTA02 and GPTA07.
- The distribution contains a GitHub-ready effect catalog in table form, the official YouTube video directly below the title, 64×64 guidance, and an explanation of WLED's effect-ID/`RSVD` slot limits.
- Removed historical performance CSVs, generated visual previews, experiment notes and development utilities from the distributable source archive.
- RC2 updates source build metadata, PlatformIO library metadata and host-test expectations only.
- Host smoke tests pass. Full Kagayaki PlatformIO compilation, all-40-effect registration and long-duration hardware qualification remain necessary before final v0.3.0 promotion.

## v0.2.2

Earlier public release with 25 effects (20 standard, 5 AudioReactive). The v0.3.0 series adds 15 effects and improves rendering performance and long-uptime phase handling.

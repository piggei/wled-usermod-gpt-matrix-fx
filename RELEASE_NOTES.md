# GPT Matrix FX — Release notes

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

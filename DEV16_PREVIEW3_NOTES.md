# dev.16-preview.3 — targeted repairs and full GPT29 replacement

- GPT26b: keep original geometry, remove phase-wrapped hue offset (continuous palette index modulo 256).
- GPT27b: normalize complete geometric angular term (after 0.16 scaling); no positional snap at phase wrap.
- GPT28b: row-tile 32-pixel scratch raster evaluation with segment AABB rejection; no framebuffer/heap allocation; correct an independent 0.7-scaled bounded-phase discontinuity.
- GPT29b: completely replaced by Prismatic Comets, O(5*pixel) procedural sweeping bright comets and curved tails. No reuse of Strange Attractor rendering.
- GPT30b and GPT01–25 + GPTA01–05: unchanged function bodies.

Host checks are not firmware or real Waveshare measurements. Compile and visually test on ESP32-S3 before release.

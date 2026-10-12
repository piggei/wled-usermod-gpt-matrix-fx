# GPT Matrix FX — 0.3.0-dev.16-preview.5

This preview promotes existing labels GPT21b–GPT30b to GPT21–GPT30, with **no modifications to those renderers**. It appends five experimental AudioReactive effects GPTA06b–GPTA10b. Prior effect registration sequence remains stable.

The five new audio beta renderers are Pulse Architecture, Harmonic Constellations, Spectral Vortex, Quantum Fireflies, and Resonance Cathedral. They all use the real 16-band WLED AudioReactive UM data via `sampleAudio()`, a safe fallback when unavailable, and no large stack buffers or per-frame heap allocation.

## Hardware limitation — WLED effect slots

User's previous `/json/info` reported `fxcount=251`; appending 5 gives **256**, at/beyond the boundary for WLED uint8 effect IDs and the 255 sentinel. **Do not flash this preview without removing sufficient other effects to leave room.** Remove at least 2 more effects for some headroom (prefer more), or validate the exact fork's ID model. The 5 new effects are appended after the existing 35; renaming may invalidate presets that select effects by their display name.

This preview is host-testable and has not been compiled or tested against the user's specific Kagayaki fork or ESP32-S3 hardware. FPS and aesthetic quality require hardware qualification.

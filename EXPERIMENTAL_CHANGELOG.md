# Experimental changelog

## v0.3.0-dev.2

- Renamed experimental modes to GPT21b and GPT22b.
- Fixed GPT22b Expansion slider: Intensity now controls maximum dot radius rather than peak brightness; radius remains capped by lattice pitch.
- Kept wave phase, grid centers, palette and speed unchanged.
- Added a deterministic slider regression (64x64, fixed wave phase): lit pixels increase from 484 at minimum Expansion to 1484 at maximum Expansion in the host mock. This is a coverage check, not a measured real-world brightness result.
- Documented the initial hardware smoke test separately from pending controlled A/B validation.
- All existing 25 effect algorithms remain unchanged.

Credit: Milo (1human/JM) suggested the original CC0 Dot ripple reference and identified the Expansion control mismatch in the experimental code.

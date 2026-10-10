# Experimental changelog

## v0.3.0-dev.2

- Renamed experimental modes to GPT21b and GPT22b.
- Fixed GPT22b Expansion slider: Intensity now controls maximum dot radius rather than peak brightness; radius remains capped by lattice pitch.
- Kept wave phase, grid centers, palette and speed unchanged.
- Added a deterministic slider regression (64x64, fixed wave phase): lit pixels increase from 484 at minimum Expansion to 1484 at maximum Expansion in the host mock. This is a coverage check, not a measured real-world brightness result.
- Documented the initial hardware smoke test separately from pending controlled A/B validation.
- All existing 25 effect algorithms remain unchanged.

Credit: Milo (1human/JM) suggested the original CC0 Dot ripple reference and identified the Expansion control mismatch in the experimental code.

## v0.3.0-dev.3

- Added GPT23b Photon Loom, GPT24b Kinetic Rosettes and GPT25b Tidal Glyphs.
- Retained GPT21b and GPT22b and all 25 original effects.
- New effects are experimental only; hardware testing pending.

## v0.3.0-dev.4

- Enlarged GPT24b Kinetic Rosettes by 22%, targeting approximately 90-95% matrix occupancy without changing its rotation, petal count or palette behavior.
- Recorded the maintainer's successful GPT22b Expansion hardware smoke test on 2026-10-10, while explicitly marking unreported firmware revision details as unknown.
- No other effect algorithm changed. Public v0.2.2 remains untouched.

## v0.3.0-dev.5
- Added runtime JSON info metadata for installed build identification.
- Replaced both Sparse Ripple `Reserved` controls with shared `Dot Spacing`.
- Replaced GPT24b `Hue` with a geometric `Size` slider.
- All stable and audio effects unchanged.

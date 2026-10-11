# GPT Matrix FX v0.3.0-dev.16-preview.2 — four new beta effects

## Scope

Builds on dev.16-preview.1 (GPT26b already approved visually on the actual Waveshare) and **appends four standard beta effects**:

- `GPT27b - Crystal Recursion`: rotating faceted geometry, nested reflective triangular grooves.
- `GPT28b - Luminous Mycelium`: branching, pulsing luminous filaments generated in a fixed local segment table.
- `GPT29b - Strange Attractor`: color-driven point trajectories from a bounded chaotic mapping.
- `GPT30b - Moire Engine`: interference between moving, angled spatial gratings.

The first 31 renderer functions, existing labels and registration order are unchanged.

The latest build reports **35 effects: 20 standard, 5 AudioReactive, 10 experimental**. GPT21b-GPT25b are not yet promoted; that will be part of the consolidated dev.16 with five new AudioReactive modes. Animatrix must be disabled if needed to make enough WLED dynamic effect slots available. Always check `/json/eff` for every GPT mode. No firmware binary is included.

## Validation performed

- Host C++17 test harness: passes all 35 registered effects at 8x8,16x32,64x64,96x48,128x128 and existing palette/audio regression checks.
- Preview generated from the actual C++ effect functions: 240 frames per new effect at 64x64 and 10 fps (24 seconds); videos and six-timepoint contact sheet are in the companion preview archive.
- Fixed-size stack scratch only (GPT28b 108 entries; GPT29b 240 points); no dynamic allocations or second framebuffer. Temporal phases used by sine/cosine are normalized once per frame.
- Earlier effects are unchanged at function body level; Stage 1, 2A and 2B regression checks are inherited from the baseline. **No full PlatformIO compilation against the user's Kagayaki WLED fork has been performed; no physical Waveshare test yet.**
- The host's Rainbow palette is an approximation of WLED's palette 11; RGB and visual aesthetics require physical panel review.

## Install and collect feedback

Install the source ZIP beside the existing WLED checkout, preserving other usermods/config. Recompile with existing PlatformIO environment. Confirm `/json/info` `u.gptMatrixFX.build=dev.16-preview.2`, `effects=35`, `experimental=10` and verify all modes appear in `/json/eff`.

Then test the **entire standard-beta group GPT26b–GPT30b** on the real 64x64 panel with your preferred palette. Host simulations are not equivalent to the HUB75 panel and should not dictate the maintainer's aesthetic judgment.

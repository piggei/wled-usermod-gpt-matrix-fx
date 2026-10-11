# GPT Matrix FX 0.3.0-dev.16-preview.1 — Waveshare hardware trial

This is a **one-new-effect preview**, not the complete dev.16 (10-new-effect) release.

## Delta versus dev.15

- Appends `GPT26b - Iridescent Membranes` as the 31st effect.
- Original 30 effect functions and effect registration order unchanged; their effect labels also remain unchanged.
- Changes build label to `dev.16-preview.1` and `u.gptMatrixFX.effects` to 31, `experimental` to 6.
- No WLED core, HUB75 driver, palette, AudioReactive, or pin configuration changes.

## Installation

Back up your existing external `wled-usermod-gpt-matrix-fx` directory. Extract the archive so that the included folder replaces its contents without changing the usermod path. Keep your existing WLED PlatformIO environment, other usermods, and board settings. Compile and flash the WLED firmware with your usual workflow. If Animatrix has been removed, keep that change; the required available slots should be verified via `/json/eff`.

This archive is **source code**, not a flashable `.bin`; no complete PlatformIO build was possible without the maintainer's WLED fork/configuration.

## After flash

- `/json/info`: `u.gptMatrixFX.build = dev.16-preview.1`, `effects = 31`, `experimental = 6`.
- `/json/eff`: verify `GPT26b - Iridescent Membranes` appears once, alongside all 30 older GPT effects.
- Choose GPT26b: controls `Flow`, `Glow`, `Film Scale`, `Hue` and defaults `sx=125 ix=185 c1=135 c2=128`.
- Send `/json/info` after ~30 s and report the visual appearance.

## Checks in this environment

- Host test suite PASS (31 rendered/registered effects; legacy status line retains text '30 FX').
- Stage-1 phase regression PASS.
- Stage-2A phase regression PASS.
- Stage-2B phase regression PASS.
- GPT26b isolated 30 s simulation completed (450 host frames), not yet a real WLED firmware build.

## Known constraints

Existing 30 renderers remain unchanged. GPT26b is an aesthetic beta and has not been reviewed on the physical Waveshare. Runtime FPS and physical palette accuracy are therefore unverified. Do not publish this as a stable release. No Git commit is made.

# New experimental effects - v0.3.0-dev.3

All three additions are non-audio, 2D-only, palette-aware beta effects. They are appended after GPT22b so existing effect registration order remains unchanged.

- **GPT23b - Photon Loom**: crossed luminous fibers, traveling pulses, alternating over/under highlights. Custom1 controls thread density.
- **GPT24b - Kinetic Rosettes**: two counter-rotating polar rosette contours with glints. Custom1 controls petal count.
- **GPT25b - Tidal Glyphs**: an open, alternating grid of arc and corner glyphs, rotated by a propagating wave. Custom1 controls grid density.

Each exposes speed and intensity plus two custom controls; there is no audio dependency or additional framebuffer.

## Hardware validation

GPT23b and GPT25b received positive initial visual feedback on a real 64x64 HUB75 matrix. GPT24b dev.3 was visually approved but appeared too small; dev.4 enlarges it by 22%, and that revision still requires hardware validation. Run each effect on a real 64x64 HUB75 display, sweep speed/intensity/custom1, test multiple palettes, and check frame rate before promotion. The stable v0.2.2 release is unchanged.

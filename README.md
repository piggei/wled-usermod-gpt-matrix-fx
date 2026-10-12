# GPT Matrix FX — 40 original WLED matrix effects

[![GPT Matrix FX - 20 Original WLED Matrix Effects](https://img.youtube.com/vi/CnXsLasY7FU/maxresdefault.jpg)](https://www.youtube.com/watch?v=CnXsLasY7FU)

**40 original, procedural 2D effects for [WLED](https://github.com/wled/WLED): 30 visual effects and 10 AudioReactive effects.** Designed primarily for a **64×64 LED matrix** and tested during development on an ESP32-S3 / Waveshare HUB75 matrix.

> **Release status:** This source package is based on **v0.3.0-rc.2**. The README is prepared for the public v0.3.0 release; the final release is still pending qualification. The rendering algorithms are frozen from the hardware-reviewed `dev.16-preview.7b` baseline; RC2 changes only release metadata and packaging.

## Matrix size and hardware

- **Recommended matrix:** 64×64 (4,096 pixels). This is the canvas on which the effects were designed, visually reviewed and tested.
- **16×16:** **Not recommended.** Real-device trials show that the intricate geometries and details do not translate well to 16×16. The renderer may run, but the result does not represent the intended artwork.
- **Other dimensions:** 2D rendering supports other matrix sizes in principle, but aesthetic quality and performance are not established for every size; they are **not** equivalent to 64×64 qualification.
- **AudioReactive effects:** require WLED's [AudioReactive](https://kno.wled.ge/advanced/audio-reactive/) support and an active audio input. Standard effects do not require sound.

The effects draw into WLED's 2D segment. Available frame rate depends on the display driver, other usermods, rendering complexity and matrix settings. Some effects are substantially more computationally expensive than others.

## Effect catalog

All 40 effects are listed below. For the most faithful demonstration of their visual appearance, watch the video above or test them on a 64×64 LED matrix. No simulated images or animated previews are included.

### Standard effects (GPT01–GPT30)

| UI name | Description |
| --- | --- |
| **GPT01 - Caustic Atlas** | Fluid caustic light patterns with shifting refractions and prismatic interference. |
| **GPT02 - Voronoi Bloom** | Growing, colorful Voronoi cells with animated borders and soft internal detail. |
| **GPT03 - Origami Reactor** | Folding angular surfaces with layered, rotating origami geometry. |
| **GPT04 - Ink Tectonics** | Flowing, cellular ink-like patterns that grow and reorganize over time. |
| **GPT05 - Event Horizon** | A luminous accretion structure circling a dark gravitational center. |
| **GPT06 - Clockwork Cathedral** | Interlocking mechanical and architectural forms with clockwork rotation. |
| **GPT07 - Impossible Lattice** | Recursive faceted lattices that drift through impossible-looking geometry. |
| **GPT08 - Aether Topography** | Undulating terrain and contour lines forming an abstract glowing landscape. |
| **GPT09 - Mandelglass** | Fractal-inspired glass surfaces with shifting reflections and intricate detail. |
| **GPT10 - Glass Circuit** | An illuminated circuit network with moving signals and glass-like traces. |
| **GPT11 - Aurora Loom** | Soft, aurora-like luminous threads flowing through layered fields. |
| **GPT12 - Lagrange Ballet** | Orbital contours tracing a graceful interplay of moving attractors. |
| **GPT13 - Chromatic Kintsugi** | Colored fractured surfaces traced by slowly moving luminous seams. |
| **GPT14 - Phantom Elevator** | Repeating perspective structures that suggest an endless vertical passage. |
| **GPT15 - Ferrofluid Theatre** | Ferrofluid-inspired magnetic spikes and smoothly deforming silhouettes. |
| **GPT16 - Riemann Current** | Winding currents and vortices producing rich shifting mathematical textures. |
| **GPT17 - Constellation Engine** | Luminous stars and connecting geometry moving in an orbital web. |
| **GPT18 - Vitreous Labyrinth** | A continuously folding labyrinth with translucent glowing passages. |
| **GPT19 - Celestial Runewheel** | Rotating circular glyphs and fine radial celestial markings. |
| **GPT20 - Zenith Quasicrystal** | Layered quasi-crystalline geometry with radiant symmetrical facets. |
| **GPT21 - Sparse Ripple Brightness** | A sparse grid of luminous dots whose brightness follows travelling waves. |
| **GPT22 - Sparse Ripple Size** | Travelling ripples that animate the size of dots in a sparse lattice. |
| **GPT23 - Photon Loom** | Intersecting, pulsing threads of light with a woven appearance. |
| **GPT24 - Kinetic Rosettes** | Orbiting, multi-petaled rosettes leaving elegant luminous traces. |
| **GPT25 - Tidal Glyphs** | Moving geometric glyphs shaped by slow tidal modulation. |
| **GPT26 - Iridescent Membranes** | Fluid iridescent membranes with smooth color fades and organic outlines. |
| **GPT27 - Crystal Recursion** | Layered crystalline facets that rotate and refract colored light. |
| **GPT28 - Luminous Mycelium** | An evolving bioluminescent branching network with glowing filaments. |
| **GPT29 - Prismatic Comets** | Bright prismatic streaks and moving luminous comet-like trails. |
| **GPT30 - Moire Engine** | Hypnotic interference from moving, overlapping moiré patterns. |

### AudioReactive effects (GPTA01–GPTA10)

| UI name | Description |
| --- | --- |
| **GPTA01 - Bass Cartographer** | Audio-driven polar contours that map bass energy into moving terrain. |
| **GPTA02 - Harmonic Wormholes** | Harmonic energy creates animated portals and luminous depth. |
| **GPTA03 - Spectral Loom** | Frequency bands weave an animated braid of glowing threads. |
| **GPTA04 - Resonant Tessellator** | Spectrum-controlled faceted tiles forming a resonant geometric mosaic. |
| **GPTA05 - Synaptic Reef** | An organic reef with independent concentric waves launched by successive beats. |
| **GPTA06 - Pulse Architecture** | Spectral towers whose heights and internal light detail follow the music. |
| **GPTA07 - Harmonic Constellations** | A full-matrix constellation with stars and links responding to audio bands. |
| **GPTA08 - Spectral Vortex** | A rotating spiral whose arms react to the frequency spectrum. |
| **GPTA09 - Spectral Ribbons** | Flowing ribbons that bend, brighten and expand with the music. |
| **GPTA10 - Resonance Cathedral** | Resonant arches and a stained-glass-inspired rosette driven by audio. |

## Installation with PlatformIO

This repository is an **external WLED usermod**, not a standalone firmware binary. Add this directory to your WLED project's existing PlatformIO `custom_usermods` configuration; keep the other configured usermods:

```ini
custom_usermods =
    symlink://../wled-usermod-gpt-matrix-fx
```

If your environment already lists other usermods, **append** the `symlink://...` entry instead of replacing the existing list. An absolute path can be provided with `symlink:///absolute/path/to/wled-usermod-gpt-matrix-fx`. Build from the WLED checkout:

```sh
pio run -e YOUR_ENVIRONMENT
```

Configure WLED's 2D matrix and segment to match your physical panel. For GPTA01–GPTA10, configure the AudioReactive usermod and audio input separately. No other core modifications are needed **unless** your particular WLED build runs out of effect IDs, as described below.

## Effect ID limit: missing effects in WLED

**Symptom:** One or more GPT effects—often those registered last—are missing from the effects menu even though compilation succeeds.

### Why this happens

WLED's current effect-registration paths use **8-bit effect IDs**, with **255 reserved** as the special argument to `strip.addEffect(255, ...)` for automatic registration. A highly customized firmware with many built-in and usermod effects can exhaust the available IDs. **Hiding an effect from the interface does not free its registration slot.** Do not rely only on the `fxcount` number to determine whether there is room: reserved (`RSVD`) slots can be reused while remaining in the effect table.

### Recommended solution for a custom WLED fork

1. **Back up the fork and export WLED presets** before altering registrations.
2. Apply the separate **WLED FX Registration Filter** patch to the *matching revision* of your fork's `wled00/FX.cpp`. The patch is not part of stock WLED or of this standalone usermod.
3. Place its `wled_fx_exclusions.h` next to `FX.cpp`. In that file select *unused built-in modes* to leave unregistered, preserving the mode implementation and original ID positions.
4. Append `-D WLED_FX_CUSTOM_FILTER` to the **existing** `build_flags` for your PlatformIO environment. **This macro has no effect unless the matching core patch is installed.**
5. Each excluded `FX_MODE_...` must appear only **once** in the filter's `switch`: duplicates cause the C++ compile error `duplicate case value`.
6. Rebuild, flash, and verify that **all 40 GPT Matrix FX names** appear in WLED's `/json/eff` list; check `u.gptMatrixFX` in `/json/info` for the module version.
7. **Check saved presets and automations.** Built-in effect IDs are retained, but dynamically assigned usermod IDs can shift when the available slots or registration order change. A preset using an old numeric FX ID can end up selecting a different effect.

The exclusion filter is **registration-only**: it does not delete built-in algorithms or necessarily reduce firmware binary size. Restore the original `FX.cpp` / remove the macro to revert. Do not disable WLED's 2D subsystem on a HUB75 build. Removing unwanted optional effect-producing usermods is another way to recover slots.

**Important:** Do not blindly copy a modified `FX.cpp` from another WLED revision. The filter must be adapted to your branch. The example filter is distributed separately from this library; consult the project's release assets or your own fork-specific patch.

## Parameters and diagnostics

Each effect exposes WLED's standard controls (such as Speed and Intensity) plus effect-specific controls where applicable. Some visual styles are palette-sensitive; check colors and animations on the real device.

`/json/info` exposes data under `u.gptMatrixFX`, including `version`, `build`, `effects`, `standard`, `audio`, `experimental`, and performance diagnostics. The module contributes diagnostics without replacing other usermod fields. The WLED global `leds.fps` reflects the full WLED/display pipeline, not only the cost of the selected effect renderer.

## Development and tests

The project includes a deterministic host WLED mock and regression tests:

```sh
sh tests/run_host_tests.sh
sh tests/run_sanitizers.sh
```

The release source archive is intentionally kept lean: it contains production code, documentation, and a host smoke-test suite, but no historical CSV diagnostics, generated previews, benchmark outputs, or experimental build notes. Full-device compilation and extended hardware stability remain release gates.

## License

See [LICENSE](LICENSE) for the repository license. External components such as WLED and AudioReactive retain their respective licenses.
